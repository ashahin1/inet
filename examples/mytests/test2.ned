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

package inet.examples.mytests;
import inet.linklayer.contract.IMACAddressTable;
import inet.linklayer.contract.IMACRelayUnit;
import inet.applications.dhcp.DHCPClient;
import inet.applications.dhcp.DHCPServer;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.WirelessHost;
import inet.node.wireless.AccessPoint;
import inet.physicallayer.base.packetlevel.PropagationBase;
import inet.physicallayer.common.packetlevel.RadioMedium;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;



//
// TODO auto-generated type
//
network Test2
{
    @display("bgb=614,405");
    submodules:
        ap: AccessPointDhcp2 {
            @display("p=217,85");
        }
        host1: WirelessHost {
            @display("p=96,202;b=86,69");
        }
        host2: WirelessHost {
            @display("p=303,203;b=111,82");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=87,68");
        }
        propagationBase: PropagationBase {
            @display("p=325,49");
        }
        configurator: IPv4NetworkConfigurator {
            //assignAddresses = default(false);
            //addStaticRoutes = default(false);
            @display("p=381,122;b=47,32");
        }
}


module AccessPointDhcp2 extends WirelessHost
{
    @display("i=device/accesspoint");
    string relayUnitType = default(firstAvailable("Ieee8021dRelay","MACRelayUnit")); // type of the IMACRelayUnit (MACRelayUnit, Ieee8021dRelay, etc)
    string macTableType = default("MACAddressTable"); // type of the ~IMACAddressTable
    //wlan[*].mgmtType = default("Ieee80211MgmtAP");
	//wlan[*].mgmnt.encapDecap = "true";   
        
    submodules:
		macTable: <macTableType> like IMACAddressTable {
            @display("p=73,19;is=s");
        }
        relayUnit: <relayUnitType> like IMACRelayUnit if sizeof(ethg)+numRadios>1 && relayUnitType != "" {
            parameters:
                @display("p=151,121");
        }
    connections allowunconnected:
        for i=0..numRadios-1 {
            wlan[i].upperLayerOut --> relayUnit.ifIn++ if sizeof(ethg)+numRadios>1;
            wlan[i].upperLayerIn <-- relayUnit.ifOut++ if sizeof(ethg)+numRadios>1;
        }
        // ethernet must be connected only AFTER wireless ports
        for i=0..sizeof(ethg)-1 {
            eth[i].phys <--> ethg[i];
            eth[i].upperLayerIn <-- relayUnit.ifOut++ if sizeof(ethg)+numRadios>1;
            eth[i].upperLayerOut --> relayUnit.ifIn++ if sizeof(ethg)+numRadios>1;
        }
}
