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
namespace ScoreProcessor {
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
	void replace_range(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const lower,ImageUtils::Grayscale const upper=255,ImageUtils::Grayscale const replacer=ImageUtils::Grayscale::WHITE);
	/*
		Replaces certainly bright pixels with a color
		@param image, must be 3 channel RGB
		@param lowerBrightness
		@param upperBrightness
		@param replacer
	*/
	void replace_by_brightness(::cimg_library::CImg<unsigned char>& image,unsigned char lowerBrightness,unsigned char upperBrightness=255,ImageUtils::ColorRGB replacer=ImageUtils::ColorRGB::WHITE);
	/*
		Replaces particularly chromatic pixels with a color
		@param image, must be 3 channel RGB
		@param lowerChroma
		@param upperChroma
		@param replacer
	*/
	void replace_by_chroma(::cimg_library::CImg<unsigned char>& image,unsigned char lowerChroma,unsigned char upperChroma=255,ImageUtils::ColorRGB replacer=ImageUtils::ColorRGB::WHITE);
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
		Fills selection with values found at the pointer
		@param image
		@param selection, rectangle to be filled
		@param values
	*/
	template<typename T>
	void fill_selection(::cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> const selection,T const* values);
	/*
		Automatically centers the image horizontally
		@param image
		@return 0 if successful shift, 1 if no shift, 2 if invalid image
	*/
	int auto_center_horiz(::cimg_library::CImg<unsigned char>& image);
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
	void auto_center_vert(::cimg_library::CImg<unsigned char>& image);
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
	/*
		Cuts a specified score page into multiple smaller images
		@param image
		@param filename, the start of the filename that the images will be saved as
		@param padding, how much white space will be put at the top and bottom of the pages
		@return the number of images created
	*/
	unsigned int cut_page(::cimg_library::CImg<unsigned char> const& image,char const* filename);

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
	*/
	void auto_rotate(::cimg_library::CImg<unsigned char>& image,double pixel_prec,double min_angle,double max_angle,double angle_prec);

