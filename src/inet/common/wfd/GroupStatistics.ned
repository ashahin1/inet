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

package inet.common.wfd;

//
// TODO auto-generated type
//
simple GroupStatistics
{
    parameters:
        int numDevices;
        int numRunsToEndSim = default(-1); //No od protocol runs before ending the simulation
        double sendInterval @unit(s) = default(5s);
        double declareGoPeriod @unit(s) = default(this.sendInterval * 10);
        double selectGoPeriod @unit(s) = default(this.declareGoPeriod);
        double switchDhcpPeriod @unit(s) = default(this.sendInterval * 2);
        double tearDownPeriod @unit(s) = default(this.switchDhcpPeriod * 2);

        @display("i=block/users,black;is=s");
        @signal[GoCount];
        @statistic[GO_Count](title="Num of GOs"; source=GoCount; record=vector,last ,stats);
        @signal[GmCount];
        @statistic[GM_Count](title="Num of GMs"; source=GmCount; record=vector,last,stats);
        @signal[PmCount];
        @statistic[PM_Count](title="Num of PMs"; source=PmCount; record=vector,last,stats);
        @signal[OrphCount];
        @statistic[Orph_Count](title="Num of Orphs"; source=OrphCount; record=vector,last,stats);
        @signal[CcCount];
        @statistic[CC_Count](title="Num of ConnComp"; source=CcCount; record=vector,last,stats);
        @signal[ConflictCount];
        @statistic[Conflict_Count](title="Num of Conflicts"; source=ConflictCount; record=vector,last,stats);

        @signal[totalReqToRespDelay];
        @statistic[totalReqToRespDelay](title="Total Request to Response Delay along all nodes"; source=totalReqToRespDelay; record=histogram);
        
        @signal[totalAllConsumedPower];
        @statistic[totalAllConsumedPower](title="Total power consumption along all nodes"; source=totalAllConsumedPower; record=sum);
        @statistic[totalAvgConsumedPower](title="total Avg power consumption along all nodes"; source="totalAllConsumedPower/(max(GoCount)+max(GmCount)+max(OrphCount))"; record=last);

        @signal[totalResidualEnergy];
        @statistic[totalResidualEnergy](title="Total residual Energy along all nodes"; source=totalResidualEnergy; record=histogram);

        //UDP stats        
        @signal[totalUdpRcvdPk](type=cPacket);
        @statistic[totalUdpRcvdPk](title="total udp received packets along all nodes"; source=totalUdpRcvdPk; record=count,"sum(packetBytes)");
        @statistic[totalAvgUdpRcvdPk](title="total Avg udp received packets along all nodes"; source="count(totalUdpRcvdPk)/(max(GoCount)+max(GmCount)+max(OrphCount))"; record=last);
        @statistic[totalUdpRcvdPkLifetime](title="total udp received packet lifetime"; source="messageAge(totalUdpRcvdPk)"; unit=s; record=histogram);
        @statistic[totalUdpThroughput](title="total udp throughput"; unit=bps; source="throughput(totalUdpRcvdPk)"; record=last);
        @statistic[totalAvgUdpThroughput](title="total Avg along all nodes"; source="throughput(totalUdpRcvdPk)/(max(GoCount)+max(GmCount)+max(OrphCount))"; record=last);

        @signal[totalUdpSentPk](type=cPacket);
        @statistic[totalUdpSentPk](title="total udp packets sent from all nodes"; source=totalUdpSentPk; record=count,"sum(packetBytes)");
        @statistic[totalAvgUdpSentPk](title="total Avg udp sent packets along all nodes"; source="count(totalUdpSentPk)/(max(GoCount)+max(GmCount)+max(OrphCount))"; record=last);

        //TCP stats        
        @signal[totalTcpRcvdPk](type=cPacket);
        @statistic[totalTcpRcvdPk](title="total tcp packets received among all nodes"; source=totalTcpRcvdPk; record=count,"sum(packetBytes)");
        @statistic[totalAvgTcpRcvdPk](title="total Avg tcp received packets along all nodes"; source="count(totalTcpRcvdPk)/(max(GoCount)+max(GmCount)+max(OrphCount))"; record=last);
        @statistic[totalTcpendToEndDelay](title="end-to-end delay"; source="messageAge(totalTcpRcvdPk)"; unit=s; record=histogram);
        @statistic[totalTcpThroughput](title="total tcp throughput"; unit=bps; source="throughput(totalTcpRcvdPk)"; record=last);
        @statistic[totalAvgTcpThroughput](title="total tcp along all nodes"; source="throughput(totalTcpRcvdPk)/(max(GoCount)+max(GmCount)+max(OrphCount))"; record=last);

        @signal[totalTcpSentPk](type=cPacket);
        @statistic[totalTcpSentPk](title="total tcp packets sent among all nodes"; source=totalTcpSentPk; record=count,"sum(packetBytes)");
        @statistic[totalAvgTcpSentPk](title="total Avg tcp sent packets along all nodes"; source="count(totalTcpSentPk)/(max(GoCount)+max(GmCount)+max(OrphCount))"; record=last);

}
