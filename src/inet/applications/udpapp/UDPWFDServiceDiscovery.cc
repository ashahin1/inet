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
using namespace ieee80211;

Define_Module(UDPWFDServiceDiscovery);

simsignal_t UDPWFDServiceDiscovery::membersChangedSignal =
        cComponent::registerSignal("membersChanged");

simsignal_t UDPWFDServiceDiscovery::endToEndDelaySignal =
        cComponent::registerSignal("endToEndDelay");

UDPWFDServiceDiscovery::UDPWFDServiceDiscovery() {
    // TODO Auto-generated constructor stub

}

UDPWFDServiceDiscovery::~UDPWFDServiceDiscovery() {
    // TODO Auto-generated destructor stub
    cancelAndDelete(protocolMsg);
}

void UDPWFDServiceDiscovery::finish() {
    recordScalar("Resolved IP Conflicts", numResolvedIpConflicts);
    recordScalar("Request Packets Received", numRequestRcvd);
    recordScalar("Response Packets Received", numResponseRcvd);
    recordScalar("Request Packets Sent", numRequestSent);
    recordScalar("Response Packets Sent", numResponseSent);
    recordScalar("Num of Times GO", numOfTimesGO);
    recordScalar("Num of Times GM", numOfTimesGM);
    recordScalar("Num of Times PM", numOfTimesPM);
    recordScalar("Num of Times Orphaned", numOfTimesOrphaned);

    UDPBasicApp::finish();
}

void UDPWFDServiceDiscovery::initialize(int stage) {
    UDPBasicApp::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        groupStatistics = getModuleFromPar<GroupStatistics>(
                par("groupStatisticsModule"), this);
        clpBrd = getModuleFromPar<ClipBoard>(par("clipBoardModule"), this);
        if (clpBrd != nullptr) {
            clpBrd->setPeersInfo(&this->peersInfo);
        }
        lifeCycleCtrl = getModuleFromPar<LifecycleController>(
                par("lifeCycleControllerModule"), this);
        device = getContainingNode(this);
        dhcpClient = device->getModuleByPath(
                par("dhcpClientAppName").stringValue());
        dhcpServer = device->getModuleByPath(
                par("dhcpServerAppName").stringValue());
        tcpMgmtClientApp = device->getModuleByPath(
                par("tcpMgmtClientAppName").stringValue());
        sdNic = device->getModuleByPath(par("sdNicName").stringValue());
        apNic = device->getModuleByPath(par("ApNicName").stringValue());
        if (apNic != nullptr) {
            apMgmt =
                    dynamic_cast<Ieee80211MgmtAP *>(apNic->getSubmodule("mgmt"));
        }
        p2pNic = device->getModuleByPath(par("p2pNicName").stringValue());
        proxyNic = device->getModuleByPath(par("proxyNicName").stringValue());

        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"),
                this);

        energyStorage = dynamic_cast<EnergyStorageBase *>(device->getSubmodule(
                "energyStorage"));
        energyGenerator = dynamic_cast<IEnergyGenerator *>(device->getSubmodule(
                "energyGenerator"));

        declareGoPeriod = par("declareGoPeriod");
        selectGoPeriod = par("selectGoPeriod");
        switchDhcpPeriod = par("switchDhcpPeriod");
        tearDownPeriod = par("tearDownPeriod");

        selectSubnetProposalType();
        selectGoDeclarationType();

        updateMyInfo(false);
        myInfo.deviceId = device->getId();

        protocolMsg = new cMessage("Protocol Message");
        clpBrd->protocolMsg = protocolMsg;
        clpBrd->isGroupOwner = &isGroupOwner;

        WATCH(myInfo.proposedSubnet);
        WATCH(isGroupOwner);

        emit(membersChangedSignal, numOfAssociatedMembers);
    }
}

void UDPWFDServiceDiscovery::selectSubnetProposalType() {
    string sType = par("subnetProposalType").stringValue();

    if (sType.compare("ISNP") == 0) {
        subnetProposalType = SubnetProposalTypes::SPT_ISNP;
    } else if (sType.compare("NO_CONFLICT_DETECTION") == 0) {
        subnetProposalType = SubnetProposalTypes::SPT_NO_CONFLICT_DETECTION;
    } else {
        subnetProposalType = SubnetProposalTypes::SPT_ISNP;
    }
}

