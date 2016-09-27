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

package inet.node.inet;
import inet.applications.common.ClipBoard;
import inet.linklayer.contract.IMACAddressTable;


//
// TODO auto-generated type
//
module WFDHost extends StandardHost
{
    string macTableType = default("MACAddressTable"); // type of the ~IMACAddressTable

    @display("i=device/palm2;bgl=4");
    numRadios = default(4);
    wlan[0].mgmtType = default("Ieee80211MgmtAdhoc");
    wlan[1].mgmtType = default("Ieee80211MgmtAP");
    wlan[2].mgmtType = default("Ieee80211MgmtSTA");
    wlan[3].mgmtType = default("Ieee80211MgmtSTA");
    wlan[1].mgmt.encapDecap = default("true");

    *.clipBoardModule = default(absPath(".clipBoard"));

    submodules:
        clipBoard: ClipBoard {
            @display("p=196,231");
        }
        macTable: <macTableType> like IMACAddressTable {
            @display("p=133,258;is=s");
        }
}
