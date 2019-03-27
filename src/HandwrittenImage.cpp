#include <climits>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <map>
#include <array>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include "HandwrittenImage.h"
#include "ConvexHullComponent.h"
#include "Point.h"
#include "GroupTree.h"
#include "MsgPrint.h"

#define PI 3.14159265

using std::map;
using std::array;
using std::queue;
using std::make_pair;
using std::min;
using std::max;

HandwrittenImage::HandwrittenImage() {
	width = -1;
	height = -1;
	charH = -1;
}

HandwrittenImage::~HandwrittenImage() {

}

void HandwrittenImage::readOneBitBMP(const char *fileName) {
	char msg[1000];
	sprintf(msg, "Reading image %s ......", fileName);
	MsgPrint::msgPrint(MsgPrint::INFO, msg);

	FILE *f = fopen(fileName, "rb");

	// validate file stream
	if (f == NULL) {
		char msg[1000];
		sprintf(msg, "Cannot open file %s.", fileName);
		MsgPrint::msgPrint(MsgPrint::ERR, msg);
	}

	// BMP file has 54 byte header
	uint8_t header[54];
	if (fread(header, 1, 54, f) != 54) {
		MsgPrint::msgPrint(MsgPrint::ERR, "Cannot read BMP header.");
	}
	
	width = *(uint32_t*)&header[18];   // width of the image in pixel
	height = *(uint32_t*)&header[22];   // height of the image in pixel
	uint16_t sig = *(uint16_t*)&header[0];  // signature of the image
	uint16_t bitCnt = *(uint16_t*)&header[28];  // # of bits per pixel

	// verify the file is 1-bit BMP
	if (sig != 0x4D42 || bitCnt != 1)
		MsgPrint::msgPrint(MsgPrint::ERR, "Image is not 1-bit BMP.\n");

	// lines are aligned on 4-byte boundary
	uint32_t lineSize = (width + 31) / 32 * 4;
	uint32_t dataSize = lineSize * height;
	uint8_t *data = new uint8_t[dataSize];
	binPix = PIXELS(width, vector<int32_t>(height, -1));

	// color table - 2 X numbers of colors bytes, 8 bytes for 1-bit BMP
	uint8_t palette[8];
	if (fread(palette, 1, 8, f) != 8) {
		MsgPrint::msgPrint(MsgPrint::ERR, "Cannot read BMP color palette.");
	}

	// read data
	if (fread(data, 1, dataSize, f) != dataSize) {
		MsgPrint::msgPrint(MsgPrint::ERR, "Cannot read BMP color data.");
	}

	// pixels' data in BMP are stored from bottom to top (first row in BMP is the bottom most row in image)
	// Here, make top left corner as (0, 0)
	for(int j = height-1; j >= 0; --j) {
		for(int i = 0 ; i <= width/8; ++i) {
			int dpos = (height-1-j)*lineSize + i;
			for(int k = 0; k < 8; k++) {
				if(i < width/8  ||  k >= 8 - width % 8) {
					binPix[i*8 + 7-k][j] = (data[dpos] >> k ) & 1 ? 0 : 1;  // in BMP 1->whit 0->black
				}
			}
		}
	}
	delete [] data;
	fclose(f);
}

void HandwrittenImage::writeBMP(const char *fileName, PIXTYPE type) const {
	char msg[1000];
	sprintf(msg, "Writing image %s ......", fileName);
	MsgPrint::msgPrint(MsgPrint::INFO, msg);

	PIXELS pix;
	COLOR color = BIN;

	switch (type) {
		case BINPIX:
			pix = binPix;
			color = BIN;
			break;
		case BINPIXBR:
			pix = binPixBR;
			color = BIN;
			break;
		case CHARH:
			pix = binPixBR;
			for (int y = 0; y < height; y += charH) {
				for (int x = 0; x < width; ++x) {
					pix[x][y] = 1;
				}
			}
			color = BIN;
			break;
		case BLURPIX:
			pix = blurPix;
			color = GRAY;
			break;
		case SPACETRACINGSEEDS:
			pix = blurPix;
			for (size_t k = 0; k < spaceTracingSeeds.size(); ++k) {
				int x = spaceTracingSeeds[k].x;
				int y = spaceTracingSeeds[k].y;
				for (int i = x-5; i <= x+5; ++i) {
					for (int j = y-5; j <= y+5; ++j) {
						if (i >= 0 && i < width && j >=0 && j < height)
							pix[i][j] = 0;
					}
				}
			}
			color = GRAY;
			break;
		case SPACETRACES:
			pix = blurPix;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					if (spaceTraces[x][y] == 1) {
						for (int i = y-2; i <= y+2; ++i) {
							if (i >= 0 && i < height)
								pix[x][i] = 0;  // black
						}
					}
				}
			}
			color = GRAY;
			break;
		case REGIONS:
			pix = regionMap;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					if (binPixBR[x][y] == 1)
						pix[x][y] = -1;
				}
			}
			color = RGB;
			break;
		case TEXTTRACINGSEEDS:
			pix = blurPix;
			for (size_t k = 0; k < textTracingSeeds.size(); ++k) {
				int x = textTracingSeeds[k].x;
				int y = textTracingSeeds[k].y;
				for (int i = x-5; i <= x+5; ++i) {
					for (int j = y-5; j <= y+5; ++j) {
						if (i >= 0 && i < width && j >=0 && j < height)
							pix[i][j] = 0;
					}
				}
			}
			color = GRAY;
			break;
		case TEXTTRACES:
			pix = binPixBR;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					if (binPixBR[x][y] == 1)
						pix[x][y] = -1;
					if (textTraces[x][y] != 0) {
						for (int i = y-3; i <= y+3; ++i) {
							if (i >= 0 && i < height)
								pix[x][i] = textTraces[x][y];
						}
					}
				}
			}
			color = RGB;
			break;
		case TEXTLINES:
			pix = textLineMap;
			color = RGB;
			break;
		case NOSLANT:
			pix = noSlantTextLineMap;
			color = RGB;
			break;
		case CONVEXHULL:
			pix = convexHullPix;
			color = RGB;
			break;
		case WORDMAP:
			pix = wordMap;
			color = RGB;
			break;
		default:
			MsgPrint::msgPrint(MsgPrint::ERR, "Unexpected PIXTYPE.");
	}
	
	if (color == BIN)
		writeOneBitBMP(fileName, pix);
	else
		write24BitBMP(fileName, pix, color);
}

