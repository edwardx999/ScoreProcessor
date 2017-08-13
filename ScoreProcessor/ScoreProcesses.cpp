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
using namespace std;
using namespace ImageUtils;
using namespace cimg_library;
namespace ScoreProcessor {
	ColorRGB averageColor(cimg_library::CImg<unsigned char>& image) {
		UINT64 r=0,g=0,b=0;
		UINT64 numpixels=image._width*image._height;
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				r+=image(x,y,0);
				g+=image(x,y,1);
				b+=image(x,y,2);
			}
		}
		r/=numpixels;
		g/=numpixels;
		b/=numpixels;
		return{static_cast<unsigned char>(r),static_cast<unsigned char>(g),static_cast<unsigned char>(b)};
	}
	Grayscale averageGray(cimg_library::CImg<unsigned char>& image) {
		UINT64 gray=0;
		UINT64 numpixels=image._width*image._height;
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				gray+=image(x,y,0);
			}
		}
		return gray/numpixels;
	}
	ColorRGB darkestColor(cimg_library::CImg<unsigned char>& image) {
		ColorRGB darkness,currcolo;
		unsigned char darkest=255,currbrig;
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				currcolo={image(x,y,0),image(x,y,1),image(x,y,2)};
				if((currbrig=currcolo.brightness())<=darkest) {
					darkness=currcolo;
					darkest=currbrig;
				}
			}
		}
		return darkness;
	}
	Grayscale darkestGray(cimg_library::CImg<unsigned char>& image) {
		Grayscale darkest=255;
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				if(image(x,y)<darkest) {
					darkest=image(x,y);
				}
			}
		}
		return darkest;
	}
	ColorRGB brightestColor(cimg_library::CImg<unsigned char>& image) {
		ColorRGB light,currcolo;
		unsigned char lightest=0,currbrig;
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				currcolo={image(x,y,0),image(x,y,1),image(x,y,2)};
				if((currbrig=currcolo.brightness())>=lightest) {
					light=currcolo;
					lightest=currbrig;
				}
			}
		}
		return light;
	}
	Grayscale brightestGray(cimg_library::CImg<unsigned char>& image) {
		Grayscale lightest=0;
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				if(image(x,y)>lightest) {
					lightest=image(x,y);
				}
			}
		}
		return lightest;
	}
	CImg<unsigned char> get_grayscale(cimg_library::CImg<unsigned char>& image) {
		CImg<unsigned char> ret(image._width,image._height,1,1);
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				ret(x,y)=(image(x,y,0)*0.2126f+image(x,y,1)*0.7152f+image(x,y,2)*0.0722f);
			}
		}
		return ret;
	}
	void binarize(cimg_library::CImg<unsigned char>& image,ColorRGB const middleColor,ColorRGB const lowColor,ColorRGB const highColor) {
		unsigned char midBrightness=middleColor.brightness();
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				if(brightness({image(x,y,0),image(x,y,1),image(x,y,2)})>midBrightness) {
					image(x,y,0)=highColor.r;
					image(x,y,1)=highColor.g;
					image(x,y,2)=highColor.b;
				}
				else {
					image(x,y,0)=lowColor.r;
					image(x,y,1)=lowColor.g;
					image(x,y,2)=lowColor.b;
				}
			}
		}
	}
	void binarize(cimg_library::CImg<unsigned char>& image,Grayscale const middleGray,Grayscale const lowGray,Grayscale const highGray) {
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				if(image(x,y)>middleGray) {
					image(x,y)=highGray;
				}
				else {
					image(x,y)=lowGray;
				}
			}
		}
	}
	void copyShiftSelection(cimg_library::CImg<unsigned char>& image,RectangleUINT const& selection,int const shiftx,int const shifty) {
		unsigned int startX=selection.left+shiftx;if(startX>image._width) startX=0;
		unsigned int endX=selection.right+shiftx;if(endX>image._width) endX=image._width;--endX;
		unsigned int startY=selection.top+shifty;if(startY>image._height) startY=0;
		unsigned int endY=selection.bottom+shifty;if(endY>image._height) endY=image._height;--endY;
		unsigned int numChannels=image._spectrum;
		if(shiftx>0&&shifty>0) {
			for(unsigned int x=endX;x>=startX;--x) {
				for(unsigned int y=endY;y>=startY;--y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else if(shiftx>0&&shifty<=0) {
			for(unsigned int x=endX;x>=startX;--x) {
				for(unsigned int y=startY;y<=endY;++y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else if(shiftx<=0&&shifty>0) {
			for(unsigned int x=startX;x<=endX;++x) {
				for(unsigned int y=endY;y>=startY;--y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else if(shiftx<=0&&shifty<=0) {
			for(unsigned int x=startX;x<=endX;++x) {
				for(unsigned int y=startY;y<=endY;++y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
	void fillSelection(cimg_library::CImg<unsigned char>& image,RectangleUINT const& selection,ColorRGB const color) {
		unsigned int startX=selection.left;//>=0?selection.left:0;
		unsigned int startY=selection.top;//>=0?selection.top:0;
		unsigned int endX=selection.right>image._width?image._width:selection.right;
		unsigned int endY=selection.bottom>image._height?image._height:selection.bottom;
		for(unsigned int x=startX;x<endX;++x) {
			for(unsigned int y=startY;y<endY;++y) {
				image(x,y,0)=color.r;
				image(x,y,1)=color.g;
				image(x,y,2)=color.b;
			}
		}
	}
	void fillSelection(cimg_library::CImg<unsigned char>& image,RectangleUINT const& selection,Grayscale const gray) {
		unsigned int startX=selection.left;//>=0?selection.left:0;
		unsigned int startY=selection.top;//>=0?selection.top:0;
		unsigned int endX=selection.right>image._width?image._width:selection.right;
		unsigned int endY=selection.bottom>image._height?image._height:selection.bottom;
		for(unsigned int x=startX;x<endX;++x) {
			for(unsigned int y=startY;y<endY;++y) {
				image(x,y)=gray;
			}
		}
	}

	int autoCenterHoriz(cimg_library::CImg<unsigned char>& image) {
		bool isRgb;
		switch(image._spectrum) {
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
		if(isRgb) {
			left=findLeft(image,WHITE_RGB,tolerance);
			right=findRight(image,WHITE_RGB,tolerance);
		}
		else {
			left=findLeft(image,WHITE_GRAYSCALE,tolerance);
			right=findRight(image,WHITE_GRAYSCALE,tolerance);
		}
		/*thread leftThread(findLeft,image,&left,WHITERGB,tolerance);
		thread rightThread(findRight,image,&right,WHITERGB,tolerance);
		leftThread.join();
		rightThread.join();*/

		//std::cout<<left<<'\n'<<right<<'\n';
		unsigned int center=image._width>>1;
		int shift=static_cast<int>(center-((left+right)>>1));
		//std::cout<<shift<<'\n';
		if(0==shift) { return 1; }
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0) {
			//toShift={shift,image._width,0,image._height};
			toFill={0,static_cast<unsigned int>(shift),0,image._height};
		}
		else if(shift<0) {
			//toShift={0,image._width+shift,0,image._height};
			toFill={static_cast<unsigned int>(image._width+shift),image._width,0,image._height};
		}
		copyShiftSelection(image,toShift,shift,0);
		if(isRgb) {
			fillSelection(image,toFill,WHITE_RGB);
		}
		else {
			fillSelection(image,toFill,WHITE_GRAYSCALE);
		}
		return 0;
	}
	unsigned int findLeft(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=0;x<image._width;++x) {
			num=0;
			for(unsigned int y=0;y<image._height;++y) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					++num;
				}
			}
			if(num>tolerance) {
				return x;
			}
		}
		return image._width-1;
	}
	unsigned int findLeft(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=0;x<image._width;++x) {
			num=0;
			for(unsigned int y=0;y<image._height;++y) {
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance) {
				return x;
			}
		}
		return image._width-1;
	}
	unsigned int findRight(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=image._width-1;x<image._width;--x) {
			num=0;
			for(unsigned int y=0;y<image._height;++y) {
				//std::cout<<"Right: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					++num;
				}
			}
			if(num>tolerance) {
				return x;
			}
		}
		return 0;
	}
	unsigned int findRight(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int x=image._width-1;x<image._width;--x) {
			num=0;
			for(unsigned int y=0;y<image._height;++y) {
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance) {
				return x;
			}
		}
		return 0;
	}

	int autoCenterVert(cimg_library::CImg<unsigned char>& image) {
		bool isRgb;
		switch(image._spectrum) {
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
		if(isRgb) {
			top=findTop(image,WHITE_RGB,tolerance);
			bottom=findBottom(image,WHITE_RGB,tolerance);
		}
		else {
			top=findTop(image,WHITE_GRAYSCALE,tolerance);
			bottom=findBottom(image,WHITE_GRAYSCALE,tolerance);
		}
		/*thread topThread(findTop,image,&top,white,tolerance);
		thread bottomThread(findBottom,image,&bottom,white,tolerance);
		topThread.join();
		bottomThread.join();*/
		//std::cout<<top<<'\n'<<bottom<<'\n';
		unsigned int center=image._height>>1;
		int shift=static_cast<int>(center-((top+bottom)>>1));
		//std::cout<<shift<<'\n';
		if(0==shift) { return 1; }
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0) {
			//toShift={shift,image._width,0,image._height};
			toFill={0,image._width,0,static_cast<unsigned int>(shift)};
		}
		else if(shift<0) {
			//toShift={0,image._width+shift,0,image._height};
			toFill={0,image._width,static_cast<unsigned int>(image._height+shift),image._height};
		}
		copyShiftSelection(image,toShift,0,shift);
		if(isRgb) {
			fillSelection(image,toFill,WHITE_RGB);
		}
		else {
			fillSelection(image,toFill,WHITE_GRAYSCALE);
		}
		return 0;
	}
	unsigned int findTop(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=0;y<image._height;++y) {
			num=0;
			for(unsigned int x=0;x<image._width;++x) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					++num;
				}
			}
			if(num>tolerance) {
				return y;
			}
		}
		return image._height-1;
	}
	unsigned int findTop(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=0;y<image._height;++y) {
			num=0;
			for(unsigned int x=0;x<image._width;++x) {
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance) {
				return y;
			}
		}
		return image._height-1;
	}
	unsigned int findBottom(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=image._height-1;y<image._height;--y) {
			num=0;
			for(unsigned int x=0;x<image._width;++x) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					++num;
				}
			}
			if(num>tolerance) {
				return y;
			}
		}
		return 0;
	}
	unsigned int findBottom(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int num;
		for(unsigned int y=image._height-1;y<image._height;--y) {
			num=0;
			for(unsigned int x=0;x<image._width;++x) {
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance) {
				return y;
			}
		}
		return 0;
	}
	void buildLeftProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ColorRGB const background) {
		container.clear();
		unsigned int bottom=image._height;
		unsigned int limit=image._width/3;
		container.resize(bottom);
		for(unsigned int y=0;y<bottom;++y) {
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x) {
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					container[y]=x;
					break;
				}
			}
		}
	}
	void buildLeftProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,Grayscale const background) {
		container.clear();
		unsigned int bottom=image._height;
		unsigned int limit=image._width/3;
		container.resize(bottom);
		for(unsigned int y=0;y<bottom;++y) {
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x) {
				if(grayDiff(image(x,y),background)>0.5f) {
					container[y]=x;
					break;
				}
			}
		}
	}
	void buildRightProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ColorRGB const background) {
		container.clear();
		unsigned int bottom=image._height;
		unsigned int limit=image._width-image._width/3;
		container.resize(bottom);
		for(unsigned int y=0;y<bottom;++y) {
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;++x) {
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					container[y]=x;
					break;
				}
			}
		}
	}
	void buildRightProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,Grayscale const background) {
		container.clear();
		unsigned int bottom=image._height;
		unsigned int limit=image._width-image._width/3;
		container.resize(bottom);
		for(unsigned int y=0;y<bottom;++y) {
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;++x) {
				if(grayDiff(image(x,y),background)>0.5f) {
					container[y]=x;
					break;
				}
			}
		}
	}
	void buildTopProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ColorRGB const background) {
		container.clear();
		unsigned int right=image._width;
		unsigned int limit=image._height/3;
		container.resize(right);
		for(unsigned int x=0;x<right;++x) {
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y) {
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					container[x]=y;
					break;
				}
			}
		}
	}
	void buildTopProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,Grayscale const background) {
		container.clear();
		unsigned int right=image._width;
		unsigned int limit=image._height/3;
		container.resize(right);
		for(unsigned int x=0;x<right;++x) {
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y) {
				if(grayDiff(image(x,y),background)>0.5f) {
					container[x]=y;
					break;
				}
			}
		}
	}
	void buildBottomProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ColorRGB const background) {
		container.clear();
		unsigned int right=image._width;
		unsigned int limit=image._height-image._height/3;
		container.resize(right);
		for(unsigned int x=0;x<right;++x) {
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;++y) {
				if(RGBColorDiff({image(x,y,0),image(x,y,1),image(x,y,2)},background)>0.5f) {
					container[x]=y;
					break;
				}
			}
		}
	}
	void buildBottomProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,Grayscale const background) {
		container.clear();
		unsigned int right=image._width;
		unsigned int limit=image._height-image._height/3;
		container.resize(right);
		for(unsigned int x=0;x<right;++x) {
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;++y) {
				if(grayDiff(image(x,y),background)>0.5f) {
					container[x]=y;
					break;
				}
			}
		}
	}
	//basically a flood fill that can't go left, assumes top row is completely clear
	void selectOutside(CImg<unsigned char>& image,vector<unique_ptr<RectangleUINT>>& resultContainer) {
		CImg<bool> safePoints(image._width,image._height,1,1);
		safePoints.fill(false);
		for(unsigned int x=0;x<image._width;++x) {
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
		while(!scanRanges.empty()) {
			r=scanRanges.top();
			scanRanges.pop();
			sleft=r.left;
			safePoints(sleft,r.y)=true;
			//scan right
			for(sright=r.right;sright<image._width&&!safePoints(sright,r.y)&&grayDiff(WHITE_GRAYSCALE,image(sright,r.y))<=tolerance;++sright) {
				safePoints(sright,r.y)=true;
			}
			resultContainer.push_back(make_unique<RectangleUINT>(RectangleUINT{sleft,sright,r.y,r.y+1U}));

			//scan in same direction vertically
			bool rangeFound=false;
			unsigned int rangeStart=0;
			unsigned int newy=r.y+r.direction;
			if(newy<image._height) {
				xL=sleft;
				while(xL<sright) {
					for(;xL<sright;++xL) {
						if(!safePoints(xL,newy)&&grayDiff(WHITE_GRAYSCALE,image(xL,newy))<=tolerance) {
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL) {
						if(safePoints(xL,newy)||grayDiff(WHITE_GRAYSCALE,image(xL,newy))>tolerance) {
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound) {
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,r.direction});
					}
				}
			}

			//scan opposite direction vertically
			newy=r.y-r.direction;
			if(newy<image._height) {
				xL=sleft;
				while(xL<r.left) {
					for(;xL<r.left;++xL) {
						if(!safePoints(xL,newy)&&grayDiff(WHITE_GRAYSCALE,image(xL,newy))<=tolerance) {
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<r.left;++xL) {
						if(safePoints(xL,newy)||grayDiff(WHITE_GRAYSCALE,image(xL,newy))>tolerance) {
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound) {
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
				xL=r.right+1;
				while(xL<sright) {
					for(;xL<sright;++xL) {
						if(!safePoints(xL,newy)&&grayDiff(WHITE_GRAYSCALE,image(xL,newy))<=tolerance) {
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL) {
						if(safePoints(xL,newy)||grayDiff(WHITE_GRAYSCALE,image(xL,newy))>tolerance) {
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound) {
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
			}
		}
	}
	unsigned int cutImage(cimg_library::CImg<unsigned char>& image,char const* filename) {
		bool isRGB;
		switch(image._spectrum) {
			case 3:
				isRGB=true;
				break;
			case 1:
				isRGB=false;
				break;
			default:
				return 0;
		}
		vector<unique_ptr<RectangleUINT>> outside;
		selectOutside(image,outside);
		sort(outside.begin(),outside.end(),[](unique_ptr<RectangleUINT>&a,unique_ptr<RectangleUINT>&b) {
			if(a->top<b->top) {
				return true;
			}
			else if(a->top==b->top) {
				return a->left<b->left;
			}
			return false;
		});

		//Converting outside selection to system boxes
		{
			unsigned int maxIndex=outside.size();
			unsigned int rowCovered=0;
			unsigned int const threshold=image._width/3;
			int row=outside[maxIndex-1]->top;
			unsigned int rowStart,rowEnd=maxIndex;
			//unsigned int numRects=0;
			for(unsigned int i=maxIndex-1;i<maxIndex;--i) {
				if(row!=outside[i]->top) {
					row=outside[i]->top;
					rowStart=i+1;
					if(rowCovered>threshold/*&&numRects<20*/) {//if it's a full row
						*outside[rowStart]={0,image._width,outside[rowStart]->top,outside[rowStart]->bottom};
						outside.erase(outside.begin()+rowStart+1,outside.begin()+rowEnd);
					}
					else {
						outside.erase(outside.begin()+rowStart+1,outside.begin()+rowEnd-1);
					}
					rowCovered=0;
					//numRects=0;
					rowEnd=rowStart;
				}
				rowCovered+=outside[i]->right-outside[i]->left;
				//++numRects;
			}

			maxIndex=outside.size();
			bool full,oldFull=outside[maxIndex-1]->top!=outside[maxIndex-2]->top;
			rowEnd=maxIndex;
			//turn rows of 2 rects with space, and full row rects into a few boxes
			for(unsigned int i=maxIndex-1;i<maxIndex;--i) {
				full=(outside[i]->left==0)&&(outside[i]->right==image._width);
				if(oldFull!=full) {
					rowStart=i+1;
					if(oldFull) {
						*outside[rowStart]={0,image._width,outside[rowStart]->top,outside[rowEnd-1]->bottom};
						outside.erase(outside.begin()+rowStart+1,outside.begin()+rowEnd);
					}
					else {
						unsigned long average=0;
						unsigned int numRects=(rowEnd-rowStart)>>1;
						for(unsigned int ufi=rowStart;ufi<rowEnd;ufi+=2) {
							average+=outside[ufi]->right;
						}
						average/=numRects;
						*outside[rowStart]={0,average,outside[rowStart]->top,outside[rowEnd-1]->bottom};
						average=0;
						for(unsigned int ufi=rowStart+1;ufi<rowEnd;ufi+=2) {
							average+=outside[ufi]->left;
						}
						average/=numRects;
						*outside[rowStart+1]={average,image._width,outside[rowStart]->top,outside[rowEnd-1]->bottom};
						outside.erase(outside.begin()+rowStart+2,outside.begin()+rowEnd);
					}
					rowEnd=rowStart;
					oldFull=full;
				}
			}
			rowStart=0;
			if(full) {
				*outside[rowStart]={0,image._width,outside[rowStart]->top,outside[rowEnd-1]->bottom};
				outside.erase(outside.begin()+rowStart+1,outside.begin()+rowEnd);
			}
			else {
				unsigned long average=0;
				unsigned int numRects=(rowEnd-rowStart)>>1;
				for(unsigned int ufi=rowStart;ufi<rowEnd;ufi+=2) {
					average+=outside[ufi]->right;
				}
				average/=numRects;
				*outside[rowStart]={0,average,outside[rowStart]->top,outside[rowEnd-1]->bottom};
				average=0;
				for(unsigned int ufi=rowStart+1;ufi<rowEnd;ufi+=2) {
					average+=outside[ufi]->left;
				}
				average/=numRects;
				*outside[rowStart+1]={average,image._width,outside[rowStart]->top,outside[rowEnd-1]->bottom};
				outside.erase(outside.begin()+rowStart+2,outside.begin()+rowEnd);
			}
		}

		//system boxes have been found, now find energy nodes of second to second last full rects, find shortest path in each, and cut there
		/*struct EnergyNode {
			unsigned int top,bottom;
			static float distance(EnergyNode& a,EnergyNode& b) {
				if(a.top>b.bottom||a.bottom<b.top)
					return INFINITY;
				return 100.0f/(b.bottom-b.top);
			}
			unsigned int mid() const {
				return (top+bottom)>>1;
			}
		};*/
		vector<vector<unsigned int>> paths;
		{
			unsigned int start=0;
			for(;start<outside.size();++start) {
				if(outside[start]->left==0&&outside[start]->right==image._width) {
					start+=3;
					break;
				}
			}
			unsigned int end=outside.size()-1;
			for(;end>start;--end) {
				if(outside[end]->left==0&&outside[end]->right==image._width) {
					end-=3;
					break;
				}
			}
			float const tolerance=0.2f;
			for(unsigned int ri=start;ri<=end;ri+=3) {
				CImg<float> map(image._width,outside[ri]->height());
				map.fill(INFINITY);
				unsigned int rectTop=outside[ri]->top;
				for(unsigned int x=0;x<image._width;++x) {
					unsigned int nodeStart;
					bool nodeFound=grayDiff(WHITE_GRAYSCALE,image(x,outside[ri]->top))<tolerance;
					if(nodeFound) {
						nodeStart=outside[ri]->top;
					}
					unsigned int y;
					float value=100000.0f;
					for(y=outside[ri]->top+1;y<outside[ri]->bottom;++y) {
						if(nodeFound) {
							if(!(nodeFound=grayDiff(WHITE_GRAYSCALE,image(x,y))<tolerance)) {
								unsigned int mid=(nodeStart+y)>>1;
								unsigned int valueDivider=1U;
								unsigned int nodeY;
								for(nodeY=nodeStart;nodeY<mid;++nodeY) {
									map(x,nodeY-rectTop)=value/(++valueDivider);
								}
								for(;nodeY<y;++nodeY) {
									map(x,nodeY-rectTop)=value/(--valueDivider);
								}
							}
						}
						else {
							if(nodeFound=grayDiff(WHITE_GRAYSCALE,image(x,y))<tolerance) {
								nodeStart=y;
							}
						}
					}
					if(nodeFound) {
						unsigned int mid=(nodeStart+y)>>1;
						unsigned int valueDivider=1U;
						unsigned int nodeY;
						for(nodeY=nodeStart;nodeY<mid;++nodeY) {
							map(x,nodeY-rectTop)=value/(++valueDivider);
						}
						for(;nodeY<y;++nodeY) {
							map(x,nodeY-rectTop)=value/(--valueDivider);
						}
					}
				}
				paths.emplace_back();
				//map.display();
				createSeam(paths.back(),map);
				//map.display();
				for(auto i=0U;i<paths.back().size();++i) {
					paths.back()[i]+=rectTop;
				}
			}
		}
		//for(unsigned int pt=0;pt<paths.size();++pt) {
		//	//cout<<"Path "<<pt<<'\n';
		//	for(auto nt=0U;nt<paths[pt].size();++nt) {
		//		//cout<<'\t'<<nt<<','<<paths[pt][nt].y()<<'\n';
		//		image(nt,paths[pt][nt])=BLACK_GRAYSCALE;
		//	}
		//	/*if(pt==1) {
		//	for(auto nt=1U;nt<paths[pt].size();++nt) {
		//	cout<<EnergyNode::distance(paths[pt][nt],paths[pt][nt-1])<<'\n';
		//	}
		//	}*/
		//}
		//image.display();
		//return 0;
		//images cut and saved
		if(paths.size()==0) {
			image.save(filename,1,3U);
			return 1;
		}
		unsigned int numImages=0;
		unsigned int bottomOfOld=0;
		for(unsigned int pathNum=0;pathNum<paths.size();++pathNum) {
			unsigned int lowestInPath=0,highestInPath=UINT_MAX;
			for(unsigned int ind=0;ind<paths[pathNum].size();++ind) {
				unsigned int nodeY=paths[pathNum][ind];
				if(nodeY>lowestInPath) {
					lowestInPath=nodeY;
				}
				else if(nodeY<highestInPath) {
					highestInPath=nodeY;
				}
			}
			unsigned int height;
			height=lowestInPath-bottomOfOld;
			CImg<unsigned char> newImage(image._width,height);
			newImage.fill(WHITE_GRAYSCALE);
			if(pathNum==0) {
				for(unsigned int x=0;x<newImage._width;++x) {
					unsigned int yMax=paths[pathNum][x];
					for(unsigned int y=0;y<yMax;++y) {
						newImage(x,y)=image(x,y);
					}
				}
			}
			else {
				for(unsigned int x=0;x<newImage._width;++x) {
					unsigned int yMax=paths[pathNum][x]-bottomOfOld;
					for(unsigned int y=paths[pathNum-1][x]-bottomOfOld;y<yMax;++y) {
						newImage(x,y)=image(x,y+bottomOfOld);
					}
				}
			}
			newImage.save(filename,++numImages,3U);
			bottomOfOld=highestInPath;
		}
		CImg<unsigned char> newImage(image._width,image._height-bottomOfOld);
		newImage.fill(WHITE_GRAYSCALE);
		unsigned int yMax=image._height-bottomOfOld;
		for(unsigned int x=0;x<newImage._width;++x) {
			for(unsigned int y=paths.back()[x]-bottomOfOld;y<yMax;++y) {
				newImage(x,y)=image(x,y+bottomOfOld);
			}
		}
		newImage.save(filename,++numImages,3U);
		return numImages;
	}

	void autoRotate(cimg_library::CImg<unsigned char>& image) {}

	void autoSkew(cimg_library::CImg<unsigned char>& image) {}
	line<unsigned int> findTopLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	line<unsigned int> findBottomLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	line<unsigned int> findLeftLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	line<unsigned int> findRightLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	void undistort(cimg_library::CImg<unsigned char>& image) {}

	int clusterClear(cimg_library::CImg<unsigned char>& image,unsigned int const lowerThreshold,unsigned int const upperThreshold,float const tolerance,bool const ignoreWithinTolerance) {
		bool isRgb;
		switch(image._spectrum) {
			case 3:
				isRgb=true;
				break;
			case 1:
				isRgb=false;
				break;
			default:
				return 2;
		}
		vector<shared_ptr<RectangleUINT>> ranges;
		if(isRgb) {
			globalSelect(image,ranges,tolerance,ignoreWithinTolerance?WHITE_RGB:BLACK_RGB,ignoreWithinTolerance);
		}
		else {
			globalSelect(image,ranges,tolerance,ignoreWithinTolerance?WHITE_GRAYSCALE:BLACK_GRAYSCALE,ignoreWithinTolerance);
		}
		vector<unique_ptr<Cluster>> clusters;
		clusterRanges(clusters,ranges);
		if(isRgb) {
			for(unsigned int c=0;c<clusters.size();++c) {
			#define currentClusterToDraw (*clusters[c])
				unsigned int size=currentClusterToDraw.size();
				if(size<=upperThreshold&&size>=lowerThreshold) {
					for(unsigned int i=0;i<currentClusterToDraw.getRanges().size();++i) {
						fillSelection(image,*currentClusterToDraw.getRanges()[i],WHITE_RGB);
					}
				}
			#undef currentClusterToDraw
			}
		}
		else {
			for(unsigned int c=0;c<clusters.size();++c) {
			#define currentClusterToDraw (*clusters[c])
				unsigned int size=currentClusterToDraw.size();
				if(size<=upperThreshold&&size>=lowerThreshold) {
					for(unsigned int i=0;i<currentClusterToDraw.getRanges().size();++i) {
						fillSelection(image,*currentClusterToDraw.getRanges()[i],WHITE_GRAYSCALE);
					}
				}
			#undef currentClusterToDraw
			}
		}
	}
	void globalSelect(cimg_library::CImg<unsigned char> const& image,vector<shared_ptr<RectangleUINT>>& container,float const tolerance,ColorRGB const color,bool const ignoreWithinTolerance) {
		unsigned int rangeFound=0,rangeStart=0,rangeEnd=0;
		for(unsigned int y=0;y<image._height;++y) {
			for(unsigned int x=0;x<image._width;++x) {
				switch(rangeFound) {
					case 0: {
						if((RGBColorDiff(color,{image(x,y,0),image(x,y,1),image(x,y,2)})<=tolerance)^ignoreWithinTolerance) {
							rangeFound=1;
							rangeStart=x;
						}
						break;
					}
					case 1: {
						if((RGBColorDiff(color,{image(x,y,0),image(x,y,1),image(x,y,2)})>tolerance)^ignoreWithinTolerance) {
							rangeFound=2;
							rangeEnd=x;
						}
						else {
							break;
						}
					}
					case 2: {
						container.push_back(make_shared<RectangleUINT>(RectangleUINT{rangeStart,rangeEnd,y,y+1}));
						rangeFound=0;
						break;
					}
				}
			}
			if(1==rangeFound) {
				container.push_back(make_shared<RectangleUINT>(RectangleUINT{rangeStart,image._width,y,y+1}));
				rangeFound=0;
			}
		}
		compressRectangles(container);
	}
	void floodSelect(cimg_library::CImg<unsigned char> const& image,std::vector<std::shared_ptr<RectangleUINT>>& resultContainer,float const tolerance,Grayscale const gray,vec2_t<unsigned int> start) {
		CImg<bool> safePoints(image._width,image._height,1,1);
		safePoints.fill(false);
		unsigned int xL=start.x;
		while(xL<=start.x&&grayDiff(gray,image(xL,start.y))<=tolerance) {
			safePoints(xL,start.y)=true;
			--xL;
		}
		++xL;
		unsigned int xR=start.x+1;
		unsigned int maxR=image._width;
		while(xR<maxR&&grayDiff(gray,image[xR,start.y])<=tolerance) {
			safePoints(xR,start.y)=true;
			++xR;
		}
		resultContainer.push_back(make_shared<RectangleUINT>(RectangleUINT{xL,xR,start.y,start.y+1}));
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
		while(!scanRanges.empty()) {
			r=scanRanges.top();
			scanRanges.pop();
			//scan left
			for(sleft=r.left-1;sleft<image._width&&!safePoints(sleft,r.y)&&grayDiff(gray,image(sleft,r.y))<=tolerance;--sleft) {
			}
			++sleft;

			//scan right
			for(sright=r.right;sright<image._width&&!safePoints(sright,r.y)&&grayDiff(gray,image(sright,r.y))<=tolerance;++sright) {
			}
			resultContainer.push_back(make_shared<RectangleUINT>(RectangleUINT{sleft,sright,r.y,r.y+1}));

			//scan in same direction vertically
			bool rangeFound=false;
			unsigned int rangeStart=0;
			unsigned int newy=r.y+r.direction;
			if(newy<image._height) {
				xL=sleft;
				while(xL<sright) {
					for(;xL<sright;++xL) {
						if(!safePoints(xL,newy)&&grayDiff(gray,image(xL,newy))<=tolerance) {
							rangeFound=true;
							rangeStart=xL++;
							safePoints(xL,newy)=true;
							break;
						}
					}
					for(;xL<sright;++xL) {
						if(safePoints(xL,newy)||grayDiff(gray,image(xL,newy))>tolerance) {
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound) {
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,r.direction});
					}
				}
			}

			//scan opposite direction vertically
			newy=r.y-r.direction;
			if(newy<image._height) {
				xL=sleft;
				while(xL<r.left) {
					for(;xL<r.left;++xL) {
						if(!safePoints(xL,newy)&&grayDiff(gray,image(xL,newy))<=tolerance) {
							rangeFound=true;
							rangeStart=xL++;
							safePoints(xL,newy)=true;
							break;
						}
					}
					for(;xL<r.left;++xL) {
						if(safePoints(xL,newy)||grayDiff(gray,image(xL,newy))>tolerance)
							break;
						safePoints(xL,newy)=true;
					}
					if(rangeFound) {
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
				xL=r.right+1;
				while(xL<sright) {
					for(;xL<sright;++xL) {
						if(!safePoints(xL,newy)&&grayDiff(gray,image(xL,newy))<=tolerance) {
							rangeFound=true;
							rangeStart=xL++;
							safePoints(xL,newy)=true;
							break;
						}
					}
					for(;xL<sright;++xL) {
						if(safePoints(xL,newy)||grayDiff(gray,image(xL,newy))>tolerance)
							break;
						safePoints(xL,newy)=true;
					}
					if(rangeFound) {
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
			}
		}
	}
	void globalSelect(cimg_library::CImg<unsigned char> const& image,std::vector<std::shared_ptr<RectangleUINT>>& resultContainer,float const tolerance,Grayscale const gray,bool const ignoreWithinTolerance) {
		unsigned int rangeFound=0,rangeStart=0,rangeEnd=0;
		for(unsigned int y=0;y<image._height;++y) {
			for(unsigned int x=0;x<image._width;++x) {
				switch(rangeFound) {
					case 0: {
						if((grayDiff(image(x,y),gray)<=tolerance)^ignoreWithinTolerance) {
							rangeFound=1;
							rangeStart=x;
						}
						break;
					}
					case 1: {
						if((grayDiff(image(x,y),gray)>tolerance)^ignoreWithinTolerance) {
							rangeFound=2;
							rangeEnd=x;
						}
						else {
							break;
						}
					}
					case 2: {
						resultContainer.push_back(make_shared<RectangleUINT>(RectangleUINT{rangeStart,rangeEnd,y,y+1U}));
						rangeFound=0;
						break;
					}
				}
			}
			if(1==rangeFound) {
				resultContainer.push_back(make_shared<RectangleUINT>(RectangleUINT{rangeStart,image._width,y,y+1}));
				rangeFound=0;
			}
		}
		compressRectangles(resultContainer);
	}
	int autoPadding(cimg_library::CImg<unsigned char>& image,int const paddingSize) {
		return 1;
	}
	unsigned int combinescores(std::vector<std::unique_ptr<std::string>>& filenames,unsigned int const padding,unsigned int const optimalHeight) {
		unsigned int numImages=0;
		return numImages;
	}
	void compress(cimg_library::CImg<unsigned char>& image,int const minPadding,unsigned int const optimalHeight) {

	}
}