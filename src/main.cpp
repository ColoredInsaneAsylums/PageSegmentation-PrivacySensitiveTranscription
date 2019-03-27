#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include "HandwrittenImage.h"
#include "ConfigParser.h"

using std::string;
using std::map;

int main(int argc, char *argv[]) {
	if (argc != 6) {
		fprintf (stderr, "Error: Wrong number of arguments, expected 6, got %d.\n", argc);
		exit(1);
	}
	char *configFile = argv[1];
	string infile = argv[2];
	string outdir = argv[3];
	string prefix = argv[4];
	bool dumpall = (string(argv[5]) != "0");
	if (outdir[outdir.length()-1] != '/')
		outdir += '/';
	
	// parse config file
	ConfigParser configParser(configFile);
	map<string, double> configs = configParser.getConfigs();

	HandwrittenImage img;
	img.readOneBitBMP(infile.c_str());
	img.removeBorder(configs["border_removal_horizontal_segment_weight"], configs["border_removal_vertial_segment_weight"], configs["border_removal_segment_sum_threshold"]);
	img.calcCharHeight(configs["charH_convergence_diff"], configs["charH_cutoff_ratio"]);

	int charH = img.getCharH();
	img.blur(configs["blur_width"]*charH, configs["blur_height"]*charH);

	img.initBlurPixFstOrdParDerivY(configs["first_order_partial_derivative_of_y_window_height"]*charH);
	img.initBlurPixScdOrdParDerivY(configs["second_order_partial_derivative_of_y_window_height"]*charH);
	img.initSpaceTracingSeeds(configs["space_tracing_seeds_distance"]*charH, configs["space_tracing_seeds_distance"]*charH);
	img.segmentRegions();
	img.labelRegions(configs["region_area_min"]*charH*charH, configs["region_black_pixel_percentage_min"], configs["region_black_pixel_percentage_max"]);
	img.initTextTracingSeeds(configs["text_tracing_seeds_distance"]*charH, configs["text_tracing_seeds_distance"]*charH);
	img.locateTextLineCenters();
	img.assignComponentsToRegions();
	img.slantCorrection();
	img.genConvexHullComponents();
	img.extractWord(configs["word_center_strap_width"], configs["word_width_min"]*charH, configs["word_height_min"]*charH,
			        configs["word_gap_threshold"]*charH, configs["word_alpha"]);
	img.writeWords((outdir + prefix).c_str());

	if (dumpall) {
		//img.writeBMP((outdir + prefix + "_bin.bmp").c_str(), HandwrittenImage::BINPIX);
		img.writeBMP((outdir + prefix + "_binBR.bmp").c_str(), HandwrittenImage::BINPIXBR);
		img.writeBMP((outdir + prefix + "_charH.bmp").c_str(), HandwrittenImage::CHARH);
		img.writeBMP((outdir + prefix + "_blur.bmp").c_str(), HandwrittenImage::BLURPIX);
		img.writeBMP((outdir + prefix + "_spaceSeeds.bmp").c_str(), HandwrittenImage::SPACETRACINGSEEDS);
		img.writeBMP((outdir + prefix + "_spaceTraces.bmp").c_str(), HandwrittenImage::SPACETRACES);
		img.writeBMP((outdir + prefix + "_regions.bmp").c_str(), HandwrittenImage::REGIONS);
		img.writeBMP((outdir + prefix + "_textSeeds.bmp").c_str(), HandwrittenImage::TEXTTRACINGSEEDS);
		img.writeBMP((outdir + prefix + "_textTraces.bmp").c_str(), HandwrittenImage::TEXTTRACES);
		img.writeBMP((outdir + prefix + "_textLines.bmp").c_str(), HandwrittenImage::TEXTLINES);
		img.writeBMP((outdir + prefix + "_noSlant.bmp").c_str(), HandwrittenImage::NOSLANT);
		img.writeBMP((outdir + prefix + "_convexHull.bmp").c_str(), HandwrittenImage::CONVEXHULL);
		img.writeBMP((outdir + prefix + "_words.bmp").c_str(), HandwrittenImage::WORDMAP);
	}

	return 0;
}
