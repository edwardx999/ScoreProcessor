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
	unsigned int Cluster::size() const
	{
		unsigned int size=0;
		for(auto const& rect:ranges)
		{
			size+=rect.area();
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
		for(auto const rect:ranges)
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

#define xfirst_point_finder(xdef,xterm,xcomp,ydef,yterm,ycomp) \
	ImageUtils::Point<unsigned int> point={xdef,ydef}; \
	for(auto const& rect:ranges) \
	{ \
		if(rect. xterm xcomp point.x&&rect. yterm ycomp point.y) \
		{ \
			point={rect. xterm,rect. yterm}; \
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
}