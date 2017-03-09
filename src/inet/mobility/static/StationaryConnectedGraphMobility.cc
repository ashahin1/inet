/*
 * Copyright (C) 2017 Ahmed Shahin <ashahin1@umbc.edu, ahmed3012005@gmail.com>
 * Copyright (C) 2013 OpenSim Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "inet/mobility/static/StationaryConnectedGraphMobility.h"

namespace inet {

Define_Module(StationaryConnectedGraphMobility);

double StationaryConnectedGraphMobility::previousNodeX = -1;
double StationaryConnectedGraphMobility::previousNodeY = -1;
double StationaryConnectedGraphMobility::previousNodeZ = -1;

void StationaryConnectedGraphMobility::setInitialPosition() {
    double txPowerRange = par("txPowerRange");

    Coord pos = getRandomPosition();
    double dist = 0.0f;

    if (previousNodeX == -1 || previousNodeY == -1 || previousNodeZ == -1) {
        //This is the first node, so no need to do a check for range
        EV_DETAIL << "\nFirst Node";
    } else {
        //We need to check for range
        while ((dist = sqrt(
                pow(pos.x - previousNodeX, 2) + pow(pos.y - previousNodeY, 2)))
                >= txPowerRange) {
            pos = getRandomPosition();
        }
    }
    //Save the previous values to compare it with new nodes
    previousNodeX = pos.x;
    previousNodeY = pos.y;
    previousNodeZ = 0;

    lastPosition.x = pos.x;
    lastPosition.y = pos.y;
    lastPosition.z = 0;

    EV_DETAIL << "\nx=" << pos.x << "   ,y=" << pos.y;

    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}

} // namespace inet

