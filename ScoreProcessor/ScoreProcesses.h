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
#include "../NeuralNetwork/neural_net.h"
namespace ScoreProcessor {

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
	unsigned int cut_page(::cimg_library::CImg<unsigned char> const& image,char const* filename,cut_heuristics const& ch={1000,80,20,0,128});

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
