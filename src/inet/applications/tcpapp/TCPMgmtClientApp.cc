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
    if (ttlMsg != nullptr) {
        cancelAndDelete(ttlMsg);
        //delete ttlMsg;
    }
}

void TCPMgmtClientApp::initialize(int stage) {
    TCPBasicClientApp::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"),
                this);
        clpBrd = getModuleFromPar<ClipBoard>(par("clipBoardModule"), this);
        if (clpBrd != nullptr) {
            clpBrd->setHeartBeatMapClient(&this->heartBeatMap);
        } else {
            cRuntimeError("Can't access clipBoard Module");
        }

        device = getContainingNode(this);
        sdNic = device->getModuleByPath(par("sdNicName").stringValue());
        p2pNic = device->getModuleByPath(par("p2pNicName").stringValue());
        pxNic = device->getModuleByPath(par("pxNicName").stringValue());

        WATCH(myHeartBeatRecord.ipAddress);
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

                myHeartBeatRecord = HeartBeatRecord();
                initMyHeartBeatRecord();
                heartBeatMap.clear();

                if(!ttlMsg) {
                    ttlMsg = new cMessage("ttlMsg");
                    ttlMsg->setKind(TTL_MSG);
                }
                if (!ttlMsg->isScheduled()) {
                    scheduleAt(simTime() + par("decTtlPeriod"), ttlMsg);
                }
            }
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER) {
            cancelEvent(timeoutMsg);
            if (socket.getState() == TCPSocket::CONNECTED || socket.getState() == TCPSocket::CONNECTING || socket.getState() == TCPSocket::PEER_CLOSED) {
                //close();
                abort();
                if(ttlMsg != nullptr) {
                    /*if(ttlMsg->isScheduled()) {
                     cancelAndDelete(ttlMsg);
                     }*/
                    cancelEvent(ttlMsg);
                }
                // TODO: wait until socket is closed
            }
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
        int numRemoved = removeZeroTtl();
        if (numRemoved > 0) {
            char buff[40];
            sprintf(buff, "Number of (TTL) removed = %d", numRemoved);
            device->bubble(buff);
        }

        scheduleAt(simTime() + par("decTtlPeriod"), msg);
    } else {
        //Let's try to limit the sending of messages to only when the node is operational
        //I hope that this fixes the various unexpected socket errors.
        if (isNodeUp()) {
            TCPAppBase::handleMessage(msg);
        } else {
            delete msg;
        }
    }
}

void TCPMgmtClientApp::abort() {
    setStatusString("aborting");
    EV_INFO << "issuing ABORT command\n";
    socket.abort();
    emit(connectSignal, -1L);
}