void HandwrittenImage::writeOneBitBMP(const char *fileName, const PIXELS &pix) const {
	char msg[1000];
	FILE *f = fopen(fileName, "wb");

	// validate file stream
	if (f == NULL) {
		sprintf(msg, "Cannot open file %s.", fileName);
		MsgPrint::msgPrint(MsgPrint::ERR, msg);
	}

	// make sure pix contains value
	if (pix.size() == 0 || pix[0].size() == 0) {
		sprintf(msg, "Cannot write to file %s, data is invalid.", fileName);
		MsgPrint::msgPrint(MsgPrint::ERR, msg);
	}

	int w = pix.size();
	int h = pix[0].size();

	// 1-bit BMP header
	int lineSize = (w + 31) / 32 * 4;
	uint8_t header[54];
	*(uint16_t*)&header[0] = 0x4D42;  // signature of the image
	*(uint32_t*)&header[2] = 54 + 8 + lineSize*h;  // file size
	*(uint16_t*)&header[6] = 0;  // reserved 0
	*(uint16_t*)&header[8] = 0;  // reserved 1
	*(uint32_t*)&header[10] = 54 + 8;  // offset to start of pixel data
	*(uint32_t*)&header[14] = 40;  // header size
	*(uint32_t*)&header[18] = w;  // width
	*(uint32_t*)&header[22] = h;  // height
	*(uint16_t*)&header[26] = 1;  // image planes
	*(uint16_t*)&header[28] = 1;  // bit per pixel
	*(uint32_t*)&header[30] = 0;  // compression type
	*(uint32_t*)&header[34] = 0;
	*(uint32_t*)&header[38] = 0;
	*(uint32_t*)&header[42] = 0;
	*(uint32_t*)&header[46] = 0;
	*(uint32_t*)&header[50] = 0;

	// 1-bit BMP color table
	uint8_t palette[8];
	*(uint32_t*)&palette[0] = 0;
	*(uint32_t*)&palette[4] = 0xFFFFFFFF;
	
	fwrite(header, 1, 54, f);
	fwrite(palette, 1, 8, f);
	uint8_t oneByte[1];
	// the bottom most line in image is the first line in BMP
	for (int j = h-1; j >= 0; --j) {
		for (int i = 0; i < lineSize; ++i) {
			oneByte[0] = 0;
			for (int k = 7; k >= 0; --k) {
				int x = i*8 + 7-k;
				if (x < w && pix[x][j] == 0) { // white pixel, 1 in BMP
					oneByte[0] += 1<<k;
				}
			}
			fwrite(oneByte, 1, 1, f);
		}
	}
	fclose(f);
}

void HandwrittenImage::write24BitBMP(const char *fileName, const PIXELS &pix, COLOR color) const {
	char msg[1000];
	if (color != GRAY && color != RGB)
		MsgPrint::msgPrint(MsgPrint::ERR, "Function 'write24BitBMP' only accept COLOR = GRAY or RGB");

	FILE *f = fopen(fileName, "wb");

	// validate file stream
	if (f == NULL) {
		sprintf(msg, "Cannot open file %s.", fileName);
		MsgPrint::msgPrint(MsgPrint::ERR, msg);
	}

	// make sure pix contains value
	if (pix.size() == 0 || pix[0].size() == 0) {
		sprintf(msg, "Cannot write to file %s, data is invalid.", fileName);
		MsgPrint::msgPrint(MsgPrint::ERR, msg);
	}

	int w = pix.size();
	int h = pix[0].size();

	// 24-bit BMP header
	int lineSize = (w*24 + 31) / 32 * 4;
	uint8_t header[54];
	*(uint16_t*)&header[0] = 0x4D42;  // signature of the image
	*(uint32_t*)&header[2] = 54 + lineSize*h;  // file size
	*(uint16_t*)&header[6] = 0;  // reserved 0
	*(uint16_t*)&header[8] = 0;  // reserved 1
	*(uint32_t*)&header[10] = 54;  // offset to start of pixel data
	*(uint32_t*)&header[14] = 40;  // header size
	*(uint32_t*)&header[18] = w;  // width
	*(uint32_t*)&header[22] = h;  // width
	*(uint16_t*)&header[26] = 1;  // image planes
	*(uint16_t*)&header[28] = 24;  // bit per pixel
	*(uint32_t*)&header[30] = 0;  // compression type
	*(uint32_t*)&header[34] = 0;
	*(uint32_t*)&header[38] = 0;
	*(uint32_t*)&header[42] = 0;
	*(uint32_t*)&header[46] = 0;
	*(uint32_t*)&header[50] = 0;

	// 24-bit BMP doesn't have color table

	fwrite(header, 1, 54, f);

	// color order of 24-bit BMP is Blue Green Red
	uint8_t bgr[3];
	if (color == GRAY) {
		// bottom most line in image is the first line in BMP
		for (int j = height-1; j >= 0; --j) {
			for (int i = 0; i < lineSize; i += 3) {
				int x = i/3;
				if (x < w) {
					bgr[0] = pix[x][j];
					bgr[1] = bgr[0];
					bgr[2] = bgr[0];
					fwrite(bgr, 1, 3, f);
				}
				else { // padding bits
					bgr[0] = 0;
					bgr[1] = 0;
					bgr[2] = 0;
					fwrite(bgr, 1, lineSize-w*3, f);
				}
			}
		}
	}
	else {  // color == RGB
		// color order of 24-bit BMP is Blue Green Red
		const int colors[12][3] = {
			{34, 35, 227},
			{0, 229, 224},
			{178, 113, 38},
			{91, 142, 0},
			{1, 145, 241},
			{137, 56, 109},
			{11, 198, 253},
			{31, 98, 234},
			{125, 3, 196},
			{153, 78, 68},
			{187, 150, 6},
			{38, 187, 140}
		};

		// bottom most line in image is the first line in BMP
		for (int j = h-1; j >= 0; --j) {
			for (int i = 0; i < lineSize; i += 3) {
				int x = i/3;
				if (x < w) {
					// -1: content black pixel
					if (pix[x][j] == -1) {
						bgr[0] = 0;
						bgr[1] = 0;
						bgr[2] = 0;
						fwrite(bgr, 1, 3, f);
					}
					// 0: white space
					else if (pix[x][j] == 0) {
						bgr[0] = 255;
						bgr[1] = 255;
						bgr[2] = 255;
						fwrite(bgr, 1, 3, f);
					}
					else {
						int index = pix[x][j] % 12;
						bgr[0] = colors[index][0];
						bgr[1] = colors[index][1];
						bgr[2] = colors[index][2];
						fwrite(bgr, 1, 3, f);
					}
				}
				else { // padding bits
					bgr[0] = 0;
					bgr[1] = 0;
					bgr[2] = 0;
					fwrite(bgr, 1, lineSize-w*3, f);
				}
			}
		}
	}
	fclose(f);
}

