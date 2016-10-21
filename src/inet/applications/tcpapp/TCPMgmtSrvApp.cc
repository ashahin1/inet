//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "inet/applications/tcpapp/TCPMgmtSrvApp.h"

#include "inet/applications/tcpapp/HeartBeatMsg_m.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeOperations.h"

#include <vector>

namespace inet {

using namespace std;

Define_Module(TCPMgmtSrvApp);

TCPMgmtSrvApp::TCPMgmtSrvApp() {
    // TODO Auto-generated constructor stub

}

TCPMgmtSrvApp::~TCPMgmtSrvApp() {
    // TODO Auto-generated destructor stub
    cancelAndDelete(ttlMsg);
}

void TCPMgmtSrvApp::initialize(int stage) {
    TCPGenericSrvApp::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"),
                this);
        clpBrd = getModuleFromPar<ClipBoard>(par("clipBoardModule"), this);
        if (clpBrd != nullptr) {
            clpBrd->setHeartBeatMapServer(&this->heartBeatMap);
        } else {
            cRuntimeError("Can't access clipBoard Module");
        }

        device = getContainingNode(this);
        sdNic = device->getModuleByPath(par("sdNicName").stringValue());

//        ttlMsg = new cMessage("ttlMsg");
//        ttlMsg->setKind(TTL_MSG);
    }
}

void TCPMgmtSrvApp::handleMessage(cMessage* msg) {
    if ((*clpBrd->isGroupOwner) == true) {
        if ((msg->isSelfMessage()) && (msg->getKind() == TTL_MSG)) {
            decreasePeersTtl();
            int numRemoved = removeZeroTtl();
            if (numRemoved > 0) {
                char buff[40];
                sprintf(buff, "Number of (TTL) removed = %d", numRemoved);
                device->bubble(buff);
            }

            scheduleAt(simTime() + par("decTtlPeriod"), msg);
        } else {
            TCPGenericSrvApp::handleMessage(msg);
        }
    } else {
        heartBeatMap.clear();
        delete msg;
    }
}

void TCPMgmtSrvApp::sendOrSchedule(cMessage* msg, simtime_t delay) {
    if (!ttlMsg) {
        ttlMsg = new cMessage("ttlMsg");
        ttlMsg->setKind(TTL_MSG);
    }

    initMyHeartBeatRecord();
    heartBeatMap[myHeartBeatRecord.devId] = myHeartBeatRecord;

    HeartBeatMsg *hbPeerMsg = dynamic_cast<HeartBeatMsg*>(msg);

    if (hbPeerMsg != nullptr) {
        //If its a HB Msg, extract the embedded record and store it in our map
        HeartBeatMap hbMap = hbPeerMsg->getHeartBeatMap();
        //Clients should send only one record
        if (hbMap.size() == 1) {
            for (auto& hb : hbMap) {
                heartBeatMap[hb.first] = hb.second;
            }
        }
        //The client should tell us whether its time to send a reply or not
        if (!hbPeerMsg->getReplayNow()) {
            delete msg;
            return;
        }

        //Calculate the needed assignments for proxy members
        //We should be in proxy selection state to do it
        if (clpBrd->protocolMsg->getKind() == SET_PROXY_DHCP) {
            calcPxAssignments();
        }
    }

    TCPGenericSrvApp::sendOrSchedule(msg, delay);
}

int TCPMgmtSrvApp::getHbMsgSenderID(HeartBeatMsg* pxAssignMsg) {
    int prevDevID = -1;
    HeartBeatMap hbMap = pxAssignMsg->getHeartBeatMap();
    //Clients should send only one record
    if (hbMap.size() == 1) {
        for (auto& hb : hbMap) {
            prevDevID = hb.first;
        }
    }
    return prevDevID;
}

void TCPMgmtSrvApp::sendPxAssignment(int PrevSenderID,
        HeartBeatMsg* pxAssignMsg) {
    //Now we need to send proxy member assignments to GMs
    //This will be done each time we send a normal heart beat(which have the list of IPs, etc)
    //But this one will contain a another map with one record that have the list of
    //reachable ssid filled with only one ssid (the one that this member is supposed to cover).

    uint msgByteLen = (sizeof(int) + sizeof(HeartBeatRecord));
    pxAssignMsg->setByteLength(msgByteLen);
    pxAssignMsg->setNextIsProxyAssignment(false);
    //Now get the mapping and send the to the device
    HeartBeatMap pxHbMap = getPxAssignmentMap(PrevSenderID);
    pxAssignMsg->setHeartBeatMap(pxHbMap);
    pxAssignMsg->setIsProxyAssignment(true);

    //And finally queue it for sending ;)
    TCPGenericSrvApp::sendBack(pxAssignMsg);
}

