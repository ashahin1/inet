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

#include "inet/applications/common/ClipBoard.h"

namespace inet {

Define_Module(ClipBoard);

ClipBoard::ClipBoard() {
    // TODO Auto-generated constructor stub
    protocolMsg = nullptr;
    isGroupOwner = nullptr;
    heartBeatMapClient = nullptr;
    heartBeatMapServer = nullptr;
    peersInfo = nullptr;
}

ClipBoard::~ClipBoard() {
    // TODO Auto-generated destructor stub
}

void ClipBoard::initialize(int stage) {
    WATCH(heartBeatMapClient);
    WATCH(heartBeatMapServer);
    WATCH(peersInfo);
    WATCH(protocolMsg);
    WATCH(*isGroupOwner);

    rankAlpha = par("rankAlpha").doubleValue();
    rankBeta = par("rankBeta").doubleValue();
    rankGamma = par("rankGamma").doubleValue();
    maxBatteryCapacity = par("maxBatteryCapacity").doubleValue();
}

HeartBeatMap* ClipBoard::getHeartBeatMapClient() {
    return heartBeatMapClient;
}

void ClipBoard::setHeartBeatMapClient(HeartBeatMap* heartBeatMap) {
    this->heartBeatMapClient = heartBeatMap;
}

HeartBeatMap* ClipBoard::getHeartBeatMapServer() {
    return heartBeatMapServer;
}

void ClipBoard::setHeartBeatMapServer(HeartBeatMap* heartBeatMap) {
    this->heartBeatMapServer = heartBeatMap;
}

DevicesInfo* ClipBoard::getPeersInfo() {
    return peersInfo;
}

void ClipBoard::setPeersInfo(DevicesInfo* peersInfo) {
    this->peersInfo = peersInfo;
}

double ClipBoard::getRank(bool isCharging, double Capacity, double level) {
    double rank = 0.0f;
    rank += (isCharging ? rankAlpha : 0);
    rank += (Capacity * 1.0f / maxBatteryCapacity) * rankBeta;
    rank += level * rankGamma;
    return rank;
}

double ClipBoard::getRank(DeviceInfo pInfo) {
    return getRank(pInfo.isCharging, pInfo.batteryCapacity, pInfo.batteryLevel);
}

const string& ClipBoard::getProxySsid() const {
    return proxy_ssid;
}

void ClipBoard::setProxySsid(const string& proxySsid) {
    proxy_ssid = proxySsid;
}

void ClipBoard::refreshDisplay() const {
    // refresh statistics
    char buf[32];
    sprintf(buf, "HBC:%lu  HBS:%lu  PI:%lu",
            (heartBeatMapClient ? heartBeatMapClient->size() : 0),
            (heartBeatMapServer ? heartBeatMapServer->size() : 0),
            (peersInfo ? peersInfo->size() : 0));
    getDisplayString().setTagArg("t", 0, buf);
}

} /* namespace inet */

