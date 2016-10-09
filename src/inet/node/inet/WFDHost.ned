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
    @display("i=device/palm2;bgl=4");
    string macTableType = default("MACAddressTable"); // type of the ~IMACAddressTable  

    numRadios = default(4);

    wlan[0].mgmtType = default("Ieee80211MgmtAdhoc");
    wlan[1].mgmtType = default("Ieee80211MgmtAP");
    wlan[2].mgmtType = default("Ieee80211MgmtSTA");
    wlan[3].mgmtType = default("Ieee80211MgmtSTA");

    wlan[1].mgmt.encapDecap = default("true");
    wlan[1].mgmt.ssid = default("DIRECT"+string(intuniform(0,9999))+string(intuniform(0,9999)));
    //wlan[1].mgmt.beaconInterval = default(1s);
    //wlan[1].radio.channelNumber = default(intuniform(1,13));
    //wlan[2].radio.channelNumber = default(intuniform(1,13));

	*.agent.default_ssid = default("DIRECT-XXXXXXXX");
    //*.mgmt.numChannels = default(13);
    *.clipBoardModule = default(absPath(".clipBoard"));

    submodules:
        clipBoard: ClipBoard {
            @display("p=196,231");
        }
        macTable: <macTableType> like IMACAddressTable {
            @display("p=133,258;is=s");
        }
}
