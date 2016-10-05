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

#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
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
    cancelAndDelete(protocolMsg);
}

void UDPWFDServiceDiscovery::finish() {
    recordScalar("IP Conflicts", numIpConflicts);
    recordScalar("Request Packets Received", numRequestRcvd);
    recordScalar("Response Packets Received", numResponseRcvd);
    recordScalar("Request Packets Sent", numRequestSent);
    recordScalar("Response Packets Sent", numResponseSent);
    recordScalar("Num of Times Orphaned", numOfTimesOrphaned);

    UDPBasicApp::finish();
}

void UDPWFDServiceDiscovery::initialize(int stage) {
    UDPBasicApp::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        clpBrd = getModuleFromPar<ClipBoard>(par("clipBoardModule"), this);
        lifeCycleCtrl = getModuleFromPar<LifecycleController>(
                par("lifeCycleControllerModule"), this);
        cModule *device = getContainingNode(this);
        dhcpClient = device->getModuleByPath(
                par("dhcpClientAppName").stringValue());
        dhcpServer = device->getModuleByPath(
                par("dhcpServerAppName").stringValue());
        apNic = device->getModuleByPath(par("ApNicName").stringValue());
        p2pNic = device->getModuleByPath(par("p2pNicName").stringValue());
        proxyNic = device->getModuleByPath(par("proxyNicName").stringValue());
        energyStorage = dynamic_cast<EnergyStorageBase *>(device->getSubmodule(
                "energyStorage"));
        energyGenerator = dynamic_cast<IEnergyGenerator *>(device->getSubmodule(
                "energyGenerator"));

        updateMyInfo(false);

        endToEndDelayVec.setName("SrvDsc End-to-End Delay");
        protocolMsg = new cMessage("Protocol Message");
    }
}

void UDPWFDServiceDiscovery::processStart() {
    UDPBasicApp::processStart();

    protocolMsg->setKind(DECLARE_GO);
    scheduleAt(simTime() + par("declareGoPeriod").doubleValue(), protocolMsg);
}

void UDPWFDServiceDiscovery::handleMessageWhenUp(cMessage* msg) {
    if (msg->isSelfMessage()) {
        if (msg->getKind() == DECLARE_GO) {
            //Compare the rank to the collected ones
            //if my rank is the best
            //  start the wlan1 (AP)
            //  set the ipadress to the proposedOne
            //  start dhcp server and change its parameters to match the proposed subnet
            //  declare my self as GO to start sending SAP info
            //
            updateMyInfo(true);
            isGroupOwner = getBestRankDevice() == nullptr;
            if (isGroupOwner) {
                if (subnetConflicting())
                    getConflictFreeSubnet();
                turnApInterfaceOn();
                setApIpAddress();
                setDhcpServerParams();
                turnDhcpServerOn();
            }

            protocolMsg->setKind(SELECT_GO);
            scheduleAt(simTime() + par("selectGoPeriod").doubleValue(),
                    protocolMsg);
        } else if (msg->getKind() == SELECT_GO) {
            //start wlan2 and set its target ssid
            //start dhcp client and set its intf to wlan2
            if (!isGroupOwner) {
                DeviceInfo *bestGo = getBestRankGO();
                if (bestGo != nullptr) {
                    changeP2pSSID(bestGo->ssid.c_str());
                    turnP2pInterfaceOn();
                    switchDhcpClientToGroup();
                    turnDhcpClientOn();
                }else{
                    EV_INFO << "Orphaned Device Found";
                    numOfTimesOrphaned++;
                }
            }

            protocolMsg->setKind(SET_PROXY_DHCP);
            scheduleAt(simTime() + par("switchDhcpPeriod").doubleValue(),
                    protocolMsg);
        } else if (msg->getKind() == SET_PROXY_DHCP) {
            //start wlan3 and set its target ssid
            //switch dhcp client to wlan3 intf
            if (!isGroupOwner) {
                if (!noGoAround()) {
                    changeProxySSID("xyz");//TODO: get the ssid from the tcp app
                    turnProxyInterfaceOn();
                    switchDhcpClientToProxy();
                }
            }

            protocolMsg->setKind(PROTOCOL_TEARDOWN);
            scheduleAt(simTime() + par("tearDownPeriod").doubleValue(),
                    protocolMsg);
        } else if (msg->getKind() == PROTOCOL_TEARDOWN) {
            //reset every thing
            isGroupOwner = false;
            peersInfo.clear();
            myInfo = DeviceInfo();
            //turn other modules off (dhcp, wlan, etc)
            turnModulesOff();

            protocolMsg->setKind(DECLARE_GO);
            scheduleAt(simTime() + par("declareGoPeriod").doubleValue(),
                    protocolMsg);
        } else {
            UDPBasicApp::handleMessageWhenUp(msg);
        }

    } else {
        UDPBasicApp::handleMessageWhenUp(msg);
    }
}

