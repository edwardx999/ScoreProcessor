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
#ifndef SCORE_PROCESSES_H
#define SCORE_PROCESSES_H
#include "CImg.h"
#include "ImageUtils.h"
#include <vector>
#include <memory>
#include "Cluster.h"
#include <assert.h>
#include <functional>
#include <array>
#include <mutex>
#include "lib/threadpool/thread_pool.h"
#include "../NeuralNetwork/neural_net.h"
namespace ScoreProcessor {

	template<typename T>
	class vertical_iterator {
		T* _top;
		size_t _width;
	public:
		vertical_iterator(cil::CImg<T> const& img,unsigned int column,unsigned int spectrum=0) noexcept:_top{img.data()+column+size_t{spectrum}*img._width*img._height},_width{img._width}{}
		vertical_iterator(cil::CImg<T>& img,unsigned int column,unsigned int spectrum=0) noexcept:_top{img.data()+column+size_t{spectrum}*img._width*img._height},_width{img._width}{}
		vertical_iterator(T* top,unsigned int width) noexcept:_top{top}, _width{width}{}
		T& operator[](size_t s) const noexcept
		{
			return *(_top+s*_width);
		}
		T& operator*() const noexcept
		{
			return *_top;
		}
		T* operator->() const noexcept
		{
			return &**this;
		}
		vertical_iterator& operator++() noexcept
		{
			_top+=_width;
			return *this;
		}
		vertical_iterator& operator--() noexcept
		{
			_top-=_width;
			return *this;
		}		
		vertical_iterator operator++(int) noexcept
		{
			auto copy{*this};
			++(*this);
			return copy;
		}
		vertical_iterator operator--(int) noexcept
		{
			auto copy{*this};
			--(*this);
			return copy;
		}
		vertical_iterator& operator+=(std::ptrdiff_t d) noexcept
		{
			_top+=d*_width;
			return *this;
		}
		vertical_iterator& operator-=(std::ptrdiff_t d) noexcept
		{
			_top-=d*_width;
			return *this;
		}
		friend vertical_iterator operator+(vertical_iterator v,std::ptrdiff_t t) noexcept
		{
			v+=t;
			return v;
		}
		friend vertical_iterator operator+(std::ptrdiff_t t,vertical_iterator v) noexcept
		{
			v+=t;
			return v;
		}
		friend vertical_iterator operator-(vertical_iterator v,std::ptrdiff_t t) noexcept
		{
			v-=t;
			return v;
		}
		friend std::ptrdiff_t operator-(vertical_iterator const& v,vertical_iterator const& v2) noexcept
		{
			assert(v._width==v2._width);
			return (v._top-v2._top)/v._width;
		}
#define vertical_operator_comp_op(op)\
		template<typename T, typename U>\
		friend bool operator op(vertical_iterator<T> const& a,vertical_iterator<U> const& b) noexcept\
		{\
			assert(a._width==b._width);\
			return a._top op b._top;\
		}
		vertical_operator_comp_op(==)
			vertical_operator_comp_op(<)
			vertical_operator_comp_op(>)
			vertical_operator_comp_op(<=)
			vertical_operator_comp_op(>=)
			vertical_operator_comp_op(!=)
#undef vertical_operator_comp_op
	};

	template<typename T>
	vertical_iterator(cil::CImg<T> const&,unsigned int,unsigned int) -> vertical_iterator<T const>;

	template<typename T>
	vertical_iterator(cil::CImg<T>&,unsigned int,unsigned int) -> vertical_iterator<T>;

	class ExclusiveThreadPool {
	public:
		exlib::thread_pool& pool() const;
		void set_thread_count(unsigned int nt);
		ExclusiveThreadPool(unsigned int num_threads=std::thread::hardware_concurrency());
		~ExclusiveThreadPool();
	};

