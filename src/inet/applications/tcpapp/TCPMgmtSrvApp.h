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

#ifndef INET_APPLICATIONS_TCPAPP_TCPMGMTSRVAPP_H_
#define INET_APPLICATIONS_TCPAPP_TCPMGMTSRVAPP_H_

#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/applications/common/ClipBoard.h"
#include "inet/applications/tcpapp/TCPGenericSrvApp.h"

namespace inet {

class TCPMgmtSrvApp: public TCPGenericSrvApp {
protected:
    ClipBoard *clpBrd = nullptr;
    IInterfaceTable *ift = nullptr;
    cModule *sdNic = nullptr;

    cMessage *ttlMsg = nullptr;

    HeartBeatMap heartBeatMap;
    HeartBeatRecord myHeartBeatRecord;
public:
    TCPMgmtSrvApp();
    virtual ~TCPMgmtSrvApp();
protected:
    virtual void initialize(int stage) override;
    virtual void sendBack(cMessage *msg) override;
    virtual void sendOrSchedule(cMessage *msg, simtime_t delay) override;
    virtual void handleMessage(cMessage *msg) override;

private:
    void initMyHeartBeatRecord();
    void decreasePeersTtl();
    void removeZeroTtl();
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_TCPAPP_TCPMGMTSRVAPP_H_ */