void TCPMgmtClientApp::refreshDisplay() const {
    char buf[80];
    sprintf(buf, "%s\nrcvd: %d pks %d bytes\nsent: %d pks %d bytes",
            status.c_str(), packetsRcvd, bytesRcvd, packetsSent, bytesSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void TCPMgmtClientApp::setStatusString(const char* s) {
    status = s;
}

void TCPMgmtClientApp::setConnectAddressToGoIP() {
    //change the connection target to be the default gateway (GO)
    const InterfaceEntry* ie =
            const_cast<const InterfaceEntry*>(ift->getInterfaceByName(
                    par("ipInterface")));
    uint myIpInt = ie->ipv4Data()->getIPAddress().getInt();
    //From this GM IP we can get the GO IP.
    //As it differs only in the first octet.
    //Which should be 1 for the GO.
    uint GoIpInt = (myIpInt & 0xFFFFFF00) | 0x00000001;
    IPv4Address GoIp(GoIpInt);
    par("connectAddress").setStringValue(GoIp.str());
}

void TCPMgmtClientApp::handleTimer(cMessage* msg) {
    if (msg->getKind() == MSGKIND_CONNECT) {
        initMyHeartBeatRecord();
        //Check if my ip address is already assigned by the DHCP or not.
        if (myHeartBeatRecord.ipAddress.compare("") == 0) {
            //schedule the connection for another time
            simtime_t d = simTime() + (simtime_t) par("idleInterval");
            rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
            return;
        } else {
            setConnectAddressToGoIP();
        }

        TCPBasicClientApp::handleTimer(msg);
    } else if (msg->getKind() == MSGKIND_SEND) {
        if (socket.getState() != socket.LOCALLY_CLOSED) {
            sendRequest();
            numRequestsToSend--;
        }

        simtime_t d = simTime() + (simtime_t) par("thinkTime");
        rescheduleOrDeleteTimer(d, MSGKIND_SEND);

    } else {
        TCPBasicClientApp::handleTimer(msg);
    }

}

void TCPMgmtClientApp::sendRequest() {
    //long requestLength = par("requestLength");
    long replyLength = par("replyLength");
    //if (requestLength < 1)
    //    requestLength = 1;
    if (replyLength < 1)
        replyLength = 1;

    HeartBeatMsg *hbMsg = new HeartBeatMsg("data");
    //hbMsg->setByteLength(requestLength);
    hbMsg->setExpectedReplyLength(replyLength);
    hbMsg->setServerClose(false);
    HeartBeatMap hbMap;
    hbMap[myHeartBeatRecord.devId] = myHeartBeatRecord;
    //insert my record in the msg
    hbMsg->setHeartBeatMap(hbMap);
    uint msgByteLen = sizeof(int) + sizeof(HeartBeatRecord);
    hbMsg->setByteLength(msgByteLen);
    if (numRequestsToSend <= 1) {
        //The GO should replay to our request only when it is the last one in the session
        //numRequestsToSend is used here to mimic the protocol time that the GO should
        //wait until sending an HB msg with all the gathered informations about the GMs
        hbMsg->setReplayNow(true);
    } else {
        hbMsg->setReplayNow(false);

        simtime_t d = simTime() + (simtime_t) par("thinkTime");
        rescheduleOrDeleteTimer(d, MSGKIND_SEND);
    }

    EV_INFO << "sending request with " << msgByteLen
                   //requestLength
                   << " bytes, expected reply length " << replyLength
                   << " bytes," << "remaining " << numRequestsToSend - 1
                   << " request\n";

    sendPacket(hbMsg);
}

void TCPMgmtClientApp::socketDataArrived(int connId, void* ptr, cPacket* msg,
        bool urgent) {

    HeartBeatMsg *hbMsg = dynamic_cast<HeartBeatMsg*>(msg);

    if (hbMsg != nullptr) {
        HeartBeatMap hbMap = hbMsg->getHeartBeatMap();

        if (!hbMsg->getIsProxyAssignment()) {
            //If its a HB Msg, extract the embedded records and store it in our map
            for (auto& hb : hbMap) {
                heartBeatMap[hb.first] = hb.second;
            }
        } else {
            //Change the ssid of the pxNic to the one given in the msg
            HeartBeatRecord hbRec = hbMap[myHeartBeatRecord.devId];
            const char *ssid = hbRec.reachableSSIDs[0].c_str();
            //Move the assignment back to the udp_sd_app
            /*if (pxNic != nullptr)
             pxNic->getSubmodule("agent")->par("default_ssid").setStringValue(
             ssid);*/
            clpBrd->setProxySsid(ssid);
        }
        nextMsgIsPxAssignment = hbMsg->getNextIsProxyAssignment();
    }

    //Allow for receiving another message (Proxy Assignment) from the GO.
    //It could arrive or not based on if this device is selected or not
    if ((numRequestsToSend > 0) || nextMsgIsPxAssignment) {
        EV_INFO << "reply arrived\n";
    } else if (socket.getState() != TCPSocket::LOCALLY_CLOSED) {
        EV_INFO << "reply to last request arrived, closing session\n";
        close();
    }

    TCPAppBase::socketDataArrived(connId, ptr, msg, urgent);
}

void TCPMgmtClientApp::initMyHeartBeatRecord() {
    if (device != nullptr) {
        int devID = device->getId();
        myHeartBeatRecord.devId = devID;
    }

    if (ift != nullptr) {
        const InterfaceEntry* ie =
                const_cast<const InterfaceEntry*>(ift->getInterfaceByName(
                        par("ipInterface")));
        IPv4Address myIP = ie->ipv4Data()->getIPAddress();
        if (!myIP.isUnspecified()) {
            myHeartBeatRecord.ipAddress = myIP.str();
        }
        myHeartBeatRecord.macAddress = ie->getMacAddress().str();
    }

    if (p2pNic != nullptr) {
        myGoSSID =
                p2pNic->getSubmodule("agent")->par("default_ssid").stringValue();
    }

    myHeartBeatRecord.reachableSSIDs.clear();
    if (clpBrd != nullptr) {
        DevicesInfo *pInfo = clpBrd->getPeersInfo();
        for (auto& pf : *pInfo) {
            //check if it is a GO (has ssid)
            if (pf.second.ssid.compare("") != 0) {
                //make sure that we do not include the SSID of our GO
                if (pf.second.ssid.compare(myGoSSID) != 0) {
                    myHeartBeatRecord.reachableSSIDs.push_back(pf.second.ssid);
                }
            }
        }
    }
    //dump reachable ssids
    /*EV_DETAIL << "\n  id->" << myHeartBeatRecord.devId << " can reach ";
     for (const string& ssid : myHeartBeatRecord.reachableSSIDs) {
     EV_DETAIL << "  " << ssid;
     }*/
    //
    myHeartBeatRecord.ttl = par("hb_ttl");
}

void TCPMgmtClientApp::decreasePeersTtl() {
    for (auto& pf : heartBeatMap) {
        pf.second.ttl -= 1;
    }
}

int TCPMgmtClientApp::removeZeroTtl() {
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

} /* namespace inet */
