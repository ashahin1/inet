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
        @statistic[totalReqToRespDelay](title="Total Request to Response Delay along all nodes"; source=totalReqToRespDelay; record=stats);
        @signal[totalAllConsumedPower];
        @statistic[totalAllConsumedPower](title="Total power consumption along all nodes"; source=totalAllConsumedPower; record=sum,histogram);
        @signal[totalResidualEnergy];
        @statistic[totalResidualEnergy](title="Total residual Energy along all nodes"; source=totalResidualEnergy; record=stats,histogram);

        //UDP stats
        @signal[totalUdpSentPk](type=cPacket);
        @signal[totalUdpRcvdPk](type=cPacket);
        @statistic[totalUdpRcvdPk](title="total udp received packets received along all nodes"; source=totalUdpRcvdPk; record=count,"sum(packetBytes)"; interpolationmode=none);
        @statistic[totalUdpThroughput](title="total udp throughput"; unit=bps; source="throughput(totalUdpRcvdPk)"; record=last);
        @statistic[totalUdpSentPk](title="total udp packets sent from all nodes"; source=totalUdpSentPk; record=count,"sum(packetBytes)"; interpolationmode=none);
        @statistic[totalUdpRcvdPkLifetime](title="total udp received packet lifetime"; source="messageAge(totalUdpRcvdPk)"; unit=s; record=stats,histogram; interpolationmode=none);
        //TCP stats
        @signal[totalTcpSentPk](type=cPacket);
        @signal[totalTcpRcvdPk](type=cPacket);
        @statistic[totalTcpRcvdPk](title="packets received"; source=totalTcpRcvdPk; record=count,"sum(packetBytes)"; interpolationmode=none);
        @statistic[totalTcpSentPk](title="packets sent"; source=totalTcpSentPk; record=count,"sum(packetBytes)"; interpolationmode=none);
        @statistic[totalTcpendToEndDelay](title="end-to-end delay"; source="messageAge(totalTcpRcvdPk)"; unit=s; record=stats,histogram; interpolationmode=none);
}
