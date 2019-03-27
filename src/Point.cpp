#include "Point.h"

Point::Point () {
	x = -1;
	y = -1;
}

Point::Point (int xCoord, int yCoord) {
	x = xCoord;
	y = yCoord;
}

bool Point::operator!= (const Point &p) const {
	return (this->x != p.x || this->y != p.y);
}

bool Point::operator== (const Point &p) const {
	return !((*this) != p);
}