	/*
		Fast approximate anti-aliasing
	*/
	template<typename T>
	bool fxaa(cil::CImg<T>& img, std::common_type_t<T,short> contrast_threshold,std::common_type_t<float,T> gamma,std::common_type_t<float,T> subpixel_blending=1)
	{
		if(img.width()<3||img.height()<3||img.spectrum()<1||img.depth()<1)
		{
			return false;
		}
		cil::CImg<T> copy{img._width,img._height,img._depth,img._spectrum};
		//cheap using first layer as luminance
		auto const width = img._width;
		auto const height=img._height;
		using p=decltype(contrast_threshold);
		for(unsigned int y=0;y<width;++y)
		{
			for(unsigned int x=0;x<height;++x)
			{
				p nw=img._atXY(x-1,y-1);
				p n=img._atXY(x,y-1);
				p ne=img._atXY(x+1,y-1);
				p w=img._atXY(x-1,y);
				p m=img(x,y);
				p e=img._atXY(x+1,y);
				p sw=img._atXY(x-1,y+1);
				p s=img._atXY(x,y+1);
				p se=img._atXY(x,y+1);
				auto minmax=std::minmax({n,s,e,w});
				p contrast=p{minmax.second}-p{minmax.first};
				if(contrast>contrast_threshold) //found an edge 
				{
					using f=std::common_type_t<float,p>;
					constexpr f weight=1.41421356237;
					f blend=(weight*(n+e+s+w)+nw+ne+sw+se)/(4*weight+4);
					f filter=std::clamp<f>(std::abs(blend-m)/contrast,0,1);
					filter=filter*filter*(3-2*x);
					f hcontrast=weight*std::abs(n+s-2*m)+std::abs(ne+se-2*e)+std::abs(nw+sw-2*w);
					f vcontrast=weight*std::abs(e+w-2*m)+std::abs(ne+nw-2*n)+std::abs(se+sw-2*s);
					bool horizontal=hcontrast>=vcontrast;
					auto [pgradient,ngradient]=horizontal?std::make_tuple(f{s}-m,f{n}-m):std::make_tuple(f{e}-m,f{w}-m);
					auto blend_factor=blend*blend*subpixel_blending;
					if(pgradient>ngradient)
					{
						auto gradient_threshold=pgradient*0.5;
						if(horizontal) //only need to look to left if horizontal
						{
							auto edge_luminance=f{s}+m;
							unsigned int x_s=x;
							for(;x_s<width;++x_s)
							{
								if(img(x_s,y)-edge_luminance>=gradient_threshold) break;
							}
						}
						else
						{
							auto edge_luminance=f{e}+m;
						}
					}
					else
					{
						auto gradient_threshold=ngradient*0.5;
						if(horizontal)
						{
							auto edge_luminance=f{n}+m;
						}
						else
						{
							auto edge_luminance=f{w}+m;
						}
					}
				}
			}
		}
		img=std::move(copy);
		return true;
	}

	namespace mlaa_det {
		enum class orientation:char {
			up=1,flat=2,down=3
		};
		template<typename T>
		auto operator*(orientation o,T val) noexcept
		{
			return static_cast<char>(o)*val;
		}
		template<typename T>
		auto operator*(T val,orientation o) noexcept
		{
			return static_cast<char>(o)*val;
		}
		struct edge_t {
			using orientation=mlaa_det::orientation;
			unsigned int begin;
			unsigned int end;
			orientation begin_orientation;
			orientation end_orientation;
		};
		template<typename Iter,typename U>
		edge_t find_edge(Iter row,Iter next_row,unsigned int start,unsigned int end,U contrast_threshold) noexcept
		{
			auto detect_orientation=[row,next_row,contrast_threshold](unsigned int x,orientation& orient)
			{
				auto const upper=std::abs(U{row[x]}-U(row[x-1]));
				auto const lower=std::abs(U{next_row[x]}-U(next_row[x-1]));
				if(upper>contrast_threshold||lower>contrast_threshold)
				{
					if(upper>lower)
					{
						orient=orientation::up;
					}
					else
					{
						orient=orientation::down;
					}
				}
				else
				{
					orient=orientation::flat;
				}
			};
			for(;start<end;++start)
			{
				if(std::abs(U{row[start]}-U{next_row[start]})>contrast_threshold)
				{
					edge_t edge;
					edge.begin=start;
					++start;
					for(;start<end;++start)
					{
						if(std::abs(U{row[start]}-U{next_row[start]})<=contrast_threshold)
						{
							edge.end=start;
							if(start+1<end)
							{
								detect_orientation(start,edge.end_orientation);
							}
							else
							{
								edge.end_orientation=orientation::flat;
							}
							break;
						}
					}
					if(start>=end)
					{
						edge.end_orientation=orientation::flat;
						edge.end=end;
					}
					if(edge.begin>0)
					{
						detect_orientation(edge.begin,edge.begin_orientation);
					}
					else
					{
						edge.begin_orientation=orientation::flat;
					}
					return edge;
				}
			}
			return {-1U,-1U};
		}

