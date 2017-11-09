#ifndef SCORE_PROCESSES_H
#define SCORE_PROCESSES_H
#include "CImg.h"
#include "ImageUtils.h"
#include <vector>
#include <memory>
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
		Cuts a specified score image into multiple smaller images
		@param image
		@param filename, the start of the filename that the images will be saved as
		@param padding, how much white space will be put at the top and bottom of the pages
		@return the number of images created
	*/
	unsigned int cut_image(::cimg_library::CImg<unsigned char> const& image,char const* const filename);

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

	/*
		Finds all pixels within or without certain tolerance of selected color
		@param image, must be 3 channel RGB
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param color, selected color
		@param ignoreWithinTolerance, whether selected pixels must be within tolerance
		@return where the selected pixels will go as a vector of rectangles pointers
	*/
	::std::vector<ImageUtils::Rectangle<unsigned int>> global_select(::cimg_library::CImg<unsigned char> const& image,float const tolerance,ImageUtils::ColorRGB const color,bool const ignoreWithinTolerance);
	/*
		Finds all pixels within or without certain tolerance of selected color
		@param image, must be 1 channel grayscale
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param gray, selected gray
		@param ignoreWithinTolerance, whether selected pixels must be within tolerance
		@return where the selected pixels will go as a vector of rectangles pointers
	*/
	::std::vector<ImageUtils::Rectangle<unsigned int>> global_select(::cimg_library::CImg<unsigned char> const& image,float const tolerance,ImageUtils::Grayscale const gray,bool const ignoreWithinTolerance);

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
	Paints over black clusters within certain size thresholds with white
	@param image
	@param lowerThreshold, clusters must be above this size to be cleared
	@param upperThreshold, clusters must be below this size to be cleared
	@param tolerance, the tolerance 0.0f-1.0f between colors
	@param ignoreWithinTolerance, whether selected pixels must be within tolerance
	@return 0 if worked, 2 if improper image
	*/
	int clear_clusters(::cimg_library::CImg<unsigned char>& image,unsigned int const lowerThreshold,unsigned int const upperThreshold,float const tolerance,bool const ignoreWithinTolerance);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
		@return 0 if worked, 1 if already padded, 2 if improper image
	*/
	int auto_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const vertPadding,unsigned int const horizPaddingIfTall,unsigned int const minHorizPadding);
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
	unsigned int combine_scores(::std::vector<::std::unique_ptr<::std::string>>& filenames,unsigned int const horizPadding,unsigned int const minVertPadding,unsigned int const maxVertPadding,unsigned int const optimalHeight);
	void compress(::cimg_library::CImg<unsigned char>& image,unsigned int const minPadding,unsigned int const optimalHeight,float min_energy=0);
	::cimg_library::CImg<float> create_vertical_energy(::cimg_library::CImg<unsigned char> const& refImage);
	::cimg_library::CImg<float> create_compress_energy(::cimg_library::CImg<unsigned char> const& refImage);


}
#include "ScoreProcessesT.cpp"
#endif // !1
