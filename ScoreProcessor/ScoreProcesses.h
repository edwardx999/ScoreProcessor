#ifndef SCORE_PROCESSES_H
#define SCORE_PROCESSES_H
#include "CImg.h"
#include "ImageUtils.h"
#include <vector>
#include <memory>
#include "Cluster.h"
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
	void replace_range(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const lower,ImageUtils::Grayscale const upper=255,ImageUtils::Grayscale replacer=ImageUtils::Grayscale::WHITE);
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
	void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const& selection,ImageUtils::ColorRGB const color);
	/*
		Fills selection with a certain color
		@param image, must be 1 channel grayscale
		@param selection, the rectangle that will be filled
		@param gray, the gray the selection will be filled with
	*/
	void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const& selection,ImageUtils::Grayscale const gray);
	/*
		Fills selection with values found at the pointer
		@param image
		@param selection, rectangle to be filled
		@param values
	*/
	template<typename T>
	void fill_selection(::cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> const& selection,T const* const values);
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
	int auto_center_vert(::cimg_library::CImg<unsigned char>& image);
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
	unsigned int cut_page(::cimg_library::CImg<unsigned char> const& image,char const* const filename);

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
	*/
	void auto_rotate(::cimg_library::CImg<unsigned char>& image);
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
		T const* const color,
		float(*color_diff)(T const* const,T const* const),
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
		T const* const color,
		float(*color_diff)(T const* const,T const* const),
		float tolerance,
		bool invert_selection,
		unsigned int min_size,unsigned int max_size,
		T const* const background);
	/*
	*/
	void remove_border(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const color=ImageUtils::Grayscale::BLACK,float const tolerance=0.5);

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
		@param allowable_deviance, the maximum number of pixels that the padding can deviate from optimum
		@param optimal_height, the optimal height for spliced pages
	*/
	unsigned int splice_pages(::std::vector<::std::string> const& filenames,unsigned int const horiz_padding,unsigned int const optimal_padding,unsigned int const min_padding,unsigned int const optimal_height);
	unsigned int splice_pages_greedy(std::string const& output,::std::vector<::std::string> const& filenames,unsigned int optimal_height);
	void compress(::cimg_library::CImg<unsigned char>& image,unsigned int const minPadding,unsigned int const optimalHeight,float min_energy=0);
	::cimg_library::CImg<float> create_vertical_energy(::cimg_library::CImg<unsigned char> const& refImage);
	::cimg_library::CImg<float> create_compress_energy(::cimg_library::CImg<unsigned char> const& refImage);

	void rescale_colors(::cimg_library::CImg<unsigned char>&,unsigned char min,unsigned char mid,unsigned char max=255);
}
#include "ScoreProcessesT.cpp"
#endif // !1