		template<typename Iter>
		void blend(Iter row,Iter next_row,double const x_start,double const y_start,double const x_end,double const y_end,double const gamma) noexcept
		{
			auto m=(y_end-y_start)/(x_end-x_start);
			if(y_start==1)
			{
				m=std::abs(m);
			}
			else
			{
				m=-std::abs(m);
			}
			bool const upper=y_start<1||y_end<1;
			auto const area_adjustment=y_start==1?0:0.5;
			auto write_row=upper?row:next_row;
			auto read_row=upper?next_row:row;
			auto x=x_start;
			auto mix=[gamma](auto a,auto b,auto area)
			{
				return std::round(std::pow(area*std::pow(a,gamma)+(1-area)*std::pow(b,gamma),1/gamma));
			};
			if(std::floor(x)==x)
			{
				for(;x<x_end-0.5;++x) //half integers should be exact
				{
					auto area=m*(x-x_start+0.5)+area_adjustment;
					size_t x_coord=x;
					write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				}
				if(x<x_end)
				{
					auto area=(area_adjustment+m*(x-x_start))/4;
					size_t x_coord=x;
					write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				}
			}
			else
			{
				auto area=(area_adjustment+m*0.5)/2;
				size_t x_coord=x;
				write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				x+=0.5;
				for(;x<x_end;++x) //half integers should be exact
				{
					auto area=m*(x-x_start+0.5)+area_adjustment;
					if(area>0.5) area=1-area;
					size_t x_coord=x;
					write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				}
			}
		}
	}

	/*
		Morphological Antialiasing
	*/
	template<typename T>
	bool mlaa(cil::CImg<T>& img,std::common_type_t<T,short> contrast_threshold,double gamma)
	{
		auto const height=img._height;
		auto const width=img._width;
		if(height<2||width<2) return false;
		auto const hm1=height-1;
		auto const wm1=width-1;
		using p=decltype(contrast_threshold);
		using namespace mlaa_det;
		bool did_something=false;
		auto const size=size_t{height}*width;
		auto const spectrum=img._spectrum;
		auto copy{img};
		for(unsigned int y=0;y<hm1;++y) //scan for horizontal edge
		{
			T* const row=img.data()+y*width;
			T* const next_row=row+width;
			edge_t edge=find_edge(row,next_row,0,width,contrast_threshold);
			for(;edge.begin!=-1;edge=find_edge(row,next_row,edge.end,width,contrast_threshold))
			{
				auto do_blend=[row=copy.data()+y*width,width,gamma,spectrum,size](double x_start,double y_start,double x_end,double y_end)
				{
					for(unsigned int i=0;i<spectrum;++i)
					{
						auto const layer_row=row+i*size;
						mlaa_det::blend(layer_row,layer_row+width,x_start,y_start,x_end,y_end,gamma);
					}
				};
				if(edge.end-edge.begin==1||(edge.begin_orientation==orientation::flat&&edge.end_orientation==orientation::flat))
				{
					continue;
				}
				did_something=true;
				switch(edge.begin_orientation)
				{
				case orientation::flat:
					do_blend(edge.begin,1,edge.end,0.5*edge.end_orientation);
					break;
				case orientation::down:
				case orientation::up:
					switch(edge.end_orientation)
					{
					case orientation::down:
					case orientation::up:
					{
						auto const mid_dist=(double(edge.end)-edge.begin)/2;
						auto const mid=edge.begin+mid_dist;
						do_blend(edge.begin,0.5*edge.begin_orientation,mid,1);
						do_blend(mid,1,edge.end,0.5*edge.end_orientation);
					}					
					break;
					case orientation::flat:
						do_blend(edge.begin,0.5*edge.begin_orientation,edge.end,1);
					}
					break;
				}
			}
		}
		for(unsigned int x=0;x<wm1;++x) //scan for vertical edge
		{
			vertical_iterator column{img,x};
			vertical_iterator next_column{img,x+1};
			edge_t edge=find_edge(column,next_column,0,height,contrast_threshold);
			for(;edge.begin!=-1;edge=find_edge(column,next_column,edge.end,height,contrast_threshold))
			{
				auto do_blend=[&copy,x,gamma,spectrum,size](double x_start,double y_start,double x_end,double y_end)
				{
					for(unsigned int i=0;i<spectrum;++i)
					{
						vertical_iterator column{copy,x,i};
						vertical_iterator next{copy,x+1,i};
						mlaa_det::blend(column,next,x_start,y_start,x_end,y_end,gamma);
					}
				};
				if(edge.begin_orientation==orientation::flat&&edge.end_orientation==orientation::flat)
				{
					continue;
				}
				did_something=true;
				switch(edge.begin_orientation)
				{
				case orientation::flat:
					do_blend(edge.begin,1,edge.end,0.5*edge.end_orientation);
					break;
				case orientation::down:
				case orientation::up:
					switch(edge.end_orientation)
					{
					case orientation::down:
					case orientation::up:
					{
						auto const mid_dist=(double(edge.end)-edge.begin)/2;
						auto const mid=edge.begin+mid_dist;
						do_blend(edge.begin,0.5*edge.begin_orientation,mid,1);
						do_blend(mid,1,edge.end,0.5*edge.end_orientation);
					}					
					break;
					case orientation::flat:
						do_blend(edge.begin,0.5*edge.begin_orientation,edge.end,1);
					}
					break;
				}
			}
		}
		img=std::move(copy);
		return did_something;
	}