// each segments is represented as a (start, end) pair
struct segment {
	int st; // start point of the segment
	int ed; // end point of the sefment
	int type; // 0: white segment, 1: black segment
	segment(int st, int ed, int type) {
		this->st = st;
		this->ed = ed;
		this->type = type;
	}
	int len() {
		return ed - st + 1;
	}
};

// for a given pixel, if the black horizontal_segment_length * hWeight + vertical_segment_length * vWeight > threshold
// then this pixel is a border pixel
void HandwrittenImage::removeBorder(double hWeight, double vWeight, double threshold) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Removing Border ......");
	this->binPixBR = binPix;
	vector< vector<segment> > hSeg, vSeg;
	vector<segment> allSeg;

	// merge horizontal segments
	for (int y = 0; y < height; ++y) {
		int st = 0;
		allSeg.clear(); // segments of a row
		for (int x = 1; x < width; ++x) {
			if (binPix[x-1][y] != binPix[x][y]) {
				allSeg.push_back(segment(st, x-1, binPix[x-1][y]));
				st = x;
			}
		}
		allSeg.push_back(segment(st, width-1, binPix[width-1][y])); // handle last black pixel row in each row
		hSeg.push_back(allSeg);
	}

	// merge vertical segments
	for (int x = 0; x < width; ++x) {
		int st = 0;
		allSeg.clear(); // segments of a row
		for (int y = 1; y < height; ++y) {
			if (binPix[x][y-1] != binPix[x][y]) {
				allSeg.push_back(segment(st, y-1, binPix[x][y-1]));
				st = y;
			}
		}
		allSeg.push_back(segment(st, height-1, binPix[x][height-1])); // handle last black pixel row in each row
		vSeg.push_back(allSeg);
	}

	// remove border
	// store the sum of hSegment and vSegment length that run through each pixel
	PIXELS segLenMap = PIXELS(width, vector<int32_t>(height, 0)); 

	for (int y = 0; y < height; ++y) {
		for (size_t s = 0; s < hSeg[y].size(); ++s) {
			if (hSeg[y][s].type == 1) {
				for (int i = hSeg[y][s].st; i <= hSeg[y][s].ed; ++i)
					segLenMap[i][y] = hWeight * hSeg[y][s].len();
			}
		}
	}
	for (int x = 0; x < width; ++x) {
		for (size_t s = 0; s < vSeg[x].size(); ++s) {
			if (vSeg[x][s].type == 1) {
				for (int i = vSeg[x][s].st; i <= vSeg[x][s].ed; ++i)
					segLenMap[x][i] += vWeight * vSeg[x][s].len();
			}
		}
	}

	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (segLenMap[x][y] > threshold*height) {
				binPixBR[x][y] = 0;
			}
		}
	}
}

