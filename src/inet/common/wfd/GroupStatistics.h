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

#ifndef INET_COMMON_WFD_GROUPSTATISTICS_H_
#define INET_COMMON_WFD_GROUPSTATISTICS_H_

#include <omnetpp/csimplemodule.h>
#include "inet/common/ModuleAccess.h"
#include <vector>
#include <map>
#include <hash_map>
#include <string>

namespace inet {

using namespace std;
using namespace __gnu_cxx;

struct GoInfo {
    int devId;
    string ssid;
    vector<int> associatedGMs;
    vector<int> associatedPMs;
};

typedef map<int, GoInfo> GoInfoMap;

struct Compare {
    int num;
    Compare(const int& num) :
            num(num) {
    }
};

#ifndef INET_APPLICATIONS_UDPAPP_UDPWFDSERVICEDISCOVERY_H_
bool operator==(const std::pair<string, int>&p, const Compare& c) {
    return c.num == p.second;
}
bool operator==(const Compare& c, const std::pair<string, int>&p) {
    return c.num == p.second;
}
#endif

class GroupStatistics: public omnetpp::cSimpleModule, public cListener {
public:
    /**
     * The signal that is used to collect the number of GOs during each protocol run.
     */
    static simsignal_t goCountSignal;
    /**
     * The signal that is used to collect the number of GMs during each protocol run.
     */
    static simsignal_t gmCountSignal;
    /**
     * The signal that is used to collect the number of PMs during each protocol run.
     */
    static simsignal_t pmCountSignal;
    /**
     * The signal that is used to collect the number of Orphaned Members during each protocol run.
     */
    static simsignal_t orphCountSignal;
    /**
     * The signal that is used to collect the number of Orphaned Members during each protocol run.
     */
    static simsignal_t connectedComponectCountSignal;
    /**
     * The signal that is used to collect the number of subnet conflicts (after settling) during each protocol run.
     */
    static simsignal_t conflictCountSignal;

    static simsignal_t totalReqToRespDelaySignal;
    static simsignal_t totalAllConsumedPowerSignal;
    static simsignal_t totalResidualEnergySignal;
    static simsignal_t totalUdpSentPkSignal;
    static simsignal_t totalUdpRcvdPkSignal;
    static simsignal_t totalTcpSentPkSignal;
    static simsignal_t totalTcpRcvdPkSignal;

protected:
    int goCount;
    int gmCount;
    int pmCount;
    int orphCount;
    int connectedComponentCount;

    int curProtocolRun = 0;

    int numDevices;
    int numRunsToEndSim;
    double sendInterval;
    double declareGoPeriod;
    double selectGoPeriod;
    double switchDhcpPeriod;
    double tearDownPeriod;

    //vectors for stats recording

    cMessage *validDataMsg = nullptr;
    cMessage *resetMsg = nullptr;

    GoInfoMap goInfoMap;
    map<string, int> ssidToDevIdMap; //caches the GOs ssids and their id mapping
    map<int, int> devIdToIndexMap; //Maps devIds to indexes starting from 0 .. noOfNodes-1
    int curIndex;
    hash_map<int, std::vector<int> > group; //A map that holds each connected component members.

    vector<int> orphanedList; //A map that hold the list of orphaned devices

    // A map that holds the number of conflicts per each subnet
    // Here we let each GO records its subnet and we increase the count for it
    // if a subnet has a count > 1 it means we have a conflict in the assignment.
    map<string, int> assignedSubnetCount;
    int conflictCount;

    long totalSentRequests = 0;
    long totalRcvdRequests = 0;
    long totalSentResponces = 0;
    long totalRcvdResponces = 0;
    long totalUdpPacketsSent = 0;
    long totalUdpPacketsRcvd = 0;
    long totalResolsedIpConflicts = 0;

public:
    GroupStatistics();
    virtual ~GroupStatistics();
    void addGO(int devId, string ssid);
    void addGM(int devId, string goSsid);
    void addPM(int devId, string goSsid);
    void addOrph(int devId);
    void addSubnet(string subnet);
    int getGmCount() const;
    int getGoCount() const;
    int getPmCount() const;
    void clearAll();
    void updateConlictCount();
    void writeGroupStats();
    void recordGroupStats();
    string getModuleNameFromId(int id);
    string getModuleNameFromIndex(int index);
    int getConnectedComponentCount() const;
    void calcGraphConnectivity();
    int getConflictCount() const;
    int getOrphCount() const;
    long getTotalRcvdRequests() const;
    void addTotalRcvdRequests(long totalRcvdRequests = 1);
    long getTotalRcvdResponces() const;
    void addTotalRcvdResponces(long totalRcvdResponces = 1);
    long getTotalSentRequests() const;
    void addTotalSentRequests(long totalSentRequests = 1);
    long getTotalSentResponces() const;
    void addTotalSentResponces(long totalSentResponces = 1);
    long getTotalResolsedIpConflicts() const;
    void addTotalResolsedIpConflicts(long totalResolsedIpConflicts = 1);
    long getTotalUdpPacketsRcvd() const;
    void addTotalUdpPacketsRcvd(long totalPacketsRcvd = 1);
    long getTotalUdpPacketsSent() const;
    void addTotalUdpPacketsSent(long totalPacketsSent = 1);

protected:
    virtual void initialize(int stage) override;
    virtual void refreshDisplay() const override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID,
            double d, cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID,
            const SimTime& t, cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, long l,
            cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID,
            cObject *obj, cObject *details) override;
};

} /* namespace inet */

#endif /* INET_COMMON_WFD_GROUPSTATISTICS_H_ */
