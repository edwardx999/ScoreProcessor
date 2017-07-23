#include "stdafx.h"
#include "Cluster.h"
#include <stack>
#include <algorithm>
using namespace std;
namespace ScoreProcessor {
	Cluster::Cluster() {
		ranges=make_unique<vector<ImageUtils::Rectangle*>>();
	}
	Cluster::~Cluster() {
		for(int i=0;i<ranges->size();++i) {
			delete (*ranges)[i];
		}
	}
	struct ClusterPart {
		Cluster* cluster;
		ImageUtils::Rectangle* const rect;
		ClusterPart(ImageUtils::Rectangle* const rect):cluster(nullptr),rect(rect) {}
	};
	struct ClusterTestNode {
		ClusterPart* parent;
		bool const isTop;
		int const y;
		bool operator<(ClusterTestNode const& other) const {
			return y<other.y;
		}
		bool operator>(ClusterTestNode const& other) const {
			return y>other.y;
		}
		ClusterTestNode(ClusterPart* parent,bool isTop):
			parent(parent),
			isTop(isTop),
			y(isTop?parent->rect->top:parent->rect->bottom) {}
	};
	void Cluster::clusterRanges(vector<Cluster*>& clusters,vector<ImageUtils::Rectangle*>& ranges) {
		sort(ranges.begin(),ranges.end(),[](ImageUtils::Rectangle* a,ImageUtils::Rectangle* b) {return *a<*b;});
		vector<ClusterPart*> parts;
		vector<ClusterTestNode*> tests;
		for(int i=0;i<ranges.size();++i) {
			parts.push_back(new ClusterPart(ranges[i]));
			tests.push_back(new ClusterTestNode(*parts.end(),true));
			tests.push_back(new ClusterTestNode(*parts.end(),false));
		}
		sort(tests.begin(),tests.end(),[](ClusterTestNode* a,ClusterTestNode*b) {return *a<*b;});
		stack<int> searchStack;
		int max=tests.size();
		for(int i=max-1;i>=0;--i) {
		#define currentNode tests[i]
			if(currentNode->isTop) continue;
			if(!currentNode->parent->cluster) {//if cluster is nullptr
				Cluster* currentCluster=new Cluster();
				clusters.push_back(currentCluster);
				searchStack.push(i);
				int searchIndex;
				while(!searchStack.empty()) {
					searchIndex=searchStack.top();
					searchStack.pop();
				#define searchNode tests[searchIndex]
					if(searchNode->parent->cluster) continue;
					searchNode->parent->cluster=currentCluster;
					currentCluster->ranges->push_back(searchNode->parent->rect);
					for(int s=searchIndex-1;s>=0;--s) {
						if(!tests[s]->isTop) {
							if(tests[s]->y==searchNode->parent->rect->top&&tests[s]->parent->rect->overlapsX(*searchNode->parent->rect)) {
								searchStack.push(s);
							}
							else if(tests[s]->y<searchNode->parent->rect->top) {
								break;
							}
						}
					}
					for(int s=searchIndex+1;s<max;++s) {
						if(tests[s]->isTop) {
							if(tests[s]->y==searchNode->parent->rect->bottom&&tests[s]->parent->rect->overlapsX(*searchNode->parent->rect)) {
								searchStack.push(s);
							}
							else if(tests[s]->y>searchNode->parent->rect->bottom) {
								break;
							}
						}
					}
				#undef searchNode
				}
			}
		#undef currentNode
			for(int i=0;i<parts.size();++i) {
				delete parts[i];
			}
			for(int i=0;i<tests.size();++i) {
				delete tests[i];
			}
		}
	}
}