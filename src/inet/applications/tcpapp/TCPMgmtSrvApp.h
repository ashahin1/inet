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

#include "/home/ahmed/or-tools/include/algorithms/hungarian.h"

namespace inet {

using namespace std;
typedef map<string, int> PxAssignment;

class TCPMgmtSrvApp: public TCPGenericSrvApp, public cListener {
public:
    static simsignal_t rcvdMSPkSignal;
    static simsignal_t sentMSPkSignal;
protected:
    ClipBoard *clpBrd = nullptr;
    IInterfaceTable *ift = nullptr;
    cModule *sdNic = nullptr;
    cModule *apNic = nullptr;
    cModule *device = nullptr;

    ProxyAssignmentTypes proxyAssignmentType;

    cMessage *ttlMsg = nullptr;

    DevicesInfo *peersInfo;

    HeartBeatMap heartBeatMap;
    HeartBeatRecord myHeartBeatRecord;
    PxAssignment pxAssignment;

    string mySSID = "";

    bool isOperational = true;
    bool socketListening = true;
public:
    TCPMgmtSrvApp();
    virtual ~TCPMgmtSrvApp();
protected:
    virtual void initialize(int stage) override;
    virtual void sendBack(cMessage *msg) override;
    virtual void sendOrSchedule(cMessage *msg, simtime_t delay) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual bool handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID,
            cObject *obj, cObject *details) override;

private:
    void initMyHeartBeatRecord();
    void decreasePeersTtl();
    int removeZeroTtl();
    void sendPxAssignment(int PrevSenderID, HeartBeatMsg* pxAssignMsg);
    bool isProxyCandidate(int prevDevId);
    HeartBeatMap getPxAssignmentMap(int prevDevID);
    void calcPxAssignments();
    int getHbMsgSenderID(HeartBeatMsg* pxAssignMsg);
    bool canReachSsid(int devId, string ssid);
    void selectProxyAssignmentType();
    void buildSsidCoverage(std::map<string, int>& ssidCoverage);
    void buildMembersCoverage(std::map<int, int>& membersCoverage);
    void populateCostMatrix(const std::vector<string>& ssidList,
            const std::vector<int>& membersList,
            std::vector<vector<double> >& cost);
    void populatePxAssignmentMunkres(hash_map<int, int> direct_assignment,
            const std::vector<int>& membersList,
            const std::vector<string>& ssidList);
    void populatePxAssignmentFirstAvailable(const std::vector<string>& ssidList,
            const std::vector<int>& membersList);
    void populatePxAssignmentRandom(const std::vector<string>& ssidList,
            const std::vector<int>& membersList);
    void populatePxAssignments(std::vector<string> ssidList,
            std::vector<int> membersList);
    void reopenSocket();
    void closeSocket();
};

} /* namespace inet */

#endif /* INET_APPLICATIONS_TCPAPP_TCPMGMTSRVAPP_H_ */
