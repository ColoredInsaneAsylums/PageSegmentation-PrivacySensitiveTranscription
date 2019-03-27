#include <algorithm>
#include <climits>
#include <cassert>
#include <unordered_map>
#include <queue>
#include <vector>
#include <cstdlib>
#include <utility>
#include "HandwrittenImage.h"
#include "ConvexHullComponent.h"
#include "Point.h"

using std::min;
using std::max;
using std::queue;
using std::vector;
using std::pair;
using std::make_pair;
using std::unordered_map;
using std::abs;

ConvexHullComponent::ConvexHullComponent (HandwrittenImage::PIXELS &pix, int xCoord, int yCoord, int mark) {
	assert(pix.size() != 0);

	wordID = -1;
	regionID = pix[xCoord][yCoord];
	assert(regionID != mark);

	startPoint = Point(xCoord, yCoord);

	unordered_map< int, pair<int, int> > outliers;  // map[xCoord] = (min yCoord, max yCoord)
	int width = pix.size(), height = pix[0].size();

	queue<Point> q;
	q.push(Point(xCoord, yCoord));

	while (!q.empty()) {
		int x = q.front().x;
		int y = q.front().y;
		q.pop();
		if (pix[x][y] == regionID) {
			// update component outliers
			if (outliers.find(x) != outliers.end()) {
				outliers[x].first = min(outliers[x].first, y);
				outliers[x].second = max(outliers[x].second, y);
			}
			else {
				outliers[x] = make_pair(y, y);
			}

			pix[x][y] = mark;
			if (x > 0)
				q.push(Point(x-1, y));
			if (x < width-1)
				q.push(Point(x+1, y));
			if (y > 0)
				q.push(Point(x, y-1));
			if (y < height-1)
				q.push(Point(x, y+1));
		}
	}

	xl = INT_MAX;
	xh = INT_MIN;
	yl = INT_MAX;
	yh = INT_MIN;
	vector<Point> upperPoints, lowerPoints;
	for (unordered_map< int, pair<int, int> >::iterator it = outliers.begin(); it != outliers.end(); ++it) {
		lowerPoints.push_back(Point(it->first, it->second.first));
		upperPoints.push_back(Point(it->first, it->second.second));
		xl = min(xl, it->first);
		xh = max(xh, it->first);
		yl = min(yl, it->second.first);
		yh = max(yh, it->second.second);
	}
	sort(lowerPoints.begin(), lowerPoints.end(),
			[](const Point &a, const Point &b) {
				return (a.x < b.x);
			}
		);
	sort(upperPoints.begin(), upperPoints.end(),
			[](const Point &a, const Point &b) {
				return (a.x < b.x);
			}
		);

	// construct convex hull
	assert(upperPoints.size() == lowerPoints.size());

	vector<Point> upperHull, lowerHull;
	if (upperPoints.size() == 1) { // single pixel component
		if (upperPoints[0] == lowerPoints[0]) {
			vertices.push_back(upperPoints[0]);
		}
		else {
			vertices.push_back(upperPoints[0]);
			vertices.push_back(lowerPoints[0]);
		}
	}
	else { // not a single pixel component
		// construct upper hull
		upperHull.push_back(upperPoints[0]);
		upperHull.push_back(upperPoints[1]);
		for (size_t i = 2; i < upperPoints.size(); ++i) {
			int k = upperHull.size() - 1;
			while (k > 0 && turnDir(upperHull[k-1], upperHull[k], upperPoints[i]) >= 0) {
				upperHull.pop_back();
				k -= 1;
			}
			upperHull.push_back(upperPoints[i]);
		}
		// construct lower hull
		lowerHull.push_back(lowerPoints[0]);
		lowerHull.push_back(lowerPoints[1]);
		for (size_t i = 2; i < lowerPoints.size(); ++i) {
			int k = lowerHull.size() - 1;
			while (k > 0 && turnDir(lowerHull[k-1], lowerHull[k], lowerPoints[i]) <= 0) {
				lowerHull.pop_back();
				k -= 1;
			}
			lowerHull.push_back(lowerPoints[i]);
		}

		vertices = upperHull;
		for (int i = lowerHull.size()-1; i >= 0; --i) {
			if (find(upperHull.begin(), upperHull.end(), lowerHull[i]) == upperHull.end()) {
				vertices.push_back(lowerHull[i]);
			}
		}
	}
	vertices.push_back(vertices[0]);  // make the last vertices same to the first one

	// calculate center of gravity
	vector<Point> &v = vertices;
	double Cx = 0, Cy = 0, A = 0;
	for (int i = 0; i < (int)v.size()-1; ++i) {
		Cx += ((v[i].x + v[i+1].x) * ((double)v[i].x*v[i+1].y - (double)v[i+1].x*v[i].y));
		Cy += ((v[i].y + v[i+1].y) * ((double)v[i].x*v[i+1].y - (double)v[i+1].x*v[i].y));
		A += ((double)v[i].x*v[i+1].y - (double)v[i+1].x*v[i].y);
	}
	if (A == 0) {
		gravityCenter = Point((xl+xh)/2, (yl+yh)/2);
	}
	else {
		A /= 2;
		Cx /= (6*A);
		Cy /= (6*A);
		gravityCenter = Point(Cx, Cy);
	}
}

