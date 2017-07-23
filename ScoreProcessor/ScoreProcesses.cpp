#include "stdafx.h"
#include "ScoreProcesses.h"
#include <thread>
#include "ImageUtils.h"
#include "Cluster.h"
#include <iostream>
using namespace std;
namespace ScoreProcessor {
	int copyShiftSelection(cimg_library::CImg<unsigned char>* const image,ImageUtils::Rectangle const* const selection,int const shiftx,unsigned int const shifty) {
		int startX=selection->left+shiftx;if(startX<0) startX=0;
		int endX=selection->right+shiftx;if(endX>image->width()) endX=image->width();--endX;
		int startY=selection->top+shifty;if(startY<0) startY=0;
		int endY=selection->bottom+shifty;if(endY>image->height()) endY=image->height();--endY;
		if(shiftx>0&&shifty>0) {
			for(int x=endX;x>=startX;--x) {
				for(int y=endY;y>=startY;--y) {
					*(image->data(x,y,0,0))=(*image)(x-shiftx,y-shifty,0,0);
					*(image->data(x,y,0,1))=(*image)(x-shiftx,y-shifty,0,1);
					*(image->data(x,y,0,2))=(*image)(x-shiftx,y-shifty,0,2);
				}
			}
		}
		else if(shiftx>0&&shifty<=0) {
			for(int x=endX;x>=startX;--x) {
				for(int y=startY;y<=endY;++y) {
					*(image->data(x,y,0,0))=(*image)(x-shiftx,y-shifty,0,0);
					*(image->data(x,y,0,1))=(*image)(x-shiftx,y-shifty,0,1);
					*(image->data(x,y,0,2))=(*image)(x-shiftx,y-shifty,0,2);
				}
			}
		}
		else if(shiftx<=0&&shifty>0) {
			for(int x=startX;x<=endX;++x) {
				for(int y=endY;y>=startY;--y) {
					*(image->data(x,y,0,0))=(*image)(x-shiftx,y-shifty,0,0);
					*(image->data(x,y,0,1))=(*image)(x-shiftx,y-shifty,0,1);
					*(image->data(x,y,0,2))=(*image)(x-shiftx,y-shifty,0,2);
				}
			}
		}
		else if(shiftx<=0&&shifty<=0) {
			for(int x=startX;x<=endX;++x) {
				for(int y=startY;y<=endY;++y) {
					*(image->data(x,y,0,0))=(*image)(x-shiftx,y-shifty,0,0);
					*(image->data(x,y,0,1))=(*image)(x-shiftx,y-shifty,0,1);
					*(image->data(x,y,0,2))=(*image)(x-shiftx,y-shifty,0,2);
				}
			}
		}
		return 0;
	}
	int fillSelection(cimg_library::CImg<unsigned char>* const image,ImageUtils::Rectangle const* const selection,ImageUtils::ColorRGB const color) {
		int startX=selection->left>=0?selection->left:0;
		int startY=selection->top>=0?selection->top:0;
		int endX=selection->right>image->width()?image->width():selection->right;
		int endY=selection->bottom>image->height()?image->height():selection->bottom;
		for(int x=startX;x<endX;++x) {
			for(int y=startY;y<endY;++y) {
				*(image->data(x,y,0,0))=color.r;
				*(image->data(x,y,0,1))=color.g;
				*(image->data(x,y,0,2))=color.b;
			}
		}
		return 0;
	}
	int autoCenterHoriz(cimg_library::CImg<unsigned char>* const image) {
		int left,right;
		ImageUtils::ColorRGB white{255,255,255};
		unsigned int tolerance=image->height()>>6;
		thread leftThread(findLeft,image,&left,white,tolerance);
		thread rightThread(findRight,image,&right,white,tolerance);
		leftThread.join();
		rightThread.join();
		//std::cout<<left<<'\n'<<right<<'\n';
		int center=image->width()/2;
		int shift=center-(left+right)/2;
		//std::cout<<shift<<'\n';
		if(0==shift) { return 1; }
		ImageUtils::Rectangle toShift,toFill;
		toShift={0,image->width(),0,image->height()};
		if(shift>0) {
			//toShift={shift,image->width(),0,image->height()};
			toFill={0,shift,0,image->height()};
		}
		else if(shift<0) {
			//toShift={0,image->width()+shift,0,image->height()};
			toFill={image->width()+shift,image->width(),0,image->height()};
		}
		copyShiftSelection(image,&toShift,shift,0);
		fillSelection(image,&toFill,white);
		return 0;
	}

