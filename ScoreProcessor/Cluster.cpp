#include "stdafx.h"
#include "Cluster.h"
#include <stack>
#include <algorithm>
using namespace std;
namespace ScoreProcessor {
	Cluster::Cluster():ranges() {}
	Cluster::~Cluster() {}
	unsigned int Cluster::size() const {
		unsigned int size=0;
		for(unsigned int i=0;i<ranges.size();++i)
		{
			size+=ranges[i].area();
		}
		return size;
	}
	vector<ImageUtils::RectangleUINT> const& Cluster::get_ranges() const {
		return ranges;
	}
	ImageUtils::Rectangle<unsigned int> Cluster::bounding_box() const {
		ImageUtils::Rectangle<unsigned int> box={~0,0,~0,0};
		for(auto const& rect:ranges)
		{
			if(rect.top<box.top)
			{
				box.top=rect.top;
			}
			if(rect.bottom>box.bottom)
			{
				box.bottom=rect.bottom;
			}
			if(rect.left<box.left)
			{
				box.left=rect.left;
			}
			if(rect.right>box.right)
			{
				box.right=rect.right;
			}
		}
		return box;
	}
	ImageUtils::Point<unsigned int> Cluster::center() const {
		ImageUtils::Point<double> center={0.0,0.0};
		double denom=0.0;
		for(auto const& rect:ranges)
		{
			double area=rect.area();
			auto rect_center=rect.center<double>();
			center.x+=area*rect_center.x;
			center.y+=area*rect_center.y;
			denom+=area;
		}
		center.x/=denom;
		center.y/=denom;
		return {static_cast<unsigned int>(center.x),static_cast<unsigned int>(center.y)};
	}
#define xfirst_point_finder(xdef,xterm,xcomp,ydef,yterm,ycomp) \
	ImageUtils::Point<unsigned int> point={xdef,ydef}; \
	for(auto const& rect:ranges) \
	{ \
		if(rect. ## xterm ## xcomp ## point.x&&rect. ## yterm ## ycomp ## point.y) \
		{ \
			point={rect. ## xterm ##,rect. ## yterm}; \
		} \
	} \
	return point

	ImageUtils::Point<unsigned int> Cluster::right_top() const {
		xfirst_point_finder(0,right,>,~0,top,<);
	}
	ImageUtils::Point<unsigned int> Cluster::right_bottom() const {
		xfirst_point_finder(0,right,>,0,bottom,>);
	}
	ImageUtils::Point<unsigned int> Cluster::left_top() const {
		xfirst_point_finder(~0,left,<,~0,top,<);
	}
	ImageUtils::Point<unsigned int> Cluster::left_bottom() const {
		xfirst_point_finder(~0,left,<,0,bottom,>);
	}
#undef xfirst_point_finder
	ImageUtils::vertical_line<unsigned int> Cluster::right_side() const {
		ImageUtils::vertical_line<> line={0,~0,0};
		for(auto const& rect:ranges)
		{
			if(rect.right>line.x)
			{
				line.x=rect.right;
				line.top=rect.top;
				line.bottom=rect.bottom;
			}
			else if(rect.right==line.x)
			{
				if(rect.top<line.top)
				{
					line.top=rect.top;
				}
				if(rect.bottom>line.bottom)
				{
					line.bottom=rect.bottom;
				}
			}
		}
		return line;
	}
	::std::vector<::std::unique_ptr<Cluster>> Cluster::cluster_ranges(::std::vector<ImageUtils::RectangleUINT> const& ranges) {
		::std::vector<::std::unique_ptr<Cluster>> cluster_container;
		struct ClusterPart {
			Cluster* cluster;
			ImageUtils::RectangleUINT rect;
			ClusterPart(ImageUtils::RectangleUINT const& rect):cluster(nullptr),rect(rect) {}
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
				y(isTop?parent->rect.top:parent->rect.bottom) {}
		};
		vector<ClusterPart> parts;
		vector<unique_ptr<ClusterTestNode>> tests;
		for(unsigned int i=0;i<ranges.size();++i)
		{
			parts.emplace_back(ranges[i]);
		}
		for(unsigned int i=0;i<parts.size();++i)
		{
			tests.push_back(make_unique<ClusterTestNode>(parts.data()+i,true));
			tests.push_back(make_unique<ClusterTestNode>(parts.data()+i,false));
		}
		sort(tests.begin(),tests.end(),[](unique_ptr<ClusterTestNode>& a,unique_ptr<ClusterTestNode>& b) {return *a<*b;});
		stack<unsigned int> searchStack;
		unsigned int const max=tests.size();
		for(unsigned int i=max-1;i<max;--i)
		{
			unique_ptr<ClusterTestNode> const& currentNode=tests[i];
			if(currentNode->isTop) continue;
			if(!currentNode->parent->cluster)
			{
				unique_ptr<Cluster> currentCluster=make_unique<Cluster>();
				searchStack.push(i);
				unsigned int searchIndex;
				while(!searchStack.empty())
				{
					searchIndex=searchStack.top();
					searchStack.pop();
					unique_ptr<ClusterTestNode> const& searchNode=tests[searchIndex];
					if(searchNode->parent->cluster) continue;
					searchNode->parent->cluster=currentCluster.get();
					currentCluster->ranges.emplace_back(searchNode->parent->rect);
					for(unsigned int s=searchIndex-1;s<max;--s)
					{
						if(!tests[s]->isTop)
						{
							if(tests[s]->y==searchNode->parent->rect.top&&tests[s]->parent->rect.overlaps_x(searchNode->parent->rect))
							{
								searchStack.push(s);
							}
							else if(tests[s]->y<searchNode->parent->rect.top)
							{
								break;
							}
						}
					}
					for(unsigned int s=searchIndex+1;s<max;++s)
					{
						if(tests[s]->isTop)
						{
							if(tests[s]->y==searchNode->parent->rect.bottom&&tests[s]->parent->rect.overlaps_x(searchNode->parent->rect))
							{
								searchStack.push(s);
							}
							else if(tests[s]->y>searchNode->parent->rect.bottom)
							{
								break;
							}
						}
					}
				#undef searchNode
				}
				cluster_container.push_back(move(currentCluster));
			}
		}
		return cluster_container;
	}
}