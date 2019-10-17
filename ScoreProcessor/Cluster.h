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
#ifndef CLUSTER_H
#define CLUSTER_H
#include <vector>
#include "ImageUtils.h"
#include <memory>
#include <utility>
#include <stack>
#include <algorithm>
namespace ScoreProcessor {

	class Cluster {
	private:
		::std::vector<ImageUtils::Rectangle<unsigned int>> ranges;
	public:
		/*
			Returns the area of the cluster
			(i.e. the sum of the areas of all sub-rectangles)
		*/
		unsigned int size() const;
		/*
			Returns the bounding box of the cluster
			If cluster contains no rectangles, returns {MAX_UINT,0,MAXUINT,0}
		*/
		ImageUtils::Rectangle<unsigned int> bounding_box() const;
		/*
			Returns the center of mass of the cluster
			If cluster contains no rectangles, returns {0,0}
		*/
		template<typename T>
		ImageUtils::Point<T> center() const
		{
			if(ranges.size()==0)
			{
				return {T(0),T(0)};
			}
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
			return {T(center.x),T(center.y)};
		}
		/*
			Returns the highest rightmost point of the cluster
		*/
		ImageUtils::Point<unsigned int> right_top() const;
		/*
		Returns the lowest rightmost point of the cluster
		*/
		ImageUtils::Point<unsigned int> right_bottom() const;
		ImageUtils::Point<unsigned int> left_top() const;
		ImageUtils::Point<unsigned int> left_bottom() const;
		/*
			Returns the vertical_line representing the right side of the cluster
		*/
		ImageUtils::vertical_line<unsigned int> right_side() const;
		/*
			Gets the ranges of the cluster
		*/
		::std::vector<ImageUtils::Rectangle<unsigned int>> const& get_ranges() const;

		template<typename Comp=std::less<>>
		void sort(Comp c=Comp()) noexcept
		{
			std::sort(ranges.begin(),ranges.end(),c);
		}
	private:

		template<typename OverlapFunc>
		static ::std::vector<Cluster> cluster_ranges_base(::std::vector<ImageUtils::Rectangle<unsigned int>> const& ranges,OverlapFunc cf)
		{
			::std::vector<Cluster> cluster_container;
			struct ClusterPart {
				bool clustered;
				ImageUtils::Rectangle<unsigned int> const* rect;
				ClusterPart(ImageUtils::Rectangle<unsigned int> const* rect):clustered(false),rect(rect)
				{}
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
			std::vector<ClusterPart> parts;parts.reserve(ranges.size());
			std::vector<ClusterTestNode> tests;tests.reserve(ranges.size()*2);
			for(unsigned int i=0;i<ranges.size();++i)
			{
				parts.emplace_back(ranges.data()+i);
				tests.emplace_back(parts.data()+i,true);
				tests.emplace_back(parts.data()+i,false);
			}
			std::sort(tests.begin(),tests.end());
			std::stack<unsigned int> search_stack;
			size_t const max=tests.size();
			for(unsigned int i=max-1;i<max;--i)
			{
				ClusterTestNode const& current_node=tests[i];
				if(current_node.is_top())
				{
					continue;
				}
				if(!current_node.parent->clustered)
				{
					Cluster current_cluster;
					search_stack.push(i);
					while(!search_stack.empty())
					{
						unsigned int search_index=search_stack.top();
						search_stack.pop();
						ClusterTestNode const& search_node=tests[search_index];
						if(search_node.parent->clustered)
						{
							continue;
						}
						search_node.parent->clustered=true;
						current_cluster.ranges.push_back(*search_node.parent->rect);
						for(unsigned int s=search_index-1;s<max;--s)
						{
							if(tests[s].is_bottom())
							{
								if(tests[s].y<search_node.parent->rect->top)
								{
									break;
								}
								if(tests[s].y==search_node.parent->rect->top&&
									cf(*tests[s].parent->rect,*search_node.parent->rect))
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
								if(tests[s].y==search_node.parent->rect->bottom&&
									cf(*tests[s].parent->rect,*search_node.parent->rect))
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

	public:

			/*
		Given a vector of rectangles,
		Returns a vector of pointers to clusters made from those rectangles
	*/
		inline static ::std::vector<Cluster> cluster_ranges(::std::vector<ImageUtils::Rectangle<unsigned int>> const& ranges)
		{
			using R=ImageUtils::Rectangle<unsigned int>;
			return cluster_ranges_base(ranges,[](R a,R b)
			{
				return a.overlaps_x(b);
			});
		}

		inline static ::std::vector<Cluster> cluster_ranges_8way(::std::vector<ImageUtils::Rectangle<unsigned int>> const& ranges)
		{
			using R=ImageUtils::Rectangle<unsigned int>;
			return cluster_ranges_base(ranges,[](R a,R b)
			{
				return (a.left<=b.right&&a.right>=b.left);
			});
		}
	};
}
#endif // !CLUSTER_H
