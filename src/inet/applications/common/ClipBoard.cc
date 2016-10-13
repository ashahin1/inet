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

void ClipBoard::refreshDisplay() const {
    // refresh statistics
    char buf[32];
    sprintf(buf, "HBC:%lu  HBS:%lu  PI:%lu", heartBeatMapClient->size(),
            heartBeatMapServer->size(), peersInfo->size());
    getDisplayString().setTagArg("t", 0, buf);
}

} /* namespace inet */

