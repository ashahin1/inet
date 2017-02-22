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
    cancelAndDelete(validDataMsg);
    cancelAndDelete(resetMsg);
}

void GroupStatistics::initialize(int stage) {
    numDevices = par("numDevices");
    sendInterval = par("sendInterval").doubleValue();
    declareGoPeriod = par("declareGoPeriod").doubleValue();
    selectGoPeriod = par("selectGoPeriod").doubleValue();
    switchDhcpPeriod = par("switchDhcpPeriod").doubleValue();
    tearDownPeriod = par("tearDownPeriod").doubleValue();

    WATCH(goCount);
    WATCH(gmCount);
    WATCH(pmCount);

    validDataMsg = new cMessage("validDataMsg");
    scheduleAt(
            simTime() + declareGoPeriod + selectGoPeriod + switchDhcpPeriod
                    + 0.5, validDataMsg);

    resetMsg = new cMessage("restMsg");

}

void GroupStatistics::refreshDisplay() const {
    char buf[80];
    sprintf(buf, "GOs: %d GMs: %d PMs: %d", goCount, gmCount, pmCount);
    getDisplayString().setTagArg("t", 0, buf);
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
        goCount++;
    }

    //Add/update a cache entry for this GO
    ssidToDevIdMap[ssid] = devId;
}

void GroupStatistics::addGM(int devId, string goSsid) {
    int goDevId = ssidToDevIdMap[goSsid];

    vector<int> *aGMs = &goInfoMap[goDevId].associatedGMs;
    //Check if devId already exists
    if (std::find(aGMs->begin(), aGMs->end(), devId) != aGMs->end()) {
        return;
    } else {
        aGMs->push_back(devId);
        gmCount++;
    }
}

void GroupStatistics::addPM(int devId, string goSsid) {
    int goDevId = ssidToDevIdMap[goSsid];

    vector<int> *aPMs = &goInfoMap[goDevId].associatedPMs;
    //Check if devId already exists
    if (std::find(aPMs->begin(), aPMs->end(), devId) != aPMs->end()) {
        return;
    } else {
        aPMs->push_back(devId);
        pmCount++;
    }
}

void GroupStatistics::clearAll() {
    goCount = 0;
    gmCount = 0;
    pmCount = 0;

    goInfoMap.clear();
    ssidToDevIdMap.clear();
}

void GroupStatistics::writeGroupStats() {
    EV_DETAIL << "\n==================Group Statistics======================\n";

    for (auto& gInfo : goInfoMap) {
        EV_DETAIL << "\nGROUP of:\n" << getModuleNameFromId(gInfo.first) << "\t"
                         << gInfo.second.ssid;

        EV_DETAIL << "\nGM List: ";
        for (const int& gmId : gInfo.second.associatedGMs) {
            EV_DETAIL << getModuleNameFromId(gmId) << ", ";
        }

        EV_DETAIL << "\nPM List: ";
        for (const int& pmId : gInfo.second.associatedPMs) {
            EV_DETAIL << getModuleNameFromId(pmId) << ", ";
        }

        EV_DETAIL << "\n+++++++++++++++++++++++++++++++++++++++\n";
    }

    EV_DETAIL
                     << "\n================End of Group Statistics====================\n";
}

string GroupStatistics::getModuleNameFromId(int id) {
    return cSimulation::getActiveSimulation()->getModule(id)->getFullName();
}

void GroupStatistics::handleMessage(cMessage* msg) {
    if (msg == validDataMsg) {
        writeGroupStats();

        scheduleAt(simTime() + tearDownPeriod, resetMsg);
    } else if (msg == resetMsg) {
        clearAll();

        scheduleAt(
                simTime() + declareGoPeriod + selectGoPeriod + switchDhcpPeriod,
                validDataMsg);
    } else {
        delete msg;
    }
}

} /* namespace inet */