// do BFS, color a connected component at (xCoord, yCoord) in pix from val1 to val2
void HandwrittenImage::colorComponent(HandwrittenImage::PIXELS &pix, int xCoord, int yCoord, int val1, int val2, CONNMODE mode) {
	if (pix[xCoord][yCoord] != val1) {
		printf ("%d, %d\n", val1, pix[xCoord][yCoord]);
		MsgPrint::msgPrint(MsgPrint::ERR, "Wrong input arguments to call 'colorComponent'");
	}
	queue<Point> q;
	q.push(Point(xCoord, yCoord));

	if (mode == NEIGHBOR4) {
		while (!q.empty()) {
			int x = q.front().x;
			int y = q.front().y;
			q.pop();
			if (pix[x][y] == val1) {
				pix[x][y] = val2;
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
	}
	else if (mode == NEIGHBOR8) {
		while (!q.empty()) {
			int x = q.front().x;
			int y = q.front().y;
			q.pop();
			if (pix[x][y] == val1) {
				pix[x][y] = val2;
				if (x > 0)
					q.push(Point(x-1, y));
				if (x < width-1)
					q.push(Point(x+1, y));
				if (y > 0)
					q.push(Point(x, y-1));
				if (y < height-1)
					q.push(Point(x, y+1));
				if (x > 0 && y > 0)
					q.push(Point(x-1, y-1));
				if (x < width-1 && y > 0)
					q.push(Point(x+1, y-1));
				if (x > 0 && y < height-1)
					q.push(Point(x-1, y+1));
				if (x < width-1 && y < height-1)
					q.push(Point(x+1, y+1));
			}
		}
	}
	else
		MsgPrint::msgPrint(MsgPrint::ERR, "Unexpected CONNMODE to call 'colorComponent'");
}

struct HandwrittenImage::ComponentInfo {
	int area, xl, xh, yl, yh; // area and bounding box of a connected component
	ComponentInfo() {
		area = 0;
		xl = INT_MAX;
		xh = INT_MIN;
		yl = INT_MAX;
		yh = INT_MIN;
	}
};

// do BFS, get MsgPrint::INFOrmation of a connected component at (xCoord, yCoord) in pix from val1 to val2
HandwrittenImage::ComponentInfo HandwrittenImage::getComponentInfo(HandwrittenImage::PIXELS &pix, int xCoord, int yCoord, int val1, int val2) {
	if (pix[xCoord][yCoord] != val1)
		MsgPrint::msgPrint(MsgPrint::ERR, "Wrong input arguments to call 'getComponentInfo'");
	ComponentInfo res;
	queue<Point> q;
	q.push(Point(xCoord, yCoord));
	while (!q.empty()) {
		int x = q.front().x;
		int y = q.front().y;
		q.pop();
		
		if (pix[x][y] == val1) {
			// update res
			res.area += 1;
			res.xl = min(res.xl, x);
			res.xh = max(res.xh, x);
			res.yl = min(res.yl, y);
			res.yh = max(res.yh, y);

			pix[x][y] = val2;
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
	return res;
}

// do BFS, color a connected region at (xCoord, yCoord) in pix from val1 to val2
// similar function as colorComponent
void HandwrittenImage::colorRegion(HandwrittenImage::PIXELS &pix, int xCoord, int yCoord, int val1, int val2) {
	if (pix[xCoord][yCoord] != val1)
		MsgPrint::msgPrint(MsgPrint::ERR, "Wrong input arguments to call 'colorRegion'");
	queue<Point> q;
	q.push(Point(xCoord, yCoord));
	while (!q.empty()) {
		int x = q.front().x;
		int y = q.front().y;
		q.pop();
		if (pix[x][y] == val1) {
			pix[x][y] = val2;
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
}

struct HandwrittenImage::RegionInfo {
	int area, blackPixCnt, xl, xh, yl, yh;
	RegionInfo() {
		area = 0;
		blackPixCnt = 0;
		xl = INT_MAX;
		xh = INT_MIN;
		yl = INT_MAX;
		yh = INT_MIN;
	}
};

// do BFS, get MsgPrint::INFOrmation of a Region at (xCoord, yCoord) in pix from val1 to val2
HandwrittenImage::RegionInfo HandwrittenImage::getRegionInfo(HandwrittenImage::PIXELS &pix, int xCoord, int yCoord, int val1, int val2) {
	if (pix[xCoord][yCoord] != val1)
		MsgPrint::msgPrint(MsgPrint::ERR, "Wrong input arguments to call 'getRegionInfo'");
	RegionInfo res;
	queue<Point> q;
	q.push(Point(xCoord, yCoord));
	while (!q.empty()) {
		int x = q.front().x;
		int y = q.front().y;
		q.pop();
		
		if (pix[x][y] == val1) {
			// update res
			res.area += 1;
			res.xl = min(res.xl, x);
			res.xh = max(res.xh, x);
			res.yl = min(res.yl, y);
			res.yh = max(res.yh, y);
			if (binPixBR[x][y] == 1)
				res.blackPixCnt += 1;

			pix[x][y] = val2;
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
	return res;
}

void HandwrittenImage::calcCharHeight(double diffPct, double cutoffFactor) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Calculating average charactor height ......");
	PIXELS tmpBinPix = binPixBR;
	// List of height, width of the component bounding box, and area (# of black pixels) of the component
	vector<int> hList, wList, aList;
	vector<bool> isValid;  // component is considered for charH calculation
	
	// traverse all components
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (tmpBinPix[x][y] == 1) {
				// mark visited pixel as -1
				ComponentInfo info = getComponentInfo(tmpBinPix, x, y, 1, -1);
				hList.push_back(info.yh - info.yl + 1);
				wList.push_back(info.xh - info.xl + 1);
				aList.push_back(info.area);

				isValid.push_back(true);
			}
		}
	}

	// get weighted average charactor height, use area as weight
	int lastWAvgCharH = height;
	while (true) {
		double wAvgCharH = 0;
		double totArea = 0;
		for (size_t i = 0; i < hList.size(); ++i) {
			if (isValid[i]) {
				wAvgCharH += hList[i]*aList[i];
				totArea += aList[i];
			}
		}
		wAvgCharH /= totArea;
		
		// if diff is smaller than diffPct, stop iteration
		if (lastWAvgCharH - wAvgCharH < diffPct*lastWAvgCharH) {
			this->charH = wAvgCharH;
			return;
		}

		// remove too high component (most likely touched components)
		int threshold = cutoffFactor*wAvgCharH;
		for (size_t i = 0; i < hList.size(); ++i) {
			if (hList[i] > threshold)
				isValid[i] = false;
		}
		lastWAvgCharH = wAvgCharH;
	}
}

void HandwrittenImage::blur(int blurW, int blurH) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Blurring the image ......");
	if (blurH >= height)
		MsgPrint::msgPrint(MsgPrint::ERR, "Too big window height for image blurring ......");
	if (blurW >= width)
		MsgPrint::msgPrint(MsgPrint::ERR, "Too big window width for image blurring ......");

	blurPix = binPixBR;

	for (int y = 0; y < height; ++y) {
		int yl = max(y-blurH/2, 0);
		int yh = min(y+blurH/2, height-1);
		int sum = 0;
		for (int x = 0; x < width; ++x) {
			int xll = x-blurW/2;
			int xhh = x+blurW/2;
			int xl = max(xll, 0);
			int xh = min(xhh, width-1);
			// initialize sum of blur area
			if (x == 0) {
				sum = 0;
				for (int i = xl; i <= xh; ++i) {
					for (int j = yl; j <= yh; ++j) {
						sum += binPixBR[i][j];
					}
				}
			}
			// don't need to recalculate the whole blur area at each move
			// just need to subtract one column, and/or add one column
			else if (xll <= 0 && xhh < width) {
				for (int i = yl; i <= yh; ++i)
					sum += binPixBR[xh][i];
			}
			else if (xll > 0 && xhh < width) {
				for (int i = yl; i <= yh; ++i)
					sum += (binPixBR[xh][i] - binPixBR[xl-1][i]);
			}
			else if (xll > 0 && xhh >= width) {
				for (int i = yl; i <= yh; ++i)
					sum -= binPixBR[xl-1][i];
			}
			blurPix[x][y] = 255 - 255*sum/(xh-xl+1)/(yh-yl+1);
		}
	}
}

void HandwrittenImage::initBlurPixFstOrdParDerivY (int winH) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Initializing first-order partial derivative of Y of blurred image ......");
	if (winH >= height)
		MsgPrint::msgPrint(MsgPrint::ERR, "Too big window height for first-order partial derivative calculation ......");

	int ofs = winH/2;
	blurPixFstOrdParDerivY = PIXELS(width, vector<int>(height, 0));

	for (int x = 0; x < width; ++x) {
		int suml = 0;
		int sumh = 0;
		for (int y = 0; y < height; ++y) {
			int yll = y - ofs;
			int yhh = y + ofs;
			int yl = max(yll, 0);
			int yh = min(yhh, height-1);

			// don't recalculate suml & sumh at each move
			// only need to add/subtract several numbers to update suml & sumh
			if (y == 0) {
				suml = 0;
				sumh = 0;
				for (int i = yl; i <= y; ++i)
					suml += blurPix[x][i];
				for (int i = y; i <= yh; ++i)
					sumh += blurPix[x][i];
			}
			else if (yll <= 0 && yhh < height) {
				suml += blurPix[x][y];
				sumh += (blurPix[x][yh] - blurPix[x][y-1]);
			}
			else if (yll > 0 && yhh < height) {
				suml += (blurPix[x][y] - blurPix[x][yl-1]);
				sumh += (blurPix[x][yh] - blurPix[x][y-1]);
			}
			else if (yll > 0 && yhh >= height) {
				suml += (blurPix[x][y] - blurPix[x][yl-1]);
				sumh -= blurPix[x][y-1];
			}
			blurPixFstOrdParDerivY[x][y] = sumh/(yh-y+1) - suml/(y-yl+1);
		}
	}
}

void HandwrittenImage::initBlurPixScdOrdParDerivY (int winH) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Initializing second-order partial derivative of Y of blurred image ......");
	if (winH >= height)
		MsgPrint::msgPrint(MsgPrint::ERR, "Too big window height for second-order partial derivative calculation ......");

	int ofs = winH/2;
	blurPixScdOrdParDerivY = PIXELS(width, vector<int>(height, 0));

	for (int x = 0; x < width; ++x) {
		int suml = 0;
		int sumh = 0;
		for (int y = 0; y < height; ++y) {
			int yll = y - ofs;
			int yhh = y + ofs;
			int yl = max(yll, 0);
			int yh = min(yhh, height-1);

			// don't recalculate suml & sumh at each move
			// only need to add/subtract several numbers to update suml & sumh
			if (y == 0) {
				suml = 0;
				sumh = 0;
				for (int i = yl; i <= y; ++i)
					suml += blurPixFstOrdParDerivY[x][i];
				for (int i = y; i <= yh; ++i)
					sumh += blurPixFstOrdParDerivY[x][i];
			}
			else if (yll <= 0 && yhh < height) {
				suml += blurPixFstOrdParDerivY[x][y];
				sumh += (blurPixFstOrdParDerivY[x][yh] - blurPixFstOrdParDerivY[x][y-1]);
			}
			else if (yll > 0 && yhh < height) {
				suml += (blurPixFstOrdParDerivY[x][y] - blurPixFstOrdParDerivY[x][yl-1]);
				sumh += (blurPixFstOrdParDerivY[x][yh] - blurPixFstOrdParDerivY[x][y-1]);
			}
			else if (yll > 0 && yhh >= height) {
				suml += (blurPixFstOrdParDerivY[x][y] - blurPixFstOrdParDerivY[x][yl-1]);
				sumh -= blurPixFstOrdParDerivY[x][y-1];
			}
			blurPixScdOrdParDerivY[x][y] = sumh/(yh-y+1) - suml/(y-yl+1);
		}
	}
}

// hSeedDist, vSeedDist: distance between adjacent seedss
void HandwrittenImage::initSpaceTracingSeeds(int hSeedDist, int vSeedDist) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Initializing in-line space tracing seeds ......");

	// initialize space tracing seeds
	spaceTracingSeeds.clear();
	for (int i = 0; i < width; i += hSeedDist) {
		for (int j = 0; j < height; j += vSeedDist) {
			int x = i, y = j;
			int origDeriv = blurPixFstOrdParDerivY[x][y];

			// find local whitest point in current pixel column
			while (origDeriv * blurPixFstOrdParDerivY[x][y] > 0) {
				if (blurPixFstOrdParDerivY[x][y] > 0) {
					if (++y >= height) {
						y = height-1;
						break;
					}
				}
				else {
					if (--y < 0) {
						y = 0;
						break;
					}
				}
			}
	
			// only keep seeds in the white space
			// remove seeds that get trapped in text area
			if (blurPixScdOrdParDerivY[x][y] < 0)
				spaceTracingSeeds.push_back(Point(x, y));
		}
	}
}

void HandwrittenImage::traceSpace(int seedX, int seedY) {
	// this point has been traced
	if (spaceTraces[seedX][seedY] == 1)
		return;

	// trace[x] = y, store a trace
	vector<int> trace(width, 0);
	trace[seedX] = seedY;  // initialize a trace at seed position
	spaceTraces[seedX][seedY] = 1;

	// seed to right trace
	for (int x = seedX+1; x < width; ++x) {
		int preX = x - 1;
		int preY = trace[preX];

		// move to the whiter area
		if (blurPixFstOrdParDerivY[preX][preY] > 0)
			trace[x] = min(preY+1, height-1);
		else if (blurPixFstOrdParDerivY[preX][preY] < 0)
			trace[x] = max(preY-1, 0);
		else
			trace[x] = preY;

		// if this point has been reached by any other trace, then stop tracing
		if (spaceTraces[x][trace[x]] == 1)
			break;
		spaceTraces[x][trace[x]] = 1;
	}

	// seed to left trace
	for (int x = seedX-1; x >= 0; --x) {
		int preX = x + 1;
		int preY = trace[preX];

		// move to the whiter area
		if (blurPixFstOrdParDerivY[preX][preY] > 0)
			trace[x] = min(preY+1, height-1);
		else if (blurPixFstOrdParDerivY[preX][preY] < 0)
			trace[x] = max(preY-1, 0);
		else
			trace[x] = preY;

		// if this point has been reached by any other trace, then stop tracing
		if (spaceTraces[x][trace[x]] == 1)
			break;
		spaceTraces[x][trace[x]] = 1;
	}
}

void HandwrittenImage::segmentRegions() {
	MsgPrint::msgPrint(MsgPrint::INFO, "Segmenting image into line regions ......");

	// 0: space area -1: potential text area
	spaceTraces = PIXELS(width, vector<int32_t>(height, 0));
	
	for (size_t i = 0; i < spaceTracingSeeds.size(); ++i) {
		traceSpace(spaceTracingSeeds[i].x, spaceTracingSeeds[i].y);
	}
}

// region area < minArea would be disgarded
// region blackRatio < minBlackRatio would be disgarded
// region blackRatio > maxBlackRatio would be disgarded
void HandwrittenImage::labelRegions (int minArea, double minBlackRatio, double maxBlackRatio) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Labeling regions ......");
	// regionMap[x]
	//     -1: untouched potential line region
	//      0: white space
	//   1..n: labeled line region
	regionMap = PIXELS(width, vector<int32_t>(height, -1));
	
	// draw in-line space onto regionMap
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (spaceTraces[x][y] == 1)
				regionMap[x][y] = 0;
		}
	}
	
	int label = 1;
	for (int y = 0; y < height; ++y) {  // y == 0 is the top most row, from top to bottom
		for (int x = 0; x < width; ++x) {
			if (regionMap[x][y] == -1) {  // only handle untouched regions
				RegionInfo res = getRegionInfo(regionMap, x, y, -1, -99);
				double blackRatio = (double)res.blackPixCnt/res.area;
				if (res.area < minArea || blackRatio < minBlackRatio || blackRatio > maxBlackRatio)
					colorRegion(regionMap, x, y, -99, 0);
				else
					colorRegion(regionMap, x, y, -99, label++);
			}
		}
	}
}