	//BackgroundFinder:: returns true if a pixel is NOT part of the background
	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_left(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf)
	{
		unsigned int num=0;
		auto const size=img._width*img._height;
		for(unsigned int x=0;x<img._width;++x)
		{
			for(unsigned int y=0;y<img._height;++y)
			{
				auto pix=&img(x,y);
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return x;
			}
		}
		return img._width-1;
	}
	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_right(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf)
	{
		unsigned int num=0;
		auto const size=img._width*img._height;
		for(unsigned int x=img._width-1;x<img._width;--x)
		{
			for(unsigned int y=0;y<img._height;++y)
			{
				auto pix=&img(x,y);
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return x;
			}
		}
		return 0;
	}

	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_top(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf)
	{
		unsigned int num=0;
		auto const width=img._width;
		auto const height=img._height;
		auto const size=width*height;
		auto const data=img._data;
		for(unsigned int y=0;y<height;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				auto pix=row+x;
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return y;
			}
		}
		return height-1;
	}

	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_bottom(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf)
	{
		unsigned int num=0;
		auto const width=img._width;
		auto const height=img._height;
		auto const size=width*height;
		auto const data=img._data;
		for(unsigned int y=height-1;y<height;--y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				auto pix=row+x;
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return y;
			}
		}
		return 0;
	}

	template<typename T>
	cil::CImg<T> get_crop_fill(cil::CImg<T> const& img,ImageUtils::Rectangle<signed int> region,T fill=255)
	{
		auto const width=img.width();
		auto const height=img.height();
		auto ret=img.get_crop(region.left,region.top,region.right,region.bottom,4);
		T buffer[10];
		for(unsigned int s=0;s<img._spectrum;++s)
		{
			buffer[s]=fill;
		}
		if(region.left<0)
		{
			fill_selection(ret,{0,static_cast<unsigned int>(-region.left),0,ret._height},buffer);
		}
		if(region.right>=width)
		{
			fill_selection(ret,{static_cast<unsigned int>(width-region.left),ret._width,0,ret._height},buffer);
		}
		if(region.top<0)
		{
			fill_selection(ret,{0,ret._width,0,static_cast<unsigned int>(-region.top)},buffer);
		}
		if(region.bottom>=height)
		{
			fill_selection(ret,{0,ret._width,static_cast<unsigned int>(height-region.top),ret._height},buffer);
		}
		return ret;
	}
	/*
		Reduces colors to two colors
		@param image, must be a 3 channel RGB image
		@param middleColor, pixels darker than this color will be set to lowColor, brighter to highColor
		@param lowColor
		@param highColor
	*/
	void binarize(::cimg_library::CImg<unsigned char>& image,ImageUtils::ColorRGB const middleColor,ImageUtils::ColorRGB const lowColor,ImageUtils::ColorRGB const highColor);
	/*
		Reduces colors to two colors
		@param image, must be a 1 channel grayscale image
		@param middleGray, pixels darker than this gray will be set to lowGray, brighter to highGray
		@param lowGray
		@param highGray
	*/
	void binarize(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const middleGray,ImageUtils::Grayscale const lowGray,ImageUtils::Grayscale const highGray);
	/*
		@param image, must be a 1 channel grayscale image
		@param middleGray, pixels darker than this gray will be become more black by a factor of scale, higher whited by a factor of scale
	*/
	void binarize(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const middleGray,float const scale=2.0f);
	/*
		Replaces grays in a range with another gray
		@param image, must be 1 channel grayscale image
	*/
	bool replace_range(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const lower,ImageUtils::Grayscale const upper=255,ImageUtils::Grayscale const replacer=ImageUtils::Grayscale::WHITE);
	/*
		Replaces certainly bright pixels with a color
		@param image, must be 3 channel RGB
		@param lowerBrightness
		@param upperBrightness
		@param replacer
	*/
	bool replace_by_brightness(::cimg_library::CImg<unsigned char>& image,unsigned char lowerBrightness,unsigned char upperBrightness=255,ImageUtils::ColorRGB replacer=ImageUtils::ColorRGB::WHITE);
	/*
		Replaces particularly chromatic pixels with a color
		@param image, must be 3 channel RGB
		@param lowerChroma
		@param upperChroma
		@param replacer
	*/
	bool replace_by_hsv(::cimg_library::CImg<unsigned char>& image,ImageUtils::ColorHSV startbound,ImageUtils::ColorHSV end,ImageUtils::ColorRGB replacer=ImageUtils::ColorRGB::WHITE);
	bool replace_by_rgb(::cil::CImg<unsigned char>& image,ImageUtils::ColorRGB start,ImageUtils::ColorRGB end,ImageUtils::ColorRGB replacer);
	/*
		Copies a selection from the first image to the location of the second image
		The two images should have the same number of channels
	*/
	template<typename T>
	void copy_paste(::cimg_library::CImg<T> dest,::cimg_library::CImg<T> src,ImageUtils::Rectangle<unsigned int> selection,ImageUtils::Point<signed int> destloc);
	/*
		Shifts selection over while leaving rest unchanged
		@param image
		@param selection, the rectangle that will be shifted
		@param shiftx, the number of pixels the selection will be translated in the x direction
		@param shiftx, the number of pixels the selection will be translated in the y direction
	*/
	template<typename T>
	void copy_shift_selection(::cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> selection,int const shiftx,int const shifty);
	/*
		Fills selection with a certain color
		@param image, must be 3 channel RGB
		@param selection, the rectangle that will be filled
		@param color, the color the selection will be filled with
	*/
	void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::ColorRGB const color);
	/*
		Fills selection with a certain color
		@param image, must be 1 channel grayscale
		@param selection, the rectangle that will be filled
		@param gray, the gray the selection will be filled with
	*/
	void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::Grayscale const gray);