void UDPWFDServiceDiscovery::selectGoDeclarationType() {
    string gType = par("goDeclarationMethod").stringValue();

    if (gType.compare("EMC") == 0) {
        goDeclarationType = GoDeclarationTypes::GDT_EMC;
    } else if (gType.compare("EMC_TWO_HOP") == 0) {
        goDeclarationType = GoDeclarationTypes::GDT_EMC_TWO_HOP;
    } else if (gType.compare("RANDOM") == 0) {
        goDeclarationType = GoDeclarationTypes::GDT_RANDOM;
    } else {
        goDeclarationType = GoDeclarationTypes::GDT_EMC_TWO_HOP;
    }
}

int UDPWFDServiceDiscovery::getNumberOfMembers() const {
    int numGMs = (apMgmt ? apMgmt->getNumOfStation() : 0);
    return numGMs;
}

void UDPWFDServiceDiscovery::refreshDisplay() const {
    char buf[80];
    sprintf(buf,
            "rcvd: %d sent: %d\nrcvdReq: %d sntReq: %d\nrcvdRes: %d sntRes: %d",
            numReceived, numSent, numRequestRcvd, numRequestSent,
            numResponseRcvd, numResponseSent);
    getDisplayString().setTagArg("t", 0, buf);

    char buf2[80];
    if (isGroupOwner) {
        int numGMs = getNumberOfMembers();
        sprintf(buf2, "GO (%d)", numGMs);
    } else {
        if (isOrphaned) {
            sprintf(buf2, "Orph");
        } else if (isGroupMember) {
            sprintf(buf2, "GM->%s", myGoName.c_str());
        } else {
            sprintf(buf2, "??");
        }
    }

    char buff[100];
    sprintf(buff, "r=%f, pGo=%d, id=%d",
            const_cast<UDPWFDServiceDiscovery*>(this)->getMyRank(),
            myInfo.proposedGO, myInfo.deviceId);
    sprintf(buf2, "%s\n%s", buf2, buff);

    device->getDisplayString().setTagArg("t", 0, buf2);
}

void UDPWFDServiceDiscovery::resetDevice() {
    //reset every thing
    isGroupOwner = false;
    isOrphaned = false;
    isGroupMember = false;
    myGoName = "";
    peersInfo.clear();
    //myInfo = DeviceInfo();
    updateMyInfo(false);

    //turn other modules off (dhcp, wlan, etc)
    turnModulesOff();

    //Make sure we clear the heartbeatmaps that are in the TCPMgmtSrvApp and the TCPMgmtClientApp
    //This should be done by the tcp apps themselves, but due to the inability of the server app
    //to do lifecysles operations in the time being, I sticked with this solution
    if (clpBrd != nullptr) {
        HeartBeatMap* map = clpBrd->getHeartBeatMapClient();
        if (map)
            map->clear();

        map = clpBrd->getHeartBeatMapServer();
        if (map)
            map->clear();

        clpBrd->setProxySsid("");
    }

}

void UDPWFDServiceDiscovery::processStart() {
    UDPBasicApp::processStart();

    //reset every thing
    resetDevice();
    protocolMsg->setKind(DECLARE_GO);
    scheduleAt(simTime() + declareGoPeriod, protocolMsg);
}

bool UDPWFDServiceDiscovery::handleNodeShutdown(IDoneCallback* doneCallback) {
    UDPBasicApp::handleNodeShutdown(doneCallback);

    if (protocolMsg)
        cancelEvent(protocolMsg);

    return true;
}

bool UDPWFDServiceDiscovery::shouldBeGO() {
    bool isGo = false;

    switch (goDeclarationType) {
    case GoDeclarationTypes::GDT_EMC:
        isGo = (getBestRankDevice() == nullptr);
        break;
    case GoDeclarationTypes::GDT_EMC_TWO_HOP:
        isGo = (myInfo.deviceId == myInfo.proposedGO);
        break;
    case GoDeclarationTypes::GDT_RANDOM:
        isGo = (intrand(2, 0) % 2 == 1);
        break;
    default:
        break;
    }

    return isGo;
}

