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

class GroupStatistics: public omnetpp::cSimpleModule {
protected:
    int goCount;
    int gmCount;
    int pmCount;
    int connectedComponentCount;

    int numDevices;
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
    hash_map<int, std::vector<int> > group;

public:
    GroupStatistics();
    virtual ~GroupStatistics();
    void addGO(int devId, string ssid);
    void addGM(int devId, string goSsid);
    void addPM(int devId, string goSsid);
    int getGmCount() const;
    int getGoCount() const;
    int getPmCount() const;
    void clearAll();
    void writeGroupStats();
    void recordGroupStats();
    string getModuleNameFromId(int id);
    string getModuleNameFromIndex(int index);
    int getConnectedComponentCount() const;
    void calcGraphConnectivity();

protected:
    virtual void initialize(int stage) override;
    virtual void refreshDisplay() const override;
    virtual void handleMessage(cMessage *msg) override;
};

} /* namespace inet */

#endif /* INET_COMMON_WFD_GROUPSTATISTICS_H_ */
