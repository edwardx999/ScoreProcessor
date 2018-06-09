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
	vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3);
		unsigned int limit=image._width/2;
		vector<unsigned int> container(image._height);
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
	vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		unsigned int limit=image._width/2;
		vector<unsigned int> container(image._height);
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
	vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3);
		unsigned int limit=image._width/2;
		vector<unsigned int> container(image._height);
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
	vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		unsigned int limit=image._width/2;
		vector<unsigned int> container(image._height);
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
	vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3||image._spectrum==4);
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				if(gray_diff(image(x,y),background)>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3||image._spectrum==4);
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;--y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;--y)
			{
				if(gray_diff(image(x,y),background)>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> fattened_profile_high(vector<unsigned int> const& profile,unsigned int horiz_padding)
	{
		return exlib::fattened_profile(profile,horiz_padding,[](unsigned int a,unsigned int b)
		{
			return a<=b;
		});
	}
	vector<unsigned int> fattened_profile_low(vector<unsigned int> const& profile,unsigned int horiz_padding)
	{
		return exlib::fattened_profile(profile,horiz_padding,[](unsigned int a,unsigned int b)
		{
			return a>=b;
		});
	}
	::cimg_library::CImg<float> create_vertical_energy(::cimg_library::CImg<unsigned char> const& refImage,float const vec);

	::cimg_library::CImg<float> create_compress_energy(::cimg_library::CImg<unsigned char> const& refImage,unsigned int const min_padding)
	{
		throw std::runtime_error("Not implemented");
	}
	CImg<unsigned char> hpixelator(CImg<unsigned char> const& img)
	{
		throw std::runtime_error("Not implemented");
	}

	//basically a flood fill that can't go left, assumes top row is completely clear
	vector<unique_ptr<RectangleUINT>> select_outside(CImg<unsigned char> const& image)
	{
		assert(image._spectrum==1);
		vector<unique_ptr<RectangleUINT>> resultContainer;
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

	void add_horizontal_energy(CImg<unsigned char> const& ref,CImg<float>& map,float const hec)
	{
		for(unsigned int y=0;y<map._height;++y)
		{
			unsigned int x=0;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&]()
			{
				return node_found=gray_diff(Grayscale::WHITE,ref(x,y))<.2f;
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
	unsigned int cut_page(CImg<unsigned char> const& image,char const* filename,cut_heuristics const ch)
	{
		//bool isRGB;
		//switch(image._spectrum)
		//{
		//	case 1:
		//		isRGB=false;
		//		break;
		//	case 3:
		//		isRGB=true;
		//		//return 0;
		//		break;
		//	default:
		//		return 0;
		//}

		vector<vector<unsigned int>> paths;
		{
			struct line {
				unsigned int top,bottom,right;
			};
			float const VEC=100.0f;
			CImg<float> map=create_vertical_energy(image,VEC);
			if(ch.horizontal_energy_weight!=0)
				add_horizontal_energy(image,map,ch.horizontal_energy_weight);
			min_energy_to_right(map);
#ifndef NDEBUG
			map.display();
#endif
			auto selector=[](std::array<float,1> color)
			{
				return color[0]==INFINITY;
			};
			vector<line> boxes;
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
					auto box=(*c)->bounding_box();
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

	float auto_rotate_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary)
	{
		assert(angle_steps>0);
		assert(pixel_prec>0);
		assert(min_angle<max_angle);
		if(img._height==0||img._width==0)
		{
			return 0;
		}
		auto selector=[&img,boundary](unsigned int x,unsigned int y)
		{
			auto top=img(x,y);
			auto bottom=img(x,y+1);
			return
				//(top<=boundary&&bottom>boundary)||
				(top>boundary&&bottom<=boundary);
		};
		HoughArray<unsigned short> ha(selector,img._width,img._height-1,min_angle,max_angle,angle_steps,pixel_prec);
		auto max=*std::max_element(ha.begin(),ha.end());
		double ha_angle=M_PI_2-ha.angle();
		double angle=ha_angle*RAD_DEG;
		img.rotate(angle,2,1);
		return ha_angle;
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
		vector<unsigned int> rightProfile;
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
		vector<LineCandidate> candidates;
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

	vector<ImageUtils::Rectangle<unsigned int>> flood_select(CImg<unsigned char> const& image,float const tolerance,Grayscale const color,Point<unsigned int> start)
	{
		assert(image._spectrum==1);
		vector<ImageUtils::Rectangle<unsigned int>> result_container;
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
		if(left>right) std::swap(left,right);
		if(top>bottom) std::swap(top,bottom);

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
		image=crop_fill(
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
		unsigned int left,right;
		switch(image._spectrum)
		{
			case 1:
			case 2:
			{
				auto selector=[=](auto color)
				{
					return color[0]<background;
				};
				left=find_left<1>(image,tolerance,selector);
				right=find_right<1>(image,tolerance,selector);
				break;
			}
			default:
			{
				auto selector=[bg=3*float(background)](auto color)
				{
					return float(color[0])+color[1]+color[2]<bg;
				};
				left=find_left<3>(image,tolerance,selector);
				right=find_right<3>(image,tolerance,selector);
				break;
			}
		}
		if(left>right)
		{
			std::swap(left,right);
		}
		signed int x1=(left-left_pad);
		signed int x2=right+right_pad;
		if(x1==0&&x2==image._width-1)
		{
			return false;
		}
		image=crop_fill(image,{x1,x2,0,image.height()-1});
		return true;
	}
	bool vert_padding(CImg<unsigned char>& image,unsigned int const p)
	{
		return vert_padding(image,p,p);
	}
	bool vert_padding(CImg<unsigned char>& image,unsigned int const tp,unsigned int const bp,unsigned int tolerance,unsigned char background)
	{
		unsigned int top,bottom;
		switch(image._spectrum)
		{
			case 1:
			case 2:
			{
				auto selector=[=](auto color)
				{
					return color[0]<background;
				};
				top=find_top<1>(image,tolerance,selector);
				bottom=find_bottom<1>(image,tolerance,selector);
				break;
			}
			default:
			{
				auto selector=[bg=3*float(background)](auto color)
				{
					return float(color[0])+color[1]+color[2]<bg;
				};
				top=find_top<3>(image,tolerance,selector);
				bottom=find_bottom<3>(image,tolerance,selector);
				break;
			}
		}
		if(top>bottom)
		{
			std::swap(top,bottom);
		}
		signed int y1=top-tp;
		signed int y2=bottom+bp;
		if(y1==0&&y2==image.height()-1)
		{
			return false;
		}
		image=crop_fill(image,{0,image.width()-1,y1,y2});
		return true;
	}

	struct page:CImg<unsigned char> {
		uint top;
		uint bottom;
		page(char const* filename):CImg(filename)
		{}
		page()
		{}
		uint height() const
		{
			return bottom-top;
		}
	};
	struct spacing {
		uint bottom_sg;
		uint top_sg;
	};
	spacing find_spacing(vector<uint> const& bottom,uint size_top,vector<uint> const& top);
	CImg<uchar> splice_images(page const* imgs,size_t num,unsigned int padding)
	{
		uint height=0;
		uint width=0;
		for(size_t i=0;i<num;++i)
		{
			height+=imgs[i].height();
			if(imgs[i]._width>width)
			{
				width=imgs[i]._width;
			}
		}
		height+=padding*(num+1);
		CImg<uchar> tog(width,height,1);
		tog.fill(255);
		uint ypos=padding;
		for(size_t i=0;i<num;++i)
		{
			auto const& current=imgs[i];
			for(uint y=0;y<current._height;++y)
			{
				uint yabs=ypos+y-current.top;
				if(yabs<tog._height)
				{
					for(uint x=1;x<=current._width;++x)
					{
						uchar& tpixel=tog(tog._width-x,yabs);
						tpixel=std::min(current(current._width-x,y),tpixel);
					}
				}
			}
			ypos+=padding+current.height();
		}
		return tog;
	}
	vector<uint> get_top_profile(CImg<uchar> const& img)
	{
#define base_prof(side)\
		switch(img.spectrum())\
		{\
			case 1:\
				return build_##side##_profile(img,Grayscale::WHITE);\
			case 3:\
			case 4:\
				return build_##side##_profile(img,ColorRGB::WHITE);\
			default:\
				throw std::invalid_argument("Invalid number of layers");\
		}
		base_prof(top);
	}
	vector<uint> get_bottom_profile(CImg<uchar> const& img)
	{
		base_prof(bottom);
#undef base_prof
	}
	unsigned int splice_pages(
		vector<::std::string> const& filenames,
		unsigned int horiz_padding,
		unsigned int optimal_padding,
		unsigned int min_padding,
		unsigned int optimal_height,
		char const* output,
		unsigned int const starting_index)
	{
		if(filenames.empty())
		{
			return 0;
		}
		struct cost_pad {
			float cost;
			uint padding;
		};
		class Splice: public vector<page> {
		public:
			cost_pad pad_info;
		};

		Splice items;
		items.emplace_back(filenames[0].c_str());
		auto const& first_page=items[0];
		unsigned int touch_tolerance=first_page._width/1024+1;
		switch(first_page._spectrum)
		{
			case 1:
				items[0].top=find_top(first_page,Grayscale::WHITE,touch_tolerance);
				break;
			case 3:
			case 4:
				items[0].top=find_top(first_page,ColorRGB::WHITE,touch_tolerance);
				break;
			default:
				throw std::invalid_argument("Invalid number of layers");
		}
		if(optimal_height==-1)
		{
			optimal_height=first_page._width*6/11;
		}
		auto cost_splice=
			[optimal_padding,min_padding,optimal_height](page const* page,size_t num)
		{
			uint height=0;
			for(size_t i=0;i<num;++i)
			{
				height+=page[i].height();
			}
			uint padding;
			if(height>optimal_height)
			{
				padding=min_padding;
			}
			else
			{
				padding=(optimal_height-height)/(num+1);
				if(padding<min_padding)
				{
					padding=min_padding;
				}
			}
			height+=padding*(num+1);

			float height_cost=abs_dif(1.0f,float(height)/optimal_height);
			//height_cost=powf(height_cost,0.8f);
			float padding_cost=abs_dif(1.0f,float(padding)/optimal_padding);
			//padding_cost=powf(padding_cost,0.6f);
			return cost_pad
			{
				padding_cost+height_cost,
				padding
			};
		};
		unsigned int num_pages=0;
		//Creating Vertical profiles of each image
		vector<uint> above_bottom_profile,below_top_profile,below_bottom_profile;
		above_bottom_profile=get_bottom_profile(items[0]);
		items[0].bottom=*max_element(above_bottom_profile.begin(),above_bottom_profile.end());
		items.pad_info=cost_splice(items.data(),1);
		uint const num_digits=exlib::num_digits(filenames.size());
		for(auto it=filenames.begin()+1;it!=filenames.end();++it)
		{
			items.emplace_back(it->c_str());
			auto& above=*(items.end()-2);
			auto& below=items.back();

			below_top_profile=get_top_profile(below);
			below_bottom_profile=get_bottom_profile(below);

			//fattening profiles horizontally (adding horizontal padding)
			above_bottom_profile=fattened_profile_low(above_bottom_profile,horiz_padding);
			below_top_profile=fattened_profile_high(below_top_profile,horiz_padding);
			//finding smallest gap between pages
			auto spacing=find_spacing(above_bottom_profile,above._height,below_top_profile);
			auto old_bottom=above.bottom;
			above.bottom=spacing.bottom_sg;
			below.top=spacing.top_sg;
			below.bottom=*max_element(below_bottom_profile.begin(),below_bottom_profile.end());
			auto c=cost_splice(items.data(),items.size());
			if(c.cost<items.pad_info.cost)
			{
				items.pad_info=c;
			}
			else
			{
				above.bottom=old_bottom;
				auto tog=splice_images(items.data(),items.size()-1,items.pad_info.padding);
				tog.save(output,starting_index+num_pages,num_digits);
				++num_pages;
				items.erase(items.begin(),items.end()-1);
				items[0].top=*min_element(below_top_profile.begin(),below_top_profile.end());
				items.pad_info=cost_splice(items.data(),1);
			}
			above_bottom_profile=move(below_bottom_profile);
		}
		auto tog=splice_images(items.data(),items.size(),items.pad_info.padding);
		tog.save(output,num_pages+starting_index,num_digits);
		return num_pages+1;
	}

	spacing find_spacing(vector<uint> const& bottom_of_top,uint size_top,vector<uint> const& top_of_bottom)
	{
		auto b=bottom_of_top.rbegin();
		auto t=top_of_bottom.rbegin();
		auto end=b+min(bottom_of_top.size(),top_of_bottom.size());
		unsigned int min_spacing=std::numeric_limits<unsigned int>::max();
		spacing ret;
		for(;b!=end;++b,++t)
		{
			unsigned int cand=size_top-*b+*t;
			if(cand<min_spacing)
			{
				min_spacing=cand;
				ret.bottom_sg=*b;
				ret.top_sg=*t;
			}
		}
		return ret;
	}

	unsigned int splice_pages_nongreedy(
		::std::vector<::std::string> const& filenames,
		ImageUtils::perc_or_val horiz_padding,
		ImageUtils::perc_or_val optimal_height,
		ImageUtils::perc_or_val optimal_padding,
		ImageUtils::perc_or_val min_padding,
		char const* output,
		float excess_weight,
		float padding_weight,
		unsigned int starting_index)
	{
#ifndef NDEBUG
		std::cout<<excess_weight<<'\n';
#endif
		struct item {
			unsigned int top_raw;
			unsigned int top_kern;
			unsigned int bottom_kern;
			unsigned int bottom_raw;
		};
		auto const c=filenames.size();
		if(c<2)
		{
			throw std::invalid_argument("Need multiple pages to splice");
		}
		vector<item> pages(c);
		{
			auto conv=[](CImg<unsigned char>& img)
			{
				if(img._spectrum>2)
				{
					img=get_grayscale_simple(img);
				}
			};
			CImg<unsigned char> top(filenames[0].c_str());
			CImg<unsigned char> bottom(filenames[1].c_str());
			auto fix_perc=[basis=top._width](ImageUtils::perc_or_val& pv)
			{
				if(pv.is_perc)
				{
					pv.val=unsigned int(std::round(basis*pv.perc/100.0f));
				}
			};
			fix_perc(optimal_height);
			fix_perc(optimal_padding);
			fix_perc(min_padding);
			fix_perc(horiz_padding);
			conv(top);
			conv(bottom);
			pages[0].top_kern=(pages[0].top_raw=find_top(top,255,top._width/1024+1));
#ifndef NDEBUG
			std::cout<<"Initializing page info"<<'\n';
#endif
			for(size_t i=0;;++i)
			{
#ifndef NDEBUG
				std::cout<<i<<'\n';
#endif
				auto bot=build_bottom_profile(top,255);
				bot=fattened_profile_low(bot,horiz_padding.val);
				auto tob=build_top_profile(bottom,255);
				tob=fattened_profile_high(tob,horiz_padding.val);
				auto const sp=find_spacing(bot,top._height,tob);
				pages[i].bottom_raw=*std::max_element(bot.cbegin(),bot.cend());
				pages[i].bottom_kern=sp.bottom_sg;
				pages[i+1].top_kern=sp.top_sg;
				pages[i+1].top_raw=*std::min_element(tob.cbegin(),tob.cend());
				if(i>=c-2)
				{
					break;
				}
				top=std::move(bottom);
				conv(bottom.assign(filenames[i+2].c_str()));
			}
			pages[c-1].bottom_raw=(pages[c-1].bottom_kern=find_bottom(bottom,255,bottom._width/1024+1));
		}
		struct page_layout {
			unsigned int padding;
			unsigned int height;
		};
		auto create_layout=[=](item const* const items,size_t const n)
		{
			assert(n!=0);
			unsigned int total_height;
			if(n==1)
			{
				total_height=items[0].bottom_raw-items[0].top_raw;
			}
			else
			{
				total_height=items[0].bottom_kern-items[0].top_raw;
				for(size_t i=1;i<n-1;++i)
				{
					total_height+=items[i].bottom_kern-items[i].top_kern;
				}
				total_height+=items[n-1].bottom_raw-items[n-1].top_kern;
			}
			unsigned int minned=total_height+(n+1)*min_padding.val;
			if(minned>optimal_height.val)
			{
				return page_layout{min_padding.val,minned};
			}
			else
			{
				return page_layout{scast<uint>((optimal_height.val-total_height)/(n+1)),optimal_height.val};
			}
		};
		auto cost=[=](page_layout const p)
		{
			float numer;
			if(p.height>optimal_height.val)
			{
				numer=excess_weight*(p.height-optimal_height.val);
			}
			else
			{
				numer=optimal_height.val-p.height;
			}
			float height_cost=numer/optimal_height.val;
			height_cost=height_cost*height_cost*height_cost;
			float padding_cost=padding_weight*abs_dif(float(p.padding),optimal_padding.val)/optimal_padding.val;
			padding_cost=padding_cost*padding_cost*padding_cost;
			return height_cost+padding_cost;
		};
		struct node {
			float cost;
			page_layout layout;
			size_t previous;
		};
		vector<node> nodes(c+1);
		nodes[0].cost=0;
		for(size_t i=1;i<=c;++i)
		{
			nodes[i].cost=INFINITY;
			for(size_t j=i-1;;) //make this a binary search? no, mininum is almost always a few pages down
			{
				auto layout=create_layout(pages.data()+j,i-j);
				auto local_cost=cost(layout);
				auto total_cost=local_cost+nodes[j].cost;
				if(total_cost<nodes[i].cost)
				{
					nodes[i].cost=total_cost;
					nodes[i].previous=j;
					nodes[i].layout=layout;
				}
				else
				{
					break;
				}
				if(j==0)
				{
					break;
				}
				--j;
			}
		}
		vector<size_t> breakpoints;
		breakpoints.reserve(c);
		size_t index=c;
		do
		{
			breakpoints.push_back(index);
			index=nodes[index].previous;
		} while(index);
		breakpoints.push_back(0);
		unsigned int num_digs=exlib::num_digits(breakpoints.size()-1+starting_index);
		num_digs=num_digs<3?3:num_digs;
		size_t num_imgs=0;
		for(size_t i=breakpoints.size()-1;i>0;--i)
		{
			auto const start=breakpoints[i];
			auto const end=breakpoints[i-1];
			auto const s=end-start;
			vector<page> imgs(s);
			assert(s>0);
			imgs[0].assign(filenames[start].c_str());
			imgs[0].top=pages[start].top_raw;
			if(s==1)
			{
				imgs[0].bottom=pages[start].bottom_raw;
			}
			else
			{
				imgs[0].bottom=pages[start].bottom_kern;
				for(size_t j=1;j<s-1;++j)
				{
					imgs[j].assign(filenames[start+j].c_str());
					imgs[j].top=pages[start+j].top_kern;
					imgs[j].bottom=pages[start+j].bottom_kern;
				}
				imgs[s-1].assign(filenames[start+s-1].c_str());
				imgs[s-1].top=pages[start+s-1].top_kern;
				imgs[s-1].bottom=pages[start+s-1].bottom_raw;
			}
			splice_images(imgs.data(),s,nodes[end].layout.padding).save(output,num_imgs+starting_index,num_digs);
			++num_imgs;
		}
		return num_imgs;
	}

	unsigned int splice_pages_nongreedy_parallel(
		::std::vector<::std::string> const& filenames,
		ImageUtils::perc_or_val horiz_padding,
		ImageUtils::perc_or_val optimal_height,
		ImageUtils::perc_or_val optimal_padding,
		ImageUtils::perc_or_val min_padding,
		char const* output,
		float excess_weight,
		float padding_weight,
		unsigned int starting_index,
		unsigned int num_threads)
	{
		struct item {
			unsigned int top_raw;
			unsigned int top_kern;
			unsigned int bottom_kern;
			unsigned int bottom_raw;
		};
		auto const c=filenames.size();
		if(c<2)
		{
			throw std::invalid_argument("Need multiple pages to splice");
		}
		struct manager {
			CImg<unsigned char> image;
			char const* filename;
			unsigned int times_used=0;
			std::mutex guard;
			void load()
			{
				std::lock_guard<std::mutex> locker(guard);
				if(image._data==0)
				{
					image.load(filename);
					if(image._spectrum>=3)
					{
						image=get_grayscale_simple(image);
					}
				}
			}
			void finish()
			{
				std::lock_guard<std::mutex> locker(guard);
				++times_used;
				if(times_used==2)
				{
					delete[] image._data;
					image._data=0;
				}
			}
		};
		vector<item> pages(c);
		vector<manager> images(c);
		for(size_t i=0;i<c;++i)
		{
			images[i].filename=filenames[i].c_str();
		}
		images[0].image.load(filenames[0].c_str());
		auto fix_perc=[basis=images[0].image._width](ImageUtils::perc_or_val& pv)
		{
			if(pv.is_perc)
			{
				pv.val=unsigned int(std::round(basis*pv.perc/100.0f));
			}
		};
		fix_perc(optimal_height);
		fix_perc(optimal_padding);
		fix_perc(min_padding);
		fix_perc(horiz_padding);

		exlib::ThreadPool pool(num_threads);
		class PageTask:public exlib::ThreadTask {
			manager* work; //will find between *(work-1) and *work
			item* output; //will write to *(output-1) and *output
			unsigned int horiz_padding;
		public:
			PageTask(manager* work,item* output,unsigned int hp):work(work),output(output),horiz_padding(hp)
			{}
			void execute() override
			{
				(work-1)->load();
				work->load();
				auto const& top=(work-1)->image;
				auto const& bottom=(work)->image;
				auto bot=build_bottom_profile(top,255);
				bot=fattened_profile_low(bot,horiz_padding);
				auto tob=build_top_profile(bottom,255);
				tob=fattened_profile_high(tob,horiz_padding);
				auto const sp=find_spacing(bot,top._height,tob);
				(output-1)->bottom_raw=*std::max_element(bot.cbegin(),bot.cend());
				(output-1)->bottom_kern=sp.bottom_sg;
				output->top_kern=sp.top_sg;
				output->top_raw=*std::min_element(tob.cbegin(),tob.cend());
				(work-1)->finish();
				work->finish();
			}
		};

		class FirstTask:public exlib::ThreadTask {
			manager* work;
			item* output;
		public:
			FirstTask(manager* work,item* output):work(work),output(output)
			{}
			void execute() override
			{
				work->load();
				output->top_raw=output->top_kern=find_top(work->image,255,work->image._width/1024+1);
				work->finish();
			}
		};

		class LastTask:public exlib::ThreadTask {
			manager* work;
			item* output;
		public:
			LastTask(manager* work,item* output):work(work),output(output)
			{}
			void execute() override
			{
				work->load();
				output->bottom_raw=output->bottom_kern=find_bottom(work->image,255,work->image._width/1024+1);
				work->finish();
			}
		};

		pool.add_task<FirstTask>(images.data(),pages.data());
		for(size_t i=1;i<c;++i)
		{
			pool.add_task<PageTask>(images.data()+i,pages.data()+i,horiz_padding.val);
		}
		pool.add_task<LastTask>(images.data()+c-1,pages.data()+c-1);
		pool.start();
		pool.wait();

		struct page_layout {
			unsigned int padding;
			unsigned int height;
		};
		auto create_layout=[=](item const* const items,size_t const n)
		{
			assert(n!=0);
			unsigned int total_height;
			if(n==1)
			{
				total_height=items[0].bottom_raw-items[0].top_raw;
			}
			else
			{
				total_height=items[0].bottom_kern-items[0].top_raw;
				for(size_t i=1;i<n-1;++i)
				{
					total_height+=items[i].bottom_kern-items[i].top_kern;
				}
				total_height+=items[n-1].bottom_raw-items[n-1].top_kern;
			}
			unsigned int minned=total_height+(n+1)*min_padding.val;
			if(minned>optimal_height.val)
			{
				return page_layout{min_padding.val,minned};
			}
			else
			{
				return page_layout{scast<uint>((optimal_height.val-total_height)/(n+1)),optimal_height.val};
			}
		};
		auto cost=[=](page_layout const p)
		{
			float numer;
			if(p.height>optimal_height.val)
			{
				numer=excess_weight*(p.height-optimal_height.val);
			}
			else
			{
				numer=optimal_height.val-p.height;
			}
			float height_cost=numer/optimal_height.val;
			height_cost=height_cost*height_cost*height_cost;
			float padding_cost=padding_weight*abs_dif(float(p.padding),optimal_padding.val)/optimal_padding.val;
			padding_cost=padding_cost*padding_cost*padding_cost;
			return height_cost+padding_cost;
		};
		struct node {
			float cost;
			page_layout layout;
			size_t previous;
		};
		vector<node> nodes(c+1);
		nodes[0].cost=0;
		for(size_t i=1;i<=c;++i)
		{
			nodes[i].cost=INFINITY;
			for(size_t j=i-1;;) //make this a binary search? no, mininum is almost always a few pages down
			{
				auto layout=create_layout(pages.data()+j,i-j);
				auto local_cost=cost(layout);
				auto total_cost=local_cost+nodes[j].cost;
				if(total_cost<nodes[i].cost)
				{
					nodes[i].cost=total_cost;
					nodes[i].previous=j;
					nodes[i].layout=layout;
				}
				else
				{
					break;
				}
				if(j==0)
				{
					break;
				}
				--j;
			}
		}
		vector<size_t> breakpoints;
		breakpoints.reserve(c);
		size_t index=c;
		do
		{
			breakpoints.push_back(index);
			index=nodes[index].previous;
		} while(index);
		breakpoints.push_back(0);

		class SpliceTask:public exlib::ThreadTask {
			unsigned int num;
			unsigned int num_digs;
			char const* output;
			std::string const* fbegin;
			item const* ibegin;
			size_t num_pages;
			unsigned int padding;
		public:
			SpliceTask(
				unsigned int n,
				unsigned int nd,
				char const* out,
				std::string const* b,
				item const* ib,
				size_t np,
				unsigned int padding):
				num(n),num_digs(nd),output(out),fbegin(b),ibegin(ib),num_pages(np),padding(padding)
			{}
			void execute() override
			{
				vector<page> imgs(num_pages);
				imgs[0].load(fbegin->c_str());
				imgs[0].top=ibegin->top_raw;
				for(size_t i=0;i<num_pages;++i)
				{
					imgs[i].load(fbegin[i].c_str());
					imgs[i].top=ibegin[i].top_kern;
					imgs[i].bottom=ibegin[i].bottom_kern;
				}
				imgs[0].top=ibegin[0].top_raw;
				imgs[num_pages-1].bottom=ibegin[num_pages-1].bottom_raw;
				splice_images(imgs.data(),num_pages,padding).save(output,num,num_digs);
			}
		};

		unsigned int num_digs=exlib::num_digits(breakpoints.size()-1+starting_index);
		num_digs=num_digs<3?3:num_digs;
		unsigned int num_imgs=0;
		for(size_t i=breakpoints.size()-1;i>0;--i)
		{
			++num_imgs;
			auto const start=breakpoints[i];
			auto const end=breakpoints[i-1];
			auto const s=end-start;
			pool.add_task<SpliceTask>(
				num_imgs,num_digs,
				output,filenames.data()+start,
				pages.data()+start,s,
				nodes[end].layout.padding);
		}
		pool.start();
		pool.wait();
		return num_imgs;
	}
	void add_horizontal_energy(cimg_library::CImg<unsigned char> const& ref,cimg_library::CImg<float>& map,float const hec);
	std::vector<unsigned int> fattened_profile_high(std::vector<unsigned int> const&,unsigned int horiz_padding);
	std::vector<unsigned int> fattened_profile_low(std::vector<unsigned int> const&,unsigned int horiz_padding);
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
	CImg<float> create_vertical_energy(CImg<unsigned char> const& ref,float const vec)
	{
		CImg<float> map(ref._width,ref._height);
		map.fill(INFINITY);
		for(unsigned int x=0;x<map._width;++x)
		{
			unsigned int y=0U;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&]()
			{
				return node_found=ref(x,y)>220;
			};
			auto place_values=[&]()
			{
				unsigned int mid=(node_start+y)/2;
				float val_div=2.0f;
				unsigned int node_y;
				for(node_y=node_start;node_y<mid;++node_y)
				{
					++val_div;
					map(x,node_y)=vec/(val_div*val_div*val_div);
				}
				for(;node_y<y;++node_y)
				{
					--val_div;
					map(x,node_y)=vec/(val_div*val_div*val_div);
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
						place_values();
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
				place_values();
			}
		}
		return map;
	}
	float const COMPRESS_HORIZONTAL_ENERGY_CONSTANT=1.0f;
	CImg<float> create_compress_energy(CImg<unsigned char> const& ref)
	{
		auto map=create_vertical_energy(ref,100.0f);
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
