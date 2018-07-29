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
		/*
			Given a vector of rectangles,
			Returns a vector of pointers to clusters made from those rectangles
		*/
		static ::std::vector<Cluster> cluster_ranges(::std::vector<ImageUtils::Rectangle<unsigned int>> const& ranges);
	};
}
#endif // !CLUSTER_H
