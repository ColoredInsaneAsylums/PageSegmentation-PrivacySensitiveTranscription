#ifndef __CONFIGPARSER_H__
#define __CONFIGPARSER_H__

#include <map>
#include <vector>
#include <string>
using std::map;
using std::string;
using std::vector;

class ConfigParser {
public:
	ConfigParser(const char* configFile);
	map<string, double> getConfigs() const;
private:
	void setDefaultConfigs();
	map<string, double> configs;
	vector<string> split(const string &line, const string &delims) const;
	void deleteComments (string &line, const string &cm) const;
};

#endif
