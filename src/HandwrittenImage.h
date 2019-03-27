#ifndef __HANDWRITTENIMAGE_H__
#define __HANDWRITTENIMAGE_H__

#include <cstdint>
#include <vector>
#include "Point.h"
using std::vector;
using std::swap;

class ConvexHullComponent;

class HandwrittenImage {
public:
	enum PIXTYPE {BINPIX, BINPIXBR, CHARH, BLURPIX, SPACETRACINGSEEDS,
		          SPACETRACES, REGIONS, TEXTTRACINGSEEDS, TEXTTRACES, TEXTLINES, NOSLANT, CONVEXHULL, WORDMAP};
	typedef vector< vector<int32_t> > PIXELS;

	HandwrittenImage();
	~HandwrittenImage();
	void readOneBitBMP(const char *fileName);
	void writeBMP(const char *fileName, PIXTYPE type) const;

	void removeBorder(double hWeight, double vWeight, double threshold);
	void calcCharHeight(double diffPct, double cutoffFactor);
	void blur(int blurW, int blurH);
	void initBlurPixFstOrdParDerivY (int winH);
	void initBlurPixScdOrdParDerivY (int winH);
	void initSpaceTracingSeeds(int hSeedDist, int vSeedDist);
	void segmentRegions();
	void labelRegions(int minArea, double minBlackRatio, double maxBlackRatio);
	void initTextTracingSeeds(int hSeedDist, int vSeedDist);
	void locateTextLineCenters();
	void assignComponentsToRegions();
	void slantCorrection();
	void genConvexHullComponents();
	void extractWord(double centerStrapWidth, int minW, int minH, double threshold, double alpha);
	void writeWords(const char *basename) const;

	int getWidth() { return width; }
	int getHeight() { return height; }
	int getCharH() { return charH; }
private:
	struct ComponentInfo;
	struct RegionInfo;
	struct WordBBox;

	enum COLOR {BIN, GRAY, RGB};
	enum CONNMODE {NEIGHBOR4, NEIGHBOR8};

	void traceSpace(int seedX, int seedY);
	void traceText(int seedX, int seedY);
	void genComponentChainCode(vector<int> &res, int xCoord, int yCoord);

	ComponentInfo getComponentInfo(PIXELS &pix, int xCoord, int yCoord, int val1, int val2);
	RegionInfo getRegionInfo(PIXELS &pix, int xCoord, int yCoord, int val1, int val2);
	void colorComponent(PIXELS &pix, int xCoord, int yCoord, int val1, int val2, CONNMODE mode);
	void colorRegion(PIXELS &pix, int xCoord, int yCoord, int val1, int val2);
	int getComponentRegionID(PIXELS &pix, int xCoord, int yCoord, int val1, int val2);

	void writeOneBitBMP(const char *fileName, const PIXELS &pix) const;
	void write24BitBMP(const char *fileName, const PIXELS &pix, COLOR color) const;

	void drawLine(PIXELS &pix, Point a, Point b, int val);

	PIXELS binPix;  // original binary pixels
	PIXELS binPixBR;  // border removed binary pixels
	PIXELS blurPix;  // blur pixels in grayscale
	PIXELS blurPixFstOrdParDerivY;  // blurPix first-order partial derivative of Y
	PIXELS blurPixScdOrdParDerivY;  // blurPix second-order partial derivative of Y
	PIXELS spaceTraces;  // in-line space traces
	PIXELS regionMap;  // store line regions
	PIXELS textTraces;  // text line traces
	PIXELS textLineMap;  // store text lines, in this map all components are assigned to their corresponding lines
	PIXELS noSlantTextLineMap;  // store no slant text lines map
	PIXELS convexHullPix;
	PIXELS wordMap;
	vector<Point> spaceTracingSeeds;
	vector<Point> textTracingSeeds;
	vector<ConvexHullComponent *> allConvexHullComponents;
	vector<WordBBox> allWordBBox;
	int width;   // image width in pixel
	int height;  // image height in pixel
	int charH;   // average character height
};

#endif