/*
		Automatically centers the image horizontally
		@param image
		@return 0 if successful shift, 1 if no shift, 2 if invalid image
	*/
	bool auto_center_horiz(::cimg_library::CImg<unsigned char>& image);
	/*
		Finds the left side of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the left side
		@return the x-coordinate of the left side
	*/
	unsigned int find_left(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the left side of the the image
		@param image, must 1 channel grayscale
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the left side
		@return the x-coordinate of the left side
	*/
	unsigned int find_left(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);
	/*
		Finds the right side of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the right side
		@return the x-coordinate of the right side
	*/
	unsigned int find_right(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the left side of the the image
		@param image, must 1 channel grayscale
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the right side
		@return the x-coordinate of the right side
	*/
	unsigned int find_right(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);

	/*
		Automatically centers the image vertically
		@param image
		@return 0 if successful shift, 1 if no shift, 2 if invalid image
	*/
	bool auto_center_vert(::cimg_library::CImg<unsigned char>& image);
	/*
		Finds the top of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the top
		@return the y-coordinate of the top
	*/
	unsigned int find_top(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the top of the the image
		@param image, must 1 channel RGB
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the top
		@return the y-coordinate of the top
	*/
	unsigned int find_top(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);
	/*
		Finds the bottom of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the bottom
		@return the y-coordinate of the bottom
	*/
	unsigned int find_bottom(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the bottom of the the image
		@param image, must 1 channel RGB
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the bottom
		@return the y-coordinate of the bottom
	*/
	unsigned int find_bottom(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);

	/*
		Creates a profile of the left side of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of x coordinates of the left side
	*/
	::std::vector<unsigned int> build_left_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the left side of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return where the profile will be stored, as a vector of x coordinates of the left side
	*/
	::std::vector<unsigned int> build_left_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the right side of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of x coordinates of the right side
	*/
	::std::vector<unsigned int> build_right_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the right side of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return where the profile will be stored, as a vector of x coordinates of the right side
	*/
	::std::vector<unsigned int> build_right_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the top of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of y coordinates of the top
	*/
	::std::vector<unsigned int> build_top_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the top of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return where the profile will be stored, as a vector of y coordinates of the top
	*/
	::std::vector<unsigned int> build_top_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the bottom of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of y coordinates of the bottom
	*/
	::std::vector<unsigned int> build_bottom_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the bottom of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return container, where the profile will be stored, as a vector of y coordinates of the bottom
	*/
	::std::vector<unsigned int> build_bottom_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Selects the outside (non-systems) of a score image
		@param image, must be 1 channel grayscale
		@return where the selected rectangles go
	*/
	::std::vector<::std::unique_ptr<ImageUtils::Rectangle<unsigned int>>> select_outside(::cimg_library::CImg<unsigned char> const& image);
	struct cut_heuristics {
		unsigned int min_width;
		unsigned int min_height;
		float horizontal_energy_weight;
		unsigned int minimum_vertical_space;
		unsigned char background;
	};
	/*
		Cuts a specified score page into multiple smaller images
		@param image
		@param filename, the start of the filename that the images will be saved as
		@param padding, how much white space will be put at the top and bottom of the pages
		@return the number of images created
	*/
	unsigned int cut_page(::cimg_library::CImg<unsigned char> const& image,char const* filename,cut_heuristics const& ch={1000,80,20,0,128},int quality=100);

	/*
		Finds the line that is the top of the score image
		@param image
		@return a line defining the top of the score image
	*/
	ImageUtils::line<unsigned int> find_top_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the bottom of the score image
		@param image
		@return a line defining the bottom of the score image
	*/
	ImageUtils::line<unsigned int> find_bottom_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the left of the score image
		@param image
		@return a line defining the left of the score image
	*/
	ImageUtils::line<unsigned int> find_left_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the right of the score image
		@param image
		@return a line defining the right of the score image
	*/
	ImageUtils::line<unsigned int> find_right_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Automatically levels the image
		@param image
		@param pixel_prec precision when measuring distance from origin
		@param min_angle minimum angle of range to consider rotation (degrees), is angle of line from x-axis
		@param max_angle maximum angle of range to consider rotation (degrees), is angle of line from x-axis
		@param angle_prec precision when measuring angle
		@return rotation in degrees
	*/
	float auto_rotate(::cimg_library::CImg<unsigned char>& image,double pixel_prec,double min_angle,double max_angle,double angle_prec,unsigned char boundary=128);

	float find_angle_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary=128);
	/*
		Automatically levels the image.
		@param image
		@param pixel_prec precision when measuring distance from origin
		@param min_angle minimum angle of range to consider rotation (radians), is angle of Hesse normal form
		@param max_angle maximum angle of range to consider rotation (radians), is angle of Hesse normal form
		@param angle_steps number of angle quantization steps between min and max angle
		@return rotation in radians
	*/
	float auto_rotate_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary=128);
	/*
		Automatically deskews the image
		@param image
	*/
	bool auto_skew(::cimg_library::CImg<unsigned char>& image);
	/*
		Automatically levels and deskews the image
		@param image
	*/
	bool undistort(::cimg_library::CImg<unsigned char>& image);
	/*
		Flood selects from point
		@param image, must be 1 channel grayscale
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param gray, selected gray
		@param start, seed point of flood fill
		@return where the selected pixels will go as a vector of rectangles pointers
	*/
	::std::vector<ImageUtils::Rectangle<unsigned int>> flood_select(::cimg_library::CImg<unsigned char> const& image,float const tolerance,ImageUtils::Grayscale const gray,ImageUtils::Point<unsigned int> start);

	/*
		Removes border of the image.
	*/
	bool remove_border(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const color=ImageUtils::Grayscale::BLACK,float const tolerance=0.5);

	/*
		Does a flood fill.
	*/
	bool flood_fill(::cimg_library::CImg<unsigned char>& image,float const tolerance,ImageUtils::Grayscale const color,ImageUtils::Grayscale const replacer,ImageUtils::Point<unsigned int> start);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param vertical_padding, size in pixels of padding on top and bottom
		@param max_horizontal_padding
		@param min_horizontal_padding
		@param optimal_ratio
	*/
	bool auto_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const vertical_padding,unsigned int const max_horizontal_padding,unsigned int const min_horizontal_padding,signed int horiz_offset,float optimal_ratio=16.0f/9.0f,unsigned int tolerance=5,unsigned char background=200);
	bool cluster_padding(
		::cil::CImg<unsigned char>& img,
		unsigned int const left,
		unsigned int const right,
		unsigned int const top,
		unsigned int const bottom,
		unsigned char background_threshold);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
	*/
	bool horiz_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const padding);
	bool horiz_padding(::cimg_library::CImg<unsigned char>& img,unsigned int const left,unsigned int const right,unsigned int tolerance=5,unsigned char background=200);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
	*/
	bool vert_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const padding);
	bool vert_padding(::cimg_library::CImg<unsigned char>& img,unsigned int const top,unsigned int const bottom,unsigned int tolerance=5,unsigned char background=200);
	
	void compress(
		::cimg_library::CImg<unsigned char>& image,
		unsigned int const min_padding,
		unsigned int const optimal_height,
		unsigned char background_threshold=254);

	/*

	*/
	void rescale_colors(::cimg_library::CImg<unsigned char>&,unsigned char min,unsigned char mid,unsigned char max=255);
}
template<typename T>
void ScoreProcessor::copy_shift_selection(cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> selection,int const shiftx,int const shifty)
{
	if(shiftx==0&&shifty==0) return;
	selection.left=selection.left+shiftx;if(selection.left>image._width) selection.left=0;
	selection.right=selection.right+shiftx;if(selection.right>image._width) selection.right=image._width;
	selection.top=selection.top+shifty;if(selection.top>image._height) selection.top=0;
	selection.bottom=selection.bottom+shifty;if(selection.bottom>image._height) selection.bottom=image._height;
	unsigned int numChannels=image._spectrum;
	if(shiftx>0)
	{
		if(shifty>0) //shiftx>0 and shifty>0
		{
			for(unsigned int x=selection.right;x-->selection.left;)
			{
				for(unsigned int y=selection.bottom;y-->selection.top;)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else //shiftx>0 and shifty<=0
		{
			for(unsigned int x=selection.right;x-->selection.left;)
			{
				for(unsigned int y=selection.top;y<selection.bottom;++y)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
	else
	{
		if(shifty>0) //shiftx<=0 and shifty>0
		{
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				for(unsigned int y=selection.bottom;y-->selection.top;)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else //shiftx<=0 and shifty<=0
		{
			for(unsigned int x=selection.left;x<selection.right;++x)
			{
				for(unsigned int y=selection.top;y<selection.bottom;++y)
				{
					for(unsigned int s=0;s<numChannels;++s)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
}
namespace ScoreProcessor {

	struct padding_pack {
		unsigned int left,right,top,bottom;
	};

	template<typename T,typename FindLeft,typename FindRight,typename FindTop,typename FindBottom>
	bool padding(::cil::CImg<T>& img,padding_pack pp,FindLeft fl,FindRight fr,FindTop ft,FindBottom fb)
	{
		signed int left,right,top,bottom;
		if(pp.left==-1)
		{
			left=0;
		}
		else
		{
			left=fl(img)-pp.left;
		}
		if(pp.right=-1)
		{
			right=img.width()-1;
		}
		else
		{
			right=fr(img)+pp.right;
		}
		if(pp.top==-1)
		{
			top=0;
		}
		else
		{
			top=ft(img)-pp.top;
		}
		if(pp.bottom=-1)
		{
			bottom=img.height()-1;
		}
		else
		{
			bottom=fr(img)+pp.bottom;
		}
		if(left>right)
		{
			std::swap(left,right);
		}
		if(top>bottom)
		{
			std::swap(top,bottom);
		}
		if(left==0&&top==0&&right==img.width()-1&&bottom==img.height()-1)
		{
			return false;
		}
		img=cil::get_crop_fill(img,ImageUtils::Rectangle<int>({left,right,top,bottom}));
		return true;
	}

	template<typename T>
	void fill_selection(::cimg_library::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,T const* color)
	{
		unsigned int const width=img._width;
		unsigned int const area=img._height*width;
		unsigned int const spectrum=img._spectrum;
		T* const data=img._data;
		for(unsigned int s=0;s<spectrum;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					row[x]=color[s];
				}
			}
		}
	}

	template<typename T,size_t N>
	void fill_selection(::cimg_library::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,std::array<T,N> color)
	{
		assert(img._spectrum>=N);
		assert(sel.right<img._width);
		assert(sel.bottom<img._height);
		unsigned int const width=img._width;
		unsigned int const area=width*img._height;
		T* const data=img._data;
		for(unsigned int s=0;s<N;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					row[x]=color[s];
				}
			}
		}
	}
	struct check_fill_t {};

	template<typename T,size_t N>
	bool fill_selection(::cil::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,std::array<T,N> color,check_fill_t)
	{
		assert(img._spectrum>=N);
		assert(sel.right<img._width);
		assert(sel.bottom<img._height);
		unsigned int const width=img._width;
		unsigned int const area=width*img._height;
		T* const data=img._data;
		bool edited=false;
		for(unsigned int s=0;s<N;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					if(row[x]!=color[s])
					{
						row[x]=color[s];
						edited=true;
					}
				}
			}
		}
		return edited;
	}

	template<typename T>
	bool fill_selection(::cimg_library::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,T const* color,check_fill_t)
	{
		unsigned int const width=img._width;
		unsigned int const area=img._height*width;
		unsigned int const spectrum=img._spectrum;
		T* const data=img._data;
		bool edited=false;
		for(unsigned int s=0;s<spectrum;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					if(row[x]!=color[s])
					{
						row[x]=color[s];
						edited=true;
					}
				}
			}
		}
		return edited;
	}
}

inline void ScoreProcessor::fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::ColorRGB const color)
{
	fill_selection(image,selection,std::array<unsigned char,3>({color.r,color.g,color.b}));
}
inline void ScoreProcessor::fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::Grayscale const gray)
{
	fill_selection(image,selection,std::array<unsigned char,1>({gray}));
}

