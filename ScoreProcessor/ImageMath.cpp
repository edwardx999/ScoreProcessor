#include "stdafx.h"
#include "ImageMath.h"
#include "ImageUtils.h"
#include "moreAlgorithms.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include "shorthand.h"
#include <assert.h>
using namespace ImageUtils;
using namespace std;
using namespace misc_alg;
namespace cimg_library {

	::cimg_library::CImg<signed char> get_vertical_gradient(::cimg_library::CImg<unsigned char> const& img)
	{
		assert(img._height>1);
		CImg<signed char> grad_img(img._width,img._height);
		for(uint x=0;x<img._width;++x)
		{
			grad_img(x,0)=scast<schar>((img(x,1)/2-img(x,0)/2));
		}
		uint y;uint limit=img._height-1;
		for(uint y=1;y<limit;++y)
		{
			for(uint x=0;x<img._width;++x)
			{
				grad_img(x,y)=scast<schar>(img(x,y+1)/2-img(x,y-1)/2);
			}
		}
		for(uint x=0;x<img._width;++x)
		{
			grad_img(x,y)=scast<schar>(img(x,y)/2-img(x,y-1)/2);
		}
		return grad_img;
	}

	::cimg_library::CImg<signed char> get_horizontal_gradient(::cimg_library::CImg<unsigned char> const& img)
	{
		assert(img._width>1);
		CImg<signed char> grad_img(img._width,img._height);
		for(uint y=0;y<img._height;++y)
		{
			grad_img(0,y)=scast<schar>((img(1,y)/2-img(0,y)/2));
		}
		uint x;uint limit=img._width-1;
		for(uint x=1;x<limit;++x)
		{
			for(uint y=0;y<img._height;++y)
			{
				grad_img(x,y)=scast<schar>(img(x+1,y)/2-img(x-1,y)/2);
			}
		}
		for(uint y=0;y<img._height;++y)
		{
			grad_img(x,y)=scast<schar>(img(x,y)/2-img(x-1,y)/2);
		}
		return grad_img;
	}
	cimg_library::CImg<signed char> get_absolute_gradient(::cimg_library::CImg<unsigned char> const& img)
	{
		CImg<signed char> grad_img(img._width,img._height,2);
		//vertical
		for(uint x=0;x<img._width;++x)
		{
			grad_img(x,0,1)=scast<schar>((img(x,1)/2-img(x,0)/2));
		}
		uint y;uint limit=img._height-1;
		for(uint y=1;y<limit;++y)
		{
			for(uint x=0;x<img._width;++x)
			{
				grad_img(x,y,1)=scast<schar>(img(x,y+1)/2-img(x,y-1)/2);
			}
		}
		for(uint x=0;x<img._width;++x)
		{
			grad_img(x,y,1)=scast<schar>(img(x,y)/2-img(x,y-1)/2);
		}

		//horizontal
		for(uint y=0;y<img._height;++y)
		{
			grad_img(0,y)=scast<schar>((img(1,y)/2-img(0,y)/2));
		}
		uint x;limit=img._width-1;
		for(uint x=1;x<limit;++x)
		{
			for(uint y=0;y<img._height;++y)
			{
				grad_img(x,y)=scast<schar>(img(x+1,y)/2-img(x-1,y)/2);
			}
		}
		for(uint y=0;y<img._height;++y)
		{
			grad_img(x,y)=scast<schar>(img(x,y)/2-img(x-1,y)/2);
		}
	}
	cimg_library::CImg<unsigned char> get_brightness_spectrum(cimg_library::CImg<unsigned char> const& refImage)
	{
		CImg<unsigned char> image(refImage._width,refImage._height);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				image(x,y)=brightness({refImage(x,y,0),refImage(x,y,1),refImage(x,y,2)});
			}
		}
		return image;
	}
	/*CImg<unsigned char> vertGradKernelInit() {
	CImg<unsigned char> vgk(3,3);
	vgk(0,0)=+1;	vgk(1,0)=+2;	vgk(2,0)=+1;
	vgk(0,1)=0;		vgk(1,1)=0;		vgk(2,1)=0;
	vgk(0,0)=-1;	vgk(1,0)=-2;	vgk(2,0)=-1;
	return vgk;
	}
	CImg<unsigned char> const vertGradKernel=vertGradKernelInit();
	CImg<unsigned char> horizGradKernelInit() {
	CImg<unsigned char> vgk(3,3);
	vgk(0,0)=+1;	vgk(1,0)=0;		vgk(2,0)=-1;
	vgk(0,1)=+2;	vgk(1,1)=0;		vgk(2,1)=-2;
	vgk(0,0)=+1;	vgk(1,0)=0;		vgk(2,0)=-1;
	return vgk;
	}
	CImg<unsigned char> const horizGradKernel=horizGradKernelInit();*/
	cimg_library::CImg<unsigned char> get_gradient(::cimg_library::CImg<unsigned char> const& refImage)
	{
		return get_absolute_gradient(refImage);
	}
	ColorRGB averageColor(cimg_library::CImg<unsigned char> const& image)
	{
		UINT64 r=0,g=0,b=0;
		UINT64 numpixels=image._width*image._height;
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
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
	Grayscale averageGray(cimg_library::CImg<unsigned char> const& image)
	{
		UINT64 gray=0;
		UINT64 numpixels=image._width*image._height;
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				gray+=image(x,y,0);
			}
		}
		return gray/numpixels;
	}
	ColorRGB darkestColor(cimg_library::CImg<unsigned char> const& image)
	{
		ColorRGB darkness,currcolo;
		unsigned char darkest=255,currbrig;
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				currcolo={image(x,y,0),image(x,y,1),image(x,y,2)};
				if((currbrig=currcolo.brightness())<=darkest)
				{
					darkness=currcolo;
					darkest=currbrig;
				}
			}
		}
		return darkness;
	}
	Grayscale darkestGray(cimg_library::CImg<unsigned char> const& image)
	{
		Grayscale darkest=255;
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(image(x,y)<darkest)
				{
					darkest=image(x,y);
				}
			}
		}
		return darkest;
	}
	ColorRGB brightestColor(cimg_library::CImg<unsigned char> const& image)
	{
		ColorRGB light,currcolo;
		unsigned char lightest=0,currbrig;
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				currcolo={image(x,y,0),image(x,y,1),image(x,y,2)};
				if((currbrig=currcolo.brightness())>=lightest)
				{
					light=currcolo;
					lightest=currbrig;
				}
			}
		}
		return light;
	}
	Grayscale brightestGray(cimg_library::CImg<unsigned char> const& image)
	{
		Grayscale lightest=0;
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(image(x,y)>lightest)
				{
					lightest=image(x,y);
				}
			}
		}
		return lightest;
	}
	CImg<unsigned char> get_grayscale(cimg_library::CImg<unsigned char> const& image)
	{
		CImg<unsigned char> ret(image._width,image._height,1,1);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				ret(x,y)=(image(x,y,0)*0.2126f+image(x,y,1)*0.7152f+image(x,y,2)*0.0722f);
			}
		}
		return ret;
	}
	CImg<unsigned char> get_grayscale_simple(CImg<unsigned char> const& image)
	{
		CImg<unsigned char> ret(image._width,image._height,1,1);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				ret(x,y)=uint16_t(image(x,y,0)+image(x,y,1)+image(x,y,2))/3;
			}
		}
		return ret;
	}
	::cimg_library::CImg<unsigned int> get_hough(::cimg_library::CImg<unsigned char> const& gradImage,float lowerAngle,float upperAngle,float angleStep,unsigned int precision)
	{
		CImg<unsigned int> hough(static_cast<unsigned int>((upperAngle-lowerAngle)/angleStep)+1,static_cast<unsigned int>(hypot(gradImage._height,gradImage._width))/precision+1);
		hough.fill(0U);
		//cout<<hough._width<<'\t'<<hough._height<<endl;
		for(unsigned int x=0;x<gradImage._width;++x)
		{
			for(unsigned int y=0;y<gradImage._height;++y)
			{
				if(gradImage(x,y)>128)
				{
					for(unsigned int f=0;f<hough._width;++f)
					{
						float theta=f*angleStep+lowerAngle;
						unsigned int r=std::abs(x*std::cos(theta)+y*std::sin(theta))/precision;
						/*if(r<0||r>=hough._height) {
							cout<<f<<'\t'<<r<<'\t'<<x<<'\t'<<y<<endl;
						}
						else*/
						++hough(f,r);
					}
				}
			}
		}
		return hough;
	}
	float findHoughAngle(::cimg_library::CImg<unsigned int>& hough,float lowerAngle,float angleStep,unsigned int avgHowMany)
	{
		vector<PointUINT> topLines;
		topLines.reserve(avgHowMany);
		unsigned int f,r;
		topLines.push_back({0,0});
		for(f=0;f<hough._width;++f)
		{
			for(r=0;r<hough._height;++r)
			{
				if(topLines.size()<avgHowMany)
				{
					if(hough(f,r)>hough(topLines.back().x,topLines.back().y))
					{
						for(unsigned int i=topLines.size()-2;i<topLines.size();--i)
						{
							if(hough(f,r)<hough(topLines[i].x,topLines[i].y))
							{
								topLines.insert(topLines.begin()+i+1,{f,r});
								break;
							}
						}
					}
				}
				else
				{
					++r;
					break;
				}
			}
		}
		for(;r<hough._height;++r)
		{
		#define placeInPosition()\
			if(hough(f,r)>hough(topLines.back().x,topLines.back().y)) {\
				topLines.pop_back();\
				for(unsigned int i=avgHowMany-2;i<avgHowMany;--i) {\
					if(hough(f,r)<hough(topLines[i].x,topLines[i].y)) {\
						topLines.insert(topLines.begin()+i+1,{f,r});\
						break;\
					}\
				}\
			}
			placeInPosition();
		}
		for(;f<hough._width;++f)
		{
			for(;r<hough._height;++r)
			{
				placeInPosition();
			}
		}
	#undef placeInPosition()
		unsigned int numVotes=0;
		float averageAngle=0;
		for(unsigned int i=0;i<topLines.size();++i)
		{
			averageAngle+=(topLines[i].x*angleStep+lowerAngle)*hough(topLines[i].x,topLines[i].y);
			numVotes+=hough(topLines[i].x,topLines[i].y);
		}
		return averageAngle/numVotes+M_PI_2;
	}

	void remove_transparency(::cimg_library::CImg<unsigned char>& img,unsigned char threshold,ImageUtils::ColorRGB replacer)
	{
		for(unsigned int x=0;x<img._width;++x)
		{
			for(unsigned int y=0;y<img._height;++y)
			{
				if(img(x,y,3)<threshold)
				{
					img(x,y,0)=replacer.r;
					img(x,y,1)=replacer.g;
					img(x,y,2)=replacer.b;
				}
			}
		}
	}
}