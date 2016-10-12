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

static const int TTL_MSG = 9999;

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
typedef std::map<int, DeviceInfo> DevicesInfo;

class ClipBoard: public omnetpp::cSimpleModule {
public:
    cMessage *protocolMsg;
    bool *isGroupOwner;
protected:
    HeartBeatMap *heartBeatMap;
    DevicesInfo *peersInfo;
public:
    ClipBoard();
    virtual ~ClipBoard();
    HeartBeatMap *getHeartBeatMap();
    void setHeartBeatMap(HeartBeatMap *heartBeatMap);
    DevicesInfo *getPeersInfo();
    void setPeersInfo(DevicesInfo* peersInfo);
protected:
    virtual void initialize(int stage) override;
    virtual void refreshDisplay() const override;
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_COMMON_CLIPBOARD_H_ */
