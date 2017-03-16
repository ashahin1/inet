//
// Copyright (C) 2017 Ahmed Shahin <ashahin1@umbc.edu, ahmed3012005@gmail.com>
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//

package inet.mobility.single;

import inet.mobility.base.MovingMobilityBase;

//
// Mobility model which initially places all hosts at random making sure
// that every node has at least one reachable neighbor. The minimum distance
// between any two nodes is controlled by the txPowerRange parameter. This parameter
// should be adjusted according to the transmitting range of the radio. The
// resulting graph should be connected.
//
simple ConnectedGraphMassMobility extends MovingMobilityBase
{
    parameters:
        double txPowerRange @unit(m) = default(50m);

        double startAngle @unit(deg) = default(uniform(0deg, 360deg)); // initial angle in degrees
        bool initFromDisplayString = default(false);
        volatile double changeInterval @unit(s); // frequency of changing speed and angle (can be random)
        volatile double changeAngleBy @unit(deg); // change angle by this much (can be random)
        volatile double speed @unit(mps); // speed (can be random, updated every changeInterval)
        @class(ConnectedGraphMassMobility);
}