void HandwrittenImage::initTextTracingSeeds(int hSeedDist, int vSeedDist) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Initializing text tracing seeds ......");
	
	// initialize text tracing seeds
	textTracingSeeds.clear();
	for (int i = 0; i < width; i += hSeedDist) {
		for (int j = 0; j < height; j += vSeedDist) {
			int x = i, y = j;
			int origDeriv = blurPixFstOrdParDerivY[x][y];

			// find local whitest point in current pixel column
			while (origDeriv * blurPixFstOrdParDerivY[x][y] > 0) {
				if (blurPixFstOrdParDerivY[x][y] < 0) {
					if (++y >= height) {
						y = height-1;
						break;
					}
				}
				else {
					if (--y < 0) {
						y = 0;
						break;
					}
				}
			}
	
			// only keep seeds in the text area
			// remove seeds that get trapped in space
			if (blurPixScdOrdParDerivY[x][y] > 0)
				textTracingSeeds.push_back(Point(x, y));
		}
	}
}

void HandwrittenImage::traceText(int seedX, int seedY) {
	// region id of region that contains the seed
	int regionID = regionMap[seedX][seedY];
	// seed point has been traced or this point is in space region, return
	if (textTraces[seedX][seedY] != 0 || regionID == 0)
		return;

	// trace[x] = y, store a trace
	vector<int> trace(width, 0);
	trace[seedX] = seedY;  // initialize a trace at seed position
	spaceTraces[seedX][seedY] = regionMap[seedX][seedY];

	// seed to right trace
	for (int x = seedX+1; x < width; ++x) {
		int preX = x - 1;
		int preY = trace[preX];

		// move to the blacker area
		if (blurPixFstOrdParDerivY[preX][preY] < 0)
			trace[x] = min(preY+1, height-1);
		else if (blurPixFstOrdParDerivY[preX][preY] > 0)
			trace[x] = max(preY-1, 0);
		else
			trace[x] = preY;

		// if this point has been reached by any other trace, then stop tracing
		// or this point reaches region boundary
		if (textTraces[x][trace[x]] == regionID || regionMap[x][trace[x]] != regionID)
			break;
		textTraces[x][trace[x]] = regionID;
	}

	// seed to left trace
	for (int x = seedX-1; x >= 0; --x) {
		int preX = x + 1;
		int preY = trace[preX];

		// move to the whiter area
		if (blurPixFstOrdParDerivY[preX][preY] < 0)
			trace[x] = min(preY+1, height-1);
		else if (blurPixFstOrdParDerivY[preX][preY] > 0)
			trace[x] = max(preY-1, 0);
		else
			trace[x] = preY;

		// if this point has been reached by any other trace, then stop tracing
		// or this point reaches region boundary
		if (textTraces[x][trace[x]] == regionID || regionMap[x][trace[x]] != regionID)
			break;
		textTraces[x][trace[x]] = regionID;
	}
}

