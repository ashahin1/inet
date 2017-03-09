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

simsignal_t TCPMgmtSrvApp::rcvdMSPkSignal = registerSignal("rcvdMSPk");
simsignal_t TCPMgmtSrvApp::sentMSPkSignal = registerSignal("sentMSPk");

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
        apNic = device->getModuleByPath(par("ApNicName").stringValue());

        if (apNic != nullptr) {
            mySSID = apNic->getSubmodule("mgmt")->par("ssid").stringValue();
        }

        selectProxyAssignmentType();

        ttlMsg = new cMessage("ttlMsg");
        ttlMsg->setKind(TTL_MSG);

        getSimulation()->getSystemModule()->subscribe(TCPGenericSrvApp::sentPkSignal,
                this);
        getSimulation()->getSystemModule()->subscribe(TCPGenericSrvApp::rcvdPkSignal,
                this);
    }
}

void TCPMgmtSrvApp::receiveSignal(cComponent* source, simsignal_t signalID,
        cObject* obj, cObject* details) {
    if (signalID == TCPGenericSrvApp::sentPkSignal) {
        emit(sentMSPkSignal, obj);
    } else if (signalID == TCPGenericSrvApp::rcvdPkSignal) {
        emit(rcvdMSPkSignal, obj);
    }
}

bool TCPMgmtSrvApp::handleOperationStage(LifecycleOperation* operation,
        int stage, IDoneCallback* doneCallback) {
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if ((NodeStartOperation::Stage)stage == NodeStartOperation::STAGE_APPLICATION_LAYER) {
            if(!isOperational) {
                reopenSocket();
            }
            isOperational = true;
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER) {
            isOperational = false;
            closeSocket();
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if ((NodeCrashOperation::Stage)stage == NodeCrashOperation::STAGE_CRASH) {
            isOperational = false;
            closeSocket();
        }
    }
    else {
        throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    }
    return true;
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
            //Let's try to avoid any socket operation while we are closed
            if (isOperational) {
                TCPGenericSrvApp::handleMessage(msg);
            } else {
                delete msg;
            }
        }
    } else {
        heartBeatMap.clear();
        if (msg->getKind() != TTL_MSG) {
            delete msg;
        }
    }
}


void TCPMgmtSrvApp::closeSocket() {
    if (socketListening) {
        socket.close();
        socketListening = false;
    }
}

void TCPMgmtSrvApp::reopenSocket() {
    const char *localAddress = par("localAddress");
    int localPort = par("localPort");

    socket = TCPSocket();
    socket.setOutputGate(gate("tcpOut"));
    socket.setDataTransferMode(TCP_TRANSFER_OBJECT);
    socket.bind(
            localAddress[0] ?
                    L3AddressResolver().resolve(localAddress) : L3Address(),
            localPort);
    socket.listen();
    socketListening = true;
}

void TCPMgmtSrvApp::selectProxyAssignmentType() {
    string pType = par("proxyAssignmentType").stringValue();

    if (pType.compare("MUNKRES") == 0) {
        proxyAssignmentType = ProxyAssignmentTypes::PAT_MUNKRES;
    } else if (pType.compare("FIRST_AVAILABLE") == 0) {
        proxyAssignmentType = ProxyAssignmentTypes::PAT_FIRST_AVAILABLE;
    } else if (pType.compare("RANDOM") == 0) {
        proxyAssignmentType = ProxyAssignmentTypes::PAT_RANDOM;
    } else {
        proxyAssignmentType = ProxyAssignmentTypes::PAT_MUNKRES;
    }
}

