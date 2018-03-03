/*
Copyright(C) 2017-2018 Edward Xie

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "stdafx.h"
#include "Cluster.h"
#include <stack>
#include <algorithm>
using namespace std;
namespace ScoreProcessor {
	Cluster::Cluster():ranges() {}
	Cluster::~Cluster() {}
	unsigned int Cluster::size() const
	{
		unsigned int size=0;
		for(unsigned int i=0;i<ranges.size();++i)
		{
			size+=ranges[i].area();
		}
		return size;
	}
	vector<ImageUtils::RectangleUINT> const& Cluster::get_ranges() const
	{
		return ranges;
	}
	ImageUtils::Rectangle<unsigned int> Cluster::bounding_box() const
	{
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
	ImageUtils::Point<unsigned int> Cluster::center() const
	{
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

	ImageUtils::Point<unsigned int> Cluster::right_top() const
	{
		xfirst_point_finder(0,right,>,~0,top,<);
	}
	ImageUtils::Point<unsigned int> Cluster::right_bottom() const
	{
		xfirst_point_finder(0,right,>,0,bottom,>);
	}
	ImageUtils::Point<unsigned int> Cluster::left_top() const
	{
		xfirst_point_finder(~0,left,<,~0,top,<);
	}
	ImageUtils::Point<unsigned int> Cluster::left_bottom() const
	{
		xfirst_point_finder(~0,left,<,0,bottom,>);
	}
#undef xfirst_point_finder
	ImageUtils::vertical_line<unsigned int> Cluster::right_side() const
	{
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
	vector<unique_ptr<Cluster>> Cluster::cluster_ranges(vector<ImageUtils::Rectangle<unsigned int>> const& ranges)
	{
		vector<unique_ptr<Cluster>> cluster_container;
		struct ClusterPart {
			Cluster* cluster;
			ImageUtils::Rectangle<unsigned int> const* rect;
			ClusterPart(ImageUtils::Rectangle<unsigned int> const* rect):cluster(nullptr),rect(rect) {}
		};
		struct ClusterTestNode {
			ClusterPart* parent;
			bool _is_top;
			unsigned int y;
			bool operator<(ClusterTestNode const& other) const
			{
				if(y==other.y)
				{
					return _is_top<other._is_top;
				}
				return y<other.y;
			}
			inline bool is_top() const
			{
				return _is_top;
			}
			inline bool is_bottom() const
			{
				return !_is_top;
			}
			ClusterTestNode(ClusterPart* parent,bool is_top):
				parent(parent),
				_is_top(is_top),
				y(is_top?parent->rect->top:parent->rect->bottom)
			{}
		};
		vector<ClusterPart> parts;parts.reserve(ranges.size());
		vector<ClusterTestNode> tests;tests.reserve(ranges.size()*2);
		for(unsigned int i=0;i<ranges.size();++i)
		{
			parts.emplace_back(ranges.data()+i);
			tests.emplace_back(parts.data()+i,true);
			tests.emplace_back(parts.data()+i,false);
		}
		sort(tests.begin(),tests.end());
		stack<unsigned int> search_stack;
		unsigned int const& max=tests.size();
		for(unsigned int i=max-1;i<max;--i)
		{
			ClusterTestNode const& current_node=tests[i];
			if(current_node.is_top()) { continue; }
			if(current_node.parent->cluster==nullptr)
			{
				unique_ptr<Cluster> current_cluster=make_unique<Cluster>();
				search_stack.push(i);
				while(!search_stack.empty())
				{
					unsigned int search_index=search_stack.top();
					search_stack.pop();
					ClusterTestNode const& search_node=tests[search_index];
					if(search_node.parent->cluster!=nullptr)
					{
						continue;
					}
					search_node.parent->cluster=current_cluster.get();
					current_cluster->ranges.push_back(*search_node.parent->rect);
					for(unsigned int s=search_index-1;s<max;--s)
					{
						if(tests[s].is_bottom())
						{
							if(tests[s].y<search_node.parent->rect->top)
							{
								break;
							}
							if(tests[s].y==search_node.parent->rect->top
								&&tests[s].parent->rect->overlaps_x(*search_node.parent->rect))
							{
								search_stack.push(s);
							}
						}

					}
					for(unsigned int s=search_index+1;s<max;++s)
					{
						if(tests[s].is_top())
						{
							if(tests[s].y>search_node.parent->rect->bottom)
							{
								break;
							}
							if(tests[s].y==search_node.parent->rect->bottom
								&&tests[s].parent->rect->overlaps_x(*search_node.parent->rect))
							{
								search_stack.push(s);
							}
						}
					}
				}
				cluster_container.push_back(std::move(current_cluster));
			}
		}
		return cluster_container;
	}
}