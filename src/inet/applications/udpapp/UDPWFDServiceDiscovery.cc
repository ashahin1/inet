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

#include "inet/applications/common/ClipBoard.h"
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

void UDPWFDServiceDiscovery::sendPacket() {
    std::ostringstream str;
    str << packetName << "-" << numSent;
    ApplicationPacket *payload = new ApplicationPacket(str.str().c_str());
    payload->setByteLength(par("messageLength").longValue());
    payload->setSequenceNumber(numSent);

    L3Address destAddr = chooseDestAddr();

    emit(sentPkSignal, payload);
    //
    UDPSocket::SendOptions *sndOpt = new UDPSocket::SendOptions();
    IInterfaceTable *ift = nullptr;
    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    const InterfaceEntry *destIE =
            const_cast<const InterfaceEntry *>(ift->getInterfaceByName("wlan0"));
    sndOpt->outInterfaceId = destIE->getInterfaceId();

    ClipBoard *clpBrd = nullptr;
    clpBrd = getModuleFromPar<ClipBoard>(par("clipBoardModule"), this);
    if (clpBrd != nullptr) {
        int hitNo = clpBrd->getNumOfHits() + 1;
        clpBrd->setNumOfHits(hitNo);
        EV_INFO << "ClibBoard HitNo set to " << hitNo;
    } else {
        EV_ERROR << "Can't Access ClibBoard Module";
    }

    socket.sendTo(payload, destAddr, destPort, sndOpt);
    //
    //socket.sendTo(payload, destAddr, destPort);
    numSent++;
}

void UDPWFDServiceDiscovery::processPacket(cPacket *pk) {
    emit(rcvdPkSignal, pk);
    EV_INFO << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk)
                   << endl;
    delete pk;
    numReceived++;
}

} /* namespace inet */