void HandwrittenImage::locateTextLineCenters() {
	MsgPrint::msgPrint(MsgPrint::INFO, "Locate text line center of each region ......");

	// 0: space area -1: potential text area
	textTraces = PIXELS(width, vector<int32_t>(height, 0));
	
	for (size_t i = 0; i < textTracingSeeds.size(); ++i) {
		traceText(textTracingSeeds[i].x, textTracingSeeds[i].y);
	}
}

// if a component intersect with only one text line center, then return the region ID of that text line center
// if a component intersect with 0 or more than 1 line center, return -1
int HandwrittenImage::getComponentRegionID(HandwrittenImage::PIXELS &pix, int xCoord, int yCoord, int val1, int val2) {
	if (pix[xCoord][yCoord] != val1)
		MsgPrint::msgPrint(MsgPrint::ERR, "Wrong input arguments to call 'getComponentRegionID'");
	int res = -1;
	bool multipleCut = false;
	queue<Point> q;
	q.push(Point(xCoord, yCoord));
	while (!q.empty()) {
		int x = q.front().x;
		int y = q.front().y;
		q.pop();
		if (pix[x][y] == val1) {
			pix[x][y] = val2;

			if (multipleCut == false && textTraces[x][y] != 0) {
				if (res == -1)
					res = textTraces[x][y];
				else if (res != textTraces[x][y])
					multipleCut = true;
			}

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

	if (multipleCut)
		return -1;
	else
		return res;
}

void HandwrittenImage::assignComponentsToRegions() {
	MsgPrint::msgPrint(MsgPrint::INFO, "Assigning components to text line regions ......");

	textLineMap = binPixBR;
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (textLineMap[x][y] == 1)
				textLineMap[x][y] = -1;
		}
	}

	// color components that has only one intersection with textL line center
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (textLineMap[x][y] == -1) {
				int id = getComponentRegionID(textLineMap, x, y, -1, -99);
				if (id != -1)
					colorRegion(textLineMap, x, y, -99, id);
			}
		}
	}

	// color other components
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (textLineMap[x][y] == -99) {
				textLineMap[x][y] = regionMap[x][y];
			}
		}
	}
}

// slant correction is line-based
// this function calculate a slant angle and apply de-slant angle for each line
// freeman chain code algorithm is applied for slant angle estimation
void HandwrittenImage::slantCorrection() {
	MsgPrint::msgPrint(MsgPrint::INFO, "Correcting text slant ......");
	// first get startpoint of each connected components
	// startpoint is the left bottom corner of each components, row is searched first, then column
	vector<Point> componentStartPoints;
	PIXELS visited = textLineMap;
	int maxRegionID = 0;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (visited[x][y] > 0) {
				maxRegionID = max(visited[x][y], maxRegionID);
				componentStartPoints.push_back(Point(x, y));
				colorComponent(visited, x, y, visited[x][y], -1, NEIGHBOR8);  // mark all component as visited
			}
		}
	}

	vector< vector<int> > cc(maxRegionID+1); // chain code for each region (text line region)
	for (size_t i = 0; i < componentStartPoints.size(); ++i) {
		int x = componentStartPoints[i].x;
		int y = componentStartPoints[i].y;
		int regionID = textLineMap[x][y];
		if (regionID > 0) {
			genComponentChainCode(cc[regionID], x, y);
		}
	}

	// calculate slant angle of each region
	vector<double> slantAngle(maxRegionID+1, PI/2);
	for (int regionID = 1; regionID <= maxRegionID; ++regionID) {
		vector<int> cnt(8, 0);
		for (size_t i = 0; i < cc[regionID].size(); ++i) {
			cnt[cc[regionID][i]] += 1;
		}
		if (cnt[1] - cnt[3] != 0)
			slantAngle[regionID] = atan((double)(cnt[1]+cnt[2]+cnt[3])/(cnt[1]-cnt[3]));
	}

	// calculate slant correction reference Y coordinate of each region, here use average Y coordinate of textTrace of each region
	// use int64_t to avoid overflow
	vector<int64_t> slantRefY(maxRegionID+1, 0);
	vector<int64_t> slantRefYCnt(maxRegionID+1, 0);
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (textTraces[x][y] > 0) {
				slantRefY[textTraces[x][y]] += y;
				slantRefYCnt[textTraces[x][y]] += 1;
			}
		}
	}
	for (int regionID = 1; regionID <= maxRegionID; ++regionID) {
		if (slantRefYCnt[regionID] != 0)
			slantRefY[regionID] /= slantRefYCnt[regionID];
	}

	// do slant correction for each region
	noSlantTextLineMap = PIXELS(width, vector<int32_t>(height, 0));
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (textLineMap[x][y] > 0) {
				int regionID = textLineMap[x][y];
				int xOffset = (slantRefY[regionID]-y) * 1/tan(slantAngle[regionID]);
				if (x + xOffset >= 0 && x + xOffset < width)
					noSlantTextLineMap[x+xOffset][y] = regionID;
			}
		}
	}
}

