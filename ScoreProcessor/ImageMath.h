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
#ifndef IMAGE_MATH_H
#define IMAGE_MATH_H
#include "CImg.h"
#include "ImageUtils.h"
#include <assert.h>
#include <type_traits>
#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923
#define M_PI_4	0.78539816339744830962
#define M_PI_6	0.52359877559829887308
#define RAD_DEG 180.0/M_PI
#define DEG_RAD M_PI/180.0
namespace cimg_library {

	template<size_t NumLayers,typename T,typename ArrayToArray>
	CImg<T>& map(CImg<T>& img,ArrayToArray func)
	{
		assert(NumLayers<=img._spectrum);
		auto const data=img._data;
		auto const size=img._width*img._height; //I hope the compiler can realize this is unused if NumLayers==1
		std::array<T,NumLayers> color;
		for(unsigned int i=0;i<size;++i)
		{
			auto const pix=data+i;
			for(unsigned int s=0;s<NumLayers;++s)
			{
				color[s]=*(pix+s*size);
			}
			auto new_color=func(color);
			static_assert(new_color.size()>=NumLayers,"Mapping function outputs too few layers");
			for(unsigned int s=0;s<NumLayers;++s)
			{
				*(pix+s*size)=new_color[s];
			}
		}
		return img;
	}

	template<size_t NumLayers,typename T,typename ArrayToArray,typename ArrayToBool>
	CImg<T>& map_if(CImg<T>& img,ArrayToArray func,ArrayToBool pred)
	{
		assert(NumLayers<=img._spectrum);
		auto const data=img._data;
		auto const size=img._width*img._height; //I hope the compiler can realize this is unused if NumLayers==1
		std::array<T,NumLayers> color;
		for(unsigned int i=0;i<size;++i)
		{
			auto const pix=data+i;
			for(unsigned int s=0;s<NumLayers;++s)
			{
				color[s]=*(pix+s*size);
			}
			if(pred(color))
			{
				auto new_color=func(color);
				static_assert(new_color.size()>=NumLayers,"Mapping function outputs too few layers");
				for(unsigned int s=0;s<NumLayers;++s)
				{
					*(pix+s*size)=new_color[s];
				}
			}
		}
		return img;
	}

	template<unsigned int InputLayers,unsigned int OutputLayers=InputLayers,typename T,typename ArrayToArray>
	auto get_map(CImg<T> const& img,ArrayToArray func)
	{
		std::array<T,InputLayers> input;
		typedef std::remove_reference<decltype(func(input)[0])>::type R;
		CImg<R> ret(img._width,img._height,1,OutputLayers);
		auto const size=img._width*img._height;
		auto const idata=img._data;
		auto const odata=ret._data;
		for(auto i=0U;i<size;++i)
		{
			auto const ipix=idata+i;
			for(auto s=0U;s<InputLayers;++s)
			{
				input[s]=*(ipix+s*size);
			}
			auto output=func(input);
			static_assert(output.size()>=OutputLayers,"Mapping function outputs too few layers");
			auto const opix=odata+i;
			for(auto s=0U;s<OutputLayers;++s)
			{
				*(opix+s*size)=output[s];
			}
		}
		return ret;
	}

	template<unsigned int InputLayers,unsigned int OutputLayers=InputLayers,typename T,typename ArrayToArray>
	auto get_map(CImg<T> const& img,ArrayToArray func,ImageUtils::Rectangle<unsigned int> const selection)
	{
		std::array<T,InputLayers> input;
		auto const owidth=selection.width();
		auto const oheight=selection.height();
		auto const osize=owidth*oheight;
		CImg<std::remove_reference<decltype(func(input)[0])>::type> ret(owidth,oheight,1,OutputLayers);
		auto const isize=img._width*img._height;
		auto const idata=img._data;
		for(auto y=0U;x<oheight;++x)
		{
			auto irow=idata+(y+selection.top)*img._width;
			auto orow=ret._data+y*owidth;
			for(auto x=0U;y<owidth;++y)
			{
				auto ipix=irow+x+selection.left;
				auto opix=orow+x;
				for(auto s=0U;s<InputLayers;++s)
				{
					input[s]=*(ipix+s*isize);
				}
				auto output=func(input);
				static_assert(output.size()>=OutputLayers,"Mapping function outputs too few layers");
				for(auto s=0U;s<OutputLayers;++s)
				{
					*(opix+s*osize)=output[s];
				}
			}
		}
		return ret;
	}

