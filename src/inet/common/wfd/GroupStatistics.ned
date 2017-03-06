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
}