template<typename T>
void ScoreProcessor::copy_paste(::cimg_library::CImg<T> dest,::cimg_library::CImg<T> src,ImageUtils::Rectangle<unsigned int> selection,ImageUtils::Point<signed int> destloc)
{
	if(destloc.x<0)
	{
		select.left-=destloc.x;
		destloc.x=0;
	}
	if(destloc.y<0)
	{
		select.top-=destloc.y;
		destloc.y=0;
	}
	for(;destloc.x<dest._width&&select.left<select.right;++destloc.x,++select.left)
	{
		for(;destloc.y<dest._height&&select.top<select.bottom;++destloc.y,++select.top)
		{
			dest(destloc.x,destloc.y)=src(select.left,select.top);
		}
	}
}
namespace ScoreProcessor {
	template<unsigned int num_layers,typename T,typename Selector>
	std::vector<ImageUtils::Rectangle<unsigned int>> global_select(
		::cil::CImg<T> const& image,
		Selector keep)
	{
		static_assert(num_layers>0,"Positive number of layers required");
		assert(image._spectrum>=num_layers);
		std::array<T,num_layers> color;
		unsigned int range_found=0,range_start=0,range_end=0;
		auto const height=image._height;
		auto const width=image._width;
		size_t const size=height*width;
		auto const data=image._data;
		std::vector<ImageUtils::Rectangle<unsigned int>> container;
		for(unsigned int y=0;y<height;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				auto const pix=row+x;
				for(unsigned int i=0;i<num_layers;++i)
				{
					color[i]=*(pix+i*size);
				}
				switch(range_found)
				{
					case 0:
					{
						if(keep(color))
						{
							range_found=1;
							range_start=x;
						}
						break;
					}
					case 1:
					{
						if(!keep(color))
						{
							range_found=2;
							range_end=x;
						}
						else
						{
							break;
						}
					}
					case 2:
					{
						container.push_back(ImageUtils::Rectangle<unsigned int>{range_start,range_end,y,y+1});
						range_found=0;
						break;
					}
				}
			}
			if(1==range_found)
			{
				container.push_back(ImageUtils::Rectangle<unsigned int>{range_start,image._width,y,y+1});
				range_found=0;
			}
		}
		ImageUtils::compress_rectangles(container);
		return container;
	}

	template<typename T,size_t NL,typename PixelSelectorArrayNLToBool,typename ClusterToTrueIfClear>
	bool clear_clusters(
		::cil::CImg<T>& img,
		std::array<T,NL> replacer,
		PixelSelectorArrayNLToBool ps,
		ClusterToTrueIfClear cl)
	{
		assert(img._spectrum>=NL);
		auto rects=global_select<NL>(img,ps);
		auto clusters=ScoreProcessor::Cluster::cluster_ranges(rects);
		bool edited=false;
		for(auto it=clusters.cbegin();it!=clusters.cend();++it)
		{
			if(cl(*it))
			{
				edited=true;
				for(auto rect:it->get_ranges())
				{
					ScoreProcessor::fill_selection(img,rect,replacer);
				}
			}
		}
		return edited;
	}

	template<typename T,size_t NL,typename PixelSelectorArrayNLToBool,typename ClusterToTrueIfClear>
	bool clear_clusters_8way(
		::cil::CImg<T>& img,
		std::array<T,NL> replacer,
		PixelSelectorArrayNLToBool ps,
		ClusterToTrueIfClear cl)
	{
		assert(img._spectrum>=NL);
		auto rects=global_select<NL>(img,ps);
		auto clusters=ScoreProcessor::Cluster::cluster_ranges_8way(rects);
		bool edited=false;
		for(auto it=clusters.cbegin();it!=clusters.cend();++it)
		{
			if(cl(*it))
			{
				edited=true;
				for(auto rect:it->get_ranges())
				{
					ScoreProcessor::fill_selection(img,rect,replacer);
				}
			}
		}
		return edited;
	}

	//a function specific to fixing a problem with my scanner
	//eval_side: left is false, true is right
	//eval_direction: from top is false, true is from bottom
	void horizontal_shift(cil::CImg<unsigned char>& img,bool eval_side,bool eval_direction,unsigned char background_threshold);
	
	void vertical_shift(cil::CImg<unsigned char>&img,bool eval_bottom,bool from_right,unsigned char background_threshold);	
	
}
#endif // !1
