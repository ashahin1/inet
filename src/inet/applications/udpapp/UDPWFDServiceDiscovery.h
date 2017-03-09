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

#ifndef INET_APPLICATIONS_UDPAPP_UDPWFDSERVICEDISCOVERY_H_
#define INET_APPLICATIONS_UDPAPP_UDPWFDSERVICEDISCOVERY_H_

#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAP.h"
#include "inet/applications/udpapp/UDPBasicApp.h"
#include "inet/applications/common/ClipBoard.h"
#include "inet/applications/udpapp/ServiceDiscoveryPacket_m.h"
#include "inet/power/base/EpEnergyStorageBase.h"
#include "inet/common/lifecycle/LifecycleController.h"
#include "inet/common/wfd/GroupStatistics.h"

namespace inet {

using namespace inet::power;
using namespace std;
using namespace ieee80211;

class UDPWFDServiceDiscovery: public UDPBasicApp {
public:
    /**
     * The signal that is used to publish changes in number of
     * associated members
     */
    static simsignal_t membersChangedSignal;
    /**
     * The signal that is used to publish the delay between sending
     * a request and receiving its response.
     */
    static simsignal_t reqToRespDelaySignal;
protected:
    GroupStatistics *groupStatistics = nullptr;
    ClipBoard *clpBrd = nullptr;
    LifecycleController *lifeCycleCtrl = nullptr;
    int numResponseSent = 0;
    int numResponseRcvd = 0;
    int numRequestSent = 0;
    int numRequestRcvd = 0;
    int numResolvedIpConflicts = 0;
    int numOfTimesOrphaned = 0;
    int numOfTimesGO = 0;
    int numOfTimesGM = 0;
    int numOfTimesPM = 0;
    int numOfAssociatedMembers = 0;
    bool isGroupOwner = false;
    bool isOrphaned = false;
    bool isGroupMember = false;
    string myGoName = "";

    simtime_t declareGoPeriod;
    simtime_t selectGoPeriod;
    simtime_t switchDhcpPeriod;
    simtime_t tearDownPeriod;

    SubnetProposalTypes subnetProposalType;
    GoDeclarationTypes goDeclarationType;

    IInterfaceTable *ift = nullptr;
    EpEnergyStorageBase *energyStorage = nullptr;
    IEpEnergyGenerator *energyGenerator = nullptr;

    cModule *device = nullptr;
    cModule *dhcpClient = nullptr;
    cModule *dhcpServer = nullptr;
    cModule *tcpMgmtClientApp = nullptr;
    cModule *tcpMgmtSrvApp = nullptr;
    cModule *sdNic = nullptr;
    cModule *apNic = nullptr;
    cModule *p2pNic = nullptr;
    cModule *proxyNic = nullptr;

    Ieee80211MgmtAP *apMgmt = nullptr;

    cMessage *protocolMsg = nullptr;

    DevicesInfo peersInfo;
    DeviceInfo myInfo;
public:
    UDPWFDServiceDiscovery();
    virtual ~UDPWFDServiceDiscovery();

protected:
    virtual void sendPacket() override;
    virtual void processPacket(cPacket *msg) override;
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void processStart() override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void handleMessageWhenDown(cMessage *msg) override;
    UDPSocket::SendOptions* setDatagramOutInterface();
    virtual void refreshDisplay() const override;
    virtual bool handleNodeShutdown(IDoneCallback *doneCallback) override;

private:
    void turnModulesOff();
    void turnDhcpClientOn();
    void turnDhcpServerOn();
    void turnTcpMgmtClientAppOn();
    void turnTcpMgmtSrvAppOn();
    void turnApInterfaceOn();
    void turnP2pInterfaceOn();
    void turnProxyInterfaceOn();
    void changeP2pSSID(const char* ssid);
    void changeProxySSID(const char* ssid);
    void switchDhcpClientToProxy();
    void switchDhcpClientToGroup();
    void setApIpAddress();
    void setDhcpServerParams();
    void sendServiceDiscoveryPacket(bool isRequestPacket = true,
            bool isDeviceInfo = true, simtime_t orgSendTime = 0);
    void addDeviceInfoToPayLoad(ServiceDiscoveryResponseDeviceInfo *payload,
            simtime_t orgSendTime);
    void addSapInfoToPayLoad(ServiceDiscoveryResponseSapInfo *payload,
            simtime_t orgSendTime);
    void updateMyInfo(bool devInfoOnly);
    double getRank(bool isCharging, double Capacity, double level);
    double getRank(DeviceInfo pInfo);
    double getMyRank();
    DeviceInfo *getBestRankDevice();
    DeviceInfo *getBestRankGO();
    bool noGoAround();
    string proposeSubnet();
    bool subnetConflicting();
    string getConflictFreeSubnet();
    string getPeersConflictedSubnets();
    void addOrUpdatePeerDevInfo(int senderModuleId,
            ServiceDiscoveryResponseDeviceInfo* respDevInfo);
    void addOrUpdatePeerSapInfo(int senderModuleId,
            ServiceDiscoveryResponseSapInfo* respSapInfo);
    void resetDevice();
    bool myProposedGoNeedsUpdate();
    void clearInterfaceIpAddress(string ifNamePar);
    int getNumberOfMembers() const;
    void selectSubnetProposalType();
    void selectGoDeclarationType();
    bool shouldBeGO();
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_UDPAPP_UDPWFDSERVICEDISCOVERY_H_ */
