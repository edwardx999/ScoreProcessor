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
#include "lib\exstring\exmath.h"
#include <array>
using namespace ImageUtils;
using namespace std;
using namespace misc_alg;
namespace cimg_library {

	void place_vertical_gradient(unsigned char const* const __restrict img,unsigned int const width,unsigned int const height,signed char * const place)
	{
		typedef unsigned char const* const uccpc;
		typedef signed char* const scpc;
		constexpr unsigned char const f=2;
		{
			uccpc row_next=img+width;
			for(uint x=0;x<width;++x)
			{
				place[x]=row_next[x]/f-img[x]/f;
			}
		}
		uint const limit=(height-2)*width;
		unsigned int dif;
		for(dif=0;dif<limit;)
		{
			uccpc row_prev=img+dif;
			uccpc row_next=row_prev+2*width;
			dif+=width;
			scpc p=place+dif;
			for(uint x=0;x<width;++x)
			{
				p[x]=row_next[x]/f-row_prev[x]/f;
			}
		}
		{
			uccpc row_prev=img+dif;
			uccpc row_next=row_prev+width;
			dif+=width;
			scpc p=place+dif;
			for(uint x=0;x<width;++x)
			{
				p[x]=row_next[x]/f-row_prev[x]/f;
			}
		}
	}
	void place_horizontal_gradient(unsigned char const* const __restrict img,unsigned int const width,unsigned int const height,signed char * const place)
	{
		uint const limit=width-1;
		for(uint y=0;y<height;++y)
		{
			unsigned int const off=y*width;
			unsigned char const* const row=img+off;
			signed char* const prow=place+off;
			prow[0]=row[1]/2-img[0]/2;
			for(uint x=1;x<limit;++x)
			{
				prow[x]=row[x+1]/2-row[x-1]/2;
			}
			prow[limit]=row[limit]/2-row[limit-1]/2;
		}
	}

	CImg<signed char> get_vertical_gradient(::cimg_library::CImg<unsigned char> const& img)
	{
		assert(img._height>2);
		CImg<signed char> grad_img(img._width,img._height);
		place_vertical_gradient(img._data,img._width,img._height,grad_img._data);
		return grad_img;
	}

	CImg<signed char> get_horizontal_gradient(::cimg_library::CImg<unsigned char> const& img)
	{
		assert(img._width>1);
		CImg<signed char> grad_img(img._width,img._height);
		place_horizontal_gradient(img._data,img._width,img._height,grad_img._data);
		return grad_img;
	}

	CImg<signed char> get_absolute_gradient(::cimg_library::CImg<unsigned char> const& img)
	{
		CImg<signed char> grad_img(img._width,img._height,1,2);
		place_vertical_gradient(img._data,img._width,img._height,grad_img._data);
		place_horizontal_gradient(img._data,img._width,img._height,grad_img._data+img._height*img._width);
		return grad_img;
	}

	CImg<float> make_sobel_kernel()
	{
		CImg<float> ret(3,3,1,2);
		auto& r=ret._data;
		constexpr float const sw=0.09375f;
		constexpr float const bw=0.3125f;
		static_assert(2*sw+bw==0.5f,"Incorrect weight scale");
		r[0]=-sw;r[1]=-bw;r[2]=-sw;
		r[3]=0;r[4]=0;r[5]=0;
		r[6]=sw;r[7]=bw;r[8]=sw;
		r[9]=-sw;r[10]=0;r[11]=sw;
		r[12]=-bw;r[13]=0;r[14]=bw;
		r[15]=-sw;r[16]=0;r[17]=sw;
		return ret;
	}
	CImg<float> const sobel_kernel=make_sobel_kernel();

	CImg<signed char> get_vertical_gradient_sobel(::cimg_library::CImg<unsigned char> const& img)
	{
		static auto const k=sobel_kernel.get_shared_channel(0);
		return img.get_convolve(k,false,false);
	}

	CImg<signed char> get_horizontal_gradient_sobel(::cimg_library::CImg<unsigned char> const& img)
	{
		static auto const k=sobel_kernel.get_shared_channel(1);
		return img.get_convolve(k,false,false);
	}

	CImg<signed char> get_absolute_gradient_sobel(::cimg_library::CImg<unsigned char> const& img)
	{
		return img.get_convolve(sobel_kernel,false,false);
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
	cimg_library::CImg<unsigned char> get_gradient(::cimg_library::CImg<unsigned char> const& refImage)
	{
		return get_absolute_gradient(refImage);
	}
	ColorRGB average_color(cimg_library::CImg<unsigned char> const& image)
	{
		auto c=fold<3>(image,[](auto color,auto acc)
		{
			return decltype(acc)({acc[0]+color[0],acc[1]+color[1],acc[2]+color[2]});
		},std::array<unsigned int,3>({0,0,0}));
		auto const num_pixels=image._width*image._height;
		return {unsigned char(c[0]/num_pixels),unsigned char(c[1]/num_pixels),unsigned char(c[2]/num_pixels)};
	}
	Grayscale average_gray(cimg_library::CImg<unsigned char> const& image)
	{
		auto gray=fold<1>(image,[](auto color,auto acc)
		{
			return acc+color[0];
		},0U);
		return Grayscale(gray/(image._width*image._height));
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
				ret(x,y)=std::round((image(x,y,0)*0.2126f+image(x,y,1)*0.7152f+image(x,y,2)*0.0722f));
			}
		}
		return ret;
	}
	CImg<unsigned char> get_grayscale_simple(CImg<unsigned char> const& image)
	{
		return get_map<3,1>(image,[](auto color)
		{
			return std::array<unsigned char,1>{unsigned char(std::round((float(color[0])+color[1]+color[2])/3.0f))};
		});
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