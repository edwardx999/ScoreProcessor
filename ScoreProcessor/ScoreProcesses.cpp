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
#include "moreAlgorithms.h"
#include "shorthand.h"
#include <assert.h>
#include "ImageMath.h"
#include "lib/exstring/exmath.h"
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
	void replace_range(CImg<unsigned char>& image,Grayscale const lower,ImageUtils::Grayscale const upper,Grayscale replacer)
	{
		assert(image._spectrum==1);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(image(x,y)>=lower&&image(x,y)<=upper)
				{
					image(x,y)=replacer;
				}
			}
		}
	}
	void replace_by_brightness(CImg<unsigned char>& image,unsigned char lowerBrightness,unsigned char upperBrightness,ColorRGB replacer)
	{
		assert(image._spectrum==3);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				unsigned char pixBr=brightness({image(x,y,0),image(x,y,1),image(x,y,2)});
				if(pixBr>=lowerBrightness&&pixBr<=upperBrightness)
				{
					image(x,y,0)=replacer.r;
					image(x,y,1)=replacer.g;
					image(x,y,2)=replacer.b;
				}
			}
		}
	}
	void replace_by_chroma(CImg<unsigned char>& image,unsigned char lowerChroma,unsigned char upperChroma,ColorRGB replacer)
	{
		assert(image._spectrum==3);
		for(uint x=0;x<image._width;++x)
		{
			for(uint y=0;y<image._height;++y)
			{
				initializer_list<unsigned char> pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				unsigned char chroma=max(pixel)-min(pixel);
				if(lowerChroma<=chroma&&chroma<=upperChroma)
				{
					image(x,y,0)=replacer.r;
					image(x,y,1)=replacer.g;
					image(x,y,2)=replacer.b;
				}
			}
		}
	}
	int auto_center_horiz(CImg<unsigned char>& image)
	{
		bool isRgb;
		switch(image._spectrum)
		{
			case 3:
				isRgb=true;
				break;
			case 1:
				isRgb=false;
				break;
			default:
				return 2;
		}
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
			return 1;
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
		return 0;
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

	int auto_center_vert(CImg<unsigned char>& image)
	{
		bool isRgb;
		switch(image._spectrum)
		{
			case 3:
				isRgb=true;
				break;
			case 1:
				isRgb=false;
				break;
			default:
				return 2;
		}
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
		/*thread topThread(findTop,image,&top,white,tolerance);
		thread bottomThread(findBottom,image,&bottom,white,tolerance);
		topThread.join();
		bottomThread.join();*/
		//std::cout<<top<<'\n'<<bottom<<'\n';
		unsigned int center=image._height/2;
		int shift=static_cast<int>(center-((top+bottom)/2));
		//std::cout<<shift<<'\n';
		if(0==shift)
		{
			return 1;
		}
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0)
		{
//toShift={shift,image._width,0,image._height};
			toFill={0,image._width,0,static_cast<unsigned int>(shift)};
		}
		else if(shift<0)
		{
//toShift={0,image._width+shift,0,image._height};
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
		return 0;
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
		for(unsigned int y=0;y<image._height;++y)
		{
			//num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				if(gray_diff(image(x,y),background)>.5f)
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
		for(unsigned int y=image._height-1;y<image._height;--y)
		{
			//num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				if(gray_diff(image(x,y),background)>.5f)
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
	unsigned int cut_page(CImg<unsigned char> const& image,char const* filename)
	{
		bool isRGB;
		switch(image._spectrum)
		{
			case 1:
				isRGB=false;
				break;
			case 3:
				isRGB=true;
				return 0;
				break;
			default:
				return 0;
		}

		vector<vector<unsigned int>> paths;
		{
			typedef ImageUtils::vertical_line<unsigned int> line;
			unsigned int right=find_right(image,Grayscale::WHITE,5);
			CImg<float> map=create_vertical_energy(image);
			min_energy_to_right(map);
			//map.display();
			struct float_color {
				static float diff(float const* a,float const* b)
				{
					return static_cast<float>(*b!=INFINITY);
				}
			};
			auto const selections=global_select(map,rcast<float*>(0),float_color::diff,0.5,false);
			auto const clusters=Cluster::cluster_ranges(selections);
			vector<line> boxes;
			unsigned int threshold_width=map._width/2,
				threshold_height=map._height/13;
				//(map._height*map._width)/20;

			for(size_t i=0;i<clusters.size();++i)
			{
				//if(clusters[i]->size()>threshold)
				auto box=clusters[i]->bounding_box();
				if(box.width()>threshold_width&&box.height()>threshold_height)
				{
					boxes.push_back(clusters[i]->right_side());
				}
			}
			std::sort(boxes.begin(),boxes.end(),[](line& a,line&b)
			{
				return a.bottom<b.top;
			});
/*for(auto const& line:boxes)
{
	cout<<line.top<<' '<<line.bottom<<'\n';
}*/
			unsigned int last_row=map._width-1;
			size_t limit=boxes.size()-1;
			for(size_t i=0;i<limit;++i)
			{
				line const& current=boxes[i];
				line const& next=boxes[i+1];
				/*unsigned int y_min=current.bottom;
				float min_val=map(last_row,y_min);
				for(unsigned int y=y_min;y<next.top;++y)
				{
					float val=map(last_row,y);
					if(val<min_val)
					{
						min_val=val;
						y_min=y;
					}
				}*/
				paths.emplace_back(trace_back_seam(map,(current.bottom+next.top)/2));
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
			unsigned int lowest_in_path=paths[path_num][0],highest_in_path=paths[path_num][0];
			for(unsigned int x=1;x<paths[path_num].size();++x)
			{
				unsigned int node_y=paths[path_num][x];
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
			//newImage.fill(Grayscale::WHITE);
			if(path_num==0)
			{
				for(unsigned int x=0;x<new_image._width;++x)
				{
					unsigned int y=0;
					//unsigned int yStage1=paths[pathNum-1][x]-bottomOfOld;
					unsigned int y_stage2=paths[path_num][x];
					/*for(y=0;y<yStage1;++y) {
					newImage(x,y)=Grayscale::WHITE;
					}*/
					for(;y<y_stage2;++y)
					{
						new_image(x,y)=image(x,y);
					}
					for(;y<new_image._height;++y)
					{
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
						new_image(x,y)=Grayscale::WHITE;
					}
					for(;y<y_stage2;++y)
					{
						new_image(x,y)=image(x,y+bottom_of_old);
					}
					for(;y<new_image._height;++y)
					{
						new_image(x,y)=Grayscale::WHITE;
					}
				}
			}
			new_image.save(filename,++num_images,3U);
			bottom_of_old=highest_in_path;
		}
		CImg<unsigned char> new_image(image._width,image._height-bottom_of_old);
		//newImage.fill(Grayscale::WHITE);
		unsigned int y_max=image._height-bottom_of_old;
		for(unsigned int x=0;x<new_image._width;++x)
		{
/*for(unsigned int y=paths.back()[x]-bottomOfOld;y<yMax;++y) {
	newImage(x,y)=image(x,y+bottomOfOld);
}*/
			unsigned int y=0;
			unsigned int y_stage1=paths.back()[x]-bottom_of_old;
			//unsigned int yStage2=paths[pathNum][x]-bottomOfOld;
			for(;y<y_stage1;++y)
			{
				new_image(x,y)=Grayscale::WHITE;
			}
			for(;y<new_image._height;++y)
			{
				new_image(x,y)=image(x,y+bottom_of_old);
			}
			/*for(;y<newImage._height;++y) {
				newImage(x,y)=Grayscale::WHITE;
			}*/
		}
		new_image.save(filename,++num_images,3U);
		return num_images;
	}

	void auto_rotate(CImg<unsigned char>& image)
	{}

	void auto_skew(CImg<unsigned char>& image)
	{}
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
	void undistort(CImg<unsigned char>& image)
	{}

/*
vector<RectangleUINT> global_select(CImg<unsigned char> const& image,float const tolerance,ColorRGB const color,bool const ignoreWithinTolerance) {
	vector<RectangleUINT> container;
	unsigned int rangeFound=0,rangeStart=0,rangeEnd=0;
	for(unsigned int y=0;y<image._height;++y)
	{
		for(unsigned int x=0;x<image._width;++x)
		{
			switch(rangeFound)
			{
				case 0: {
					if((RGBColorDiff(color,{image(x,y,0),image(x,y,1),image(x,y,2)})<=tolerance)^ignoreWithinTolerance)
					{
						rangeFound=1;
						rangeStart=x;
					}
					break;
				}
				case 1: {
					if((RGBColorDiff(color,{image(x,y,0),image(x,y,1),image(x,y,2)})>tolerance)^ignoreWithinTolerance)
					{
						rangeFound=2;
						rangeEnd=x;
					}
					else
					{
						break;
					}
				}
				case 2: {
					container.push_back(RectangleUINT{rangeStart,rangeEnd,y,y+1});
					rangeFound=0;
					break;
				}
			}
		}
		if(1==rangeFound)
		{
			container.push_back(RectangleUINT{rangeStart,image._width,y,y+1});
			rangeFound=0;
		}
	}
	ImageUtils::compress_rectangles(container);
	return container;
}
*/
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
	/*
	vector<RectangleUINT> global_select(CImg<unsigned char> const& image,float const tolerance,Grayscale const gray,bool const ignoreWithinTolerance) {
		std::vector<RectangleUINT> resultContainer;
		unsigned int rangeFound=0,rangeStart=0,rangeEnd=0;
		for(unsigned int y=0;y<image._height;++y)
		{
			for(unsigned int x=0;x<image._width;++x)
			{
				switch(rangeFound)
				{
					case 0: {
						if((gray_diff(image(x,y),gray)<=tolerance)^ignoreWithinTolerance)
						{
							rangeFound=1;
							rangeStart=x;
						}
						break;
					}
					case 1: {
						if((gray_diff(image(x,y),gray)>tolerance)^ignoreWithinTolerance)
						{
							rangeFound=2;
							rangeEnd=x;
						}
						else
						{
							break;
						}
					}
					case 2: {
						resultContainer.push_back(RectangleUINT{rangeStart,rangeEnd,y,y+1U});
						rangeFound=0;
						break;
					}
				}
			}
			if(1==rangeFound)
			{
				resultContainer.push_back(RectangleUINT{rangeStart,image._width,y,y+1});
				rangeFound=0;
			}
		}
		ImageUtils::compress_rectangles(resultContainer);
		return resultContainer;
	}
	*/

	float const _16by9=16.0f/9.0f;
	int auto_padding(CImg<unsigned char>& image,unsigned int const vertical_padding,unsigned int const horizontal_padding_max,unsigned int const horizontal_padding_min,signed int horiz_offset,float optimal_ratio)
	{
		auto const tolerance=5U;
		unsigned int left,right,top,bottom;
		switch(image._spectrum)
		{
			case 1:
				left=find_left(image,Grayscale::WHITE,tolerance);
				right=find_right(image,Grayscale::WHITE,tolerance)+1;
				top=find_top(image,Grayscale::WHITE,tolerance);
				bottom=find_bottom(image,Grayscale::WHITE,tolerance)+1;
				break;
			case 3:
			case 4:
				left=find_left(image,ColorRGB::WHITE,tolerance);
				right=find_right(image,ColorRGB::WHITE,tolerance)+1;
				top=find_top(image,ColorRGB::WHITE,tolerance);
				bottom=find_bottom(image,ColorRGB::WHITE,tolerance)+1;
				break;
			default:
				return 2;
		}
		unsigned int optimal_width=optimal_ratio*(bottom-top+2*vertical_padding);
		unsigned int horizontal_padding;
		unsigned int actual_width=right-left;
		if(optimal_width>actual_width)
		{
			horizontal_padding=(optimal_width-actual_width)/2;
			if(horizontal_padding>horizontal_padding_max)
			{
				horizontal_padding=horizontal_padding_max;
			}
			if(horizontal_padding<horizontal_padding_min)
			{
				horizontal_padding=horizontal_padding_min;
			}
		}
		else
		{
			horizontal_padding=horizontal_padding_min;
		}
		image.crop(
			static_cast<signed int>(left-horizontal_padding),
			static_cast<signed int>(top-vertical_padding),
			right+horizontal_padding-1+horiz_offset,
			bottom+vertical_padding-1,
			1);
		return 0;
	}
	int horiz_padding(CImg<unsigned char>& image,unsigned int const paddingSize)
	{
		auto const tolerance=5U;
		unsigned int left,right;
		switch(image._spectrum)
		{
			case 1:
			{
				left=find_left(image,Grayscale::WHITE,tolerance);
				right=find_right(image,Grayscale::WHITE,tolerance)+1;
				break;
			}
			case 3:
			{
				left=find_left(image,ColorRGB::WHITE,tolerance);
				right=find_right(image,ColorRGB::WHITE,tolerance)+1;
				break;
			}
			default:
			{
				return 2;
			}
		}
		auto& prevLeftPadding=left;
		unsigned int prevRightPadding=image._width-right;

		if(prevLeftPadding==paddingSize&&prevRightPadding==paddingSize)
		{
			return 1;
		}
		image.crop(static_cast<signed int>(left-paddingSize),right+paddingSize-1);
		if(image._spectrum==1)
		{
			if(prevLeftPadding<paddingSize)
			{
				fill_selection(image,{0,paddingSize-left,0,image._height},Grayscale::WHITE);
			}
			if(prevRightPadding<paddingSize)
			{
				fill_selection(image,{image._width+prevRightPadding-paddingSize,image._width,0,image._height},Grayscale::WHITE);
			}
		}
		else if(image._spectrum==3)
		{
			if(prevLeftPadding<paddingSize)
			{
				fill_selection(image,{0,paddingSize-left,0,image._height},ColorRGB::WHITE);
			}
			if(prevRightPadding<paddingSize)
			{
				fill_selection(image,{image._width+prevRightPadding-paddingSize,image._width,0,image._height},ColorRGB::WHITE);
			}
		}
		return 0;
	}
	int vert_padding(CImg<unsigned char>& image,unsigned int const paddingSize)
	{
		auto const tolerance=5U;
		unsigned int top,bottom;
		switch(image._spectrum)
		{
			case 1:
				top=find_top(image,Grayscale::WHITE,tolerance);
				bottom=find_bottom(image,Grayscale::WHITE,tolerance)+1;
				break;
			case 3:
				top=find_top(image,ColorRGB::WHITE,tolerance);
				bottom=find_bottom(image,ColorRGB::WHITE,tolerance)+1;
				break;
			default:
				return 2;
		}
		auto& prevTopPadding=top;
		unsigned int prevBottomPadding=image._height-bottom;

		if(prevTopPadding==paddingSize&&prevBottomPadding==paddingSize)
		{
			return 1;
		}
		image.crop(0,static_cast<signed int>(top-paddingSize),image._width-1,bottom+paddingSize-1);
		if(image._spectrum==1)
		{
			if(prevTopPadding<paddingSize)
			{
				fill_selection(image,{0,image._width,0,paddingSize-top},Grayscale::WHITE);
			}
			if(prevBottomPadding<paddingSize)
			{
				fill_selection(image,{0,image._width,image._height+prevBottomPadding-paddingSize,image._height},Grayscale::WHITE);
			}
		}
		else if(image._spectrum==3)
		{
			if(prevTopPadding<paddingSize)
			{
				fill_selection(image,{0,image._width,0,paddingSize-top},ColorRGB::WHITE);
			}
			if(prevBottomPadding<paddingSize)
			{
				fill_selection(image,{0,image._width,image._height+prevBottomPadding-paddingSize,image._height},ColorRGB::WHITE);
			}
		}
		return 0;
	}

	struct page:CImg<unsigned char> {
		uint top;
		uint bottom;
		page(char const* filename):CImg(filename)
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
						//tpixel=current(current._width-x,y)>>1+tpixel>>1;
					}
				}
			}
			ypos+=padding+current.height();
		}
		//tog.display();
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
		char const* output)
	{
		if(filenames.size()<2)
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
		unsigned int touch_tolerance=first_page._width/1000+1;
		switch(first_page._spectrum)
		{
			case 1:
				items[0].top=find_top(first_page,Grayscale::WHITE,touch_tolerance);
				break;
			case 3:
			case 4:
				items[0].top=find_top(first_page,Grayscale::WHITE,touch_tolerance);
				break;
			default:
				throw std::invalid_argument("Invalid number of layers");
		}
		if(optimal_height==-1)
		{
			optimal_height=first_page._width*4/7;
		}
		function<cost_pad(page const*,size_t)> cost_splice=
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
			float height_cost=abs_dif(height,optimal_height);
			height_cost=powf(height_cost,.6);
			float padding_cost=abs_dif(padding,optimal_padding);
			padding_cost=powf(padding_cost,0.6);
			return cost_pad
			{
				2*padding_cost+height_cost,
				padding
			};
		};
		unsigned int num_pages=0;
		//Creating Vertical profiles of each image
		vector<uint> above_bottom_profile,below_top_profile,below_bottom_profile;
		above_bottom_profile=get_bottom_profile(items[0]);
		items[0].bottom=*max_element(above_bottom_profile.begin(),above_bottom_profile.end());
		items.pad_info=cost_splice(items.data(),1);
		uint num_digits=exlib::num_digits(filenames.size());;
		for(auto it=filenames.begin()+1;it!=filenames.end();++it)
		{
			//cout<<filenames[i].c_str()<<'\n';
			//char const* filename=filenames[i].c_str();
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
				++num_pages;
				//cout<<num_pages<<'\n';
				tog.save(output,num_pages,3);
				//cout<<"Finished saving "<<num_pages<<'\n';
				//cout<<(void const*)items.back().img.data()<<'\n';
				items.erase(items.begin(),items.end()-1);
				//cout<<(void const*)items.back().img.data()<<'\n'<<'\n';
				//cout<<"Erased all but last\n";
				items[0].top=*min_element(below_top_profile.begin(),below_top_profile.end());
				//cout<<"Assigned new top\n";
				items.pad_info=cost_splice(items.data(),1);
				//cout<<"Reevaluated cost\n";
			}
			above_bottom_profile=move(below_bottom_profile);
		}
		auto tog=splice_images(items.data(),items.size(),items.pad_info.padding);
		++num_pages;
		//cout<<num_pages<<'\n';
		tog.save(output,num_pages,3);
		//cout<<"Finished saving "<<num_pages<<'\n';
		return num_pages;
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
	vector<unsigned int> fattened_profile_high(vector<unsigned int> const& profile,unsigned int horiz_padding)
	{
	#define fatten_base(func)\
		vector<unsigned int> res(profile.size());\
		auto rt=res.begin();\
		auto pt=profile.begin();\
		for(;rt!=res.end();++rt,++pt)\
		{\
			auto beg=pt-horiz_padding;\
			if(beg<profile.begin())\
			{\
				beg=profile.begin();\
			}\
			auto ed=pt+horiz_padding+1;\
			if(ed>profile.end())\
			{\
				ed=profile.end();\
			}\
			*rt=* ##func## (beg,ed);\
		}\
		return res;
		fatten_base(std::min_element);
	}
	vector<unsigned int> fattened_profile_low(vector<unsigned int> const& profile,unsigned int horiz_padding)
	{
		fatten_base(std::max_element);
	#undef fatten_base
	}
	void combine_images(std::string const& output,std::vector<CImg<unsigned char>> const& pages,unsigned int& num);
	unsigned int splice_pages_greedy(std::string const& output,::std::vector<::std::string> const& filenames,unsigned int optimal_height)
	{
		unsigned int current_height=0,old_height=0;
		unsigned int num_pages=0;
		std::vector<CImg<unsigned char>> fit;
		for(size_t i=0;i<filenames.size();++i)
		{
			CImg<unsigned char> last(filenames[i].c_str());
			if(last._spectrum==3)
			{
				last=get_grayscale(last);
			}
			current_height+=last._height;
			if(abs_dif(current_height,optimal_height)>=abs_dif(optimal_height,old_height))
			{
				combine_images(output,fit,num_pages);
				fit.clear();
				current_height=last._height;
			}
			old_height=current_height;
			fit.emplace_back(std::move(last));
		}
		combine_images(output,fit,num_pages);
		return num_pages;
	}
	void combine_images(std::string const& output,std::vector<CImg<unsigned char>> const& pages,unsigned int& num)
	{
		if(pages.empty())
		{
			return;
		}
		unsigned int height=0;
		unsigned int width=0;
		for(auto const& page:pages)
		{
			if(page._width>width)
			{
				width=page._width;
			}
			height+=page._height;
		}
		CImg<unsigned char> new_image(width,height,1);
		unsigned int y_abs=0;
		for(auto const& page:pages)
		{
			for(unsigned int y=0;y<page._height;++y,++y_abs)
			{
				unsigned int x_off;
				for(x_off=1;x_off<=page._width;++x_off)
				{
					new_image(width-x_off,y_abs)=page(page._width-x_off,y);
				}
				for(;x_off<=width;++x_off)
				{
					new_image(width-x_off,y_abs)=Grayscale::WHITE;
				}
			}
		}
		new_image.save(output.c_str(),++num,3);
	}
	void compress(CImg<unsigned char>& image,unsigned int const min_padding,unsigned int const optimal_height,float min_energy)
	{
		if(image._height<=optimal_height)
		{
			return;
		}
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
	}
	float const VERTICAL_ENERGY_CONSTANT=100.0f;
	CImg<float> create_vertical_energy(CImg<unsigned char> const& ref)
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
				return node_found=gray_diff(Grayscale::WHITE,ref(x,y))<.2f;
			};
			auto place_values=[&]()
			{
				unsigned int mid=(node_start+y)/2;
				float val_div=2.0f;
				unsigned int node_y;
				for(node_y=node_start;node_y<mid;++node_y)
				{
					++val_div;
					map(x,node_y)=VERTICAL_ENERGY_CONSTANT/(val_div*val_div*val_div);
				}
				for(;node_y<y;++node_y)
				{
					--val_div;
					map(x,node_y)=VERTICAL_ENERGY_CONSTANT/(val_div*val_div*val_div);
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
	float const HORIZONTAL_ENERGY_CONSTANT=1.0f;
	CImg<float> create_compress_energy(CImg<unsigned char> const& ref)
	{
		auto map=create_vertical_energy(ref);
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
					map(node_x,y)=HORIZONTAL_ENERGY_CONSTANT*multiplier;
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
	void remove_border(CImg<unsigned char>& image,Grayscale const color,float const tolerance)
	{
		uint right=image._width-1;
		uint bottom=image._height-1;
		for(uint x=0;x<=right;++x)
		{
			flood_fill(image,tolerance,color,Grayscale::WHITE,{x,0});
			flood_fill(image,tolerance,color,Grayscale::WHITE,{x,bottom});
		}
		for(uint y=1;y<bottom;++y)
		{
			flood_fill(image,tolerance,color,Grayscale::WHITE,{0,y});
			flood_fill(image,tolerance,color,Grayscale::WHITE,{right,y});
		}
	}
	void flood_fill(CImg<unsigned char>& image,float const tolerance,Grayscale const color,Grayscale const replacer,Point<unsigned int> point)
	{
		auto rects=flood_select(image,tolerance,color,point);
		for(auto const& rect:rects)
		{
			fill_selection(image,rect,replacer);
		}
	}
	void rescale_colors(CImg<unsigned char>& img,unsigned char min,unsigned char mid,unsigned char max)
	{
		assert(min<mid);
		assert(mid<max);
		double scale_up=scast<double>(255-mid)/scast<double>(max-mid),
			scale_down=scast<double>(mid)/scast<double>(mid-min);
		for(uint x=0;x<img._width;++x)
		{
			for(uint y=0;y<img._height;++y)
			{
				unsigned char& pixel=img(x,y);
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
}
