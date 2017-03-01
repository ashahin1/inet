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

#ifndef INET_APPLICATIONS_COMMON_CLIPBOARD_H_
#define INET_APPLICATIONS_COMMON_CLIPBOARD_H_

#include <omnetpp/csimplemodule.h>
#include <vector>

#include "inet/applications/tcpapp/HeartBeatMsg_m.h"

namespace inet {

enum ProtocolStates {
    PROTOCOL_START = 1000,
    DECLARE_GO = 1001,
    SELECT_GO = 1002,
    SET_PROXY_DHCP = 1003,
    PROTOCOL_TEARDOWN = 1004
};

enum SubnetProposalTypes {
    SPT_ISNP = 1010, SPT_NO_CONFLICT_DETECTION = 1011
};

enum GoDeclarationTypes {
    GDT_EMC = 1020, GDT_EMC_TWO_HOP = 1021, GDT_RANDOM = 1022
};

enum ProxyAssignmentTypes {
    PAT_MUNKRES = 1030, PAT_FIRST_AVAILABLE = 1031
};

static const int TTL_MSG = 9999;

struct DeviceInfo {
public:
    int deviceId = -1;
    double batteryCapacity = -1.0f;
    double batteryLevel = -1;
    bool isCharging = false;
    string proposedSubnet = "-";
    string conflictedSubnets = "";
    string ssid = "";
    string key = "";
    int proposedGO = -1;
};
typedef std::map<int, DeviceInfo> DevicesInfo;

class ClipBoard: public omnetpp::cSimpleModule {
public:
    cMessage *protocolMsg;
    bool *isGroupOwner;
protected:
    HeartBeatMap *heartBeatMapClient;
    HeartBeatMap *heartBeatMapServer;
    DevicesInfo *peersInfo;

    string proxy_ssid = "";

    double rankAlpha;
    double rankBeta;
    double rankGamma;
    double maxBatteryCapacity;
public:
    ClipBoard();
    virtual ~ClipBoard();
    HeartBeatMap *getHeartBeatMapClient();
    void setHeartBeatMapClient(HeartBeatMap *heartBeatMap);
    HeartBeatMap *getHeartBeatMapServer();
    void setHeartBeatMapServer(HeartBeatMap *heartBeatMap);
    DevicesInfo *getPeersInfo();
    void setPeersInfo(DevicesInfo* peersInfo);
    double getRank(bool isCharging, double Capacity, double level);
    double getRank(DeviceInfo pInfo);
    const string& getProxySsid() const;
    void setProxySsid(const string& proxySsid = "");

protected:
    virtual void initialize(int stage) override;
    virtual void refreshDisplay() const override;
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_COMMON_CLIPBOARD_H_ */