void UDPWFDServiceDiscovery::handleMessageWhenUp(cMessage* msg) {
    if (msg->isSelfMessage()) {
        if (msg->getKind() == DECLARE_GO) {
            //clear the ips from the previous run (in case this is a new run after teardown)
            clearInterfaceIpAddress("groupInterface");
            clearInterfaceIpAddress("proxyInterface");
            //Compare the rank to the collected ones
            //if my rank is the best
            //  start the wlan1 (AP)
            //  set the ipadress to the proposedOne
            //  start dhcp server and change its parameters to match the proposed subnet
            //  declare my self as GO to start sending SAP info
            //
            updateMyInfo(true);
            isGroupOwner = shouldBeGO();
            if (isGroupOwner) {
                if (subnetConflicting())
                    getConflictFreeSubnet();
                turnApInterfaceOn();
                setApIpAddress();
                setDhcpServerParams();
                turnDhcpServerOn();
                numOfTimesGO++;

                //Add an entry for stats collection
                if (groupStatistics) {
                    groupStatistics->addGO(myInfo.deviceId, myInfo.ssid);
                    groupStatistics->addSubnet(myInfo.proposedSubnet);
                }
            }

            protocolMsg->setKind(SELECT_GO);
            scheduleAt(simTime() + selectGoPeriod, protocolMsg);
        } else if (msg->getKind() == SELECT_GO) {
            //start wlan2 and set its target ssid
            //start dhcp client and set its intf to wlan2
            if (!isGroupOwner) {
                DeviceInfo *bestGo = getBestRankGO();
                if (bestGo != nullptr) {
                    isGroupMember = true;
                    changeP2pSSID(bestGo->ssid.c_str());
                    turnP2pInterfaceOn();
                    switchDhcpClientToGroup();
                    turnDhcpClientOn();
                    turnTcpMgmtClientAppOn();
                    numOfTimesGM++;

                    //Add an entry for stats collection
                    if (groupStatistics) {
                        groupStatistics->addGM(myInfo.deviceId, bestGo->ssid);
                    }
                } else {
                    EV_INFO << "Orphaned Device Found";
                    isOrphaned = true;
                    numOfTimesOrphaned++;
                    if (groupStatistics) {
                        groupStatistics->addOrph(myInfo.deviceId);
                    }
                }
            }

            protocolMsg->setKind(SET_PROXY_DHCP);
            scheduleAt(simTime() + switchDhcpPeriod, protocolMsg);
        } else if (msg->getKind() == SET_PROXY_DHCP) {
            //start wlan3 and set its target ssid
            //switch dhcp client to wlan3 intf
            if (!isGroupOwner) {
                if (!noGoAround()) {
                    //changeProxySSID("xyz");
                    //Check if an ssid is assigned by the GO.
                    //if no ssid, it means that this GM is not working as a proxy.
                    string pSsid = clpBrd->getProxySsid();
                    if (pSsid.compare("") != 0) {
                        changeProxySSID(pSsid.c_str());
                        turnProxyInterfaceOn();
                        switchDhcpClientToProxy();
                        numOfTimesPM++;

                        //Add an entry for stats collection
                        if (groupStatistics) {
                            groupStatistics->addPM(myInfo.deviceId, pSsid);
                        }
                    }
                }
            }

            protocolMsg->setKind(PROTOCOL_TEARDOWN);
            scheduleAt(simTime() + tearDownPeriod, protocolMsg);
        } else if (msg->getKind() == PROTOCOL_TEARDOWN) {
            resetDevice();

            protocolMsg->setKind(DECLARE_GO);
            scheduleAt(simTime() + declareGoPeriod, protocolMsg);
        } else {
            UDPBasicApp::handleMessageWhenUp(msg);
        }

        if (isGroupOwner) {
            int mCount = getNumberOfMembers();
            if (numOfAssociatedMembers != mCount) {
                numOfAssociatedMembers = mCount;
                emit(membersChangedSignal, numOfAssociatedMembers);
            }
        }

    } else {
        UDPBasicApp::handleMessageWhenUp(msg);
    }
}

UDPSocket::SendOptions* UDPWFDServiceDiscovery::setDatagramOutInterface() {
    //Set the output interface for the datagram to let the broadcast to pass
    UDPSocket::SendOptions* sndOpt = new UDPSocket::SendOptions();

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

    //Declare that the sender is this device
    int sId = myInfo.deviceId;
    payload->setSenderId(sId);

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
    //If this device is a group owner, then it should not sent any discovery requests.
    if (!isGroupOwner) {
        //Stop the requests for GMs after the selectingGo phase.
        //We her check the scheduled message that define the next phase
        //Once the Go selection is done we schedule the message for Setting the proxy
        //Of course this means that we need here to check that the message kind is SET_PROXY_DHCP
        //In addition, we need to make sure that no packets are send after proxy selection and until
        //the teardown signal is sent, so we need to check for PROTOCOL_TEARDOWN msg kind also.
        if (protocolMsg != nullptr)
            if ((protocolMsg->getKind() == SET_PROXY_DHCP)
                    || (protocolMsg->getKind() == PROTOCOL_TEARDOWN))
                return;

        //Send discovery requests per the defined schedule
        sendServiceDiscoveryPacket();
    }
}

