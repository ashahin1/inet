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

package inet.applications.udpapp;

//
// TODO auto-generated type
//
simple UDPWFDServiceDiscovery extends UDPBasicApp
{    
    parameters:
        string clipBoardModule = default("");
        string interface = default("wlan0"); //The interface for sending and receiving the discovery packets
        string groupInterface = default("wlan2"); //The interface that proforms the WFD group operations
        string proxyInterface = default("wlan3"); //The interface for proxy connection to the other group
        string dhcpAppName = default(".udpApp[1]");
        
        packetName = default("ServiceDiscovery");
        receiveBroadcast = default(true);
        localPort = default(5000);
        destPort = default(5000);
        destAddresses = default("255.255.255.255");
        @class(UDPWFDServiceDiscovery);
}
