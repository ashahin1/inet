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

namespace inet {

using namespace inet::power;
using namespace std;

struct DeviceInfo {
public:
    double batteryCapacity = -1.0f;
    double batteryLevel = -1;
    bool isCharging = false;
    string propsedSubnet = "-";
    string conflictedSubnets = "";
    string ssid = "";
    string key = "";
};

class UDPWFDServiceDiscovery: public UDPBasicApp {
protected:
    ClipBoard *clpBrd = nullptr;
    int numResponseSent = 0;
    int numResponseRcvd = 0;
    int numRequestSent = 0;
    int numRequestRcvd = 0;
    int numIpConflicts = 0;
    bool isGroupOwner = false;
//    string proposedSubnet = "";
//    string conflictedSubnets = "";
//    string sapSSID = "";
//    string sapKey = "";

    EnergyStorageBase *energyStorage = nullptr;
    IEnergyGenerator *energyGenerator = nullptr;
    cModule *dhcpClient = nullptr;
    cModule *dhcpServer = nullptr;
    cModule *apNic = nullptr;

    cOutVector endToEndDelayVec;

    map<int, DeviceInfo> peersInfo;
    DeviceInfo myInfo;
public:
    UDPWFDServiceDiscovery();
    virtual ~UDPWFDServiceDiscovery();
    virtual void sendPacket() override;
    virtual void processPacket(cPacket *msg) override;
    virtual void initialize(int stage) override;

private:
    void switchDhcpClientToProxy();
    void switchDhcpClientToGroup();
    void sendServiceDiscoveryPacket(bool isRequestPacket = true,
            bool isDeviceInfo = true, simtime_t orgSendTime = 0);
    void addDeviceInfoToPayLoad(ServiceDiscoveryResponseDeviceInfo *payload,
            simtime_t orgSendTime);
    void addSapInfoToPayLoad(ServiceDiscoveryResponseSapInfo *payload,
            simtime_t orgSendTime);
    DeviceInfo getMyInfo();
    double getRank(bool isCharging, double Capacity, double level);
    double getRank(DeviceInfo pInfo);
    double getMyRank();
    DeviceInfo *getBestRankDevice();
    string proposeSubnet();
    bool subnetConflicting();
    string getConflictFreeSubnet();
    string getPeersConflictedSubnets();
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_UDPAPP_UDPWFDSERVICEDISCOVERY_H_ */