UDPSocket::SendOptions* UDPWFDServiceDiscovery::setDatagramOutInterface() {
    //Set the output interface for the datagram to let the broadcast to pass
    UDPSocket::SendOptions* sndOpt = new UDPSocket::SendOptions();
    IInterfaceTable *ift = nullptr;
    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    const InterfaceEntry* destIE =
            const_cast<const InterfaceEntry*>(ift->getInterfaceByName(
                    par("interface")));
    sndOpt->outInterfaceId = destIE->getInterfaceId();
    return sndOpt;
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
    UDPSocket::SendOptions* sndOpt = setDatagramOutInterface();
    socket.sendTo(payload, destAddr, destPort, sndOpt);
    numSent++;
}

void UDPWFDServiceDiscovery::sendPacket() {
    //Send requests per the defined schedule
    if (!isGroupOwner) {
        sendServiceDiscoveryPacket();
    }
}

void UDPWFDServiceDiscovery::addOrUpdatePeerDevInfo(int senderModuleId,
        ServiceDiscoveryResponseDeviceInfo* respDevInfo) {

    DeviceInfo pInfo;
    pInfo.batteryCapacity = respDevInfo->getBatteryCapacity();
    pInfo.batteryLevel = respDevInfo->getBatteryLevel();
    pInfo.isCharging = respDevInfo->getIsCharging();
    pInfo.propsedSubnet = respDevInfo->getPropsedSubnet();
    if (peersInfo.count(senderModuleId) > 0) {
        DeviceInfo* pf = &peersInfo[senderModuleId];
        pf->batteryCapacity = pInfo.batteryCapacity;
        pf->batteryLevel = pInfo.batteryLevel;
        pf->isCharging = pInfo.isCharging;
        pf->propsedSubnet = pInfo.propsedSubnet;
    } else {
        peersInfo[senderModuleId] = pInfo;
    }
}

void UDPWFDServiceDiscovery::addOrUpdatePeerSapInfo(int senderModuleId,
        ServiceDiscoveryResponseSapInfo* respSapInfo) {

    DeviceInfo pInfo;
    pInfo.ssid = respSapInfo->getSsid();
    pInfo.key = respSapInfo->getKey();
    if (peersInfo.count(senderModuleId) > 0) {
        DeviceInfo* pf = &peersInfo[senderModuleId];
        pf->ssid = pInfo.ssid;
        pf->key = pInfo.key;
    } else {
        peersInfo[senderModuleId] = pInfo;
    }
}

