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
#include "ImageMath.h"
#include "ImageUtils.h"
#include "moreAlgorithms.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include "shorthand.h"
#include <assert.h>
#include <functional>
using namespace ImageUtils;
using namespace std;
using namespace misc_alg;
namespace cimg_library {

	CImg<signed char> get_vertical_gradient(::cimg_library::CImg<unsigned char> const& img)
	{
		assert(img._height>2);
		CImg<signed char> grad_img(img._width,img._height);
		for(uint x=0;x<img._width;++x)
		{
			grad_img(x,0)=scast<schar>((img(x,1)/2-img(x,0)/2));
		}
		uint y;
		uint limit=img._height-1;
		for(y=1;y<limit;++y)
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

	CImg<signed char> get_horizontal_gradient(::cimg_library::CImg<unsigned char> const& img)
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
	CImg<signed char> get_absolute_gradient(::cimg_library::CImg<unsigned char> const& img)
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
	CImg<unsigned char> get_brightness_spectrum(cimg_library::CImg<unsigned char> const& refImage)
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
	ColorRGB average_color(cimg_library::CImg<unsigned char> const& image)
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
	Grayscale average_gray(cimg_library::CImg<unsigned char> const& image)
	{
		unsigned long long gray=0;
		unsigned long long numpixels=image._width*image._height;
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				gray+=image(x,y,0);
			}
		}
		return gray/numpixels;
	}
	ColorRGB darkest_color(cimg_library::CImg<unsigned char> const& image)
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
	Grayscale darkest_gray(cimg_library::CImg<unsigned char> const& image)
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
	ColorRGB brightest_color(cimg_library::CImg<unsigned char> const& image)
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
	Grayscale brightest_gray(cimg_library::CImg<unsigned char> const& image)
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
				ret(x,y)=(uint16_t(image(x,y,0))+uint16_t(image(x,y,1))+uint16_t(image(x,y,2)))/3;
			}
		}
		return ret;
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

	HoughArray::HoughArray(
		CImg<signed char> const& gradient,
		double lower_angle,double upper_angle,
		unsigned int num_steps,
		double precision,
		signed char threshold)
		:
		CImg(num_steps+1,(rmax=hypot(gradient._width,gradient._height))*2/precision+1),
		theta_min(lower_angle),
		angle_dif(upper_angle-lower_angle),
		angle_steps(num_steps),
		precision(precision)
	{
		threshold=std::abs(threshold);
		fill(0);
		for(uint x=0;x<gradient._width;++x)
		{
			for(uint y=0;y<gradient._height;++y)
			{
				if(std::abs(gradient(x,y))>threshold)
				{
					for(uint f=0;f<=angle_steps;++f)
					{
						double theta=angle_dif*f/angle_steps+theta_min;
						double r=x*std::cos(theta)+y*std::sin(theta);
						unsigned int y=((r+rmax)/(2*rmax))/precision*_height;
						++CImg<unsigned int>::operator()(f,y);
						++CImg<unsigned int>::operator()(f,y+1);
					}
				}
			}
		}
	}
	unsigned int& HoughArray::operator()(double theta,double r)
	{
		unsigned int x=(theta-theta_min)/angle_dif*angle_steps;
		unsigned int y=((r+rmax)/(2*rmax))/precision*_height;
		//printf("%u\t%u\n",x,y);
		return CImg<unsigned int>::operator()(x,y);
	}
	double HoughArray::angle() const
	{
		if(empty())
		{
			return 0;
		}
		auto max_it=begin();
		for(auto it=max_it+1;it!=end();++it)
		{
			if(*it>*max_it)
			{
				max_it=it;
			}
		}
		return std::distance(begin(),max_it)%_width*angle_dif/angle_steps+theta_min;
	}
}