#include <fstream>
#include <utility>
#include <string>
#include "ConfigParser.h"
#include "MsgPrint.h"

using std::ifstream;
using std::string;

ConfigParser::ConfigParser(const char *configFile) {
	configs.clear();
	setDefaultConfigs();
	ifstream f(configFile);
	if (!f.is_open()) {
		MsgPrint::msgPrint(MsgPrint::WARN, "Use default config settings. (Config file is not specified or cannot be opened)");
	}

	string line = "";
	int lineCnt = 0;
	while (getline(f, line)) {
		lineCnt += 1;
		deleteComments(line, "//");
		vector<string> words = split(line, " \t\n");
		int sz = words.size();
		if (sz == 0) {
			continue;
		}
		else if (sz == 2) {
			if (configs.find(words[0]) != configs.end()) {
				configs[words[0]] = stod(words[1]);
			}
			else {
				char msg[1000];
				sprintf (msg, "Unknown variable '%s' at config file '%s' line %d.", words[0].c_str(), configFile, lineCnt);
				MsgPrint::msgPrint(MsgPrint::ERR, msg);
			}
		}
		else { // sz != 0 && sz != 2
			char msg[1000];
			sprintf (msg, "Syntax error at config file '%s' line %d.", configFile, lineCnt);
			MsgPrint::msgPrint(MsgPrint::ERR, msg);
		}
	}
	/*
	for (map<string, double>::iterator it = configs.begin(); it != configs.end(); ++it) {
		printf ("%s, %.2f\n", it->first.c_str(), it->second);
	}
	*/
}

void ConfigParser::setDefaultConfigs() {
	configs.clear();

	// set default configs
	configs["charH_convergence_diff"] = 0.05;  // if (currentCharH - lastCharH) / lastCharH < this value, then break the loop, and set charH to currentCharH
	configs["charH_cutoff_ratio"] = 1.8;  // recalcluate charH only use components that have height < this value * lastCharH
	configs["blur_width"] = 8;  // unit charH
	configs["blur_height"] = 0.8;  // unit charH
	configs["first_order_partial_derivative_of_y_window_height"] = 2;  // unit charH
	configs["second_order_partial_derivative_of_y_window_height"] = 1;  // unit charH
	configs["space_tracing_seeds_distance"] = 0.5;  // unit charH, distance between adjacent seeds
	configs["region_area_min"] = 1;  // unit charH*charH
	configs["region_black_pixel_percentage_min"] = 0.01;  // out of 1
	configs["region_black_pixel_percentage_max"] = 0.50;  // out of 1
	configs["text_tracing_seeds_distance"] = 0.5;  // unit charH, distance between adjacent seeds
	configs["word_center_strap_width"] = 0.33; // unit charH, only consider components intersects with the center strap of each line
	configs["word_width_min"] = 0.2;  // unit charH
	configs["word_height_min"] = 0.2;  // unit charH
	configs["word_gap_threshold"] = 0.5;  // unit charH
	configs["word_alpha"] = 1.5;  // alpha*intra-word-gap < min(leftGap, rightGap)
	// for a given pixel, if the horizontal weight * black horizontal segment length  + vertical weight * black vertical segment length > threshold
	// then this pixel is a border pixel
	configs["border_removal_horizontal_segment_weight"] = 0.3;
	configs["border_removal_vertial_segment_weight"] = 1.0;
	configs["border_removal_segment_sum_threshold"] = 0.05; // unit image height
}

map<string, double> ConfigParser::getConfigs() const {
	return configs;
}

vector<string> ConfigParser::split(const string &line, const string &delims) const {
	vector<string> res;
	string word = "";

	int i = 0;
	int sz = line.length();
	while (i < sz && delims.find(line[i]) != string::npos) // delete leading delim
		++i;
	while (i < sz) {
		if (delims.find(line[i]) != string::npos) {
			res.push_back(word);
			word = "";
			++i;
			while (i < sz && delims.find(line[i]) != string::npos)
				++i;
		}
		else {
			word += line[i];
			++i;
		}
	}
	if (word != "")
		res.push_back(word);
	return res;
}

void ConfigParser::deleteComments (string &line, const string &cm) const {
	size_t pos = line.find(cm);
	if (pos != string::npos)
		line = line.substr(0, pos);
}
