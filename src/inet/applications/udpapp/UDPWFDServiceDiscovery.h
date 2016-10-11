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

#include "inet/applications/udpapp/UDPBasicApp.h"
#include "inet/applications/common/ClipBoard.h"
#include "inet/applications/udpapp/ServiceDiscoveryPacket_m.h"
#include "inet/power/base/EnergyStorageBase.h"
#include "inet/common/lifecycle/LifecycleController.h"

namespace inet {

using namespace inet::power;
using namespace std;

class UDPWFDServiceDiscovery: public UDPBasicApp {
protected:
    ClipBoard *clpBrd = nullptr;
    LifecycleController *lifeCycleCtrl = nullptr;
    int numResponseSent = 0;
    int numResponseRcvd = 0;
    int numRequestSent = 0;
    int numRequestRcvd = 0;
    int numIpConflicts = 0;
    int numOfTimesOrphaned = 0;
    bool isGroupOwner = false;

    simtime_t declareGoPeriod;
    simtime_t selectGoPeriod;
    simtime_t switchDhcpPeriod;
    simtime_t tearDownPeriod;

    IInterfaceTable *ift = nullptr;
    EnergyStorageBase *energyStorage = nullptr;
    IEnergyGenerator *energyGenerator = nullptr;
    cModule *dhcpClient = nullptr;
    cModule *dhcpServer = nullptr;
    cModule *tcpMgmtClientApp = nullptr;
    cModule *apNic = nullptr;
    cModule *p2pNic = nullptr;
    cModule *proxyNic = nullptr;

    cMessage *protocolMsg = nullptr;

    cOutVector endToEndDelayVec;

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
    virtual void handleMessageWhenUp(cMessage *msg); // override;
    UDPSocket::SendOptions* setDatagramOutInterface();

private:
    void turnModulesOff();
    void turnDhcpClientOn();
    void turnDhcpServerOn();
    void turnTcpMgmtClientAppOn();
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
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_UDPAPP_UDPWFDSERVICEDISCOVERY_H_ */
