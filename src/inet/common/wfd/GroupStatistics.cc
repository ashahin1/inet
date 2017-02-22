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

#include "inet/common/wfd/GroupStatistics.h"
#include <algorithm>

namespace inet {

Define_Module(GroupStatistics);

GroupStatistics::GroupStatistics() {
    // TODO Auto-generated constructor stub
    clearAll();
}

GroupStatistics::~GroupStatistics() {
    // TODO Auto-generated destructor stub
}

void GroupStatistics::initialize(int stage) {
    numDevices = par("numDevices");

    WATCH(goCount);
    WATCH(gmCount);
    WATCH(pmCount);

}

void GroupStatistics::refreshDisplay() const {
}

int GroupStatistics::getGmCount() const {
    return gmCount;
}

int GroupStatistics::getGoCount() const {
    return goCount;
}

int GroupStatistics::getPmCount() const {
    return pmCount;
}

void GroupStatistics::addGO(int devId, string ssid) {
    //Search if the GO is already their
    //Add an entry if this GO not existed

    //Update GoInfoMap
    if (goInfoMap.count(devId) > 0) {
        GoInfo* gf = &goInfoMap[devId];
        gf->ssid = ssid;
    } else {
        GoInfo gInfo;
        gInfo.devId = devId;
        gInfo.ssid = ssid;
        goInfoMap[devId] = gInfo;
    }

    goCount = goInfoMap.size();

    //Add/update a cache entry for this GO
    ssidToDevIdMap[ssid] = devId;
}

void GroupStatistics::addGm(int devId, string goSsid) {
    int goDevId = ssidToDevIdMap[goSsid];

    vector<int> *aGMs = &goInfoMap[goDevId].associatedGMs;
    //Check if devId already exists
    if (std::find(aGMs->begin(), aGMs->end(), devId) != aGMs->end()) {
        return;
    } else {
        aGMs->push_back(devId);
        gmCount = aGMs->size();
    }
}

void GroupStatistics::addPm(int devId, string goSsid) {
    int goDevId = ssidToDevIdMap[goSsid];

    vector<int> *aPMs = &goInfoMap[goDevId].associatedPMs;
    //Check if devId already exists
    if (std::find(aPMs->begin(), aPMs->end(), devId) != aPMs->end()) {
        return;
    } else {
        aPMs->push_back(devId);
        pmCount = aPMs->size();
    }
}

void GroupStatistics::clearAll() {
    goCount = 0;
    gmCount = 0;
    pmCount = 0;

    goInfoMap.clear();
    ssidToDevIdMap.clear();
}

} /* namespace inet */
