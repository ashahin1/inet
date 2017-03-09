/*
 * Copyright (C) 2006 Isabel Dietrich <isabel.dietrich@informatik.uni-erlangen.de>
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

void StationaryConnectedGraphMobility::setInitialPosition() {
    double txPowerRange = par("txPowerRange");

    Coord pos = getRandomPosition();

    lastPosition.x = pos.x;
    lastPosition.y = pos.y;
    lastPosition.z = 0;

    //previousNodeX = lastPosition.x;
    //previousNodeY = lastPosition.y;
    //previousNodeZ = lastPosition.z;


    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}

} // namespace inet

