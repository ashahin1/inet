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

#include "inet/applications/common/ClipBoard.h"

namespace inet {

Define_Module(ClipBoard);

ClipBoard::ClipBoard() {
    // TODO Auto-generated constructor stub
    numOfHits = 0;
}

ClipBoard::~ClipBoard() {
    // TODO Auto-generated destructor stub
}

int ClipBoard::getNumOfHits() {
    return numOfHits;
}

void ClipBoard::setNumOfHits(int numOfHits) {
    this->numOfHits = numOfHits;
    EV_INFO << "ClipBoard No of Hit = " << this->numOfHits;
}

} /* namespace inet */
