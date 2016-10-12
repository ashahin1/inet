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

package inet.applications.tcpapp;

//
// TODO auto-generated type
//
simple TCPMgmtClientApp extends TCPBasicClientApp
{
    string interfaceTableModule;   // The path to the InterfaceTable module
    string clipBoardModule = default("");
    string ipInterface = default("wlan2");
    string sdNicName = default(".wlan[0]");
    int hb_ttl = default(30);
    double decTtlPeriod @unit(s) = default(1s);

    thinkTime = default(1s);
    idleInterval = default(1s);
    numRequestsPerSession = default(5);
    dataTransferMode = default("object");
    startTime = default(10s);
    reconnectInterval = default(1s);
    @class(TCPMgmtClientApp);
}
