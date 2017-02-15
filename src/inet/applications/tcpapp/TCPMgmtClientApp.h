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

#ifndef INET_APPLICATIONS_TCPAPP_TCPMGMTCLIENTAPP_H_
#define INET_APPLICATIONS_TCPAPP_TCPMGMTCLIENTAPP_H_

#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/applications/common/ClipBoard.h"
#include "inet/applications/tcpapp/TCPBasicClientApp.h"

namespace inet {

class TCPMgmtClientApp: public TCPBasicClientApp {
protected:
    string status = "";
    string myGoSSID = "";
    bool nextMsgIsPxAssignment = false;

    ClipBoard *clpBrd = nullptr;
    IInterfaceTable *ift = nullptr;
    cModule *sdNic = nullptr;
    cModule *p2pNic = nullptr;
    cModule *pxNic = nullptr;
    cModule *device = nullptr;

    cMessage *ttlMsg = nullptr;

    HeartBeatMap heartBeatMap;
    HeartBeatRecord myHeartBeatRecord;
public:
    TCPMgmtClientApp();
    virtual ~TCPMgmtClientApp();

protected:
    virtual void initialize(int stage) override;
    virtual bool handleOperationStage(LifecycleOperation *operation, int stage,
            IDoneCallback *doneCallback) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleTimer(cMessage *msg) override;
    virtual void sendRequest() override;
    virtual void socketDataArrived(int connId, void *ptr, cPacket *msg,
            bool urgent) override;
    virtual void refreshDisplay() const override;
    virtual void setStatusString(const char *s) override;

private:
    void initMyHeartBeatRecord();
    void decreasePeersTtl();
    int removeZeroTtl();
    void setConnectAddressToGoIP();
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_TCPAPP_TCPMGMTCLIENTAPP_H_ */
