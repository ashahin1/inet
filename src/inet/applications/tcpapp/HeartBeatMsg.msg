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

cplusplus {{
    #include <vector>
    #include <map>
    #include "inet/applications/common/HeartBeatRecord.h"
	#include "GenericAppMsg_m.h"
	typedef std::map<int, inet::HeartBeatRecord> HeartBeatMap;
}}

namespace inet;
struct HeartBeatRecord;
class noncobject HeartBeatMap;
packet GenericAppMsg;


//
// TODO generated message class
//
packet HeartBeatMsg extends GenericAppMsg {
	HeartBeatMap heartBeatMap;
	bool replayNow = false;
	bool nextIsProxyAssignment = false;
	bool isProxyAssignment = false;
}