// generate chain code representative of a component
void HandwrittenImage::genComponentChainCode(vector<int> &res, int xCoord, int yCoord) {
	/* chain code <=> direction mapping
	   dir[chain code] = {x offset, y offset}, where chain code is a int from 0-7. x offset, y offset are 1, 0, or -1
		  3  2  1
		   \ | /
		4 --   -- 0
		   / | \
		  5  6  7    */
	int dir[8][2] = {
		{1, 0},    // 0: 0 degree
		{1, 1},    // 1: 45
		{0, 1},    // 2: 90
		{-1, 1},   // 3: 135
		{-1, 0},   // 4: 180
		{-1, -1},  // 5: 225
		{0, -1},   // 6: 270
		{1, -1},   // 7: 315
	};
	int lineID = textLineMap[xCoord][yCoord];

	// make sure component has more than 1 pixels
	bool hasNeighbor = false;
	for (int i = 0; i < 8; ++i) {
		int x = xCoord + dir[i][0];
		int y = yCoord + dir[i][1];
		if (x >= 0 && x < width && y >= 0 && y < height && textLineMap[x][y] == lineID) {
			hasNeighbor = true;
			break;
		}
	}
	if (hasNeighbor == false)
		return;

	int lastDir = 0, curDir = 0;
	int curX = xCoord, curY = yCoord;
	do {
	 	//printf ("take: %d, %d <=> %d, %d\n", curX, curY, xCoord, yCoord);
		curDir = (lastDir+6) % 8;  // start searching from lastDir - 90 degree
		while (true) {
			int x = curX + dir[curDir][0];
			int y = curY + dir[curDir][1];
			if (x < 0 || x >= width || y < 0 || y >= height || textLineMap[x][y] != lineID)
				curDir = (curDir+1) % 8;  // next step is lastDir + 45 degree
			else
				break;
		}
		curX += dir[curDir][0];
		curY += dir[curDir][1];
		res.push_back(curDir);
		lastDir = curDir;
	} while (curX != xCoord || curY != yCoord); // keep searching until back to startpoint
}

void HandwrittenImage::genConvexHullComponents() {
	MsgPrint::msgPrint(MsgPrint::INFO, "Generating convex hull of all components ......");
	convexHullPix = noSlantTextLineMap;
	PIXELS tmpPix = noSlantTextLineMap;
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			if (tmpPix[x][y] > 0) {
				allConvexHullComponents.push_back(new ConvexHullComponent(tmpPix, x, y, -1));

				// draw the convex hull
				vector<Point> &v = allConvexHullComponents.back()->vertices;
				for (size_t i = 0; i < v.size()-1; ++i) {
					if (v[i] != v[i+1])
						drawLine(convexHullPix, v[i], v[i+1], -1);
				}
				// draw center of gravity
				Point &gc = allConvexHullComponents.back()->gravityCenter;
				for (int x = gc.x-2; x <= gc.x+2; ++x) {
					for (int y = gc.y-2; y <= gc.y+2; ++y) {
						if (x >= 0 && x < width && y >=0 && y < height)
							convexHullPix[x][y] = -1;
					}
				}
			}
		}
	}
	sort(allConvexHullComponents.begin(), allConvexHullComponents.end(),
			[](const ConvexHullComponent *a, const ConvexHullComponent *b) {
				if (a->regionID == b->regionID)
					return a->gravityCenter.x < b->gravityCenter.x;
				return a->regionID < b->regionID;
			}
		);
}

struct HandwrittenImage::WordBBox {
	int wordID;
	int regionID;
	int xl, xh, yl, yh;
	WordBBox (int _wordID, int _regionID, int _xl, int _xh, int _yl, int _yh) {
		wordID = _wordID;
		regionID = _regionID;
		xl = _xl;
		xh = _xh;
		yl = _yl;
		yh = _yh;
	}
};

