#ifndef __GROUPTREE_H__
#define __GROUPTREE_H__

#include <vector>
#include <utility>

using std::pair;
using std::vector;

struct GroupTreeNode {
	int start; // group start index
	int end;   // group end index
	vector<GroupTreeNode *> subGroups;

	GroupTreeNode(int st, int ed);
	~GroupTreeNode();
	bool divideGroup(const vector<double> &gaps, double alpha);
};

struct GroupTree {
	GroupTree(const vector<double> &g, double t, double a);
	~GroupTree();
	void grouping();
	vector< pair<int, int> > getGroupingResult() const;

	GroupTreeNode *mainGroup;
	vector<double> gaps;
	double threshold;
	double alpha;
};

#endif
