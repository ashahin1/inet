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

import inet.applications.common.ClipBoard;
import inet.common.figures.DelegateSignalConfigurator;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.WirelessHost;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;

//
// TODO auto-generated type
//
network Test4
{
    @display("bgb=650,500;bgg=100,1,grey95;bgl=2");
	@figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);
    
    submodules:
        hostA: WirelessHostClipBoard {
            @display("p=60,135");
        }
        hostB: WirelessHostClipBoard {
            @display("p=151,144");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=587.664,225.792");
        }

        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=578.59204,143.136");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=579.60004,290.30402");
        }
        figureHelper: DelegateSignalConfigurator {
            @display("p=580,350");
        }
        
}

module WirelessHostClipBoard extends WirelessHost
{
    parameters:
        *.clipBoardModule = default(absPath(".clipBoard"));
    submodules:
        clipBoard: ClipBoard {
            @display("p=141,236");
        }
}