void UDPWFDServiceDiscovery::addOrUpdatePeerDevInfo(int senderId,
        ServiceDiscoveryResponseDeviceInfo* respDevInfo) {

    DeviceInfo pInfo;
    pInfo.deviceId = senderId;
    pInfo.batteryCapacity = respDevInfo->getBatteryCapacity();
    pInfo.batteryLevel = respDevInfo->getBatteryLevel();
    pInfo.isCharging = respDevInfo->getIsCharging();
    pInfo.proposedSubnet = respDevInfo->getProposedSubnet();
    pInfo.conflictedSubnets = respDevInfo->getConflictedSubnets();
    pInfo.proposedGO = respDevInfo->getProposedGO();
    if (peersInfo.count(senderId) > 0) {
        DeviceInfo* pf = &peersInfo[senderId];
        pf->batteryCapacity = pInfo.batteryCapacity;
        pf->batteryLevel = pInfo.batteryLevel;
        pf->isCharging = pInfo.isCharging;
        pf->proposedSubnet = pInfo.proposedSubnet;
        pf->conflictedSubnets = pInfo.conflictedSubnets;
        pf->proposedGO = pInfo.proposedGO;
    } else {
        peersInfo[senderId] = pInfo;
    }
}

void UDPWFDServiceDiscovery::addOrUpdatePeerSapInfo(int senderId,
        ServiceDiscoveryResponseSapInfo* respSapInfo) {

    DeviceInfo pInfo;
    pInfo.deviceId = senderId;
    pInfo.ssid = respSapInfo->getSsid();
    pInfo.key = respSapInfo->getKey();
    if (peersInfo.count(senderId) > 0) {
        DeviceInfo* pf = &peersInfo[senderId];
        pf->ssid = pInfo.ssid;
        pf->key = pInfo.key;
    } else {
        peersInfo[senderId] = pInfo;
    }
}

void UDPWFDServiceDiscovery::processPacket(cPacket *pk) {
    emit(rcvdPkSignal, pk);
    EV_INFO << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk)
                   << endl;

    int senderId = 0;

    if (ServiceDiscoveryPacket *sdPk =
            dynamic_cast<ServiceDiscoveryPacket *>(pk)) {
        senderId = sdPk->getSenderId();
    }

    if (ServiceDiscoveryRequest *sdReq =
            dynamic_cast<ServiceDiscoveryRequest*>(pk)) {
        //A request is received from a nearby device, so we send a response.
        updateMyInfo(false); //update myInfo to be used in the next calculation. It also checks for conflicts in subnets
        if (isGroupOwner || (protocolMsg->getKind() == DECLARE_GO)) {
            sendServiceDiscoveryPacket(false, true, sdReq->getOrgSendTime());
        }
        //We need to halt responding with SapInfo until we make sure that other devices are selecting their Groups
        if (isGroupOwner && (protocolMsg->getKind() == SELECT_GO)) {
            sendServiceDiscoveryPacket(false, false, sdReq->getOrgSendTime());
        }
        numRequestRcvd++;
    } else {
        simtime_t eed = 0;
        if (ServiceDiscoveryResponseDeviceInfo *respDevInfo =
                dynamic_cast<ServiceDiscoveryResponseDeviceInfo *>(pk)) {

            addOrUpdatePeerDevInfo(senderId, respDevInfo);

            updateMyInfo(false);
            eed = simTime() - respDevInfo->getOrgSendTime();
        } else if (ServiceDiscoveryResponseSapInfo *respSapInfo =
                dynamic_cast<ServiceDiscoveryResponseSapInfo *>(pk)) {

            addOrUpdatePeerSapInfo(senderId, respSapInfo);
            eed = simTime() - respSapInfo->getOrgSendTime();
        }

        if (eed != 0) {
            emit(endToEndDelaySignal, eed);
            numResponseRcvd++;
        }
    }
    delete pk;
    numReceived++;
}

void UDPWFDServiceDiscovery::clearInterfaceIpAddress(string ifNamePar) {
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(
            par("interfaceTableModule"), this);
    const char *ifName = par(ifNamePar.c_str()).stringValue();

    InterfaceEntry *ie = nullptr;

    if (strlen(ifName) > 0) {
        ie = ift->getInterfaceByName(ifName);
        if (ie == nullptr)
            throw cRuntimeError("Interface \"%s\" does not exist", ifName);

        IPv4Address ip;
        IPv4Address mask;

        ie->ipv4Data()->setIPAddress(ip);
        ie->ipv4Data()->setNetmask(mask);
    }
}

