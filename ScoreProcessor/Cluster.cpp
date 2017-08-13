#include "stdafx.h"
#include "Cluster.h"
#include <stack>
#include <algorithm>
using namespace std;
namespace ScoreProcessor {
	Cluster::Cluster():ranges() {}
	Cluster::~Cluster() {}
	unsigned int Cluster::size() {
		unsigned int size=0;
		for(unsigned int i=0;i<ranges.size();++i) {
			size+=ranges[i]->area();
		}
		return size;
	}
	vector<shared_ptr<ImageUtils::RectangleUINT>> const& Cluster::getRanges() const {
		return ranges;
	}
	void clusterRanges(vector<unique_ptr<Cluster>>& clusterContainer,vector<shared_ptr<ImageUtils::RectangleUINT>>& ranges) {
		struct ClusterPart {
			Cluster* cluster;
			shared_ptr<ImageUtils::RectangleUINT> rect;
			ClusterPart(shared_ptr<ImageUtils::RectangleUINT> const& rect):cluster(nullptr),rect(rect) {}
		};
		struct ClusterTestNode {
			ClusterPart* parent;
			bool const isTop;
			unsigned int const y;
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
		vector<ClusterPart> parts;
		vector<unique_ptr<ClusterTestNode>> tests;
		for(unsigned int i=0;i<ranges.size();++i) {
			parts.emplace_back(ranges[i]);
		}
		for(unsigned int i=0;i<parts.size();++i) {
			tests.push_back(make_unique<ClusterTestNode>(parts.data()+i,true));
			tests.push_back(make_unique<ClusterTestNode>(parts.data()+i,false));
		}
		sort(tests.begin(),tests.end(),[](unique_ptr<ClusterTestNode>& a,unique_ptr<ClusterTestNode>& b) {return *a<*b;});
		stack<unsigned int> searchStack;
		unsigned int const max=tests.size();
		for(unsigned int i=max-1;i<max;--i) {
		#define currentNode tests[i]
			if(currentNode->isTop) continue;
			if(!currentNode->parent->cluster) {
				unique_ptr<Cluster> currentCluster=make_unique<Cluster>();
				searchStack.push(i);
				unsigned int searchIndex;
				while(!searchStack.empty()) {
					searchIndex=searchStack.top();
					searchStack.pop();
				#define searchNode tests[searchIndex]
					if(searchNode->parent->cluster) continue;
					searchNode->parent->cluster=currentCluster.get();
					currentCluster->ranges.emplace_back(searchNode->parent->rect);
					for(unsigned int s=searchIndex-1;s<max;--s) {
						if(!tests[s]->isTop) {
							if(tests[s]->y==searchNode->parent->rect->top&&tests[s]->parent->rect->overlapsX(*(searchNode->parent->rect))) {
								searchStack.push(s);
							}
							else if(tests[s]->y<searchNode->parent->rect->top) {
								break;
							}
						}
					}
					for(unsigned int s=searchIndex+1;s<max;++s) {
						if(tests[s]->isTop) {
							if(tests[s]->y==searchNode->parent->rect->bottom&&tests[s]->parent->rect->overlapsX(*(searchNode->parent->rect))) {
								searchStack.push(s);
							}
							else if(tests[s]->y>searchNode->parent->rect->bottom) {
								break;
							}
						}
					}
				#undef searchNode
				}
				clusterContainer.push_back(move(currentCluster));
			}
		#undef currentNode
		}
	}
}