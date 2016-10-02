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
#include "inet/applications/udpapp/UDPWFDServiceDiscovery.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/transportlayer/contract/udp/UDPControlInfo_m.h"

namespace inet {

Define_Module(UDPWFDServiceDiscovery);

UDPWFDServiceDiscovery::UDPWFDServiceDiscovery() {
    // TODO Auto-generated constructor stub

}

UDPWFDServiceDiscovery::~UDPWFDServiceDiscovery() {
    // TODO Auto-generated destructor stub
}

void UDPWFDServiceDiscovery::initialize(int stage) {
    UDPBasicApp::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        clpBrd = getModuleFromPar<ClipBoard>(par("clipBoardModule"), this);

        endToEndDelayVec.setName("SrvDsc End-to-End Delay");
    }
}

void UDPWFDServiceDiscovery::sendServiceDiscoveryPacket(bool isRequestPacket,
        bool isDeviceInfo, simtime_t orgSendTime) {
    std::ostringstream str;
    str << packetName << (isRequestPacket ? "Request" : "Response") << "-"
            << (isRequestPacket ? numRequestSent : numResponseSent);

    ServiceDiscoveryPacket* payload = nullptr;

    if (isRequestPacket) {
        payload = new ServiceDiscoveryRequest(str.str().c_str());
        payload->setOrgSendTime(simTime());
        numRequestSent++;
    } else {
        if (isDeviceInfo) {
            payload = new ServiceDiscoveryResponseDeviceInfo(str.str().c_str());
            addDeviceInfoToPayLoad(
                    (ServiceDiscoveryResponseDeviceInfo *) payload,
                    orgSendTime);
        } else {
            payload = new ServiceDiscoveryResponseSapInfo(str.str().c_str());
            addSapInfoToPayLoad((ServiceDiscoveryResponseSapInfo *) payload,
                    orgSendTime);
        }
        numResponseSent++;
    }
    payload->setByteLength(par("messageLength").longValue());
    payload->setSequenceNumber(numSent);

    L3Address destAddr = chooseDestAddr();
    emit(sentPkSignal, payload);
    //Set the output interface for the datagram to let the broadcast to pass
    UDPSocket::SendOptions* sndOpt = new UDPSocket::SendOptions();
    IInterfaceTable *ift = nullptr;
    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    const InterfaceEntry* destIE =
            const_cast<const InterfaceEntry*>(ift->getInterfaceByName(
                    par("interface")));
    sndOpt->outInterfaceId = destIE->getInterfaceId();
//    if (clpBrd != nullptr) {
//        int hitNo = clpBrd->getNumOfHits() + 1;
//        clpBrd->setNumOfHits(hitNo);
//        EV_INFO << "ClibBoard HitNo set to " << hitNo;
//    } else {
//        EV_ERROR << "Can't Access ClibBoard Module";
//    }
    socket.sendTo(payload, destAddr, destPort, sndOpt);
    //switchDhcpClientToProxy();
    //
    numSent++;
}

void UDPWFDServiceDiscovery::sendPacket() {
    sendServiceDiscoveryPacket();
}

void UDPWFDServiceDiscovery::processPacket(cPacket *pk) {
    emit(rcvdPkSignal, pk);
    EV_INFO << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk)
                   << endl;

    if (ServiceDiscoveryRequest *sdReq =
            dynamic_cast<ServiceDiscoveryRequest*>(pk)) {

        sendServiceDiscoveryPacket(false, true, sdReq->getOrgSendTime());
        if (isGroupOwner) {
            sendServiceDiscoveryPacket(false, false, sdReq->getOrgSendTime());
        }
    } else {
        simtime_t eed = 0;
        if (ServiceDiscoveryResponseDeviceInfo *respDevInfo =
                dynamic_cast<ServiceDiscoveryResponseDeviceInfo *>(pk)) {

            eed = simTime() - respDevInfo->getOrgSendTime();
        } else if (ServiceDiscoveryResponseSapInfo *respSapInfo =
                dynamic_cast<ServiceDiscoveryResponseSapInfo *>(pk)) {

            eed = simTime() - respSapInfo->getOrgSendTime();
        }

        if (eed != 0)
            endToEndDelayVec.record(eed);
    }
    delete pk;
    numReceived++;
}

void UDPWFDServiceDiscovery::switchDhcpClientToProxy() {
    cModule* device = dynamic_cast<cModule*>(getOwner());
    cModule* dhcpClient = device->getModuleByPath(
            par("dhcpAppName").stringValue());
    if (opp_strcmp(dhcpClient->par("interface").stringValue(),
            par("groupInterface").stringValue()) == 0)
        dhcpClient->par("interface").setStringValue(
                par("proxyInterface").stringValue());
}

void UDPWFDServiceDiscovery::switchDhcpClientToGroup() {
    cModule* device = dynamic_cast<cModule*>(getOwner());
    cModule* dhcpClient = device->getModuleByPath(
            par("dhcpAppName").stringValue());
    if (opp_strcmp(dhcpClient->par("interface").stringValue(),
            par("proxyInterface").stringValue()) == 0)
        dhcpClient->par("interface").setStringValue(
                par("groupInterface").stringValue());
}

void UDPWFDServiceDiscovery::addDeviceInfoToPayLoad(
        ServiceDiscoveryResponseDeviceInfo* payload, simtime_t orgSendTime) {
    payload->setOrgSendTime(orgSendTime);


}

void UDPWFDServiceDiscovery::addSapInfoToPayLoad(
        ServiceDiscoveryResponseSapInfo* payload, simtime_t orgSendTime) {
    payload->setOrgSendTime(orgSendTime);

}

} /* namespace inet */
