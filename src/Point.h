#ifndef __POINT_H__
#define __POINT_H__

struct Point {
	int x;
	int y;
	Point ();
	Point (int xCoord, int yCoord);
	bool operator== (const Point &p) const;
	bool operator!= (const Point &p) const;
};

#endif
