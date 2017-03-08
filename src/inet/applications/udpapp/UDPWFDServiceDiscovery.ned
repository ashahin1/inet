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
        string groupStatisticsModule = default("");
        string lifeCycleControllerModule = default("lifecycleController");
        string interface = default("wlan0"); //The interface for sending and receiving the discovery packets
        string apInterface = default("wlan1"); //The interface for GO operations
        string groupInterface = default("wlan2"); //The interface that proforms the WFD group operations
        string proxyInterface = default("wlan3"); //The interface for proxy connection to the other group
        string dhcpClientAppName = default(".udpApp[1]");
        string dhcpServerAppName = default(".udpApp[2]");
        string tcpMgmtClientAppName = default(".tcpApp[1]");
        string tcpMgmtSrvAppName = default(".tcpApp[0]");
        string sdNicName = default(".wlan[0]");
        string ApNicName = default(".wlan[1]");
        string p2pNicName = default(".wlan[2]");
        string proxyNicName = default(".wlan[3]");
        int maxSubnetX = default(254);
        int maxSubnetY = default(254);

        double declareGoPeriod @unit(s);// = default(this.sendInterval * 10);
        double selectGoPeriod @unit(s);// = default(this.declareGoPeriod);
        double switchDhcpPeriod @unit(s);// = default (this.sendInterval * 2);
        double tearDownPeriod @unit(s);// = default(this.switchDhcpPeriod * 2);

        //Parameters for comparing my approach to a base line
        string subnetProposalType @enum("ISNP","NO_CONFLICT_DETECTION") = default("ISNP");
        string goDeclarationMethod @enum("EMC","EMC_TWO_HOP" ,"RANDOM") = default("EMC_TWO_HOP");

        packetName = default("ServiceDiscovery");
        receiveBroadcast = default(true);
        localPort = default(5000);
        destPort = default(5000);
        destAddresses = default("255.255.255.255");
        @class(UDPWFDServiceDiscovery);
        @signal[membersChanged];
        @signal[endToEndDelay];
        @statistic[numOfAssociatedMembers](title="Associated members count"; source=membersChanged; record=vector,last,stats);
        @statistic[endToEndDelay](title="Request to response delay"; source=endToEndDelay; record=vector?,stats);
}