// trun direction of O, A, B
// < 0: clockwise
// ==0: no ture
// > 0: counter clockwise
int ConvexHullComponent::turnDir (const Point &O, const Point &A, const Point &B) const {
	return (long)(A.x - O.x) * (B.y - O.y) - (long)(A.y - O.y) * (B.x - O.x);
}

double ConvexHullComponent::getDistance(const ConvexHullComponent *other, int &x1, int &y1, int &x2, int &y2) const {
	x1 = -1, x2 = -1, y1 = -1, y2 = -1;

	// first, find the convex hull segment of this component that intersects with the (this->gravityCenter, other->gravityCenter) segment
	if (this->vertices.size() == 2) {  // component only has 1 pixel
		x1 = this->gravityCenter.x;
		y1 = this->gravityCenter.y;
	}
	else {
		for (int i = 0; i < (int)this->vertices.size()-1; ++i) {
			// if a and b are in different sides of segment (c, d)
			// and c and d are in different sides of segment (a, b)
			// then segment (a, b) intersects with segment (c, d)
			const Point &a = this->gravityCenter, &b = other->gravityCenter;
			const Point &c = this->vertices[i], &d = this->vertices[i+1];
			if (((long)turnDir(a, b, c) * turnDir(a, b, d) <= 0) &&
				((long)turnDir(c, d, a) * turnDir(c, d, b) <= 0)) {
				long m1 = (long)(b.y-a.y)*a.x + (long)(a.x-b.x)*a.y;
				long m2 = (long)(d.y-c.y)*c.x + (long)(c.x-d.x)*c.y;
				long D = (long)(b.x-a.x)*(d.y-c.y) - (long)(d.x-c.x)*(b.y-a.y);
				long D1 = (long)m2*(b.x-a.x) - (long)m1*(d.x-c.x);
				long D2 = (long)m2*(b.y-a.y) - (long)m1*(d.y-c.y);
				if (D == 0)  // if D == 0, then (a, b) and (c, d) are parallel
					continue;
				else {
					x1 = D1 / D;
					y1 = D2 / D;
				}
				break;
			}
		}
	}
	// one gravityCenter is inside the other components convex hull area
	if (x1 == -1 && y1 == -1) {
		return 0.0;
	}

	// second, find the convex hull segment of other component that intersects with the (this->gravityCenter, other->gravityCenter) segment
	if (other->vertices.size() == 2) {  // component only has 1 pixel
		x2 = other->gravityCenter.x;
		y2 = other->gravityCenter.y;
	}
	else {
		for (int i = 0; i < (int)other->vertices.size()-1; ++i) {
			// if a and b are in different sides of segment (c, d)
			// and c and d are in different sides of segment (a, b)
			// then segment (a, b) intersects with segment (c, d)
			const Point &a = this->gravityCenter, &b = other->gravityCenter;
			const Point &c = other->vertices[i], &d = other->vertices[i+1];
			if (((long)turnDir(a, b, c) * turnDir(a, b, d) <= 0) &&
				((long)turnDir(c, d, a) * turnDir(c, d, b) <= 0)) {
				long m1 = (long)(b.y-a.y)*a.x + (long)(a.x-b.x)*a.y;
				long m2 = (long)(d.y-c.y)*c.x + (long)(c.x-d.x)*c.y;
				long D = (long)(b.x-a.x)*(d.y-c.y) - (long)(d.x-c.x)*(b.y-a.y);
				long D1 = (long)m2*(b.x-a.x) - (long)m1*(d.x-c.x);
				long D2 = (long)m2*(b.y-a.y) - (long)m1*(d.y-c.y);
				if (D == 0)  // if D == 0, then (a, b) and (c, d) are parallel
					continue;
				else {
					x2 = D1 / D;
					y2 = D2 / D;
				}
				break;
			}
		}
	}
	// one gravityCenter is inside the other components convex hull area
	if (x2 == -1 && y2 == -1) {
		return 0.0;
	}
	// segment (this->gravityCenter, other->gravityCenter) cross other convex hull boundary first
	// distance in this case is 0
	if (abs(x1 - this->gravityCenter.x) > abs(x2 - this->gravityCenter.x) ||
		abs(y1 - this->gravityCenter.y) > abs(y2 - this->gravityCenter.y)) {
		return 0.0;
	}
	return sqrt((double)(x2-x1)*(x2-x1) + (double)(y2-y1)*(y2-y1));
}
