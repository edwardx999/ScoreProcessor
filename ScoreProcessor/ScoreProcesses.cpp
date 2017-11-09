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
using namespace std;
using namespace ImageUtils;
using namespace cimg_library;
using namespace misc_alg;
namespace ScoreProcessor {
	void binarize(CImg<unsigned char>& image,ColorRGB const middleColor,ColorRGB const lowColor,ColorRGB const highColor) {
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
	void binarize(CImg<unsigned char>& image,Grayscale const middleGray,Grayscale const lowGray,Grayscale const highGray) {
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
	void binarize(CImg<unsigned char>& image,ImageUtils::Grayscale const middleGray,float scale) {
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
	void replace_range(CImg<unsigned char>& image,ImageUtils::Grayscale const lower,ImageUtils::Grayscale const upper,ImageUtils::Grayscale replacer) {
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
	void replace_by_brightness(CImg<unsigned char>& image,unsigned char lowerBrightness,unsigned char upperBrightness,ImageUtils::ColorRGB replacer) {
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
	void replace_by_chroma(CImg<unsigned char>& image,unsigned char lowerChroma,unsigned char upperChroma,ColorRGB replacer) {
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
	int auto_center_horiz(CImg<unsigned char>& image) {
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
		/*thread leftThread(findLeft,image,&left,WHITERGB,tolerance);
		thread rightThread(findRight,image,&right,WHITERGB,tolerance);
		leftThread.join();
		rightThread.join();*/

		//std::cout<<left<<'\n'<<right<<'\n';
		unsigned int center=image._width/2;
		int shift=static_cast<int>(center-((left+right)/2));
		//std::cout<<shift<<'\n';
		if(0==shift) { return 1; }
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0)
		{
//toShift={shift,image._width,0,image._height};
			toFill={0,static_cast<unsigned int>(shift),0,image._height};
		}
		else if(shift<0)
		{
//toShift={0,image._width+shift,0,image._height};
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
	unsigned int find_left(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=0;x<image._width;++x)
		{
			num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
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
	unsigned int find_left(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=0;x<image._width;++x)
		{
			num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return image._width-1;
	}
	unsigned int find_right(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=image._width-1;x<image._width;--x)
		{
			num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
//std::cout<<"Right: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
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
	unsigned int find_right(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=image._width-1;x<image._width;--x)
		{
			num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return 0;
	}

	int auto_center_vert(CImg<unsigned char>& image) {
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
		if(0==shift) { return 1; }
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
	unsigned int find_top(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=0;y<image._height;++y)
		{
			num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
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
	unsigned int find_top(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=0;y<image._height;++y)
		{
			num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return image._height-1;
	}
	unsigned int find_bottom(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=image._height-1;y<image._height;--y)
		{
			num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
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
	unsigned int find_bottom(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=image._height-1;y<image._height;--y)
		{
			num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return 0;
	}
	vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,ColorRGB const background) {
		unsigned int limit=image._width/3;
		vector<unsigned int> container(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x)
			{
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,Grayscale const background) {
		unsigned int limit=image._width/3;
		vector<unsigned int> container(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x)
			{
				if(grayDiff(image(x,y),background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,ColorRGB const background) {
		unsigned int limit=image._width-image._width/2;
		vector<unsigned int> container(image._height);
		container.resize(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;--x)
			{
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,Grayscale const background) {
		unsigned int limit=image._width-image._width/2;
		vector<unsigned int> container(image._height);
		container.resize(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;--x)
			{
				if(grayDiff(image(x,y),background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,ColorRGB const background) {
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height/3;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,Grayscale const background) {
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height/3;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				if(grayDiff(image(x,y),background)>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,ColorRGB const background) {
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height-image._height/3;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;++y)
			{
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,Grayscale const background) {
		vector<unsigned int> container(image._width);
		unsigned int limit=image._height-image._height/3;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;++y)
			{
				if(grayDiff(image(x,y),background)>0.5f)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	//basically a flood fill that can't go left, assumes top row is completely clear
	vector<unique_ptr<RectangleUINT>> select_outside(CImg<unsigned char> const& image) {
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
			for(sright=r.right;sright<image._width&&!safePoints(sright,r.y)&&grayDiff(Grayscale::WHITE,image(sright,r.y))<=tolerance;++sright)
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
						if(!safePoints(xL,newy)&&grayDiff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||grayDiff(Grayscale::WHITE,image(xL,newy))>tolerance)
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
						if(!safePoints(xL,newy)&&grayDiff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<r.left;++xL)
					{
						if(safePoints(xL,newy)||grayDiff(Grayscale::WHITE,image(xL,newy))>tolerance)
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
						if(!safePoints(xL,newy)&&grayDiff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||grayDiff(Grayscale::WHITE,image(xL,newy))>tolerance)
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
	unsigned int cut_image(CImg<unsigned char> const& image,char const* const filename) {
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
			unsigned int right=find_right(image,Grayscale::WHITE,5);
			CImg<float> map=create_vertical_energy(image);
			min_energy_to_right(map);
			//map.display();
			struct range {
				unsigned int top,bottom;
			};
			vector<range> ranges;
			unsigned int const lastRow=right-1;
			bool rangeFound=map(lastRow,0)!=INFINITY;
			if(rangeFound)
			{
				ranges.push_back({0,0});
			}
			for(unsigned int y=1;y<map._height;++y)
			{
				if(rangeFound)
				{
					if(!(rangeFound=map(lastRow,y)!=INFINITY))
					{
						ranges.back().bottom=y;
					}
				}
				else if((rangeFound=map(lastRow,y)!=INFINITY))
				{
					ranges.push_back({y,0});
				}
			}
			if(rangeFound)
			{
				ranges.back().bottom=map._height;
			}
			for(unsigned int ri=1;ri<ranges.size()-1;++ri)
			{
				float minValue=map(map._width-1,ranges[ri].top);
				unsigned int index=0;
				for(unsigned int y=ranges[ri].top+1;y<ranges[ri].bottom;++y)
				{
					if(map(lastRow,y)<minValue)
					{
						minValue=map(lastRow,y);
						index=y;
					}
				}
				paths.push_back(trace_back_seam(map,index));
				paths.back().reserve(image._width);
				unsigned int last=paths.back().back();
				for(unsigned int x=paths.back().size();x<image._width;++x)
				{
					paths.back().push_back(last);
				}
			}
		}
		//cout<<paths.size()<<endl;
		//CImg<unsigned char> temp=image;
		//for(unsigned int pt=0;pt<paths.size();++pt) {
		//	//cout<<"Path "<<pt<<'\n';
		//	for(auto nt=0U;nt<paths[pt].size();++nt) {
		//		//cout<<'\t'<<nt<<','<<paths[pt][nt].y()<<'\n';
		//		temp(nt,paths[pt][nt])=Grayscale::BLACK;
		//	}
		//	/*if(pt==1) {
		//	for(auto nt=1U;nt<paths[pt].size();++nt) {
		//	cout<<EnergyNode::distance(paths[pt][nt],paths[pt][nt-1])<<'\n';
		//	}
		//	}*/
		//}
		//temp.display();
		//temp.save("paths.jpg");
		//return 0;
		//images cut and saved
		if(paths.size()==0)
		{
			image.save(filename,1,3U);
			return 1;
		}
		unsigned int numImages=0;
		unsigned int bottomOfOld=0;
		for(unsigned int pathNum=0;pathNum<paths.size();++pathNum)
		{
			unsigned int lowestInPath=paths[pathNum][0],highestInPath=paths[pathNum][0];
			for(unsigned int x=1;x<paths[pathNum].size();++x)
			{
				unsigned int nodeY=paths[pathNum][x];
				if(nodeY>lowestInPath)
				{
					lowestInPath=nodeY;
				}
				else if(nodeY<highestInPath)
				{
					highestInPath=nodeY;
				}
			}
			unsigned int height;
			height=lowestInPath-bottomOfOld;
			CImg<unsigned char> newImage(image._width,height);
			//newImage.fill(Grayscale::WHITE);
			if(pathNum==0)
			{
				for(unsigned int x=0;x<newImage._width;++x)
				{
					unsigned int y=0;
					//unsigned int yStage1=paths[pathNum-1][x]-bottomOfOld;
					unsigned int yStage2=paths[pathNum][x];
					/*for(y=0;y<yStage1;++y) {
					newImage(x,y)=Grayscale::WHITE;
					}*/
					for(;y<yStage2;++y)
					{
						newImage(x,y)=image(x,y);
					}
					for(;y<newImage._height;++y)
					{
						newImage(x,y)=Grayscale::WHITE;
					}
				}
			}
			else
			{
				for(unsigned int x=0;x<newImage._width;++x)
				{
					unsigned int y=0;
					unsigned int yStage1=paths[pathNum-1][x]-bottomOfOld;
					unsigned int yStage2=paths[pathNum][x]-bottomOfOld;
					for(;y<yStage1;++y)
					{
						newImage(x,y)=Grayscale::WHITE;
					}
					for(;y<yStage2;++y)
					{
						newImage(x,y)=image(x,y+bottomOfOld);
					}
					for(;y<newImage._height;++y)
					{
						newImage(x,y)=Grayscale::WHITE;
					}
				}
			}
			newImage.save(filename,++numImages,3U);
			bottomOfOld=highestInPath;
		}
		CImg<unsigned char> newImage(image._width,image._height-bottomOfOld);
		//newImage.fill(Grayscale::WHITE);
		unsigned int yMax=image._height-bottomOfOld;
		for(unsigned int x=0;x<newImage._width;++x)
		{
/*for(unsigned int y=paths.back()[x]-bottomOfOld;y<yMax;++y) {
	newImage(x,y)=image(x,y+bottomOfOld);
}*/
			unsigned int y=0;
			unsigned int yStage1=paths.back()[x]-bottomOfOld;
			//unsigned int yStage2=paths[pathNum][x]-bottomOfOld;
			for(;y<yStage1;++y)
			{
				newImage(x,y)=Grayscale::WHITE;
			}
			for(;y<newImage._height;++y)
			{
				newImage(x,y)=image(x,y+bottomOfOld);
			}
			/*for(;y<newImage._height;++y) {
				newImage(x,y)=Grayscale::WHITE;
			}*/
		}
		newImage.save(filename,++numImages,3U);
		return numImages;
	}

	void auto_rotate(CImg<unsigned char>& image) {}

	void auto_skew(CImg<unsigned char>& image) {}
	line<unsigned int> find_top_line(CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_bottom_line(CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_left_line(CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_right_line(CImg<unsigned char> const& image) {
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
	void undistort(CImg<unsigned char>& image) {}

	int clear_clusters(CImg<unsigned char>& image,unsigned int const lowerThreshold,unsigned int const upperThreshold,float const tolerance,bool const ignoreWithinTolerance) {
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
		vector<RectangleUINT> ranges;
		if(isRgb)
		{
			ranges=global_select(image,tolerance,ignoreWithinTolerance?ColorRGB::WHITE:ColorRGB::BLACK,ignoreWithinTolerance);
		}
		else
		{
			ranges=global_select(image,tolerance,ignoreWithinTolerance?Grayscale::WHITE:Grayscale::BLACK,ignoreWithinTolerance);
		}
		vector<unique_ptr<Cluster>> clusters=
			Cluster::cluster_ranges(ranges);
		if(isRgb)
		{
			for(unsigned int c=0;c<clusters.size();++c)
			{
			#define currentClusterToDraw (*clusters[c])
				unsigned int size=currentClusterToDraw.size();
				if(size<=upperThreshold&&size>=lowerThreshold)
				{
					for(unsigned int i=0;i<currentClusterToDraw.get_ranges().size();++i)
					{
						fill_selection(image,currentClusterToDraw.get_ranges()[i],ColorRGB::WHITE);
					}
				}
			#undef currentClusterToDraw
			}
		}
		else
		{
			for(unsigned int c=0;c<clusters.size();++c)
			{
			#define currentClusterToDraw (*clusters[c])
				unsigned int size=currentClusterToDraw.size();
				if(size<=upperThreshold&&size>=lowerThreshold)
				{
					for(unsigned int i=0;i<currentClusterToDraw.get_ranges().size();++i)
					{
						fill_selection(image,currentClusterToDraw.get_ranges()[i],Grayscale::WHITE);
					}
				}
			#undef currentClusterToDraw
			}
		}
	}
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
	std::vector<RectangleUINT> flood_select(CImg<unsigned char> const& image,float const tolerance,Grayscale const gray,Point<unsigned int> start) {
		std::vector<RectangleUINT> resultContainer;
		CImg<bool> safePoints(image._width,image._height,1,1);
		safePoints.fill(false);
		unsigned int xL=start.x;
		while(xL<=start.x&&grayDiff(gray,image(xL,start.y))<=tolerance)
		{
			safePoints(xL,start.y)=true;
			--xL;
		}
		++xL;
		unsigned int xR=start.x+1;
		unsigned int maxR=image._width;
		while(xR<maxR&&grayDiff(gray,image[xR,start.y])<=tolerance)
		{
			safePoints(xR,start.y)=true;
			++xR;
		}
		resultContainer.push_back(RectangleUINT{xL,xR,start.y,start.y+1});
		struct scanRange {
			unsigned int left,right,y;
			int direction;
		};
		stack<scanRange> scanRanges;
		scanRanges.push({xL,xR,start.y,+1});
		scanRanges.push({xL,xR,start.y,-1});
		scanRange r;
		unsigned int sleft;
		unsigned int sright;
		while(!scanRanges.empty())
		{
			r=scanRanges.top();
			scanRanges.pop();
			//scan left
			for(sleft=r.left-1;sleft<image._width&&!safePoints(sleft,r.y)&&grayDiff(gray,image(sleft,r.y))<=tolerance;--sleft)
			{
			}
			++sleft;

			//scan right
			for(sright=r.right;sright<image._width&&!safePoints(sright,r.y)&&grayDiff(gray,image(sright,r.y))<=tolerance;++sright)
			{
			}
			resultContainer.push_back(RectangleUINT{sleft,sright,r.y,r.y+1});

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
						if(!safePoints(xL,newy)&&grayDiff(gray,image(xL,newy))<=tolerance)
						{
							rangeFound=true;
							rangeStart=xL++;
							safePoints(xL,newy)=true;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||grayDiff(gray,image(xL,newy))>tolerance)
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
						if(!safePoints(xL,newy)&&grayDiff(gray,image(xL,newy))<=tolerance)
						{
							rangeFound=true;
							rangeStart=xL++;
							safePoints(xL,newy)=true;
							break;
						}
					}
					for(;xL<r.left;++xL)
					{
						if(safePoints(xL,newy)||grayDiff(gray,image(xL,newy))>tolerance)
							break;
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
						if(!safePoints(xL,newy)&&grayDiff(gray,image(xL,newy))<=tolerance)
						{
							rangeFound=true;
							rangeStart=xL++;
							safePoints(xL,newy)=true;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||grayDiff(gray,image(xL,newy))>tolerance)
							break;
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
						if((grayDiff(image(x,y),gray)<=tolerance)^ignoreWithinTolerance)
						{
							rangeFound=1;
							rangeStart=x;
						}
						break;
					}
					case 1: {
						if((grayDiff(image(x,y),gray)>tolerance)^ignoreWithinTolerance)
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
	float const _16by9=16.0f/9.0f;
	int auto_padding(CImg<unsigned char>& image,unsigned int const vertPadding,unsigned int const horizPaddingIfTall,unsigned int const minHorizPadding) {
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
			case 2:
				left=find_left(image,ColorRGB::WHITE,tolerance);
				right=find_right(image,ColorRGB::WHITE,tolerance)+1;
				top=find_top(image,ColorRGB::WHITE,tolerance);
				bottom=find_bottom(image,ColorRGB::WHITE,tolerance)+1;
			default:
				return 2;
		}
	#define prevLeftPadding (left)
		unsigned int prevRightPadding=image._width-right;
	#define prevTopPadding (top)
		unsigned int prevBottomPadding=image._height-bottom;
		float wholeAspectRatio=static_cast<float>(image._width)/static_cast<float>(image._height);
		float actualAspectRatio=static_cast<float>(right-left)/static_cast<float>(bottom-top+2*vertPadding);
		unsigned int horizPaddingToUse=actualAspectRatio<_16by9?horizPaddingIfTall:minHorizPadding;
		image.crop(static_cast<signed int>(left-horizPaddingToUse),static_cast<signed int>(top-vertPadding),right+horizPaddingToUse-1,bottom+vertPadding-1);
		if(image._spectrum==1)
		{
			if(prevLeftPadding<horizPaddingToUse)
			{
				fill_selection(image,{0,horizPaddingToUse-left,0,image._height},Grayscale::WHITE);
			}
			if(prevRightPadding<horizPaddingToUse)
			{
				fill_selection(image,{image._width+prevRightPadding-horizPaddingToUse,image._width,0,image._height},Grayscale::WHITE);
			}
			if(prevTopPadding<vertPadding)
			{
				fill_selection(image,{0,image._width,0,vertPadding-top},Grayscale::WHITE);
			}
			if(prevBottomPadding<vertPadding)
			{
				fill_selection(image,{0,image._width,image._height+prevBottomPadding-vertPadding,image._height},Grayscale::WHITE);
			}
		}
		else if(image._spectrum==3)
		{
			if(prevLeftPadding<vertPadding)
			{
				fill_selection(image,{0,horizPaddingToUse-left,0,image._height},ColorRGB::WHITE);
			}
			if(prevRightPadding<horizPaddingToUse)
			{
				fill_selection(image,{image._width+prevRightPadding-horizPaddingToUse,image._width,0,image._height},ColorRGB::WHITE);
			}
			if(prevTopPadding<vertPadding)
			{
				fill_selection(image,{0,image._width,0,vertPadding-top},ColorRGB::WHITE);
			}
			if(prevBottomPadding<vertPadding)
			{
				fill_selection(image,{0,image._width,image._height+prevBottomPadding-vertPadding,image._height},ColorRGB::WHITE);
			}
		}
		return 0;
	}
	int horiz_padding(CImg<unsigned char>& image,unsigned int const paddingSize) {
		auto const tolerance=5U;
		unsigned int left,right;
		if(image._spectrum==1)
		{
			left=find_left(image,Grayscale::WHITE,tolerance);
			right=find_right(image,Grayscale::WHITE,tolerance)+1;
		}
		else if(image._spectrum==3)
		{
			left=find_left(image,ColorRGB::WHITE,tolerance);
			right=find_right(image,ColorRGB::WHITE,tolerance)+1;
		}
		else
		{
			return 2;
		}
	#define prevLeftPadding (left)
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
	int vert_padding(CImg<unsigned char>& image,unsigned int const paddingSize) {
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
	#define prevTopPadding (top)
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
	unsigned int combine_scores(vector<unique_ptr<string>>& filenames,unsigned int const horizPadding,unsigned int const minVertPadding,unsigned int const maxVertPadding,unsigned int const optimalHeight) {
		struct basicLine {
			uint y;
			uint x;//right to left, where the line ends
		};
		struct vertProfile {
			vector<basicLine> top;
			vector<basicLine> bottom;
		};
		struct boundaryDesc {
			uint top;
			uint bottom;
			uint right;
			sint distNext;
			uint inline height() const {
				return bottom-top;
			}
		};
		vector<boundaryDesc> boundaries;
		boundaries.reserve(filenames.size());
		{
			vector<vertProfile> profiles;
			profiles.reserve(filenames.size());
			//Creating Vertical profiles of each image
			for(unsigned int i=0;i<filenames.size();++i)
			{
				CImg<unsigned char> temp(filenames[i]->c_str());
				vector<unsigned int> topProf,botProf;
				if(temp._spectrum==1)
				{
					topProf=build_top_profile(temp,Grayscale::WHITE);
					botProf=build_bottom_profile(temp,Grayscale::WHITE);
				}
				else if(temp._spectrum==3)
				{
					topProf=build_top_profile(temp,ColorRGB::WHITE);
					botProf=build_bottom_profile(temp,ColorRGB::WHITE);
				}
				else
				{
					return 0;
				}
				//fattening profiles horizontally
				for(unsigned int x=0;x<topProf.size();++x)
				{
					unsigned int topVal=topProf[x];
					unsigned int botVal=botProf[x];
					unsigned int xMax=x+horizPadding+1;
					if(xMax>topProf.size())
					{
						xMax=topProf.size();
					}
					unsigned int xMin=x-horizPadding;
					if(xMin>x)
					{
						xMin=0;
					}
					for(unsigned int xOff=x+1;xOff<xMax;++xOff)
					{
						if(topProf[x]<topProf[xOff])
						{
							topProf[xOff]=topProf[x];
						}
						else
						{
							break;
						}
					}
					for(unsigned int xOff=x+1;xOff<xMax;++xOff)
					{
						if(botProf[x]>botProf[xOff])
						{
							botProf[xOff]=botProf[x];
						}
						else
						{
							break;
						}
					}
					for(unsigned int xOff=x;xOff-->xMin;)
					{
						if(topProf[x]<topProf[xOff])
						{
							topProf[xOff]=topProf[x];
						}
						else
						{
							break;
						}
					}
					for(unsigned int xOff=x;xOff-->xMin;)
					{
						if(botProf[x]>botProf[xOff])
						{
							botProf[xOff]=botProf[x];
						}
						else
						{
							break;
						}
					}
				}
				profiles.emplace_back();
				//compresses profiles into basic lines and finds top and bottom
				unsigned int top=~0,bottom=0;
				for(unsigned int x=topProf.size()-1;x-->0;)
				{
					if(topProf[x]<top)
					{
						top=topProf[x];
					}
					if(topProf[x]!=topProf[x+1])
					{
						profiles.back().top.push_back({topProf[x+1],x});
					}

					if(botProf[x]>bottom)
					{
						bottom=botProf[x];
					}
					if(botProf[x]!=botProf[x+1])
					{
						profiles.back().bottom.push_back({botProf[x+1],x});
					}
				}
				boundaries.push_back(boundaryDesc{top,bottom,(profiles.back().top[0].x+profiles.back().bottom[0].x)/2});
				profiles.back().top.push_back({topProf[0],0});
				profiles.back().bottom.push_back({botProf[0],0});
			}

			//find height and minimum kerning
			//find min combined length
			for(uint i=0;++i<profiles.size();)
			{
				uint bi=profiles[i-1].bottom.size()-1,
					ti=profiles[i].top.size()-1;

				for(;bi>0;--bi)
				{

				}
			}
		}
		unsigned int numImages=0;
		return numImages;
	}
	void compress(CImg<unsigned char>& image,unsigned int const min_padding,unsigned int const optimal_height,float min_energy) {
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
	CImg<float> create_vertical_energy(CImg<unsigned char> const& refImage) {
		CImg<float> map(refImage._width,refImage._height);
		map.fill(INFINITY);
		for(unsigned int x=0;x<map._width;++x)
		{
			unsigned int y=0U;
			unsigned int nodeStart;
			bool nodeFound;
		#define assign_nodeFound() (nodeFound=grayDiff(Grayscale::WHITE,refImage(x,y))<.2f)
			if(assign_nodeFound())
			{
				nodeStart=0;
			}
			for(y=1;y<refImage._height;++y)
			{
				if(nodeFound)
				{
					if(!assign_nodeFound())
					{
					#define placeValues() \
							unsigned int mid=(nodeStart+y)/2;\
							unsigned int valueDivider=2U;\
							unsigned int nodeY;\
							for(nodeY=nodeStart;nodeY<mid;++nodeY) {\
								map(x,nodeY)=VERTICAL_ENERGY_CONSTANT/(++valueDivider);\
							}\
							for(;nodeY<y;++nodeY) {\
								map(x,nodeY)=VERTICAL_ENERGY_CONSTANT/(--valueDivider);\
							}
						placeValues();
					}
				}
				else
				{
					if(assign_nodeFound())
					{
						nodeStart=y;
					}
				}
			}
			if(nodeFound)
			{
				placeValues();
			}
		}
		return map;
	#undef placeValues
	#undef assign_nodeFound
	}
	float const HORIZONTAL_ENERGY_CONSTANT=1.0f;
	CImg<float> create_compress_energy(CImg<unsigned char> const& refImage) {
		auto map=create_vertical_energy(refImage);
		for(auto y=0U;y<map._height;++y)
		{
			auto x=0U;
			unsigned int nodeStart;
			bool nodeFound;
		#define assign_nodeFound() (nodeFound=map(x,y)==INFINITY)
			if(assign_nodeFound())
			{
				nodeStart=0;
			}
			for(x=1;x<map._width;++x)
			{
				if(nodeFound)
				{
					if(!assign_nodeFound())
					{
					#define placeValues() \
							unsigned int multiplier=(x-nodeStart)>>1;\
							unsigned int nodeX;\
							for(nodeX=nodeStart;nodeX<x;++nodeX) {\
								map(nodeX,y)=HORIZONTAL_ENERGY_CONSTANT*multiplier;\
							}
						placeValues();
					}
				}
				else
				{
					if(assign_nodeFound())
					{
						nodeStart=x;
					}
				}
			}
			if(nodeFound)
			{
				placeValues();
			}
		}
		return map;
	#undef placeValues
	#undef assign_noteFound
	}
}
