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
#include "ScoreProcesses.h"
#include <thread>
#include "ImageUtils.h"
#include "Cluster.h"
#include <iostream>
#include <string>
#include <stack>
#include <cmath>
#include <array>
#include "moreAlgorithms.h"
#include "shorthand.h"
#include <assert.h>
#include "ImageMath.h"
#include "lib/exstring/exmath.h"
#include <atomic>
#include <mutex>
#include "lib/threadpool/ThreadPool.h"
using namespace std;
using namespace ImageUtils;
using namespace cimg_library;
using namespace misc_alg;
namespace ScoreProcessor {
	void binarize(CImg<unsigned char>& image,ColorRGB const middleColor,ColorRGB const lowColor,ColorRGB const highColor)
	{
		assert(image._spectrum==3);
		unsigned char midBrightness=middleColor.brightness();
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(brightness({image(x,y,0),image(x,y,1),image(x,y,2)})>midBrightness)
				{
					image(x,y,0)=highColor.r;
					image(x,y,1)=highColor.g;
					image(x,y,2)=highColor.b;
				}
				else
				{
					image(x,y,0)=lowColor.r;
					image(x,y,1)=lowColor.g;
					image(x,y,2)=lowColor.b;
				}
			}
		}
	}
	void binarize(CImg<unsigned char>& image,Grayscale const middleGray,Grayscale const lowGray,Grayscale const highGray)
	{
		assert(image._spectrum==1);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(image(x,y)>middleGray)
				{
					image(x,y)=highGray;
				}
				else
				{
					image(x,y)=lowGray;
				}
			}
		}
	}
	void binarize(CImg<unsigned char>& image,Grayscale const middleGray,float scale)
	{
		assert(image._spectrum==1);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(image(x,y)>middleGray)
				{
					image(x,y)=255-(255-image(x,y))/scale;
				}
				else
				{
					image(x,y)/=scale;
				}
			}
		}
	}
	bool replace_range(CImg<unsigned char>& image,Grayscale const lower,ImageUtils::Grayscale const upper,Grayscale const replacer)
	{
		bool edited=false;
		unsigned char* const limit=image.end();
		for(auto it=image.begin();it!=limit;++it)
		{
			if(*it>=lower&&*it<=upper)
			{
				if(*it!=replacer)
				{
					edited=true;
					*it=replacer;
				}
			}
		}
		return edited;
	}

	bool replace_by_brightness(CImg<unsigned char>& image,unsigned char lowerBrightness,unsigned char upperBrightness,ColorRGB replacer)
	{
		assert(image._spectrum>=3);
		bool edited=false;
		auto repl=std::array<unsigned char,3>({replacer.r,replacer.g,replacer.b});
		map_if<3U>(image,[=](auto color)
		{
			return repl;;
		},[=,&edited](auto color)
		{
			if(color==repl)
			{
				return false;
			}
			auto const brightness=(float(color[0])+color[1]+color[2])/3.0f;
			if(brightness>=lowerBrightness&&brightness<=upperBrightness)
			{
				edited=true;
				return true;
			}
			return false;
		});
		return edited;
	}
	bool replace_by_hsv(::cimg_library::CImg<unsigned char>& image,ImageUtils::ColorHSV start,ImageUtils::ColorHSV end,ImageUtils::ColorRGB replacer)
	{
		assert(image._spectrum>=3);
		bool edited=false;
		std::array<unsigned char,3> const& repl=*reinterpret_cast<std::array<unsigned char,3>*>(&replacer);
		map_if<3U>(image,[=](auto color)
		{
			return repl;
		},[=,&edited](auto color)
		{
			if(color==repl)
			{
				return false;
			}
			ColorHSV hsv=*reinterpret_cast<ColorRGB*>(&color);
			if(hsv.s>=start.s&&hsv.s<=end.s&&hsv.v>=start.v&&hsv.v<=end.v)
			{
				if(start.h<end.h)
				{
					if(hsv.h>=start.h&&hsv.h<=end.h)
					{
						edited=true;
						return true;
					}
				}
				else if(hsv.h>=start.h||hsv.h<=end.h)
				{
					edited=true;
					return true;
				}
			}
			return false;
		});
		return edited;
	}
	bool replace_by_rgb(::cil::CImg<unsigned char>& image,ImageUtils::ColorRGB start,ImageUtils::ColorRGB end,ImageUtils::ColorRGB replacer)
	{
		assert(image._spectrum>=3);
		bool edited=false;
		std::array<unsigned char,3> const& repl=*reinterpret_cast<std::array<unsigned char,3>*>(&replacer);
		map_if<3U>(image,[=](auto color)
		{
			return repl;
		},[=,&edited](auto color)
		{
			if(color==repl)
			{
				return false;
			}
			if(color[0]>=start.r&&color[0]<=end.r&&
				color[1]>=start.g&&color[1]<=end.g&&
				color[2]>=start.b&&color[2]<=end.b)
			{
				edited=true;
				return true;
			}
			return false;
		});
		return edited;
	}
	bool auto_center_horiz(CImg<unsigned char>& image)
	{
		bool isRgb=image._spectrum>=3;
		unsigned int left,right;
		unsigned int tolerance=image._height>>4;
		if(isRgb)
		{
			left=find_left(image,ColorRGB::WHITE,tolerance);
			right=find_right(image,ColorRGB::WHITE,tolerance);
		}
		else
		{
			left=find_left(image,Grayscale::WHITE,tolerance);
			right=find_right(image,Grayscale::WHITE,tolerance);
		}

		unsigned int center=image._width/2;
		int shift=static_cast<int>(center-((left+right)/2));
		if(0==shift)
		{
			return false;
		}
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0)
		{
			toFill={0,static_cast<unsigned int>(shift),0,image._height};
		}
		else if(shift<0)
		{
			toFill={static_cast<unsigned int>(image._width+shift),image._width,0,image._height};
		}
		copy_shift_selection(image,toShift,shift,0);
		if(isRgb)
		{
			fill_selection(image,toFill,ColorRGB::WHITE);
		}
		else
		{
			fill_selection(image,toFill,Grayscale::WHITE);
		}
		return false;
	}
	unsigned int find_left(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=0;x<image._width;++x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return image._width-1;
	}
	unsigned int find_left(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=0;x<image._width;++x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				if(gray_diff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return image._width-1;
	}
	unsigned int find_right(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=image._width-1;x<image._width;--x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return 0;
	}
	unsigned int find_right(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=image._width-1;x<image._width;--x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				if(gray_diff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return 0;
	}

	bool auto_center_vert(CImg<unsigned char>& image)
	{
		bool isRgb=image._spectrum>=3;
		int top,bottom;
		unsigned int tolerance=image._height>>4;
		if(isRgb)
		{
			top=find_top(image,ColorRGB::WHITE,tolerance);
			bottom=find_bottom(image,ColorRGB::WHITE,tolerance);
		}
		else
		{
			top=find_top(image,Grayscale::WHITE,tolerance);
			bottom=find_bottom(image,Grayscale::WHITE,tolerance);
		}
		unsigned int center=image._height/2;
		int shift=static_cast<int>(center-((top+bottom)/2));
		if(0==shift)
		{
			return false;
		}
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0)
		{
			toFill={0,image._width,0,static_cast<unsigned int>(shift)};
		}
		else if(shift<0)
		{
			toFill={0,image._width,static_cast<unsigned int>(image._height+shift),image._height};
		}
		copy_shift_selection(image,toShift,0,shift);
		if(isRgb)
		{
			fill_selection(image,toFill,ColorRGB::WHITE);
		}
		else
		{
			fill_selection(image,toFill,Grayscale::WHITE);
		}
		return true;
	}
	unsigned int find_top(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int y=0;y<image._height;++y)
		{
			//num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return image._height-1;
	}
	unsigned int find_top(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		auto const data=image.data();
		auto const width=image._width;
		auto const height=image._height;
		for(unsigned int y=0;y<height;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				if(gray_diff(*(row+x),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return image._height-1;
	}
	unsigned int find_bottom(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int y=image._height-1;y<image._height;--y)
		{
			//num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return 0;
	}
	unsigned int find_bottom(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		auto const data=image.data();
		auto const width=image._width;
		auto const height=image._height;
		for(unsigned int y=height-1;y<height;--y)
		{
			auto const row=data+width*y;
			for(unsigned int x=0;x<image._width;++x)
			{
				if(gray_diff(*(row+x),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return 0;
	}
	std::vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x)
			{
				if(gray_diff(image(x,y),background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		container.resize(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;--x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		container.resize(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;--x)
			{
				if(gray_diff(image(x,y),background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3||image._spectrum==4);
		std::vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		unsigned int color=static_cast<unsigned int>(background.r)+background.g+background.b;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(static_cast<unsigned int>(pixel.r)+pixel.g+pixel.b<=color)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		std::vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				if(image(x,y)<=background)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3||image._spectrum==4);
		std::vector<unsigned int> container(image._width);
		unsigned int color=static_cast<unsigned int>(background.r)+background.g+background.b;
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;--y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(static_cast<unsigned int>(pixel.r)+pixel.g+pixel.b<=color)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		std::vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>limit;--y)
			{
				if(image(x,y)<background)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}

	::cimg_library::CImg<float> create_vertical_energy(::cimg_library::CImg<unsigned char> const& refImage,float const vec,unsigned int min_vertical_space,unsigned char background);

	::cimg_library::CImg<float> create_compress_energy(::cimg_library::CImg<unsigned char> const& refImage,unsigned int const min_padding)
	{
		throw std::runtime_error("Not implemented");
	}
	CImg<unsigned char> hpixelator(CImg<unsigned char> const& img)
	{
		throw std::runtime_error("Not implemented");
	}

	//basically a flood fill that can't go left, assumes top row is completely clear
	std::vector<unique_ptr<RectangleUINT>> select_outside(CImg<unsigned char> const& image)
	{
		assert(image._spectrum==1);
		std::vector<unique_ptr<RectangleUINT>> resultContainer;
		CImg<bool> safePoints(image._width,image._height,1,1);
		safePoints.fill(false);
		for(unsigned int x=0;x<image._width;++x)
		{
			safePoints(x,0)=true;
		}
		struct scanRange {
			unsigned int left,right,y;
			int direction;
		};
		stack<scanRange> scanRanges;
		scanRanges.push({0U,image._width,0U,1});
		unsigned int xL;
		//unsigned int xR;
		scanRange r;
		unsigned int sleft;
		unsigned int sright;
		float const tolerance=0.2f;
		while(!scanRanges.empty())
		{
			r=scanRanges.top();
			scanRanges.pop();
			sleft=r.left;
			safePoints(sleft,r.y)=true;
			//scan right
			for(sright=r.right;sright<image._width&&!safePoints(sright,r.y)&&gray_diff(Grayscale::WHITE,image(sright,r.y))<=tolerance;++sright)
			{
				safePoints(sright,r.y)=true;
			}
			resultContainer.push_back(make_unique<RectangleUINT>(RectangleUINT{sleft,sright,r.y,r.y+1U}));

			//scan in same direction vertically
			bool rangeFound=false;
			unsigned int rangeStart=0;
			unsigned int newy=r.y+r.direction;
			if(newy<image._height)
			{
				xL=sleft;
				while(xL<sright)
				{
					for(;xL<sright;++xL)
					{
						if(!safePoints(xL,newy)&&gray_diff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||gray_diff(Grayscale::WHITE,image(xL,newy))>tolerance)
						{
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound)
					{
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,r.direction});
					}
				}
			}

			//scan opposite direction vertically
			newy=r.y-r.direction;
			if(newy<image._height)
			{
				xL=sleft;
				while(xL<r.left)
				{
					for(;xL<r.left;++xL)
					{
						if(!safePoints(xL,newy)&&gray_diff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<r.left;++xL)
					{
						if(safePoints(xL,newy)||gray_diff(Grayscale::WHITE,image(xL,newy))>tolerance)
						{
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound)
					{
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
				xL=r.right+1;
				while(xL<sright)
				{
					for(;xL<sright;++xL)
					{
						if(!safePoints(xL,newy)&&gray_diff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||gray_diff(Grayscale::WHITE,image(xL,newy))>tolerance)
						{
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound)
					{
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
			}
		}
		return resultContainer;
	}

	void add_horizontal_energy(CImg<unsigned char> const& ref,CImg<float>& map,float const hec,unsigned char bg)
	{
		for(unsigned int y=0;y<map._height;++y)
		{
			unsigned int x=0;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&,bg]()
			{
				return node_found=ref(x,y)>bg;
			};
			auto place_values=[&]()
			{
				unsigned int mid=(node_start+x)/2;
				float val_div=2.0f;
				unsigned int node_x;
				for(node_x=node_start;node_x<mid;++node_x)
				{
					++val_div;
					map(node_x,y)+=hec/(val_div*val_div*val_div);
				}
				for(;node_x<x;++node_x)
				{
					--val_div;
					map(node_x,y)+=hec/(val_div*val_div*val_div);
				}
			};
			if(assign_node_found())
			{
				node_start=0;
			}
			for(x=1;x<ref._width;++x)
			{
				if(node_found)
				{
					if(!assign_node_found())
					{
						place_values();
					}
				}
				else
				{
					if(assign_node_found())
					{
						node_start=x;
					}
				}
			}
			if(node_found)
			{
				place_values();
			}
		}
	}
	unsigned int cut_page(CImg<unsigned char> const& image,char const* filename,cut_heuristics const& ch)
	{
		/*
		bool isRGB;
		switch(image._spectrum)
		{
			case 1:
				isRGB=false;
				break;
			case 3:
				isRGB=true;
				//return 0;
				break;
			default:
				return 0;
		}
		*/
		std::vector<std::vector<unsigned int>> paths;
		{
			struct line {
				unsigned int top,bottom,right;
			};
			float const VEC=100.0f;
			CImg<float> map=create_vertical_energy(image,VEC,ch.minimum_vertical_space,ch.background);
			if(ch.horizontal_energy_weight!=0)
				add_horizontal_energy(image,map,ch.horizontal_energy_weight*VEC,ch.background);
			min_energy_to_right(map);
#ifndef NDEBUG
			map.display();
#endif
			
			auto selector=[](std::array<float,1> color)
			{
				return color[0]==INFINITY;
			};
			std::vector<line> boxes;
			{
				auto const selections=global_select<1U>(map,selector);
				auto const clusters=Cluster::cluster_ranges(selections);
				auto heuristic_filter=
					[=]
				(auto box)
				{
					return (box.width()>=ch.min_width&&box.height()>=ch.min_height);
				};
				for(auto c=clusters.cbegin();c!=clusters.cend();++c)
				{
					//if(clusters[i]->size()>threshold)
					auto box=c->bounding_box();
					if(heuristic_filter(box))
					{
						boxes.push_back(line{box.top,box.bottom,box.right});
					}
				}
			}
			std::sort(boxes.begin(),boxes.end(),[](line a,line b)
			{
				return a.top<b.top;
			});
			unsigned int last_row=map._width-1;
			for(size_t i=1;i<boxes.size();++i)
			{
				auto const& current=boxes[i-1];
				auto const& next=boxes[i];
				paths.emplace_back(trace_seam(map,(current.bottom+next.top)/2,(current.right+next.right)/2-1));
			}
		}
		if(paths.size()==0)
		{
			image.save(filename,1,3U);
			return 1;
		}
		/*std::sort(paths.begin(),paths.end(),[](auto const& a,auto const& b)
		{
			assert(a.size()==b.size());
			for(size_t i=a.size();i-->0;)
			{
				if(a[i]<b[i])
				{
					return true;
				}
				if(a[i]>b[i])
				{
					return false;
				}
			}
			throw std::runtime_error("Duplicate paths found (a bug), aborting");
		});*/
		unsigned int num_images=0;
		unsigned int bottom_of_old=0;
		for(unsigned int path_num=0;path_num<paths.size();++path_num)
		{
			auto const& path=paths[path_num];
			unsigned int lowest_in_path=path[0],highest_in_path=path[0];
			for(unsigned int x=1;x<path.size();++x)
			{
				unsigned int node_y=path[x];
				if(node_y>lowest_in_path)
				{
					lowest_in_path=node_y;
				}
				else if(node_y<highest_in_path)
				{
					highest_in_path=node_y;
				}
			}
			unsigned int height;
			height=lowest_in_path-bottom_of_old;
			assert(height<image._height);
			CImg<unsigned char> new_image(image._width,height);
			if(path_num==0)
			{
				for(unsigned int x=0;x<new_image._width;++x)
				{
					unsigned int y=0;
					unsigned int y_stage2=paths[path_num][x];
					for(;y<y_stage2;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=image(x,y);
					}
					for(;y<new_image._height;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=Grayscale::WHITE;
					}
				}
			}
			else
			{
				for(unsigned int x=0;x<new_image._width;++x)
				{
					unsigned int y=0;
					unsigned int y_stage1=paths[path_num-1][x]-bottom_of_old;
					unsigned int y_stage2=paths[path_num][x]-bottom_of_old;
					for(;y<y_stage1;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=Grayscale::WHITE;
					}
					for(;y<y_stage2;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=image(x,y+bottom_of_old);
					}
					for(;y<new_image._height;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=Grayscale::WHITE;
					}
				}
			}
			new_image.save(filename,++num_images,3U);
			bottom_of_old=highest_in_path;
		}
		CImg<unsigned char> new_image(image._width,image._height-bottom_of_old);
		unsigned int y_max=image._height-bottom_of_old;
		for(unsigned int x=0;x<new_image._width;++x)
		{
			unsigned int y=0;
			unsigned int y_stage1=paths.back()[x]-bottom_of_old;
			for(;y<y_stage1;++y)
			{
				new_image(x,y)=Grayscale::WHITE;
			}
			for(;y<new_image._height;++y)
			{
				new_image(x,y)=image(x,y+bottom_of_old);
			}
		}
		new_image.save(filename,++num_images,3U);
		return num_images;
	}

	float auto_rotate(CImg<unsigned char>& image,double pixel_prec,double min_angle,double max_angle,double angle_prec,unsigned char boundary)
	{
		assert(angle_prec>0);
		assert(pixel_prec>0);
		assert(min_angle<max_angle);
		return RAD_DEG*auto_rotate_bare(image,pixel_prec,min_angle*DEG_RAD+M_PI_2,max_angle*DEG_RAD+M_PI_2,(max_angle-min_angle)/angle_prec+1,boundary);
	}

	float find_angle_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary)
	{
		assert(angle_steps>0);
		assert(pixel_prec>0);
		assert(min_angle<max_angle);
		if(img._height==0||img._width==0)
		{
			return 0;
		}
		if(img._spectrum<3)
		{
			auto selector=[&img,boundary](unsigned int x,unsigned int y)
			{
				auto const top=img(x,y);
				auto const bottom=img(x,y+1);
				return
					//(top<=boundary&&bottom>boundary)||
					(top>boundary&&bottom<=boundary);
			};
			HoughArray<unsigned short> ha(selector,img._width,img._height-1,min_angle,max_angle,angle_steps,pixel_prec);
			return M_PI_2-ha.angle();
		}
		else
		{
			auto selector=[&img,boundary=3U*boundary,size=size_t(img._width)*img._height](unsigned int x,unsigned int y)
			{
				auto const ptop=&img(x,y);
				auto const pbottom=&img(x,y+1);
				auto const top=unsigned int(*ptop)+*(ptop+size)+*(ptop+2*size);
				auto const bottom=unsigned int(*pbottom)+*(pbottom+size)+*(pbottom+2*size);
				return
					//(top<=boundary&&bottom>boundary)||
					(top>boundary&&bottom<=boundary);
			};
			HoughArray<unsigned short> ha(selector,img._width,img._height-1,min_angle,max_angle,angle_steps,pixel_prec);
			return M_PI_2-ha.angle();
		}
	}

	float auto_rotate_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary)
	{
		auto angle=find_angle_bare(img,pixel_prec,min_angle,max_angle,angle_steps,boundary);
		img.rotate(angle*RAD_DEG,2,1);
		return angle;
	}

	bool auto_skew(CImg<unsigned char>& image)
	{
		return false;
	}
	line<unsigned int> find_top_line(CImg<unsigned char> const& image)
	{
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_bottom_line(CImg<unsigned char> const& image)
	{
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_left_line(CImg<unsigned char> const& image)
	{
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_right_line(CImg<unsigned char> const& image)
	{
		std::vector<unsigned int> rightProfile;
		switch(image._spectrum)
		{
			case 1:
				rightProfile=build_right_profile(image,Grayscale::WHITE);
				break;
			case 3:
				rightProfile=build_right_profile(image,ColorRGB::WHITE);
				break;
			default:
				return {{0,0},{0,0}};
		}
		struct LineCandidate {
			PointUINT start;
			PointUINT last;
			unsigned int numPoints;
			int error;//dx/dy
			float derror_dy;
		};
		std::vector<LineCandidate> candidates;
		unsigned int const limit=image._width/2+1;
		unsigned int lastIndex=0;
		for(unsigned int pi=0;pi<rightProfile.size();++pi)
		{
			if(rightProfile[pi]<=limit)
			{
				continue;
			}
			PointUINT test{rightProfile[pi],pi};
			unsigned int ci;
			for(ci=lastIndex;ci<candidates.size();++ci)
			{
#define determineLine() \
				int error=static_cast<signed int>(test.x-candidates[ci].start.x);\
				float derror_dy=static_cast<float>(error-candidates[ci].error)/(test.y-candidates[ci].last.y);\
				if(abs_dif(derror_dy,candidates[ci].derror_dy)<1.1f) {\
					candidates[ci].error=error;\
					candidates[ci].derror_dy=derror_dy;\
					candidates[ci].last=test;\
					++candidates[ci].numPoints;\
					lastIndex=ci;\
					goto skipCreatingNewCandidate;\
				}
				determineLine();
			}
			for(ci=0;ci<lastIndex;++ci)
			{
				determineLine();
			}
#undef determineLine
			candidates.push_back({test,test,1,0,0.0f});
			lastIndex=candidates.size()-1;
		skipCreatingNewCandidate:;
		}
		if(candidates.size()==0)
			return {{0,0},{0,0}};
		unsigned int maxIndex=0;
		unsigned int maxPoints=candidates[maxIndex].numPoints;
		for(unsigned int ci=0;ci<candidates.size();++ci)
		{
			if(candidates[ci].numPoints>maxPoints)
			{
				maxIndex=ci;
				maxPoints=candidates[ci].numPoints;
			}
		}
		//cout<<candidates[maxIndex].numPoints<<'\n';
		return {candidates[maxIndex].start,candidates[maxIndex].last};
	}
	bool undistort(CImg<unsigned char>& image)
	{}

	std::vector<ImageUtils::Rectangle<unsigned int>> flood_select(CImg<unsigned char> const& image,float const tolerance,Grayscale const color,Point<unsigned int> start)
	{
		assert(image._spectrum==1);
		std::vector<ImageUtils::Rectangle<unsigned int>> result_container;
		if(gray_diff(image(start.x,start.y),color)>tolerance)
		{
			return result_container;
		}
		CImg<char> unchecked(image._width,image._height,1,1);
		unchecked.fill(true);
		unsigned int left=start.x-1;
		while(left<start.x&&gray_diff(color,image(left,start.y))<=tolerance)
		{
			unchecked(left,start.y)=false;
			--left;
		}
		++left;
		unsigned int right=start.x+1;
		while(right<image._width&&gray_diff(color,image(right,start.y))<=tolerance)
		{
			unchecked(right,start.y)=false;
			++right;
		}

		result_container.push_back(ImageUtils::Rectangle<unsigned int>{left,right,start.y,start.y+1});
		struct scan_range {
			unsigned int left,right,y;
			char direction;
		};
		stack<scan_range> scan_ranges;
		scan_ranges.push({left,right,start.y,+1});
		scan_ranges.push({left,right,start.y,-1});
		while(!scan_ranges.empty())
		{
			scan_range const current_range=scan_ranges.top();
			scan_ranges.pop();
			//scan left
			for(left=current_range.left-1;
				left<image._width&&unchecked(left,current_range.y)&&gray_diff(color,image(left,current_range.y))<=tolerance;
				--left)
			{
				unchecked(left,current_range.y)=false;
			}
			++left;

			//scan right
			for(right=current_range.right;
				right<image._width&&unchecked(right,current_range.y)&&gray_diff(color,image(right,current_range.y))<=tolerance;
				++right)
			{
				unchecked(right,current_range.y)=false;
			}
			result_container.push_back(ImageUtils::Rectangle<unsigned int>{left,right,current_range.y,current_range.y+1});

			//scan in same direction vertically
			bool range_found=false;
			unsigned int range_start;
			unsigned int newy=current_range.y+current_range.direction;
			unsigned int sx=left;
			if(newy<image._height)
			{
				while(sx<right)
				{
					for(;sx<right;++sx)
					{
						if(unchecked(sx,newy)&&gray_diff(color,image(sx,newy))<=tolerance)
						{
							unchecked(sx,newy)=false;
							range_found=true;
							range_start=sx++;
							break;
						}
					}
					for(;sx<right;++sx)
					{
						if(!unchecked(sx,newy)||gray_diff(color,image(sx,newy))>tolerance)
						{
							break;
						}
						unchecked(sx,newy)=false;
					}
					if(range_found)
					{
						range_found=false;
						scan_ranges.push({range_start,sx,newy,current_range.direction});
					}
				}
			}

			//scan opposite direction vertically
			newy=current_range.y-current_range.direction;
			if(newy<image._height)
			{
				while(left<current_range.left)
				{
					for(;left<current_range.left;++left)
					{
						if(unchecked(left,newy)&&gray_diff(color,image(left,newy))<=tolerance)
						{
							unchecked(left,newy)=false;
							range_found=true;
							range_start=left++;
							break;
						}
					}
					for(;left<current_range.left;++left)
					{
						if(!unchecked(left,newy)||gray_diff(color,image(left,newy))>tolerance)
							break;
						unchecked(left,newy)=false;
					}
					if(range_found)
					{
						range_found=false;
						scan_ranges.push({range_start,left,newy,-current_range.direction});
					}
				}
				left=current_range.right;
				while(left<right)
				{
					for(;left<right;++left)
					{
						if(unchecked(left,newy)&&gray_diff(color,image(left,newy))<=tolerance)
						{
							unchecked(left,newy)=false;
							range_found=true;
							range_start=left++;
							break;
						}
					}
					for(;left<right;++left)
					{
						if(!unchecked(left,newy)||gray_diff(color,image(left,newy))>tolerance)
						{
							break;
						}
						unchecked(left,newy)=false;
					}
					if(range_found)
					{
						range_found=false;
						scan_ranges.push({range_start,left,newy,-current_range.direction});
					}
				}
			}
		}
		return result_container;
	}
	bool auto_padding(CImg<unsigned char>& image,unsigned int const vertical_padding,unsigned int const horizontal_padding_max,unsigned int const horizontal_padding_min,signed int horiz_offset,float optimal_ratio,unsigned int tolerance,unsigned char background)
	{
		unsigned int left,right,top,bottom;
		switch(image._spectrum)
		{
			case 1:
			case 2:
			{
				auto selector=[=](auto color)
				{
					return color[0]<background;
				};
				left=find_left<3>(image,tolerance,selector);
				right=find_right<3>(image,tolerance,selector)+1;
				top=find_top<3>(image,tolerance,selector);
				bottom=find_bottom<3>(image,tolerance,selector)+1;
				break;
			}
			default:
			{
				auto selector=[bg=3*float(background)](auto color)
				{
					return float(color[0])+color[1]+color[2]<bg;
				};
				left=find_left<3>(image,tolerance,selector);
				right=find_right<3>(image,tolerance,selector)+1;
				top=find_top<3>(image,tolerance,selector);
				bottom=find_bottom<3>(image,tolerance,selector)+1;
				break;
			}
		}
		if(left>right) return false;
		if(top>bottom) return false;

		unsigned int true_height=bottom-top;
		unsigned int optimal_width=optimal_ratio*(true_height+2*vertical_padding);
		unsigned int horizontal_padding;
		unsigned int actual_width=right-left;
		if(optimal_width>actual_width)
		{
			horizontal_padding=(optimal_width-actual_width)/2;
			if(horizontal_padding>horizontal_padding_max)
			{
				horizontal_padding=horizontal_padding_max;
			}
			else if(horizontal_padding<horizontal_padding_min)
			{
				horizontal_padding=horizontal_padding_min;
			}
		}
		else
		{
			horizontal_padding=horizontal_padding_min;
		}
		signed int x1=left-horizontal_padding;
		signed int y1=top-vertical_padding;
		signed int x2=right+horizontal_padding-1+horiz_offset;
		signed int y2=bottom+vertical_padding-1;
		if(x1==0&&y1==0&&x2==image.width()-1&&y2==image.height()-1)
		{
			return false;
		}
		image=get_crop_fill(
			image,
			{x1,x2,y1,y2},
			unsigned char(255)
		);
		return true;
	}
	bool horiz_padding(CImg<unsigned char>& image,unsigned int const left)
	{
		return horiz_padding(image,left,left);
	}
	bool horiz_padding(CImg<unsigned char>& image,unsigned int const left_pad,unsigned int const right_pad,unsigned int tolerance,unsigned char background)
	{
		signed int x1,x2;
		switch(image._spectrum)
		{
			case 1:
			case 2:
			{
				auto selector=[=](auto color)
				{
					return color[0]<=background;
				};
				x1=left_pad==-1?0:find_left<1>(image,tolerance,selector)-left_pad;
				x2=right_pad==-1?image.width()-1:find_right<1>(image,tolerance,selector)+right_pad;
				break;
			}
			default:
			{
				auto selector=[bg=3U*unsigned int(background)](auto color)
				{
					return unsigned int(color[0])+color[1]+color[2]<=bg;
				};
				x1=left_pad==-1?0:find_left<3>(image,tolerance,selector)-left_pad;
				x2=right_pad==-1?image.width()-1:find_right<3>(image,tolerance,selector)+right_pad;
				break;
			}
		}
		if(x1>x2)
		{
			std::swap(x1,x2);
		}
		if(x1==0&&x2==image.width()-1)
		{
			return false;
		}
		image=get_crop_fill(image,{x1,x2,0,image.height()-1});
		return true;
	}
	bool vert_padding(CImg<unsigned char>& image,unsigned int const p)
	{
		return vert_padding(image,p,p);
	}
	bool vert_padding(CImg<unsigned char>& image,unsigned int const tp,unsigned int const bp,unsigned int tolerance,unsigned char background)
	{
		signed int y1,y2;
		switch(image._spectrum)
		{
			case 1:
			case 2:
			{
				auto selector=[=](auto color)
				{
					return color[0]<=background;
				};
				y1=tp==-1?0:find_top<1>(image,tolerance,selector)-tp;
				y2=bp==-1?image.height()-1:find_bottom<1>(image,tolerance,selector)+bp;
				break;
			}
			default:
			{
				auto selector=[bg=3U*unsigned int(background)](auto color)
				{
					return unsigned int(color[0])+color[1]+color[2]<=bg;
				};
				y1=tp==-1?0:find_top<3>(image,tolerance,selector)-tp;
				y2=bp==-1?image.height()-1:find_bottom<3>(image,tolerance,selector)+bp;
				break;
			}
		}
		if(y1>y2)
		{
			std::swap(y1,y2);
		}
		if(y1==0&&y2==image.height()-1)
		{
			return false;
		}
		image=get_crop_fill(image,{0,image.width()-1,y1,y2});
		return true;
	}

	bool cluster_padding(
		::cil::CImg<unsigned char>& img,
		unsigned int const lp,
		unsigned int const rp,
		unsigned int const tp,
		unsigned int const bp,
		unsigned char bt)
	{
		auto selections=img._spectrum>2?
			global_select<3>(img,[threshold=3U*unsigned short(bt)](auto color)
		{
			return unsigned short(color[0])+color[1]+color[2]<=threshold;
		}):
			global_select<1>(img,[bt](auto color)
		{
			return color[0]<=bt;
		});
		auto clusters=Cluster::cluster_ranges(selections);
		if(clusters.size()==0)
		{
			return false;
		}
		unsigned int top_size=0;
		Cluster const* top_cluster;
		for(auto const& cluster:clusters)
		{
			auto size=cluster.size();
			if(size>top_size)
			{
				top_size=size;
				top_cluster=&cluster;
			}
		}
		auto com=top_cluster->bounding_box().center<double>();
		ImageUtils::PointUINT tl,tr,bl,br;
		double tld=0,trd=0,bld=0,brd=0;
		auto switch_if_bigger=[com](double& dist,PointUINT& contender,PointUINT cand)
		{
			double d=hypot(com.x-double(cand.x),com.y-double(cand.y));
			if(d>dist)
			{
				dist=d;
				contender=cand;
			}
		};
		for(auto const rect:top_cluster->get_ranges())
		{
			PointUINT cand;
			cand={rect.left,rect.top};
			if(cand.x<=com.x&&cand.y<=com.y) switch_if_bigger(tld,tl,cand);
			cand.x=rect.right;
			if(cand.x>=com.x&&cand.y<=com.y) switch_if_bigger(trd,tr,cand);
			cand.y=rect.bottom;
			if(cand.x>=com.x&&cand.y>=com.y) switch_if_bigger(brd,br,cand);
			cand.x=rect.left;
			if(cand.x<=com.x&&cand.y>=com.y) switch_if_bigger(bld,bl,cand);
		}
		int left=lp==-1?0:int(std::min(tl.x,bl.x))-int(lp);
		int right=rp==-1?img.width():int(std::max(tr.x,br.x))+int(rp);
		int top=tp==-1?0:int(std::min(tl.y,tr.y))-int(tp);
		int bottom=bp==-1?img.height():int(std::max(bl.y,br.y))+int(bp);
		if(left==0&&right==img.width()&&top==0&&bottom==img.height())
		{
			return false;
		}
		img=get_crop_fill(img,{left,right-1,top,bottom-1});
		return true;
	}

	void add_horizontal_energy(cimg_library::CImg<unsigned char> const& ref,cimg_library::CImg<float>& map,float const hec,unsigned char bg);

	void compress(
		CImg<unsigned char>& image,
		unsigned int const min_padding,
		unsigned int const optimal_height,
		unsigned char background_threshold)
	{
		auto copy=hpixelator(image);
		CImg<float> energy_map=create_compress_energy(copy,min_padding);
		if(image._spectrum==1)
		{
			clear_clusters(copy,std::array<unsigned char,1>({255}),[](auto val)
			{
				return val[0]<220;
			},[threshold=2*copy._width/3](Cluster const& c){
				return c.bounding_box().width()<threshold;
			});
		}
		else
		{
			throw "Bad spectrum";
		}
		copy.display();
		/*
		uint num_seams=image._height-optimal_height;
		for(uint i=0;i<num_seams;++i)
		{
			auto map=create_compress_energy(image);
			min_energy_to_right(map);
			auto seam=create_seam(map);
			for(auto s=0U;s<map._width;++s)
			{
				ImageUtils::Rectangle<uint> to_shift{map._width-s-1,map._width-s,seam[s]+1,map._height};
				copy_shift_selection(image,to_shift,0,-1);
				copy_shift_selection(map,to_shift,0,-1);
			}
			image.crop(0,0,image._width-1,image._height-2);
		}
		*/
	}
	CImg<float> create_vertical_energy(CImg<unsigned char> const& ref,float const vec,unsigned int min_vert_space,unsigned char background)
	{
		CImg<float> map(ref._width,ref._height);
		map.fill(INFINITY);
		for(unsigned int x=0;x<map._width;++x)
		{
			unsigned int y=0;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&,background]()
			{
				return node_found=ref(x,y)>background;
			};
			auto place_values=[&,vec,min_vert_space](unsigned int begin,unsigned int end)
			{
				if(end-begin>min_vert_space)
				{
					unsigned int mid=(node_start+y)/2;
					float val_div=2.0f;
					unsigned int y;
					for(y=begin;y<mid;++y)
					{
						++val_div;
						map(x,y)=vec/(val_div*val_div*val_div);
					}
					for(;y<end;++y)
					{
						--val_div;
						map(x,y)=vec/(val_div*val_div*val_div);
					}
				}
			};
			if(assign_node_found())
			{
				node_start=0;
			}
			for(y=1;y<ref._height;++y)
			{
				if(node_found)
				{
					if(!assign_node_found())
					{
						place_values(node_start,y);
					}
				}
				else
				{
					if(assign_node_found())
					{
						node_start=y;
					}
				}
			}
			if(node_found)
			{
				place_values(node_start,y);
			}
		}
		return map;
	}
	float const COMPRESS_HORIZONTAL_ENERGY_CONSTANT=1.0f;
	CImg<float> create_compress_energy(CImg<unsigned char> const& ref)
	{
		auto map=create_vertical_energy(ref,100.0f,0,127);
		for(auto y=0U;y<map._height;++y)
		{
			auto x=0U;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&]()
			{
				return (node_found=map(x,y)==INFINITY);
			};
			auto place_values=[&]()
			{
				unsigned int multiplier=(x-node_start)/2;
				for(auto node_x=node_start;node_x<x;++node_x)
				{
					map(node_x,y)=COMPRESS_HORIZONTAL_ENERGY_CONSTANT*multiplier;
				}
			};
			if(assign_node_found())
			{
				node_start=0;
			}
			for(x=1;x<map._width;++x)
			{
				if(node_found)
				{
					if(!assign_node_found())
					{
						place_values();
					}
				}
				else
				{
					if(assign_node_found())
					{
						node_start=x;
					}
				}
			}
			if(node_found)
			{
				place_values();
			}
		}
		return map;
	}
	bool remove_border(CImg<unsigned char>& image,Grayscale const color,float const tolerance)
	{
		uint right=image._width-1;
		uint bottom=image._height-1;
		bool edited=false;
		for(uint x=0;x<=right;++x)
		{
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{x,0});
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{x,bottom});
		}
		for(uint y=1;y<bottom;++y)
		{
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{0,y});
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{right,y});
		}
		return edited;
	}
	bool flood_fill(CImg<unsigned char>& image,float const tolerance,Grayscale const color,Grayscale const replacer,Point<unsigned int> point)
	{
		auto rects=flood_select(image,tolerance,color,point);
		if(rects.empty())
		{
			return false;
		}
		for(auto rect:rects)
		{
			fill_selection(image,rect,replacer);
		}
		return true;
	}
	void rescale_colors(CImg<unsigned char>& img,unsigned char min,unsigned char mid,unsigned char max)
	{
		assert(min<mid);
		assert(mid<max);
		double const scale_up=scast<double>(255-mid)/scast<double>(max-mid);
		double const scale_down=scast<double>(mid)/scast<double>(mid-min);
		unsigned char* const limit=img.begin()+img._width*img._height;
		for(unsigned char* it=img.begin();it!=limit;++it)
		{
			unsigned char& pixel=*it;
			if(pixel<=min)
			{
				pixel=0;
			}
			else if(pixel>=max)
			{
				pixel=255;
			}
			else
			{
				if(pixel>mid)
				{
					pixel=mid+(pixel-mid)*scale_up;
					assert(pixel>mid);
				}
				else
				{
					pixel=mid-(mid-pixel)*scale_down;
					assert(pixel<=mid);
				}
			}
		}
	}
}