void TCPMgmtSrvApp::sendBack(cMessage* msg) {
    HeartBeatMsg *hbPeerMsg = dynamic_cast<HeartBeatMsg*>(msg);

    if (hbPeerMsg != nullptr) {
        //dup to avoid filling the controlInfo again in case of Proxy assignment
        HeartBeatMsg *pxAssignMsg = hbPeerMsg->dup();
        //First get the original sender Id
        int prevSenderID = getHbMsgSenderID(pxAssignMsg);
        //We won't send the assignment if the device is not a candidate
        bool isPxCandiate = isProxyCandidate(prevSenderID);
        //We should be in proxy selection state to send proxy assignments
        //Thus we check the message kind, which tells what command to be done next.
        //So if it is SET_PROXY_DHCP, then it means that the current state is selecting proxies
        bool isInSetPxState = clpBrd->protocolMsg->getKind() == SET_PROXY_DHCP;
        bool needPxAssignment = isInSetPxState && isPxCandiate;

        //Modify the msg by adding the current GO map instead of the GM map
        hbPeerMsg->setHeartBeatMap(heartBeatMap);
        uint msgByteLen = (sizeof(int) + sizeof(HeartBeatRecord))
                * heartBeatMap.size();
        hbPeerMsg->setByteLength(msgByteLen);
        hbPeerMsg->setNextIsProxyAssignment(needPxAssignment);
        hbPeerMsg->setIsProxyAssignment(false);

        //Send the normal heart beat msg that have the list of IP/MAC addresses of all members
        TCPGenericSrvApp::sendBack(hbPeerMsg);

        if (needPxAssignment) {
            //Tell the device if it is selected as a proxy member
            sendPxAssignment(prevSenderID, pxAssignMsg);
        } else {
            delete pxAssignMsg;
        }
    } else {
        TCPGenericSrvApp::sendBack(msg);
    }

}

void TCPMgmtSrvApp::initMyHeartBeatRecord() {
    if (device != nullptr) {
        int devID = device->getId();
        myHeartBeatRecord.devId = devID;
    }

    if (ift != nullptr) {
        const InterfaceEntry* ie =
                const_cast<const InterfaceEntry*>(ift->getInterfaceByName(
                        par("ipInterface")));
        myHeartBeatRecord.ipAddress = ie->ipv4Data()->getIPAddress().str();
        myHeartBeatRecord.macAddress = ie->getMacAddress().str();
    }

    myHeartBeatRecord.reachableSSIDs.clear();
    myHeartBeatRecord.ttl = par("hb_ttl");
}

void TCPMgmtSrvApp::decreasePeersTtl() {
    for (auto& pf : heartBeatMap) {
        pf.second.ttl -= 1;
    }
}

int TCPMgmtSrvApp::removeZeroTtl() {
    int numRemoved = 0;
    vector<int> idsToRemove;

    for (auto& pf : heartBeatMap) {
        if (pf.second.ttl <= 0) {
            idsToRemove.push_back(pf.first);
        }
    }
    numRemoved = idsToRemove.size();
    for (uint i = 0; i < idsToRemove.size(); i++) {
        heartBeatMap.erase(idsToRemove[i]);
    }
    return numRemoved;
}

bool TCPMgmtSrvApp::isProxyCandidate(int prevDevId) {
    for (auto& pa : pxAssignment) {
        if (pa.second == prevDevId) {
            return true;
        }
    }
    return false;
}

HeartBeatMap TCPMgmtSrvApp::getPxAssignmentMap(int prevDevID) {
    HeartBeatMap hbMap;
    //first extract the deviceRecord
    if (heartBeatMap.count(prevDevID) > 0) {
        HeartBeatRecord hbRec = heartBeatMap[prevDevID];

        hbRec.reachableSSIDs.clear();
        for (auto& pa : pxAssignment) {
            if (pa.second == prevDevID) {
                hbRec.reachableSSIDs.push_back(pa.first);
                break;
            }
        }
        hbMap[prevDevID] = hbRec;
    }

    return hbMap;
}

void TCPMgmtSrvApp::calcPxAssignments() {
    pxAssignment.clear();
    //Do the actual calculation

    //make sure that we excluded our ssid
}

} /* namespace inet */