void HandwrittenImage::extractWord(double centerStrapWidth, int minW, int minH, double threshold, double alpha) {
	MsgPrint::msgPrint(MsgPrint::INFO, "Extracting words ......");

	vector< vector<ConvexHullComponent *> > componentsOnTextTrace;

	for (size_t index = 0; index < allConvexHullComponents.size(); ++index) {
		ConvexHullComponent *chc = allConvexHullComponents[index];
		if ((chc->xh - chc->xl < minW) || (chc->yh - chc->yl < minH))
			continue;

		int id = chc->regionID;
		// avoid out_of_range
		for (int k = componentsOnTextTrace.size(); k <= id; ++k) {
			componentsOnTextTrace.push_back(vector<ConvexHullComponent *>());
		}
		// only add components that intersect with textTraces center strap (center - 1/6*charH, cneter + 1/6*charH)
		int x = chc->gravityCenter.x;
		for (int y = chc->yl - centerStrapWidth/2*charH; y <= chc->yh + centerStrapWidth/2*charH; ++y) {
			if (textTraces[x][y] == id) {
				componentsOnTextTrace[id].push_back(chc);
				break;
			}
		}
	}

	// assign components on the textTraces to their corresponding words
	int curWordID = 1;
	for (size_t region_id = 0; region_id < componentsOnTextTrace.size(); ++region_id) {
		vector<double> gaps;
		gaps.push_back(width);  // leftgap of the first component is postive infinity
		for (int cc = 0; cc < (int)componentsOnTextTrace[region_id].size()-1; ++cc) {
			int xl = -1, xh = -1, yl = -1, yh = -1;
			double dist = componentsOnTextTrace[region_id][cc]->getDistance(componentsOnTextTrace[region_id][cc+1], xl, yl, xh, yh);
			gaps.push_back(dist);

			if (dist != 0)
				drawLine(convexHullPix, Point(xl, yl), Point(xh, yh), 12);
		}
		gaps.push_back(width);  // rightgap of the last component is positive infinity

		if (gaps.size() > 2) { // region contain at least valid component
			GroupTree gTree(gaps, threshold, alpha);
			gTree.grouping();
			vector< pair<int, int> > groups = gTree.getGroupingResult();
			for (size_t i = 0; i < groups.size(); ++i) {
				for (int k = groups[i].first; k <= groups[i].second; ++k) {
					componentsOnTextTrace[region_id][k]->wordID = curWordID;
				}
				curWordID += 1;
			}
		}
	}

	// assign components off the textTraces to their closest assigned component's wordID
	for (size_t i = 0; i < allConvexHullComponents.size(); ++i) {
		ConvexHullComponent *ptr = allConvexHullComponents[i];
		if (ptr->wordID == -1) {  // unassigned component
			double leftDist = 0, rightDist = 0;

			// find closest assigned component on the left
			int left = i-1;
			while (left >= 0 && allConvexHullComponents[left]->wordID == -1) --left;
			if (left < 0 || allConvexHullComponents[left]->regionID != ptr->regionID) // there is no component on the left
				leftDist = width;
			else {
				int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
				leftDist = ptr->getDistance(allConvexHullComponents[left], x1, y1, x2, y2);
				//if (leftDist != 0)
				//	drawLine(convexHullPix, Point(x1, y1), Point(x2, y2), 12);
			}

			// find closest assigned component on the right
			unsigned right = i+1;
			while (right < allConvexHullComponents.size() && allConvexHullComponents[right]->wordID == -1) ++right;
			if (right >= allConvexHullComponents.size() || allConvexHullComponents[right]->regionID != ptr->regionID)  // there is no components on the right
				rightDist = width;
			else {
				int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
				rightDist = ptr->getDistance(allConvexHullComponents[right], x1, y1, x2, y2);
				//if (rightDist != 0)
				//	drawLine(convexHullPix, Point(x1, y1), Point(x2, y2), 12);
			}

			if (leftDist < rightDist)
				ptr->wordID = allConvexHullComponents[left]->wordID;
			else if (leftDist > rightDist)
				ptr->wordID = allConvexHullComponents[right]->wordID;
			else {  //leftDist == rightDist
				if (leftDist == width) { // component is the only component in region and off the textTrace
					// keep words in order
					if (i == 0)
						ptr->wordID = 1;
					else
						ptr->wordID = allConvexHullComponents[i-1]->wordID + 1;
					for (size_t k = i+1; k < allConvexHullComponents.size(); ++k) {
						if (allConvexHullComponents[k]->wordID != -1)
							allConvexHullComponents[k]->wordID += 1;
					}
				}
				else
					ptr->wordID = allConvexHullComponents[left]->wordID;
			}
		}
	}

	// update wordMap
	wordMap = noSlantTextLineMap;
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			wordMap[x][y] *= -1;
		}
	}
	for (size_t i = 0; i < allConvexHullComponents.size(); ++i) {
		ConvexHullComponent *ptr = allConvexHullComponents[i];
		int x = ptr->startPoint.x, y = ptr->startPoint.y;
		colorComponent(wordMap, x, y, wordMap[x][y], ptr->wordID, NEIGHBOR4);
	}

	// generate WordBBox for each word
	map< int, array<int, 5> > allBBox;
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			int wordID = wordMap[x][y];
			if (wordID > 0) {
				if (allBBox.find(wordID) != allBBox.end()) {
					allBBox[wordID][1] = min(allBBox[wordID][1], x);  // xl
					allBBox[wordID][2] = max(allBBox[wordID][2], x);  // xh
					allBBox[wordID][3] = min(allBBox[wordID][3], y);  // yl
					allBBox[wordID][4] = max(allBBox[wordID][4], y);  // yh
				}
				else {
					allBBox[wordID][0] = noSlantTextLineMap[x][y];  // regionID
					allBBox[wordID][1] = x;  // xl
					allBBox[wordID][2] = x;  // xh
					allBBox[wordID][3] = y;  // yl
					allBBox[wordID][4] = y;  // yh
				}
			}
		}
	}
	allWordBBox.clear();
	for (map< int, array<int, 5> >::iterator it = allBBox.begin(); it != allBBox.end(); ++it) {
		allWordBBox.push_back(WordBBox(it->first, (it->second)[0], (it->second)[1], (it->second)[2], (it->second)[3], (it->second)[4]));

		/*
		// draw bbox
		WordBBox &w = allWordBBox.back();
		drawLine(wordMap, Point(w.xl, w.yl), Point(w.xl, w.yh), -1);
		drawLine(wordMap, Point(w.xl, w.yh), Point(w.xh, w.yh), -1);
		drawLine(wordMap, Point(w.xh, w.yh), Point(w.xh, w.yl), -1);
		drawLine(wordMap, Point(w.xh, w.yl), Point(w.xl, w.yl), -1);
		*/
	}
}

void HandwrittenImage::drawLine(PIXELS &pix, Point a, Point b, int val) {
	if (a == b)
		return;
	if (abs(a.x - b.x) > abs(a.y - b.y)) {
		if (a.x > b.x)
			swap(a, b);
		double k = (double)(b.y - a.y) / (b.x - a.x);
		for (int x = a.x; x <= b.x; ++x) {
			int y = a.y + k*(x - a.x);
			pix[x][y] = val;
		}
	}
	else {
		if (a.y > b.y)
			swap(a, b);
		double k = (double)(b.x - a.x) / (b.y - a.y);
		for (int y = a.y; y <= b.y; ++y) {
			int x = a.x + k*(y - a.y);
			pix[x][y] = val;
		}
	}
}

void HandwrittenImage::writeWords(const char *basename) const {
	MsgPrint::msgPrint(MsgPrint::INFO, "Writing out all words ......");
	for (size_t i = 0; i < allWordBBox.size(); ++i) {
		const WordBBox &w = allWordBBox[i];
		PIXELS oneWordPix = PIXELS(w.xh-w.xl+1, vector<int32_t>(w.yh-w.yl+1, 0));
		for (int x = w.xl; x <= w.xh; ++x) {
			for (int y = w.yl; y <= w.yh; ++y) {
				if (wordMap[x][y] == w.wordID) {
					oneWordPix[x-w.xl][y-w.yl] = 1;
				}
			}
		}
		char fileName[1000];
		sprintf(fileName, "%s_line-%d_word-%d_x-%d_y-%d_width-%d_height-%d.bmp",
				basename, w.regionID, w.wordID, w.xl, w.yl, w.xh-w.xl+1, w.yh-w.yl+1);
		writeOneBitBMP(fileName, oneWordPix);
	}
}
