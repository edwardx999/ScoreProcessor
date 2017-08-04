#include "stdafx.h"
#include "ScoreProcesses.h"
#include <thread>
#include "ImageUtils.h"
#include "Cluster.h"
#include <iostream>
#include <string>
using namespace std;
using namespace ImageUtils;
using namespace cimg_library;
namespace ScoreProcessor {
	ImageUtils::ColorRGB averageColor(cimg_library::CImg<unsigned char>& image) {
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
	ImageUtils::ColorRGB darkestColor(cimg_library::CImg<unsigned char>& image) {
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
	ImageUtils::ColorRGB brightestColor(cimg_library::CImg<unsigned char>& image) {
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
	void binarize(cimg_library::CImg<unsigned char>& image,ImageUtils::ColorRGB const middleColor,ImageUtils::ColorRGB const lowColor,ImageUtils::ColorRGB const highColor) {
		unsigned char midBrightness=middleColor.brightness();
		for(unsigned int x=0;x<image._width;++x) {
			for(unsigned int y=0;y<image._height;++y) {
				if(brightness(image(x,y,0),image(x,y,1),image(x,y,2))>midBrightness) {
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
	void copyShiftSelection(cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle const& selection,int const shiftx,int const shifty) {
		int startX=selection.left+shiftx;if(startX<0) startX=0;
		int endX=selection.right+shiftx;if(endX>image.width()) endX=image.width();--endX;
		int startY=selection.top+shifty;if(startY<0) startY=0;
		int endY=selection.bottom+shifty;if(endY>image.height()) endY=image.height();--endY;
		unsigned int numChannels=image._spectrum;
		if(shiftx>0&&shifty>0) {
			for(int x=endX;x>=startX;--x) {
				for(int y=endY;y>=startY;--y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else if(shiftx>0&&shifty<=0) {
			for(int x=endX;x>=startX;--x) {
				for(int y=startY;y<=endY;++y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else if(shiftx<=0&&shifty>0) {
			for(int x=startX;x<=endX;++x) {
				for(int y=endY;y>=startY;--y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else if(shiftx<=0&&shifty<=0) {
			for(int x=startX;x<=endX;++x) {
				for(int y=startY;y<=endY;++y) {
					for(unsigned int s=0;s<numChannels;++s) {
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
	void fillSelection(cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle const& selection,ColorRGB const color) {
		int startX=selection.left>=0?selection.left:0;
		int startY=selection.top>=0?selection.top:0;
		int endX=selection.right>image.width()?image.width():selection.right;
		int endY=selection.bottom>image._height?image._height:selection.bottom;
		for(int x=startX;x<endX;++x) {
			for(int y=startY;y<endY;++y) {
				image(x,y,0)=color.r;
				image(x,y,1)=color.g;
				image(x,y,2)=color.b;
			}
		}
	}
	void fillSelection(cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle const& selection,Grayscale const gray) {
		int startX=selection.left>=0?selection.left:0;
		int startY=selection.top>=0?selection.top:0;
		int endX=selection.right>image.width()?image.width():selection.right;
		int endY=selection.bottom>image._height?image._height:selection.bottom;
		for(int x=startX;x<endX;++x) {
			for(int y=startY;y<endY;++y) {
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
		int left,right;
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
		int center=image.width()/2;
		int shift=center-(left+right)/2;
		//std::cout<<shift<<'\n';
		if(0==shift) { return 1; }
		ImageUtils::Rectangle toShift,toFill;
		toShift={0,image.width(),0,image.height()};
		if(shift>0) {
			//toShift={shift,image.width(),0,image._height};
			toFill={0,shift,0,image.height()};
		}
		else if(shift<0) {
			//toShift={0,image.width()+shift,0,image._height};
			toFill={image.width()+shift,image.width(),0,image.height()};
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
	int findLeft(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int imageWidth=image._width;
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int x=0;x<imageWidth;++x) {
			num=0;
			for(unsigned int y=0;y<imageHeight;++y) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff(*(image.data(x,y,0)),
					*(image.data(x,y,1)),
					*(image.data(x,y,2)),
					background.r,
					background.g,
					background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				return x;
			}
		}
		return imageWidth-1;
	}
	int findLeft(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int imageWidth=image._width;
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int x=0;x<imageWidth;++x) {
			num=0;
			for(unsigned int y=0;y<imageHeight;++y) {
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance) {
				return x;
			}
		}
		return imageWidth-1;
	}
	int findRight(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int imageWidth=image._width;
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int x=imageWidth-1;x<imageWidth;--x) {
			num=0;
			for(unsigned int y=0;y<imageHeight;++y) {
				//std::cout<<"Right: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff(*(image.data(x,y,0)),*(image.data(x,y,1)),*(image.data(x,y,2)),background.r,background.g,background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				return x;
			}
		}
		return 0;
	}
	int findRight(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int imageWidth=image._width;
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int x=imageWidth-1;x<imageWidth;--x) {
			num=0;
			for(unsigned int y=0;y<imageHeight;++y) {
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
		int center=image._height/2;
		int shift=center-(top+bottom)/2;
		//std::cout<<shift<<'\n';
		if(0==shift) { return 1; }
		ImageUtils::Rectangle toShift,toFill;
		toShift={0,image.width(),0,image.height()};
		if(shift>0) {
			//toShift={shift,image.width(),0,image._height};
			toFill={0,image.width(),0,shift};
		}
		else if(shift<0) {
			//toShift={0,image.width()+shift,0,image._height};
			toFill={0,image.width(),image.height()+shift,image.height()};
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
	int findTop(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int imageWidth=image._width;
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int y=0;y<imageHeight;++y) {
			num=0;
			for(unsigned int x=0;x<imageWidth;++x) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff(*(image.data(x,y,0)),
					*(image.data(x,y,1)),
					*(image.data(x,y,2)),
					background.r,
					background.g,
					background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				return y;
			}
		}
		return imageHeight-1;
	}
	int findTop(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int imageWidth=image._width;
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int y=0;y<imageHeight;++y) {
			num=0;
			for(unsigned int x=0;x<imageWidth;++x) {
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance) {
				return y;
			}
		}
		return imageHeight-1;
	}
	int findBottom(cimg_library::CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance) {
		unsigned int imageWidth=image.width();
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int y=imageHeight-1;y<imageHeight;--y) {
			num=0;
			for(unsigned int x=0;x<imageWidth;++x) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(RGBColorDiff(*(image.data(x,y,0)),
					*(image.data(x,y,1)),
					*(image.data(x,y,2)),
					background.r,
					background.g,
					background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				return y;
			}
		}
		return 0;
	}
	int findBottom(cimg_library::CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance) {
		unsigned int imageWidth=image.width();
		unsigned int imageHeight=image._height;
		unsigned int num;
		for(unsigned int y=imageHeight-1;y<imageHeight;--y) {
			num=0;
			for(unsigned int x=0;x<imageWidth;++x) {
				if(grayDiff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance) {
				return y;
			}
		}
		return 0;
	}
	void buildLeftProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ImageUtils::ColorRGB const background) {
		container.clear();
		unsigned int bottom=image._height;
		unsigned int limit=image._width/3;
		container.resize(bottom);
		for(unsigned int y=0;y<bottom;++y) {
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x) {
				if(RGBColorDiff(image(x,y,0),image(x,y,1),image(x,y,2),background.r,background.g,background.b)>0.5f) {
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
	void buildRightProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ImageUtils::ColorRGB const background) {
		container.clear();
		unsigned int bottom=image._height;
		unsigned int limit=image._width-image._width/3;
		container.resize(bottom);
		for(unsigned int y=0;y<bottom;++y) {
			container[y]=limit;
			for(unsigned int x=image.width()-1;x>=limit;++x) {
				if(RGBColorDiff(image(x,y,0),image(x,y,1),image(x,y,2),background.r,background.g,background.b)>0.5f) {
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
			for(unsigned int x=image.width()-1;x>=limit;++x) {
				if(grayDiff(image(x,y),background)>0.5f) {
					container[y]=x;
					break;
				}
			}
		}
	}
	void buildTopProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ImageUtils::ColorRGB const background) {
		container.clear();
		unsigned int right=image._width;
		unsigned int limit=image._height/3;
		container.resize(right);
		for(unsigned int x=0;x<right;++x) {
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y) {
				if(RGBColorDiff(image(x,y,0),image(x,y,1),image(x,y,2),background.r,background.g,background.b)>0.5f) {
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
	void buildBottomProfile(cimg_library::CImg<unsigned char>& image,std::vector<unsigned int>& container,ImageUtils::ColorRGB const background) {
		container.clear();
		unsigned int right=image._width;
		unsigned int limit=image._height-image._height/3;
		container.resize(right);
		for(unsigned int x=0;x<right;++x) {
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;++y) {
				if(RGBColorDiff(image(x,y,0),image(x,y,1),image(x,y,2),background.r,background.g,background.b)>0.5f) {
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
	unsigned int cutImage(cimg_library::CImg<unsigned char>& image,char const* filename,unsigned int const padding) {
		bool isRGB;
		switch(image._spectrum) {
			case 3:
				isRGB=true;
			case 1:
				isRGB=false;
			default:
				return 0;
		}
		string origname=filename;
		vector<unsigned int> leftProfile;
		vector<unsigned int> rightProfile;
		if(isRGB) {
			buildLeftProfile(image,leftProfile,WHITE_RGB);
			buildRightProfile(image,rightProfile,WHITE_RGB);
		}
		return 1;
	}

	void autoRotate(cimg_library::CImg<unsigned char>& image) {}

	void autoSkew(cimg_library::CImg<unsigned char>& image) {}
	ImageUtils::line findTopLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	ImageUtils::line findBottomLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	ImageUtils::line findLeftLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	ImageUtils::line findRightLine(cimg_library::CImg<unsigned char> const& image) {
		return {{0,0},{0,0}};
	}
	void undistort(cimg_library::CImg<unsigned char>& image) {}

	int clusterClear(cimg_library::CImg<unsigned char>& image,int const lowerThreshold,int const upperThreshold,float const tolerance,bool const ignoreWithinTolerance) {
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
		vector<shared_ptr<ImageUtils::Rectangle>> ranges;
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
				int size=currentClusterToDraw.size();
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
				int size=currentClusterToDraw.size();
				if(size<=upperThreshold&&size>=lowerThreshold) {
					for(unsigned int i=0;i<currentClusterToDraw.getRanges().size();++i) {
						fillSelection(image,*currentClusterToDraw.getRanges()[i],WHITE_GRAYSCALE);
					}
				}
			#undef currentClusterToDraw
			}
		}
	}
	void globalSelect(cimg_library::CImg<unsigned char> const& image,vector<shared_ptr<ImageUtils::Rectangle>>& container,float const tolerance,ColorRGB const color,bool const ignoreWithinTolerance) {
		int rangeFound=0,rangeStart=0,rangeEnd=0;
		for(int y=0;y<image._height;++y) {
			for(int x=0;x<image.width();++x) {
				switch(rangeFound) {
					case 0: {
						if((RGBColorDiff(color.r,color.g,color.b,image(x,y,0),image(x,y,1),image(x,y,2))<=tolerance)^ignoreWithinTolerance) {
							rangeFound=1;
							rangeStart=x;
						}
						break;
					}
					case 1: {
						if((RGBColorDiff(color.r,color.g,color.b,image(x,y,0),image(x,y,1),image(x,y,2))>tolerance)^ignoreWithinTolerance) {
							rangeFound=2;
							rangeEnd=x;
						}
						else {
							break;
						}
					}
					case 2: {
						container.push_back(make_shared<ImageUtils::Rectangle>(ImageUtils::Rectangle{rangeStart,rangeEnd,y,y+1}));
						rangeFound=0;
						break;
					}
				}
			}
			if(1==rangeFound) {
				container.push_back(make_shared<ImageUtils::Rectangle>(ImageUtils::Rectangle{rangeStart,image.width(),y,y+1}));
				rangeFound=0;
			}
		}
		compressRectangles(container);
	}
	void globalSelect(cimg_library::CImg<unsigned char> const& image,std::vector<std::shared_ptr<ImageUtils::Rectangle>>& resultContainer,float const tolerance,Grayscale const gray,bool const ignoreWithinTolerance) {
		int rangeFound=0,rangeStart=0,rangeEnd=0;
		for(int y=0;y<image._height;++y) {
			for(int x=0;x<image.width();++x) {
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
						resultContainer.push_back(make_shared<ImageUtils::Rectangle>(ImageUtils::Rectangle{rangeStart,rangeEnd,y,y+1}));
						rangeFound=0;
						break;
					}
				}
			}
			if(1==rangeFound) {
				resultContainer.push_back(make_shared<ImageUtils::Rectangle>(ImageUtils::Rectangle{rangeStart,image.width(),y,y+1}));
				rangeFound=0;
			}
		}
		compressRectangles(resultContainer);
	}
	int autoPadding(cimg_library::CImg<unsigned char>& image,int const paddingSize) {
		return 1;
	}
}