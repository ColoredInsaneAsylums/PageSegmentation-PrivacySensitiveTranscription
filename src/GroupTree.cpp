#include <cassert>
#include <cstdlib>
#include <queue>
#include <stack>
#include "GroupTree.h"

using std::min;
using std::max;
using std::queue;
using std::stack;
using std::pair;
using std::make_pair;

GroupTreeNode::GroupTreeNode(int st, int ed) {
	start = st;
	end = ed;
	subGroups.clear();
}

GroupTreeNode::~GroupTreeNode() {
	for (size_t i = 0; i < subGroups.size(); ++i)
		delete subGroups[i];
	subGroups.clear();
}

bool GroupTreeNode::divideGroup (const vector<double> &gaps, double alpha) {
	if (start == end)  // single element group
		return false;
	int leftGap = gaps[start];
	int rightGap = gaps[end+1];
	int maxGap = -1, maxGapIndex = -1;
	for (int i = start+1; i <= end; ++i) {
		if (gaps[i] > maxGap) {
			maxGap = gaps[i];
			maxGapIndex = i;
		}
	}
	if (maxGap*alpha > min(leftGap, rightGap)) {
		// delete old sub groups
		for (size_t i = 0; i < subGroups.size(); ++i)
			delete subGroups[i];
		subGroups.clear();
		// add new sub groups
		subGroups.push_back(new GroupTreeNode(start, maxGapIndex-1));
		subGroups.push_back(new GroupTreeNode(maxGapIndex, end));
		return true;
	}
	return false;
}

GroupTree::GroupTree(const vector<double> &g, double t, double a) {
	gaps = g;
	threshold = t;
	alpha = a;
	mainGroup = NULL;
}

GroupTree::~GroupTree() {
	delete mainGroup;
}

void GroupTree::grouping() {
	assert(gaps.size() > 2);

	mainGroup = new GroupTreeNode(0, gaps.size()-2);
	// if gap larger than threshold, split into subgroups
	int lastEnd = 0;
	for (int i = 1; i <= (int)gaps.size()-2; ++i) {
		if (gaps[i] > threshold) {
			mainGroup->subGroups.push_back(new GroupTreeNode(lastEnd, i-1));
			lastEnd = i;
		}
	}
	mainGroup->subGroups.push_back(new GroupTreeNode(lastEnd, gaps.size()-2)); // handle the last subgroup

	// futher split subgroups into sub-subgroups
	queue<GroupTreeNode *> q;
	for (size_t i = 0; i < mainGroup->subGroups.size(); ++i)
		q.push(mainGroup->subGroups[i]);
	while (!q.empty()) {
		GroupTreeNode *ptr = q.front();
		q.pop();
		if (ptr->divideGroup(gaps, alpha)) {
			for (size_t i = 0; i < ptr->subGroups.size(); ++i)
				q.push(ptr->subGroups[i]);
		}
	}
}

vector< pair<int, int> > GroupTree::getGroupingResult() const {
	vector< pair<int, int> >res;
	stack<GroupTreeNode *> st;
	st.push(mainGroup);
	while (!st.empty()) {
		GroupTreeNode *ptr = st.top();
		st.pop();
		if (ptr->subGroups.size() == 0) {  // tree leaf
			res.push_back(make_pair(ptr->start, ptr->end));
		}
		else {
			for (int i = ptr->subGroups.size()-1; i >= 0; --i)
				st.push(ptr->subGroups[i]);
		}
	}
	return res;
}