void UDPWFDServiceDiscovery::processPacket(cPacket *pk) {
    emit(rcvdPkSignal, pk);
    EV_INFO << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk)
                   << endl;

    int senderModuleId = pk->getSenderModuleId();

    if (ServiceDiscoveryRequest *sdReq =
            dynamic_cast<ServiceDiscoveryRequest*>(pk)) {
        //A request is received from a nearby device, so we send a response.
        updateMyInfo(false); //update myInfo to be used in the next calculation. It also checks for conflicts in subnets
        sendServiceDiscoveryPacket(false, true, sdReq->getOrgSendTime());
        if (isGroupOwner) {
            sendServiceDiscoveryPacket(false, false, sdReq->getOrgSendTime());
        }
        numRequestRcvd++;
    } else {
        simtime_t eed = 0;
        if (ServiceDiscoveryResponseDeviceInfo *respDevInfo =
                dynamic_cast<ServiceDiscoveryResponseDeviceInfo *>(pk)) {

            addOrUpdatePeerDevInfo(senderModuleId, respDevInfo);
            eed = simTime() - respDevInfo->getOrgSendTime();
        } else if (ServiceDiscoveryResponseSapInfo *respSapInfo =
                dynamic_cast<ServiceDiscoveryResponseSapInfo *>(pk)) {

            addOrUpdatePeerSapInfo(senderModuleId, respSapInfo);
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

void UDPWFDServiceDiscovery::turnModulesOff() {
    if (lifeCycleCtrl != nullptr) {
        lifeCycleCtrl->processDirectCommand(dhcpClient, false);
        lifeCycleCtrl->processDirectCommand(dhcpServer, false);
        lifeCycleCtrl->processDirectCommand(apNic, false);
        lifeCycleCtrl->processDirectCommand(p2pNic, false);
        lifeCycleCtrl->processDirectCommand(proxyNic, false);
        changeP2pSSID("DIRECT-XXXXXXXX");
        changeProxySSID("DIRECT-XXXXXXXX");
    }
}

void UDPWFDServiceDiscovery::turnDhcpClientOn() {
    lifeCycleCtrl->processDirectCommand(dhcpClient, true);
}

void UDPWFDServiceDiscovery::turnDhcpServerOn() {
    lifeCycleCtrl->processDirectCommand(dhcpServer, true);
}

void UDPWFDServiceDiscovery::turnApInterfaceOn() {
    lifeCycleCtrl->processDirectCommand(apNic, true);
}

void UDPWFDServiceDiscovery::turnP2pInterfaceOn() {
    lifeCycleCtrl->processDirectCommand(p2pNic, true);
}

void UDPWFDServiceDiscovery::turnProxyInterfaceOn() {
    lifeCycleCtrl->processDirectCommand(proxyNic, true);
}

void UDPWFDServiceDiscovery::changeP2pSSID(const char* ssid) {
    if (p2pNic != nullptr)
        p2pNic->getSubmodule("agent")->par("default_ssid").setStringValue(ssid);
}

void UDPWFDServiceDiscovery::changeProxySSID(const char* ssid) {
    if (proxyNic != nullptr)
        proxyNic->getSubmodule("agent")->par("default_ssid").setStringValue(
                ssid);
}

void UDPWFDServiceDiscovery::switchDhcpClientToProxy() {
    if (dhcpClient != nullptr)
        if (opp_strcmp(dhcpClient->par("interface").stringValue(),
                par("groupInterface").stringValue()) == 0)
            dhcpClient->par("interface").setStringValue(
                    par("proxyInterface").stringValue());
}

void UDPWFDServiceDiscovery::switchDhcpClientToGroup() {
    if (dhcpClient != nullptr)
        if (opp_strcmp(dhcpClient->par("interface").stringValue(),
                par("proxyInterface").stringValue()) == 0)
            dhcpClient->par("interface").setStringValue(
                    par("groupInterface").stringValue());
}

void UDPWFDServiceDiscovery::setApIpAddress() {
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(
            par("interfaceTableModule"), this);
    const char *interfaceName = par("apInterface").stringValue();
    InterfaceEntry *ie = nullptr;
    if (strlen(interfaceName) > 0) {
        ie = ift->getInterfaceByName(interfaceName);
        if (ie == nullptr)
            throw cRuntimeError("Interface \"%s\" does not exist",
                    interfaceName);

        IPv4Address ip;
        string ipStr = "10." + myInfo.propsedSubnet + ".1";
        ip.set(ipStr.c_str());

        IPv4Address mask;
        mask.set("255.255.255.0");

        ie->ipv4Data()->setIPAddress(ip);
        ie->ipv4Data()->setNetmask(mask);
    }
}
void UDPWFDServiceDiscovery::setDhcpServerParams() {
    if (dhcpServer != nullptr) {
        string ipGateway = "10." + myInfo.propsedSubnet + ".1";
        string ipStr = "10." + myInfo.propsedSubnet + ".2";
        dhcpServer->par("ipAddressStart").setStringValue(ipStr.c_str());
        dhcpServer->par("subnetMask").setStringValue("255.255.255.0");
        dhcpServer->par("gateway").setStringValue(ipGateway.c_str());
    }
}

void UDPWFDServiceDiscovery::updateMyInfo(bool devInfoOnly) {
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

    if (!devInfoOnly) {
        mInfo.propsedSubnet = getConflictFreeSubnet();
        mInfo.conflictedSubnets = getPeersConflictedSubnets();

        if (apNic != nullptr) {
            mInfo.ssid = apNic->getSubmodule("mgmt")->par("ssid").str();
            mInfo.key = "";
        }
    }

    myInfo = mInfo;
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
        //return null when my rank is the best
        return nullptr;
    } else {
        return bestDevice;
    }
}

DeviceInfo *UDPWFDServiceDiscovery::getBestRankGO() {
    double bestRank = -1.0f;
    double curRank;
    DeviceInfo *bestDevice = nullptr;

    for (auto& pf : peersInfo) {
        //check if it is a GO (has ssid)
        if (pf.second.ssid.compare("") != 0) {
            curRank = getRank(pf.second);
            if (curRank > bestRank) {
                bestRank = curRank;
                bestDevice = &pf.second;
            }
        }
    }
    return bestDevice;
}

bool UDPWFDServiceDiscovery::noGoAround() {
    return getBestRankGO() == nullptr;
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
        if (pf.second.conflictedSubnets.find(myInfo.propsedSubnet)
                != string::npos)
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