void TCPMgmtSrvApp::sendOrSchedule(cMessage* msg, simtime_t delay) {
    if (!ttlMsg->isScheduled()) {
        scheduleAt(simTime() + par("decTtlPeriod"), ttlMsg);
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
        pxAssignMsg->setControlInfo(hbPeerMsg->getControlInfo()->dup());
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

bool TCPMgmtSrvApp::isProxyCandidate(int devId) {
    for (auto& pa : pxAssignment) {
        if (pa.second == devId) {
            return true;
        }
    }
    return false;
}

HeartBeatMap TCPMgmtSrvApp::getPxAssignmentMap(int devID) {
    HeartBeatMap hbMap;
    //first extract the deviceRecord
    if (heartBeatMap.count(devID) > 0) {
        HeartBeatRecord hbRec = heartBeatMap[devID];

        //Insert the ssid of the group that is chosen for this GM
        hbRec.reachableSSIDs.clear();
        for (auto& pa : pxAssignment) {
            if (pa.second == devID) {
                hbRec.reachableSSIDs.push_back(pa.first);
                break;
            }
        }
        hbMap[devID] = hbRec;
    }

    return hbMap;
}

void TCPMgmtSrvApp::buildSsidCoverage(map<string, int>& ssidCoverage) {
    //build a map that have the number of GMs that cover each SSID
    for (auto& hbMap : heartBeatMap) {
        for (int i = 0; i < hbMap.second.reachableSSIDs.size(); i++) {
            string ssid = hbMap.second.reachableSSIDs[i];
            if (ssidCoverage.count(ssid) == 0) {
                ssidCoverage[ssid] = 1;
            } else {
                ssidCoverage[ssid] = ++ssidCoverage[ssid];
            }
        }
    }
    // note that the values stored in this map are not utilized yet
}

void TCPMgmtSrvApp::buildMembersCoverage(map<int, int>& membersCoverage) {
    //build a map that have the number of SSIDs that each GM covers
    for (auto& hbMap : heartBeatMap) {
        //Exclude the GO itself from being added to the map
        if (hbMap.first != myHeartBeatRecord.devId) {
            int ssidCount = hbMap.second.reachableSSIDs.size();
            membersCoverage[hbMap.first] = ssidCount;
        }
    }
}

void TCPMgmtSrvApp::populateCostMatrix(const vector<string>& ssidList,
        const vector<int>& membersList, vector<vector<double> >& cost) {
    //Preparing the cost matrix
    EV_DETAIL << "\nCost Matrix is As Follows:\n";
    //Header
    for (int j = 0; j < ssidList.size(); j++) {
        EV_DETAIL << "\t" << ssidList[j];
    }
    //loop through all GMs that have nearby groups
    for (const int& mID : membersList) {
        //Create a row for the cost matrix
        vector<double> tmpRow;
        //loop through each possible ssid
        for (const string& gSsid : ssidList) {
            //make sure that the current GM can reach the current ssid
            bool canReach = canReachSsid(mID, gSsid);
            if (canReach) {
                double gm_cost = 0.0f;
                //We will use the same rank that we used before
                //to select the GOs, which is already available
                //from the previous step
                DeviceInfo* devInfo = &((*peersInfo)[mID]);
                gm_cost = clpBrd->getRank(*devInfo);
                /*
                 //Adjust the cost by adding a very small random number
                 //to break ties if the same device is the only device to cover
                 //two or more groups
                 double tieBreaker = drand48();// * 0.01f;
                 double adjustedCost = gm_cost * 1000 + tieBreaker * 10;
                 */
                //Note here that the cost is represented by the rank
                //The rank by definition is better when higher
                tmpRow.push_back(gm_cost);
            } else {
                //We should here add a cost of a very small number to indicate that
                //the current GM cannot reach the current SSID
                //tmpRow.push_back(-INFINITY);
                tmpRow.push_back(-9999999);
            }
        }
        //Add the prepared row to the cost matrix
        cost.push_back(tmpRow);
        //Print row
        EV_DETAIL << "\n" << mID;
        for (int iii = 0; iii < tmpRow.size(); ++iii) {
            EV_DETAIL << "\t\t" << tmpRow[iii];
        }
    }
}

void TCPMgmtSrvApp::populatePxAssignmentMunkres(
        hash_map<int, int> direct_assignment, const vector<int>& membersList,
        const vector<string>& ssidList) {

    EV_DETAIL << "\nReal Assignments Are As Follows:\n";
    for (auto& da : direct_assignment) {
        //Discard any assignments that are not valid
        int devId = membersList[da.first];
        string ssid = ssidList[da.second];
        if (canReachSsid(devId, ssid)) {
            EV_DETAIL << "\n" << devId << " ("
                             << cSimulation::getActiveSimulation()->getModule(
                                     devId)->getFullName() << ") " << "\t"
                             << ssid;
            //Adjust the assignment map, so it can be used to send assignments
            pxAssignment[ssid] = devId;
        }
    }
}

void TCPMgmtSrvApp::populatePxAssignmentFirstAvailable(
        const vector<string>& ssidList, const vector<int>& membersList) {
    map<int, bool> deviceDoneMap;
    for (const string& ssid : ssidList) {
        for (const int& mId : membersList) {
            //Check if the device can reach the ssid
            if (canReachSsid(mId, ssid)) {
                //check if we did not use it before
                if (deviceDoneMap.count(mId) == 0) {
                    //Adjust the assignment map, so it can be used to send assignments
                    pxAssignment[ssid] = mId;
                    //mark this device as done
                    deviceDoneMap[mId] = true;
                    break;
                }
            }
        }
    }
}

void TCPMgmtSrvApp::populatePxAssignmentRandom(const vector<string>& ssidList,
        const vector<int>& membersList) {
    map<int, bool> deviceDoneMap;
    for (const string& ssid : ssidList) {
        for (const int& mId : membersList) {
            //Check if the device can reach the ssid
            if (canReachSsid(mId, ssid)) {
                //check if we did not use it before
                if (deviceDoneMap.count(mId) == 0) {
                    //Now we do a random guess to use this member or skip it.
                    bool useDevice = (intrand(2, 0) % 2 == 1);
                    if (useDevice) {
                        //Adjust the assignment map, so it can be used to send assignments
                        pxAssignment[ssid] = mId;
                        //mark this device as done
                        deviceDoneMap[mId] = true;
                        break;
                    }
                }
            }
        }
    }
}

void TCPMgmtSrvApp::populatePxAssignments(vector<string> ssidList,
        vector<int> membersList) {
    if (proxyAssignmentType == ProxyAssignmentTypes::PAT_MUNKRES) {
        //Declare some variables that are needed by the Munkres algorithm implementation
        vector<vector<double> > cost;
        hash_map<int, int> direct_assignment;
        hash_map<int, int> reverse_assignment;
        //Preparing the cost matrix
        populateCostMatrix(ssidList, membersList, cost);
        //Now we start the Munkres (Hungarian) algorithm
        //We need to maximize the cost of assignments in this case
        operations_research::MaximizeLinearAssignment(cost, &direct_assignment,
                &reverse_assignment);
        //Now preparing the assignment map that will be used to notify members
        //about their assignments
        populatePxAssignmentMunkres(direct_assignment, membersList, ssidList);
    } else if (proxyAssignmentType
            == ProxyAssignmentTypes::PAT_FIRST_AVAILABLE) {
        //Use the first available method where we assign to a group the first member in the
        // list that can reach it. If the member is already assigned to another group, we
        // skip it and go for the next one.
        populatePxAssignmentFirstAvailable(ssidList, membersList);
    } else if (proxyAssignmentType == ProxyAssignmentTypes::PAT_RANDOM) {
        //We use the same technique as in FirstAvailable, but instead of selecting the member that comes first,
        // we draw a random variable and use it to check if we use it or skip it
        populatePxAssignmentRandom(ssidList, membersList);
    }
}

void TCPMgmtSrvApp::calcPxAssignments() {
    pxAssignment.clear();
    peersInfo = clpBrd->getPeersInfo();

    //Do the actual calculation

    map<string, int> ssidCoverage;
    //build a map that have the number of GMs that cover each SSID
    buildSsidCoverage(ssidCoverage);

    map<int, int> membersCoverage;
    //build a map that have the number of SSIDs that each GM covers
    buildMembersCoverage(membersCoverage);

    vector<string> ssidList;
    vector<int> membersList;

    EV_DETAIL << "\nSSID Coverage is As Follows:\n";
    for (auto& sc : ssidCoverage) {
        EV_DETAIL << "\n" << sc.first << "\t" << sc.second;
        //Populate the ssid list with all known GOs around
        ssidList.push_back(sc.first);
    }

    EV_DETAIL << "\nMembers Coverage is As Follows:\n";
    for (auto& mc : membersCoverage) {
        EV_DETAIL << "\n" << mc.first << "\t" << mc.second;
        //Populate the members list with all members that can reach other groups
        //If the member does not cover any ssid, we should exclude it.
        if (mc.second > 0) {
            membersList.push_back(mc.first);
        }
    }

    //Populate the pxAssignmentMap according to the selected policy.
    //We have to compare our approach to a baseline, so we introduced
    // two more approaches, first available, and random
    populatePxAssignments(ssidList, membersList);
}

bool TCPMgmtSrvApp::canReachSsid(int devId, string ssid) {
    HeartBeatRecord *hbRec = &heartBeatMap[devId];
    bool found = false;
    for (int i = 0; i < hbRec->reachableSSIDs.size(); i++) {
        if (hbRec->reachableSSIDs[i].compare(ssid) == 0) {
            found = true;
            break;
        }
    }
    return found;
}

} /* namespace inet */