	int findLeft(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance) {
		int imageWidth=image->width();
		int imageHeight=image->height();
		unsigned int num;
		for(int x=0;x<imageWidth;++x) {
			num=0;
			for(int y=0;y<imageHeight;++y) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(ImageUtils::RGBColorDiff(*(image->data(x,y,0,0)),
					*(image->data(x,y,0,1)),
					*(image->data(x,y,0,2)),
					background.r,
					background.g,
					background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				*where=x;
				return 0;
			}
		}
		return 1;
	}

	int findRight(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance) {
		int imageWidth=image->width();
		int imageHeight=image->height();
		unsigned int num;
		for(int x=imageWidth-1;x>=0;--x) {
			num=0;
			for(int y=0;y<imageHeight;++y) {
				//std::cout<<"Right: "<<x<<" "<<y<<std::endl;
				if(ImageUtils::RGBColorDiff(*(image->data(x,y,0,0)),*(image->data(x,y,0,1)),*(image->data(x,y,0,2)),background.r,background.g,background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				*where=x;
				return 0;
			}
		}
		return 1;
	}
	int autoCenterVert(cimg_library::CImg<unsigned char>* const image) {
		int top,bottom;
		ImageUtils::ColorRGB white{255,255,255};
		unsigned int tolerance=image->width()>>6;
		thread topThread(findTop,image,&top,white,tolerance);
		thread bottomThread(findBottom,image,&bottom,white,tolerance);
		topThread.join();
		bottomThread.join();
		//std::cout<<top<<'\n'<<bottom<<'\n';
		int center=image->height()/2;
		int shift=center-(top+bottom)/2;
		//std::cout<<shift<<'\n';
		if(0==shift) { return 1; }
		ImageUtils::Rectangle toShift,toFill;
		toShift={0,image->width(),0,image->height()};
		if(shift>0) {
			//toShift={shift,image->width(),0,image->height()};
			toFill={0,image->width(),0,shift};
		}
		else if(shift<0) {
			//toShift={0,image->width()+shift,0,image->height()};
			toFill={0,image->width(),image->height()+shift,image->height()};
		}
		copyShiftSelection(image,&toShift,0,shift);
		fillSelection(image,&toFill,white);
		return 0;
	}
	int findTop(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance) {
		int imageWidth=image->width();
		int imageHeight=image->height();
		unsigned int num;
		for(int y=0;y<imageHeight;++y) {
			num=0;
			for(int x=0;x<imageWidth;++x) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(ImageUtils::RGBColorDiff(*(image->data(x,y,0,0)),
					*(image->data(x,y,0,1)),
					*(image->data(x,y,0,2)),
					background.r,
					background.g,
					background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				*where=y;
				return 0;
			}
		}
		return 1;
	}
	int findBottom(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance) {
		int imageWidth=image->width();
		int imageHeight=image->height();
		unsigned int num;
		for(int y=imageHeight-1;y>=0;--y) {
			num=0;
			for(int x=0;x<imageWidth;++x) {
				//std::cout<<"Left: "<<x<<" "<<y<<std::endl;
				if(ImageUtils::RGBColorDiff(*(image->data(x,y,0,0)),
					*(image->data(x,y,0,1)),
					*(image->data(x,y,0,2)),
					background.r,
					background.g,
					background.b)>.5f)
					++num;
			}
			if(num>tolerance) {
				*where=y;
				return 0;
			}
		}
		return 1;
	}
	int cutImage(cimg_library::CImg<unsigned char>* const image,char const* filename) {
		return 0;
	}
	int autoRotate(cimg_library::CImg<unsigned char>* const image) {
		return 0;
	}
	int autoSkew(cimg_library::CImg<unsigned char>* const image) {
		return 0;
	}
	int undistort(cimg_library::CImg<unsigned char>* const image) {
		return 0;
	}
	int clusterClear(cimg_library::CImg<unsigned char>* const image,unsigned int const tolerance,bool const mode) {
		return 0;
	}
	int globalSelect(cimg_library::CImg<unsigned char>* const image,std::vector<ImageUtils::Rectangle*> container,float const tolerance,ImageUtils::ColorRGB color) {
		return 1;
	}
	int autoPadding(cimg_library::CImg<unsigned char>* const image,int const paddingSize) {
		return 0;
	}
}