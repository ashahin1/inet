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

package inet.mobility.static;

import inet.mobility.base.MobilityBase;

//
// Mobility model which places all hosts at random making sure
// that every node has at least one reachable neighbor. The minimum distance
// between any two nodes is controlled by the txPowerRange parameter. This parameter
// should be adjusted according to the transmitting range of the radio. The
// resulting graph should be connected.
//
simple StationaryConnectedGraphMobility extends MobilityBase
{
    parameters:
        double txPowerRange @unit(m)= default(50m);
        @class(StationaryConnectedGraphMobility);
}

