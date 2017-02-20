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

import inet.common.figures.DelegateSignalConfigurator;
import inet.common.lifecycle.LifecycleController;
import inet.common.scenario.ScenarioManager;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.WFDHost;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;


//Wi-Fi Direct Network
network WFDNetwork
{
    int deviceCount = default(3);
    string mediumType = default("Ieee80211ScalarRadioMedium");

    @display("bgg=100,2,grey95;bgl=5");
    @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

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
        radioMedium: <mediumType> like IRadioMedium {
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