void UDPWFDServiceDiscovery::turnModulesOff() {
    if (lifeCycleCtrl != nullptr) {
        lifeCycleCtrl->processDirectCommand(dhcpClient, false);
        lifeCycleCtrl->processDirectCommand(dhcpServer, false);
        lifeCycleCtrl->processDirectCommand(tcpMgmtClientApp, false);
        lifeCycleCtrl->processDirectCommand(apNic, false);
        lifeCycleCtrl->processDirectCommand(p2pNic, false);
        lifeCycleCtrl->processDirectCommand(proxyNic, false);
        changeP2pSSID("DIRECT-XYZXYZ");
        changeProxySSID("DIRECT-XXXXXXXX");
    }
}

void UDPWFDServiceDiscovery::turnDhcpClientOn() {
    lifeCycleCtrl->processDirectCommand(dhcpClient, true);
}

void UDPWFDServiceDiscovery::turnDhcpServerOn() {
    lifeCycleCtrl->processDirectCommand(dhcpServer, true);
}

void UDPWFDServiceDiscovery::turnTcpMgmtClientAppOn() {
    lifeCycleCtrl->processDirectCommand(tcpMgmtClientApp, true);
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
    //Exit if no proxyInterface is defined, which means we do not have a NIC for proxy in this simScenario
    if (opp_strcmp(par("proxyInterface").stringValue(), "") == 0) {
        return;
    }

    if (dhcpClient != nullptr)
        if (opp_strcmp(dhcpClient->par("interface").stringValue(),
                par("groupInterface").stringValue()) == 0)
            dhcpClient->par("interface").setStringValue(
                    par("proxyInterface").stringValue());
}

void UDPWFDServiceDiscovery::switchDhcpClientToGroup() {
    //Exit if no groupInterface is defined, which means we do not have a NIC for p2p in this simScenario
    if (opp_strcmp(par("groupInterface").stringValue(), "") == 0) {
        return;
    }

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
        string ipStr = "10." + myInfo.proposedSubnet + ".1";
        ip.set(ipStr.c_str());

        IPv4Address mask;
        mask.set("255.255.255.0");

        ie->ipv4Data()->setIPAddress(ip);
        ie->ipv4Data()->setNetmask(mask);
    }
}
void UDPWFDServiceDiscovery::setDhcpServerParams() {
    if (dhcpServer != nullptr) {
        string ipGateway = "10." + myInfo.proposedSubnet + ".1";
        string ipStr = "10." + myInfo.proposedSubnet + ".2";
        dhcpServer->par("ipAddressStart").setStringValue(ipStr.c_str());
        dhcpServer->par("subnetMask").setStringValue("255.255.255.0");
        dhcpServer->par("gateway").setStringValue(ipGateway.c_str());
    }
}

void UDPWFDServiceDiscovery::updateMyInfo(bool devInfoOnly) {
    //myInfo.deviceId is updated in initialize;
    if (energyStorage != nullptr) {
        myInfo.batteryCapacity = energyStorage->getNominalCapacity().get();
        myInfo.batteryLevel = energyStorage->getResidualCapacity().get()
                / myInfo.batteryCapacity;
    }
    double genPower = 0;
    if (energyGenerator != nullptr) {
        genPower = energyGenerator->getPowerGeneration().get();
    }
    myInfo.isCharging = !(genPower == 0);

    if (!devInfoOnly) {
        if (myInfo.proposedSubnet.compare("-") == 0) {
            myInfo.proposedSubnet = proposeSubnet();
        } else if (subnetConflicting()) {
            myInfo.proposedSubnet = getConflictFreeSubnet();
        }
        myInfo.conflictedSubnets = getPeersConflictedSubnets();

        if (apNic != nullptr) {
            myInfo.ssid =
                    apNic->getSubmodule("mgmt")->par("ssid").stringValue();
            //Passphrases in INet are not implemented, so keep the key empty for now
            myInfo.key = "";
        }

        if (myProposedGoNeedsUpdate()) {
            //Now update my proposed GO. It will be send to other devices
            //So they could know that I have another one who is better than me.
            DeviceInfo *bstDev = getBestRankDevice();
            myInfo.proposedGO = (
                    bstDev == nullptr ? myInfo.deviceId : bstDev->deviceId);
        }
    }

    ASSERT(myInfo.proposedSubnet.compare("-") != 0);
}

