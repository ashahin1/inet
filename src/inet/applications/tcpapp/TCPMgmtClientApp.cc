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

#include "inet/applications/tcpapp/TCPMgmtClientApp.h"

#include "inet/applications/tcpapp/HeartBeatMsg_m.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeOperations.h"

namespace inet {

#define MSGKIND_CONNECT    0
#define MSGKIND_SEND       1

Define_Module(TCPMgmtClientApp);

TCPMgmtClientApp::TCPMgmtClientApp() {
    // TODO Auto-generated constructor stub

}

TCPMgmtClientApp::~TCPMgmtClientApp() {
    // TODO Auto-generated destructor stub
    cancelAndDelete(ttlMsg);
}

void TCPMgmtClientApp::initialize(int stage) {
    TCPBasicClientApp::initialize(stage);

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
    }
}

bool TCPMgmtClientApp::handleOperationStage(LifecycleOperation *operation,
        int stage, IDoneCallback *doneCallback) {
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if ((NodeStartOperation::Stage)stage == NodeStartOperation::STAGE_APPLICATION_LAYER) {
            simtime_t now = simTime();
            simtime_t start = std::max(startTime, now);
            if (timeoutMsg && ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))) {
                timeoutMsg->setKind(MSGKIND_CONNECT);
                scheduleAt(start, timeoutMsg);

                initMyHeartBeatRecord();
                heartBeatMap.clear();

                ttlMsg = new cMessage("ttlMsg");
                ttlMsg->setKind(TTL_MSG);
            }
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER) {
            cancelEvent(timeoutMsg);
            if (socket.getState() == TCPSocket::CONNECTED || socket.getState() == TCPSocket::CONNECTING || socket.getState() == TCPSocket::PEER_CLOSED)
            close();
            // TODO: wait until socket is closed
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if ((NodeCrashOperation::Stage)stage == NodeCrashOperation::STAGE_CRASH)
        cancelEvent(timeoutMsg);
    }
    else
    throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

void TCPMgmtClientApp::handleMessage(cMessage* msg) {
    if ((msg->isSelfMessage()) && (msg->getKind() == TTL_MSG)) {
        decreasePeersTtl();
        removeZeroTtl();

        scheduleAt(simTime() + par("decTtlPeriod"), msg);
    } else {
        TCPAppBase::handleMessage(msg);
    }
}

void TCPMgmtClientApp::handleTimer(cMessage* msg) {
    if (msg->getKind() == MSGKIND_SEND) {
        sendRequest();
        numRequestsToSend--;

        simtime_t d = simTime() + (simtime_t) par("thinkTime");
        rescheduleOrDeleteTimer(d, MSGKIND_SEND);

    } else {
        TCPMgmtClientApp::handleTimer(msg);
    }

}

void TCPMgmtClientApp::sendPacket(cPacket* msg) {
    HeartBeatMsg *hbMsg = dynamic_cast<HeartBeatMsg *>(msg);

    if (hbMsg != nullptr) {
        HeartBeatMap hbMap;
        hbMap[myHeartBeatRecord.devId] = myHeartBeatRecord;
        //insert my record in the msg
        hbMsg->setHeartBeatMap(hbMap);
        if (numRequestsToSend <= 1) {
            //The GO should replay to our request only when it is the last one in the session
            //numRequestsToSend is used here to mimic the protocol time that the GO should
            //wait until sending an HB msg with all the gathered informations about the GMs
            hbMsg->setReplayNow(true);
        } else {
            hbMsg->setReplayNow(false);
        }
    }

    TCPAppBase::sendPacket(msg);
}

void TCPMgmtClientApp::socketDataArrived(int connId, void* ptr, cPacket* msg,
        bool urgent) {
    TCPAppBase::socketDataArrived(connId, ptr, msg, urgent);

    HeartBeatMsg *hbMsg = dynamic_cast<HeartBeatMsg*>(msg);

    if (hbMsg != nullptr) {
        //If its a HB Msg, extract the embedded records and store it in our map
        HeartBeatMap hbMap = hbMsg->getHeartBeatMap();

        for (auto& hb : hbMap) {
            heartBeatMap[hb.first] = hb.second;
        }

    }

    if (numRequestsToSend > 0) {
        EV_INFO << "reply arrived\n";
    } else if (socket.getState() != TCPSocket::LOCALLY_CLOSED) {
        EV_INFO << "reply to last request arrived, closing session\n";
        close();
    }
}

void TCPMgmtClientApp::initMyHeartBeatRecord() {
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
    if (clpBrd != nullptr) {
        DevicesInfo *pInfo = clpBrd->getPeersInfo();
        for (auto& pf : *pInfo) {
            //check if it is a GO (has ssid)
            if (pf.second.ssid.compare("") != 0) {
                myHeartBeatRecord.reachableSSIDs.push_back(pf.second.ssid);
            }
        }
    }
    myHeartBeatRecord.ttl = par("hb_ttl");
}

void TCPMgmtClientApp::decreasePeersTtl() {
    for (auto& pf : heartBeatMap) {
        pf.second.ttl -= 1;
    }
}

void TCPMgmtClientApp::removeZeroTtl() {
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
