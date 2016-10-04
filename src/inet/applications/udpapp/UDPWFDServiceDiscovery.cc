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

#include "inet/power/base/PowerDefs.h"

namespace inet {

using namespace units::values;

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
        cModule *device = getContainingNode(this);
        dhcpClient = device->getModuleByPath(
                par("dhcpClientAppName").stringValue());
        dhcpServer = device->getModuleByPath(
                par("dhcpServerAppName").stringValue());
        apNic = device->getModuleByPath(par("ApNicName").stringValue());
        energyStorage = dynamic_cast<EnergyStorageBase *>(device->getSubmodule(
                "energyStorage"));
        energyGenerator = dynamic_cast<IEnergyGenerator *>(device->getSubmodule(
                "energyGenerator"));

        myInfo = getMyInfo();

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
    //Send requests per the defined schedule
    sendServiceDiscoveryPacket();
}

void UDPWFDServiceDiscovery::processPacket(cPacket *pk) {
    emit(rcvdPkSignal, pk);
    EV_INFO << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk)
                   << endl;

    int senderModuleId = pk->getSenderModuleId();

    if (ServiceDiscoveryRequest *sdReq =
            dynamic_cast<ServiceDiscoveryRequest*>(pk)) {
        //A request is received from a nearby device, so we send a response.
        myInfo = getMyInfo(); //update myInfo to be used in the next calculation. It also checks for conflicts in subnets
        sendServiceDiscoveryPacket(false, true, sdReq->getOrgSendTime());
        if (isGroupOwner) {
            sendServiceDiscoveryPacket(false, false, sdReq->getOrgSendTime());
        }
        numRequestRcvd++;
    } else {
        simtime_t eed = 0;
        if (ServiceDiscoveryResponseDeviceInfo *respDevInfo =
                dynamic_cast<ServiceDiscoveryResponseDeviceInfo *>(pk)) {

            //This is a response that contains the device info
            //Let's:
            //1- Store it
            //2- Compute the rank for the device
            //3- Check for conflicts in the proposed IP
            //4-

            DeviceInfo pInfo;
            pInfo.batteryCapacity = respDevInfo->getBatteryCapacity();
            pInfo.batteryLevel = respDevInfo->getBatteryLevel();
            pInfo.isCharging = respDevInfo->getIsCharging();
            pInfo.propsedSubnet = respDevInfo->getPropsedSubnet();

            if (peersInfo.count(senderModuleId) > 0) {
                DeviceInfo *pf = &peersInfo[senderModuleId];
                pf->batteryCapacity = pInfo.batteryCapacity;
                pf->batteryLevel = pInfo.batteryLevel;
                pf->isCharging = pInfo.isCharging;
                pf->propsedSubnet = pInfo.propsedSubnet;
            } else {
                peersInfo[senderModuleId] = pInfo;
            }
            eed = simTime() - respDevInfo->getOrgSendTime();
        } else if (ServiceDiscoveryResponseSapInfo *respSapInfo =
                dynamic_cast<ServiceDiscoveryResponseSapInfo *>(pk)) {

            DeviceInfo pInfo;
            pInfo.ssid = respSapInfo->getSsid();
            pInfo.key = respSapInfo->getKey();

            if (peersInfo.count(senderModuleId) > 0) {
                DeviceInfo *pf = &peersInfo[senderModuleId];
                pf->ssid = pInfo.ssid;
                pf->key = pInfo.key;
            } else {
                peersInfo[senderModuleId] = pInfo;
            }

            eed = simTime() - respSapInfo->getOrgSendTime();
        }

        if (eed != 0) {
            endToEndDelayVec.record(eed);
            numResponseRcvd++;
        }
    }
    delete pk;
    numReceived++;
}

void UDPWFDServiceDiscovery::switchDhcpClientToProxy() {
    if (opp_strcmp(dhcpClient->par("interface").stringValue(),
            par("groupInterface").stringValue()) == 0)
        dhcpClient->par("interface").setStringValue(
                par("proxyInterface").stringValue());
}

void UDPWFDServiceDiscovery::switchDhcpClientToGroup() {
    if (opp_strcmp(dhcpClient->par("interface").stringValue(),
            par("proxyInterface").stringValue()) == 0)
        dhcpClient->par("interface").setStringValue(
                par("groupInterface").stringValue());
}