bool UDPWFDServiceDiscovery::myProposedGoNeedsUpdate() {
    //For sack of comparison, we added the original EMC selection behavior and a RANDOM one.
    //Thus, if we have any of these modes we do not need to update our proposed GO
    if ((goDeclarationType == GoDeclarationTypes::GDT_EMC)
            || (goDeclarationType == GoDeclarationTypes::GDT_RANDOM)) {
        return false;
    }

    if (myInfo.proposedGO == -1) {
        return true;
    }

    if ((peersInfo.count(myInfo.proposedGO) > 0)
            && (peersInfo[myInfo.proposedGO].deviceId
                    == peersInfo[myInfo.proposedGO].proposedGO)) {
        return false;
    } else {
        return true;
    }
}

void UDPWFDServiceDiscovery::addDeviceInfoToPayLoad(
        ServiceDiscoveryResponseDeviceInfo* payload, simtime_t orgSendTime) {
    payload->setOrgSendTime(orgSendTime);
    payload->setProposedGO(myInfo.proposedGO);
    payload->setProposedSubnet(myInfo.proposedSubnet.c_str());
    payload->setConflictedSubnets(myInfo.conflictedSubnets.c_str());

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
    return clpBrd->getRank(isCharging, Capacity, level);
}

double UDPWFDServiceDiscovery::getRank(DeviceInfo pInfo) {
    return clpBrd->getRank(pInfo.isCharging, pInfo.batteryCapacity,
            pInfo.batteryLevel);
}

double UDPWFDServiceDiscovery::getMyRank() {
    return getRank(myInfo);
}

DeviceInfo *UDPWFDServiceDiscovery::getBestRankDevice() {
    double bestRank = -1.0f;
    double myRank = getMyRank();
    double curRank;
    DeviceInfo *bestDevice = nullptr;

    //For sack of comparison, we need to keep the old behavior of EMC as a choice
    bool oldEMC = (goDeclarationType == GoDeclarationTypes::GDT_EMC);
    bool newEMC = (goDeclarationType == GoDeclarationTypes::GDT_EMC_TWO_HOP);

    for (auto& pf : peersInfo) {
        //In case of EMC_TWO HOP, check first if the device does not have another device that he sees that
        //has a rank better than him.

        //In case of EMC, we do not need to perform such a check
        if ((newEMC && (pf.second.proposedGO == pf.second.deviceId))
                || oldEMC) {
            curRank = getRank(pf.second);
            if (curRank > bestRank) {
                bestRank = curRank;
                bestDevice = &pf.second;
            }
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
                myGoName = cSimulation::getActiveSimulation()->getModule(
                        pf.first)->getFullName();
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
    //For the sack of comparing with a baseline, we added a no conflict detection option
    if (subnetProposalType == SubnetProposalTypes::SPT_ISNP) {
        for (auto& pf1 : peersInfo) {
            for (auto& pf2 : peersInfo) {
                if (pf1.first != pf2.first) {
                    if (pf1.second.proposedSubnet.compare(
                            pf2.second.proposedSubnet) == 0) {
                        cfStr += pf1.second.proposedSubnet + ";";
                    }
                }
            }
        }
    }
    return cfStr;
}

bool UDPWFDServiceDiscovery::subnetConflicting() {
    //For the sack of comparing with a baseline, we added a no conflict detection option
    if (subnetProposalType == SubnetProposalTypes::SPT_ISNP) {
        for (auto& pf : peersInfo) {
            //Check if my subnet is in the detected conflicts of other devices
            if (pf.second.conflictedSubnets.find(myInfo.proposedSubnet)
                    != string::npos)
                return true;

            //Check if my subnet is conflicting with proposed subnets of other devices
            if (pf.second.proposedSubnet.compare(myInfo.proposedSubnet) == 0)
                return true;
        }
    } else if (subnetProposalType
            == SubnetProposalTypes::SPT_NO_CONFLICT_DETECTION) {
        return false;
    }

    return false;
}

string UDPWFDServiceDiscovery::getConflictFreeSubnet() {
    string proSubnet = proposeSubnet();
    while (subnetConflicting()) {
        proSubnet = proposeSubnet();
        myInfo.proposedSubnet = proSubnet;
        numResolvedIpConflicts++;
    }
    return proSubnet;
}

} /* namespace inet */
