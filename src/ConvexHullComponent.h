#ifndef __CONVEXHULLCOMPONENT_H__
#define __CONVEXHULLCOMPONENT_H__

#include "HandwrittenImage.h"
#include "Point.h"

class ConvexHullComponent {
public:
	ConvexHullComponent(HandwrittenImage::PIXELS &pix, int x, int y, int mark);
	double getDistance(const ConvexHullComponent *other, int&, int &, int&, int&) const;

	vector<Point> vertices;
	int regionID;
	int wordID;
	int xl, xh, yl, yh;  // bounding box
	Point startPoint;
	Point gravityCenter;
private:
	int turnDir (const Point &O, const Point &A, const Point &B) const;
};

#endif
