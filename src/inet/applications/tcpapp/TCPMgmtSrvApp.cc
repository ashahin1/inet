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
            clpBrd->setHeartBeatMap(&this->heartBeatMap);
        } else {
            cRuntimeError("Can't access clipBoard Module");
        }

        cModule *device = getContainingNode(this);
        sdNic = device->getModuleByPath(par("sdNicName").stringValue());

//        ttlMsg = new cMessage("ttlMsg");
//        ttlMsg->setKind(TTL_MSG);
    }
}

void TCPMgmtSrvApp::handleMessage(cMessage* msg) {
    if ((*clpBrd->isGroupOwner) == true) {
        if ((msg->isSelfMessage()) && (msg->getKind() == TTL_MSG)) {
            decreasePeersTtl();
            removeZeroTtl();

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
        if(!hbPeerMsg->getReplayNow()){
            delete msg;
            return;
        }
    }

    TCPGenericSrvApp::sendOrSchedule(msg, delay);
}

void TCPMgmtSrvApp::sendBack(cMessage* msg) {
    HeartBeatMsg *hbPeerMsg = dynamic_cast<HeartBeatMsg*>(msg);

    //Modify the msg by adding the current GO map instead of the GM map
    if (hbPeerMsg != nullptr) {
        hbPeerMsg->setHeartBeatMap(heartBeatMap);
    }
    TCPGenericSrvApp::sendBack(msg);
}

void TCPMgmtSrvApp::initMyHeartBeatRecord() {
    if (sdNic != nullptr) {
        int devID = sdNic->getSubmodule("radio")->getId();
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

void TCPMgmtSrvApp::removeZeroTtl() {
    vector<int> idsToRemove;

    for (auto& pf : heartBeatMap) {
        if (pf.second.ttl <= 0) {
            idsToRemove.push_back(pf.first);
        }
    }

    for (uint i = 0; i < idsToRemove.size(); i++) {
        heartBeatMap.erase(idsToRemove[i]);
    }
}

} /* namespace inet */
