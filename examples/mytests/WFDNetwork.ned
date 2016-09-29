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

import inet.common.lifecycle.LifecycleController;
import inet.common.scenario.ScenarioManager;
import inet.common.figures.DelegateSignalConfigurator;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.WFDHost;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;


//
// TODO auto-generated type
//
network WFDNetwork
{
    int deviceCount = default(3);
    @display("bgb=600,600;bgg=100,1,grey95;bgl=3");
    submodules:
        scenarioManager: ScenarioManager {
            @display("p=68.544,276.19202");
        }
        lifecycleController: LifecycleController {
            @display("p=69.552,227.80801");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=68.544,66.528");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=67.536,19.152");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=68.544,121.968");
        }
        figureHelper: DelegateSignalConfigurator {
            @display("p=68.544,169.34401");
        }
        device[deviceCount]: WFDHost;
}