	template<unsigned int NumLayers,typename T,typename ArrayToArray>
	CImg<T>& map(CImg<T>& img,ArrayToArray func,ImageUtils::Rectangle<unsigned int> const selection)
	{
		unsigned int const width=img._width;
		unsigned int const size=width*img._height;
		auto const data=img._data;
		std::array<T,NumLayers> color;
		for(unsigned int y=selection.top;y<selection.bottom;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				auto const pix=row+x;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					color[s]=*(pix+s*size);
				}
				auto new_color=func(color);
				static_assert(new_color.size()>=NumLayers,"Mapping function outputs too few layers");
				for(unsigned int s=0;s<NumLayers;++s)
				{
					*(pix+s*size)=color[s];
				}
			}
		}
		return map;
	}

	template<unsigned int NumLayers,typename T,typename ArrayToBool>
	bool or_map(CImg<T> const& img,ArrayToBool pred)
	{
		auto const data=img._data;
		auto const size=img._width*img._height; //I hope the compiler can realize this is unused if NumLayers==1
		std::array<T,NumLayers> color;
		for(unsigned int i=0;i<size;++i)
		{
			auto const pix=data+i;
			for(unsigned int s=0;s<NumLayers;++s)
			{
				color[s]=*(pix+s*size);
			}
			if(func(color))
			{
				return true;
			}
		}
		return false;
	}

	template<unsigned int NumLayers,typename T,typename ArrayToArray>
	bool or_map(CImg<T>& img,ArrayToArray func,ImageUtils::Rectangle<unsigned int> const selection)
	{
		auto const width=img._width;
		auto const size=width*img._height;
		auto const data=img._data;
		std::array<T,NumLayers> color;
		for(auto y=selection.top;y<selection.bottom;++y)
		{
			auto const row=data+y*width;
			for(auto x=selection.left;x<selection.right;++x)
			{
				auto const pix=row+x;
				for(auto s=0U;s<NumLayers;++s)
				{
					color[s]=*(pix+s*size);
				}
				if(func(color))
				{
					return true;
				}

			}
		}
		return false;
	}

	template<unsigned int NumLayers,typename T,typename ArrayToBool>
	bool and_map(CImg<T> const& img,ArrayToBool pred)
	{
		auto const data=img._data;
		auto const size=img._width*img._height; //I hope the compiler can realize this is unused if NumLayers==1
		std::array<T,NumLayers> color;
		for(unsigned int i=0;i<size;++i)
		{
			auto const pix=data+i;
			for(unsigned int s=0;s<NumLayers;++s)
			{
				color[s]=*(pix+s*size);
			}
			if(!func(color))
			{
				return false;
			}
		}
		return true;
	}

	template<unsigned int NumLayers,typename T,typename ArrayToArray>
	bool and_map(CImg<T>& img,ArrayToArray func,ImageUtils::Rectangle<unsigned int> const selection)
	{
		unsigned int const width=img._width;
		unsigned int const size=width*img._height;
		auto const data=img._data;
		std::array<T,NumLayers> color;
		for(unsigned int y=selection.top;y<selection.bottom;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				auto const pix=row+x;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					color[s]=*(pix+s*size);
				}
				if(!func(color))
				{
					return false;
				}

			}
		}
		return true;
	}

	template<unsigned int NumLayers,typename T,typename ArrayRToR,typename R>
	R fold(CImg<T>const & img,ArrayRToR func,R acc)
	{
		auto const data=img._data;
		auto const size=img._width*img._height; //I hope the compiler can realize this is unused if NumLayers==1
		std::array<T,NumLayers> color;
		for(unsigned int i=0;i<size;++i)
		{
			auto const pix=data+i;
			for(unsigned int s=0;s<NumLayers;++s)
			{
				color[s]=*(pix+s*size);
			}
			acc=func(color,acc);
		}
		return acc;
	}

	template<unsigned int Numlayers,typename T,typename ArrayRToR,typename R>
	R fold(CImg<T>const & img,ArrayRToR func,R acc,ImageUtils::Rectangle<unsigned int> const selection)
	{
		unsigned int const width=img._width;
		unsigned int const size=width*img._height;
		auto const data=img._data;
		std::array<T,NumLayers> color;
		for(unsigned int y=selection.top;y<selection.bottom;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				auto const pix=row+x;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					color[s]=*(pix+s*size);
				}
				acc=func(color,acc);
			}
		}
		return acc;
	}

	template<unsigned int NumLayers,typename T,typename ArrayArrayToBool>
	auto min_pixel(CImg<T> const& img,ArrayArrayToBool comp)
	{
		assert(NumLayers<=img._spectrum);
		auto const data=img._data;
		auto loc=data;
		std::array<T,NumLayers> min_value;
		auto const size=img._width*img._height; //I hope the compiler can realize this is unused if NumLayers==1
		std::array<T,NumLayers> color;
		for(unsigned int i=0;i<size;++i)
		{
			auto const pix=data+i;
			for(unsigned int s=0;s<NumLayers;++s)
			{
				color[s]=*(pix+s*size);
			}
			if(comp(color,min_value))
			{
				min_value=color;
				loc=pix;
			}
		}
		return std::make_pair<decltype(loc),decltype(min_value)>(loc,min_value);
	}

	template<unsigned int NumLayers,typename T,typename ArrayArrayToBool>
	auto min_pixel(CImg<T> const& img,ArrayArrayToBool comp,ImageUtils::Rectangle<unsigned int> const selection)
	{
		assert(NumLayers<=img._spectrum);
		auto const data=img._data;
		auto loc=data;
		std::array<T,NumLayers> min_value;
		auto const size=img._width*img._height; //I hope the compiler can realize this is unused if NumLayers==1
		std::array<T,NumLayers> color;
		for(unsigned int y=selection.top;y<selection.bottom;++y)
		{
			auto const row=y*width;
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				auto const pix=row+x;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					color[s]=*(pix+s*size);
				}
				if(comp(color,min_value))
				{
					min_value=color;
					loc=pix;
				}
			}
		}
		return std::make_pair<decltype(loc),decltype(min_value)>(loc,min_value);
	}

	/*
	Returns an image that has the vertical brightness gradient at each point
	*/
	::cimg_library::CImg<signed char> get_vertical_gradient(::cimg_library::CImg<unsigned char> const&);
	/*
	Returns an image that has the absolute horizontal brightness gradient at each point
	*/
	::cimg_library::CImg<signed char> get_horizontal_gradient(::cimg_library::CImg<unsigned char> const&);
	/*
	Returns an image that has the absolute brightness gradient at each point, in 2 layers
	*/
	cimg_library::CImg<signed char> get_absolute_gradient(::cimg_library::CImg<unsigned char> const&);

	/*
	Returns an image that has the vertical brightness gradient at each point
	*/
	::cimg_library::CImg<signed char> get_vertical_gradient_sobel(::cimg_library::CImg<unsigned char> const&);
	/*
	Returns an image that has the absolute horizontal brightness gradient at each point
	*/
	::cimg_library::CImg<signed char> get_horizontal_gradient_sobel(::cimg_library::CImg<unsigned char> const&);
	/*
	Returns an image that has the absolute brightness gradient at each point, in 2 layers
	*/
	cimg_library::CImg<signed char> get_absolute_gradient_sobel(::cimg_library::CImg<unsigned char> const&);
	/*
	Returns an image that has the absolute brightness gradient at each point
	*/
	cimg_library::CImg<unsigned char> get_gradient(::cimg_library::CImg<unsigned char> const&);
	/*
	Returns an image that has the brightness at each point
	*/
	::cimg_library::CImg<unsigned char> get_brightness_spectrum(::cimg_library::CImg<unsigned char> const&);
	/*
	Returns the average RGB color of the image
	@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB average_color(::cimg_library::CImg<unsigned char const>& image);
	/*
	Returns the average grayness of the image
	@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale average_gray(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the darkest color in the image
	@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB darkest_color(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the darkest gray in the image
	@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale darkest_gray(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the brightest color in the image
	@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB brightest_color(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the brightest gray in the image
	@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale brightest_gray(::cimg_library::CImg<unsigned char> const& image);
	/*
	Converts image to a 1 channel grayscale image
	@param image, must be 3 or 4 channel RGB
	*/
	::cimg_library::CImg<unsigned char> get_grayscale(::cimg_library::CImg<unsigned char> const& image);
	/*
	Converts image to a 1 channel grayscale image, just takes average of 3.
	*/
	::cimg_library::CImg<unsigned char> get_grayscale_simple(::cimg_library::CImg<unsigned char> const& image);

	inline void convert_grayscale_simple(cil::CImg<unsigned char>& img)
	{
		assert(img._spectrum>=3);
		unsigned int const size=img._width*img._height;
		unsigned char* const rstart=img._data;
		unsigned char* const gstart=img._data+size;
		unsigned char* const bstart=img._data+2*size;
		for(unsigned int i=0;i<size;++i)
		{
			rstart[i]=(uint16_t(rstart[i])+gstart[i]+bstart[i])/3;
		}
		img._spectrum=1;
	}
	bool fill_transparency(::cil::CImg<unsigned char>& img,ImageUtils::ColorRGB back);

	bool remove_transparency(::cimg_library::CImg<unsigned char>& img,unsigned char threshold,ImageUtils::ColorRGB replacer);

	template<typename CountType>
	class HoughArray:public CImg<CountType> {
	private:
		double rmax;
		double theta_min;
		double angle_dif;
		unsigned int angle_steps;
		double precision;
	public:
		HoughArray(
			CImg<signed char> const& gradient,
			double lower_angle=M_PI_2-M_PI_2/18,double upper_angle=M_PI_2+M_PI_2/18,
			unsigned int num_steps=300,
			double precision=1.0f,
			signed char threshold=64);
		template<typename Selector>
		HoughArray(
			Selector vote_caster,
			unsigned int width,
			unsigned int height,
			double lower_angle=M_PI_2-M_PI_2/18,double upper_angle=M_PI_2+M_PI_2/18,
			unsigned int num_steps=300,
			double precision=1.0f);
		unsigned int& operator()(double theta,double r);
		double angle() const;
		std::vector<ImageUtils::line_norm<double>> top_lines(size_t num) const;
	};

	template<typename CountType>
	template<typename Selector>
	HoughArray<CountType>::HoughArray(
		Selector vote_caster,
		unsigned int width,
		unsigned int height,
		double lower_angle,double upper_angle,
		unsigned int num_steps,
		double precision):
		CImg(num_steps+1,(rmax=hypot(width,height))*2/precision),
		theta_min(lower_angle),
		angle_dif(upper_angle-lower_angle),
		angle_steps(num_steps),
		precision(precision)
	{
		CImg<CountType>::fill(0);
		double step=(_height-1)/precision;
		for(uint y=0;y<height;++y)
		{
			for(uint x=0;x<width;++x)
			{
				if(vote_caster(x,y))
				{
					for(uint f=0;f<=angle_steps;++f)
					{
						double theta=angle_dif*f/angle_steps+theta_min;
						double r=x*std::cos(theta)+y*std::sin(theta);
						unsigned int y=((r+rmax)/(2*rmax))*step;
						auto const inc=this->data()+y*_width+f;
						++(*inc);
						++(*(inc+_width));
					}
				}
			}
		}
	}

	template<typename CountType>
	HoughArray<CountType>::HoughArray(
		CImg<signed char> const& gradient,
		double lower_angle,double upper_angle,
		unsigned int num_steps,
		double precision,
		signed char threshold)
		:
		CImg(num_steps+1,(rmax=hypot(gradient._width,gradient._height))*2/precision),
		theta_min(lower_angle),
		angle_dif(upper_angle-lower_angle),
		angle_steps(num_steps),
		precision(precision)
	{
		threshold=std::abs(threshold);
		fill(0);
		double step=(_height-1)/precision;
		signed char const* const data=gradient.data();
		for(uint y=0;y<gradient._height;++y)
		{
			signed char const* const row=data+y*gradient._width;
			for(uint x=0;x<gradient._width;++x)
			{
				if(std::abs(*(row+x))>threshold)
				{
					for(uint f=0;f<=angle_steps;++f)
					{
						double theta=angle_dif*f/angle_steps+theta_min;
						double r=x*std::cos(theta)+y*std::sin(theta);
						unsigned int y=((r+rmax)/(2*rmax))*step;
						unsigned int* const inc=this->data()+y*_width+f;
						++(*inc);
						++(*(inc+_width));
					}
				}
			}
		}
	}
	template<typename CountType>
	unsigned int& HoughArray<CountType>::operator()(double theta,double r)
	{
		unsigned int x=(theta-theta_min)/angle_dif*angle_steps;
		unsigned int y=((r+rmax)/(2*rmax))/precision*(_height-1);
		//printf("%u\t%u\n",x,y);
		return CImg<unsigned int>::operator()(x,y);
	}

	template<typename CountType>
	double HoughArray<CountType>::angle() const
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
		return (max_it-begin())%_width*angle_dif/angle_steps+theta_min;
	}

	template<typename CountType>
	std::vector<ImageUtils::line_norm<double>> HoughArray<CountType>::top_lines(size_t n) const
	{
		exlib::LimitedSet<decltype(begin())> top;
		auto comp=[](auto a,auto b)
		{
			return *a>*b;
		};
		for(auto it=begin();it!=end();++it)
		{
			top.insert(it,comp);
		}
		double step=angle_dif/angle_steps;
		std::vector<ImageUtils::line_norm<double>> lines(top.size());
		auto il=lines.begin();
		for(auto it:top)
		{
			auto d=std::distance(begin(),it);
			auto x=d%_width;
			auto y=d/_width;
			(*il).theta=x*step+theta_min;
			(*il).r=2*rmax*y*precision/(_height-1)-rmax;
		}
		return lines;
	}
}
#endif // !IMAGE_MATH_H