	/*
		Automatically levels the image.
		@param image
		@param pixel_prec precision when measuring distance from origin
		@param min_angle minimum angle of range to consider rotation (radians), is angle of Hesse normal form
		@param max_angle maximum angle of range to consider rotation (radians), is angle of Hesse normal form
		@param angle_steps number of angle quantization steps between min and max angle
	*/
	void auto_rotate_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps);
	/*
		Automatically deskews the image
		@param image
	*/
	void auto_skew(::cimg_library::CImg<unsigned char>& image);
	/*
		Automatically levels and deskews the image
		@param image
	*/
	void undistort(::cimg_library::CImg<unsigned char>& image);
	template<typename T>
	::std::vector<ImageUtils::Rectangle<unsigned int>> global_select(
		::cimg_library::CImg<T>& image,
		T const* color,
		float(*color_diff)(T const*,T const*),
		float tolerance,
		bool invert_selection);
	/*
		Finds all pixels within or without certain tolerance of selected color
		@param image, must be 3 channel RGB
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param color, selected color
		@param ignoreWithinTolerance, whether selected pixels must be within tolerance
		@return where the selected pixels will go as a vector of rectangles pointers
	*/
	//::std::vector<ImageUtils::Rectangle<unsigned int>> global_select(::cimg_library::CImg<unsigned char> const& image,float const tolerance,ImageUtils::ColorRGB const color,bool const ignoreWithinTolerance);
	/*
		Finds all pixels within or without certain tolerance of selected color
		@param image, must be 1 channel grayscale
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param gray, selected gray
		@param ignoreWithinTolerance, whether selected pixels must be within tolerance
		@return where the selected pixels will go as a vector of rectangles pointers
	*/
	//::std::vector<ImageUtils::Rectangle<unsigned int>> global_select(::cimg_library::CImg<unsigned char> const& image,float const tolerance,ImageUtils::Grayscale const gray,bool const ignoreWithinTolerance);

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
		Clears clusters from the page
		@param image
		@param color, pointer to the color to replace, spectrum of image is used to determine size of color
		@param color_diff, function used to differentiate colors
		@param tolerance, the tolerance between 0.0f-1.0f between colors
		@param min_size
		@param max_size, clusters betwen these sizes will be erased
		@param background, pointer to color to fill with
	*/
	template<typename T>
	void clear_clusters(
		::cimg_library::CImg<T>& image,
		T const* color,
		float(*color_diff)(T const*,T const*),
		float tolerance,
		bool invert_selection,
		unsigned int min_size,unsigned int max_size,
		T const* background);
	template<typename T,typename U>
	void clear_clusters(
		::cimg_library::CImg<T>& img,
		U color,
		float tolerance,
		bool invert_selection,
		unsigned int min_size,
		unsigned int max_size,
		U background);
	/*
		Removes border of the image.
	*/
	void remove_border(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const color=ImageUtils::Grayscale::BLACK,float const tolerance=0.5);

	/*
		Does a flood fill.
	*/
	void flood_fill(::cimg_library::CImg<unsigned char>& image,float const tolerance,ImageUtils::Grayscale const color,ImageUtils::Grayscale const replacer,ImageUtils::Point<unsigned int> start);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param vertical_padding, size in pixels of padding on top and bottom
		@param max_horizontal_padding
		@param min_horizontal_padding
		@param optimal_ratio
		@return 0 if worked, 1 if already padded, 2 if improper image
	*/
	int auto_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const vertical_padding,unsigned int const max_horizontal_padding,unsigned int const min_horizontal_padding,signed int horiz_offset,float optimal_ratio=16.0f/9.0f);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
		@return 0 if worked, 1 if already padded, 2 if improper image
	*/
	int horiz_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const paddingSize);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
		@return 0 if worked, 1 if already padded, 2 if improper image
	*/
	int vert_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const paddingSize);
	/*
		Combines pages together to achieve optimal size for each page
		Aligns right side of each image
		@param filenames, vector of filenames of pages to be combined
		@param horiz_padding, the padding size in pixels between content on the page
		@param optimal_padding, half the optimal padding between pages
		@param optimal_height, the optimal height for spliced pages, if -1, is 4/7 the width of the first page
	*/
	unsigned int splice_pages(
		::std::vector<::std::string> const& filenames,
		unsigned int horiz_padding,
		unsigned int optimal_padding,
		unsigned int min_padding,
		unsigned int optimal_height,
		char const* output,
		unsigned int const starting_index=1);
	unsigned int splice_pages_nongreedy(
		::std::vector<::std::string> const& filenames,
		unsigned int horiz_padding,
		unsigned int optimal_height,
		unsigned int optimal_padding,
		unsigned int min_padding,
		char const* output,
		float excess_weight=10,
		float padding_weight=1,
		unsigned int starting_index=1);
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
template<typename T>
void ScoreProcessor::fill_selection(::cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> const selection,T const* values)
{
	auto const sl=image._spectrum;
	for(unsigned int x=selection.left;x<selection.right;++x)
	{
		for(unsigned int y=selection.top;y<selection.bottom;++y)
		{
			for(unsigned int s=0;s<sl;++s)
			{
				image(x,y,s)=values[s];
			}
		}
	}
}
inline void ScoreProcessor::fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::ColorRGB const color)
{
	assert(image._spectrum>=3);
	assert(selection.right<image._width);
	assert(selection.bottom<image._height);
	for(unsigned int x=selection.left;x<selection.right;++x)
	{
		for(unsigned int y=selection.top;y<selection.bottom;++y)
		{
			image(x,y,0)=color.r;
			image(x,y,1)=color.g;
			image(x,y,2)=color.b;
		}
	}
}
inline void ScoreProcessor::fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::Grayscale const gray)
{
	assert(image._spectrum>=1);
	assert(selection.right<image._width);
	assert(selection.bottom<image._height);
	unsigned char* const data=image.data();
	unsigned int const width=image._width;
	for(unsigned int y=selection.top;y<selection.bottom;++y)
	{
		auto const row=data+y*width;
		for(unsigned int x=selection.left;x<selection.right;++x)
		{
			*(row+x)=gray;
		}
	}
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
template<typename T>
void ScoreProcessor::clear_clusters(
	::cimg_library::CImg<T>& image,
	T const* color,
	float(*color_diff)(T const*,T const*),
	float tolerance,
	bool invert_selection,
	unsigned int min_size,unsigned int max_size,
	T const* background)
{
	auto ranges=global_select(image,color,color_diff,tolerance,invert_selection);
	auto clusters=ScoreProcessor::Cluster::cluster_ranges(ranges);
	for(auto const& cluster:clusters)
	{
		unsigned int size=cluster->size();
		if(min_size<=size&&size<max_size)
		{
			auto const& rects=cluster->get_ranges();
			for(auto const& rect:rects)
			{
				ScoreProcessor::fill_selection(image,rect,background);
			}
		}
	}
}

namespace ScoreProcessor {
	template<typename T,unsigned int num_layers,typename Selector,typename D=unsigned int>
	std::vector<ImageUtils::Rectangle<D>> global_select(
		::cil::CImg<T> const& image,
		Selector keep)
	{
		static_assert(num_layers>0,"Positive number of layers required");
		assert(image._spectrum>=num_layers);
		std::array<T,num_layers> color;
		std::vector<ImageUtils::Rectangle<D>> container;
		for(unsigned int y=0;y<image._height;++y)
		{
			auto const row=image._data+y*image._width;
			for(unsigned int x=0;x<image._width;++x)
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
						container.push_back(ImageUtils::Rectangle<D>{range_start,range_end,y,y+1});
						range_found=0;
						break;
					}
				}
			}
			if(1==range_found)
			{
				container.push_back(ImageUtils::Rectangle<D>{range_start,image._width,y,y+1});
				range_found=0;
			}
		}
		ImageUtils::compress_rectangles(container);
		return container;
	}
	inline std::vector<ImageUtils::Rectangle<unsigned int>> global_select
	(::cil::CImg<unsigned char> const& image,std::function<bool(unsigned char)> const& keep)
	{
		assert(image._spectrum>0);
		std::vector<ImageUtils::Rectangle<unsigned int>> container;
		unsigned int range_found=0,range_start=0,range_end=0;
		for(unsigned int y=0;y<image._height;++y)
		{
			auto row=image._data+y*image._width;
			for(unsigned int x=0;x<image._width;++x)
			{
				switch(range_found)
				{
					case 0:
					{
						if(keep(*(row+x)))
						{
							range_found=1;
							range_start=x;
						}
						break;
					}
					case 1:
					{
						if(!keep(*(row+x)))
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
						container.push_back(ImageUtils::RectangleUINT{range_start,range_end,y,y+1});
						range_found=0;
						break;
					}
				}
			}
			if(1==range_found)
			{
				container.push_back(ImageUtils::RectangleUINT{range_start,image._width,y,y+1});
				range_found=0;
			}
		}
		ImageUtils::compress_rectangles(container);
		return container;
	}
	inline std::vector<ImageUtils::Rectangle<unsigned int>> global_select
	(::cil::CImg<unsigned char> const& image,std::function<bool(ImageUtils::ColorRGB)> const& keep)
	{
		assert(image._spectrum>2);
		std::vector<ImageUtils::Rectangle<unsigned int>> container;
		unsigned int range_found=0,range_start=0,range_end=0;
		auto size=image._height*image._width;
		for(unsigned int y=0;y<image._height;++y)
		{
			auto row=image._data+y*image._width;
			for(unsigned int x=0;x<image._width;++x)
			{
				auto pix=row+x;
				switch(range_found)
				{
					case 0:
					{
						if(keep(ImageUtils::ColorRGB{*pix,*(pix+size),*(pix+2*size)}))
						{
							range_found=1;
							range_start=x;
						}
						break;
					}
					case 1:
					{
						if(!keep(ImageUtils::ColorRGB{*pix,*(pix+size),*(pix+2*size)}))
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
						container.push_back(ImageUtils::RectangleUINT{range_start,range_end,y,y+1});
						range_found=0;
						break;
					}
				}
			}
			if(1==range_found)
			{
				container.push_back(ImageUtils::RectangleUINT{range_start,image._width,y,y+1});
				range_found=0;
			}
		}
		ImageUtils::compress_rectangles(container);
		return container;
	}
	inline void clear_clusters(
		::cil::CImg<unsigned char>& image,
		unsigned char background,
		std::function<bool(unsigned char)> const& selector,
		std::function<bool(ScoreProcessor::Cluster const&)> const& clear
	)
	{
		assert(image._spectrum>0);
		auto rects=global_select(image,selector);
		auto clusters=ScoreProcessor::Cluster::cluster_ranges(rects);
		for(auto const& cl:clusters)
		{
			if(clear(*cl))
			{
				for(auto const& rect:cl->get_ranges())
				{
					ScoreProcessor::fill_selection(image,rect,background);
				}
			}
		}
	}
	inline void clear_clusters(
		::cil::CImg<unsigned char>& image,
		unsigned char background,
		std::function<bool(ImageUtils::ColorRGB)> const& selector,
		std::function<bool(ScoreProcessor::Cluster const&)> const& clear
	)
	{
		assert(image._spectrum>0);
		auto rects=global_select(image,selector);
		auto clusters=ScoreProcessor::Cluster::cluster_ranges(rects);
		for(auto const& cl:clusters)
		{
			if(clear(*cl))
			{
				for(auto const& rect:cl->get_ranges())
				{
					ScoreProcessor::fill_selection(image,rect,background);
				}
			}
		}
	}
	inline void clear_clusters(
		::cimg_library::CImg<unsigned char>& img,
		ImageUtils::Grayscale color,
		float tolerance,
		bool invert_selection,
		unsigned int min_size,
		unsigned int max_size,
		ImageUtils::Grayscale background)
	{
		assert(img._spectrum==1);
		ScoreProcessor::clear_clusters(
			img,
			reinterpret_cast<unsigned char const*>(&color),
			ImageUtils::Grayscale::color_diff,
			tolerance,
			invert_selection,
			min_size,
			max_size,
			reinterpret_cast<unsigned char const*>(&background));
	}
}
template<>
inline void ScoreProcessor::clear_clusters<unsigned char,ImageUtils::ColorRGB>(
	::cimg_library::CImg<unsigned char>& img,
	ImageUtils::ColorRGB color,
	float tolerance,
	bool invert_selection,
	unsigned int min_size,
	unsigned int max_size,
	ImageUtils::ColorRGB background)
{
	assert(img._spectrum==3||img._spectrum==4);
	ScoreProcessor::clear_clusters(
		img,
		reinterpret_cast<unsigned char const*>(&color),
		ImageUtils::ColorRGB::color_diff,
		tolerance,invert_selection,
		min_size,
		max_size,
		reinterpret_cast<unsigned char const*>(&background));
}
template<typename T>
::std::vector<ImageUtils::Rectangle<unsigned int>> ScoreProcessor::global_select(
	::cimg_library::CImg<T>& image,
	T const* color,
	float(*color_diff)(T const*,T const*),
	float tolerance,
	bool invert_selection)
{
	vector<ImageUtils::Rectangle<unsigned int>> container;
	//std::unique_ptr<T[]> pixel_color=make_unique<T[]>(3);
	unique_ptr<T[]> pixel_color(new T[image._spectrum]);
	unsigned int range_found=0,range_start=0,range_end=0;
	for(unsigned int y=0;y<image._height;++y)
	{
		for(unsigned int x=0;x<image._width;++x)
		{
			auto load_color=[&]()
			{
				for(unsigned int i=0;i<image._spectrum;++i)
				{
					pixel_color[i]=image(x,y,i);
				}
			};
			switch(range_found)
			{
				case 0:
				{
					load_color();
					if((color_diff(color,pixel_color.get())<=tolerance)^invert_selection)
					{
						range_found=1;
						range_start=x;
					}
					break;
				}
				case 1:
				{
					load_color();
					if((color_diff(color,pixel_color.get())>tolerance)^invert_selection)
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
					container.push_back(RectangleUINT{range_start,range_end,y,y+1});
					range_found=0;
					break;
				}
			}
		}
		if(1==range_found)
		{
			container.push_back(RectangleUINT{range_start,image._width,y,y+1});
			range_found=0;
		}
	}
	ImageUtils::compress_rectangles(container);
	return container;
}
/*
template<typename T,typename Color>
::std::vector<ImageUtils::Rectangle<unsigned int>> ScoreProcessor::global_select(
::cimg_library::CImg<T>& image,
Color color,
float tolerance,
bool invert_selection) {
vector<ImageUtils::Rectangle<unsigned int>> container;
Color pixel_color;
unsigned int rangeFound=0,rangeStart=0,rangeEnd=0;
for(unsigned int y=0;y<image._height;++y)
{
for(unsigned int x=0;x<image._width;++x)
{
switch(rangeFound)
{
case 0: {
#define load_color() for(unsigned int i=0;i<image._spectrum;++i) reinterpret_cast<T*>(&pixel_color)[i]=image(x,y,i);
load_color();
if((color.difference(pixel_color)<=tolerance)^invert_selection)
{
rangeFound=1;
rangeStart=x;
}
break;
}
case 1: {
load_color();
if((color.difference(pixel_color)>tolerance)^invert_selection)
{
rangeFound=2;
rangeEnd=x;
}
else
{
break;
}
}
#undef load_color
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
*/
#endif // !1