DeviceInfo UDPWFDServiceDiscovery::getMyInfo() {
    DeviceInfo mInfo;
    if (energyStorage != nullptr) {
        mInfo.batteryCapacity = energyStorage->getNominalCapacity().get();
        mInfo.batteryLevel = energyStorage->getResidualCapacity().get()
                / mInfo.batteryCapacity;
    }
    double genPower = 0;
    if (energyGenerator != nullptr) {
        genPower = energyGenerator->getPowerGeneration().get();
    }
    mInfo.isCharging = !(genPower == 0);

    mInfo.propsedSubnet = getConflictFreeSubnet();
    mInfo.conflictedSubnets = getPeersConflictedSubnets();

    if (apNic != nullptr) {
        mInfo.ssid = apNic->getSubmodule("mgmt")->par("ssid").str();
        mInfo.key = "";
    }

    return mInfo;
}

void UDPWFDServiceDiscovery::addDeviceInfoToPayLoad(
        ServiceDiscoveryResponseDeviceInfo* payload, simtime_t orgSendTime) {
    payload->setOrgSendTime(orgSendTime);

    if (energyStorage != nullptr) {
        payload->setBatteryCapacity(myInfo.batteryCapacity);
        payload->setBatteryLevel(myInfo.batteryLevel);
        payload->setIsCharging(myInfo.isCharging);
    }
}

void UDPWFDServiceDiscovery::addSapInfoToPayLoad(
        ServiceDiscoveryResponseSapInfo* payload, simtime_t orgSendTime) {
    payload->setOrgSendTime(orgSendTime);

    if (apNic != nullptr) {
        payload->setSsid(myInfo.ssid.c_str());
        payload->setKey(myInfo.key.c_str());
    }
}

double UDPWFDServiceDiscovery::getRank(bool isCharging, double Capacity,
        double level) {
    double rank = 0.0f;
    rank += (isCharging ? par("rankAlpha").doubleValue() : 0);
    rank += (Capacity * 1.0f / par("maxBatteryCapacity").doubleValue())
            * par("rankBeta").doubleValue();
    rank += level * par("rankGamma").doubleValue();

    return rank;
}

double UDPWFDServiceDiscovery::getRank(DeviceInfo pInfo) {
    return getRank(pInfo.isCharging, pInfo.batteryCapacity, pInfo.batteryLevel);
}

double UDPWFDServiceDiscovery::getMyRank() {
    return getRank(myInfo);
}

DeviceInfo *UDPWFDServiceDiscovery::getBestRankDevice() {
    double bestRank = -1.0f;
    double myRank = getMyRank();
    double curRank;
    DeviceInfo *bestDevice = nullptr;

    for (auto& pf : peersInfo) {
        curRank = getRank(pf.second);
        if (curRank > bestRank) {
            bestRank = curRank;
            bestDevice = &pf.second;
        }
    }

    if (myRank >= bestRank) {
        return nullptr;
    } else {
        return bestDevice;
    }
}

string UDPWFDServiceDiscovery::proposeSubnet() {
    int maxSubnetX = par("maxSubnetX").longValue();
    int maxSubnetY = par("maxSubnetY").longValue();

    //Avoid 10.0.x.x and 10.1.x.x and 10.10.x.x
    int ip2Octet = intuniform(1, maxSubnetX);
    while ((ip2Octet == 0) || (ip2Octet == 1) || (ip2Octet == 10))
        ip2Octet = intuniform(1, maxSubnetX);

    //Avoid 10.x.0.x and 10.x.1.x
    int ip3octet = intuniform(1, maxSubnetY);
    while ((ip3octet == 0) || (ip3octet == 1))
        ip3octet = intuniform(1, maxSubnetY);

    ostringstream str;
    str << ip2Octet << "." << ip3octet;
    return str.str();
}

string UDPWFDServiceDiscovery::getPeersConflictedSubnets() {
    string cfStr = "";
    for (auto& pf1 : peersInfo) {
        for (auto& pf2 : peersInfo) {
            if (pf1.first != pf2.first) {
                if (pf1.second.propsedSubnet.compare(pf2.second.propsedSubnet)
                        == 0) {
                    cfStr += pf1.second.propsedSubnet + ";";
                }
            }
        }
    }
    return cfStr;
}

bool UDPWFDServiceDiscovery::subnetConflicting() {
    for (auto& pf : peersInfo) {
        //Check if my subnet is in the detected conflicts of other devices
        if (pf.second.conflictedSubnets.find(myInfo.propsedSubnet) != string::npos)
            return true;

        //Check if my subnet is conflicting with proposed subnets of other devices
        if (pf.second.propsedSubnet.compare(myInfo.propsedSubnet) == 0)
            return true;
    }
    return false;
}

string UDPWFDServiceDiscovery::getConflictFreeSubnet() {
    string proSubnet = proposeSubnet();
    while (subnetConflicting()) {
        proSubnet = proposeSubnet();
        numIpConflicts++;
    }
    return proSubnet;
}

} /* namespace inet */
