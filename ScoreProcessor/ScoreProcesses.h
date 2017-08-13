#pragma once
#include "CImg.h"
#include "ImageUtils.h"
#include <vector>
#include <memory>
namespace ScoreProcessor {
	/*
		Returns the average RGB color of the image
		@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB averageColor(cimg_library::CImg<unsigned char>& image);
	/*
		Returns the average grayness of the image
		@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale averageGray(cimg_library::CImg<unsigned char>& image);
	/*
		Returns the darkest color in the image
		@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB darkestColor(cimg_library::CImg<unsigned char>& image);
	/*
		Returns the darkest gray in the image
		@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale darkestGray(cimg_library::CImg<unsigned char>& image);
	/*
		Returns the brightest color in the image
		@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB brightestColor(cimg_library::CImg<unsigned char>& image);
	/*
		Returns the brightest gray in the image
		@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale brightestGray(cimg_library::CImg<unsigned char>& image);
	/*
		Converts image to a 1 channel grayscale image
		@param image, must be 3 channel RGB
	*/
	cimg_library::CImg<unsigned char> get_grayscale(cimg_library::CImg<unsigned char>& image);
	/*
		Reduces colors to two colors
		@param image, must be a 3 channel RGB image
		@param middleColor, pixels darker than this color will be set to lowColor, brighter to highColor
		@param lowColor
		@param highColor
	*/
	void binarize(cimg_library::CImg<unsigned char>& image,ImageUtils::ColorRGB const middleColor,ImageUtils::ColorRGB const lowColor,ImageUtils::ColorRGB const highColor);
	/*
		Reduces colors to two colors
		@param image, must be a 1 channel grayscale image
		@param middleGray, pixels darker than this gray will be set to lowGray, brighter to highGray
		@param lowGray
		@param highGray
	*/
	void binarize(cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const middleGray,ImageUtils::Grayscale const lowGray,ImageUtils::Grayscale const highGray);

	/*
		Shifts selection over while leaving rest unchanged
		@param image
		@param selection, the rectangle that will be shifted
		@param shiftx, the number of pixels the selection will be translated in the x direction
		@param shiftx, the number of pixels the selection will be translated in the y direction
	*/
	void copyShiftSelection(cimg_library::CImg<unsigned char>& image,ImageUtils::RectangleUINT const& selection,int const shiftx,int const shifty);
	/*
		Fills selection with a certain color
		@param image, must be 3 channel RGB
		@param selection, the rectangle that will be filled
		@param color, the color the selection will be filled with
	*/
	void fillSelection(cimg_library::CImg<unsigned char>& image,ImageUtils::RectangleUINT const& selection,ImageUtils::ColorRGB const color);
	/*
		Fills selection with a certain color
		@param image, must be 1 channel grayscale
		@param selection, the rectangle that will be filled
		@param gray, the gray the selection will be filled with
	*/
	void fillSelection(cimg_library::CImg<unsigned char>& image,ImageUtils::RectangleUINT const& selection,ImageUtils::Grayscale const gray);

	/*
		Automatically centers the image horizontally
		@param image
		@return 0 if successful shift, 1 if no shift, 2 if invalid image
	*/
	int autoCenterHoriz(cimg_library::CImg<unsigned char>& image);
	/*
		Finds the left side of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the left side
		@return the x-coordinate of the left side
	*/
	unsigned int findLeft(cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the left side of the the image
		@param image, must 1 channel grayscale
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the left side
		@return the x-coordinate of the left side
	*/
	unsigned int findLeft(cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);
	/*
		Finds the right side of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the right side
		@return the x-coordinate of the right side
	*/
	unsigned int findRight(cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the left side of the the image
		@param image, must 1 channel grayscale
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the right side
		@return the x-coordinate of the right side
	*/
	unsigned int findRight(cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);

	/*
		Automatically centers the image vertically
		@param image
		@return 0 if successful shift, 1 if no shift, 2 if invalid image
	*/
	int autoCenterVert(cimg_library::CImg<unsigned char>& image);
	/*
		Finds the top of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the top
		@return the y-coordinate of the top
	*/
	unsigned int findTop(cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the top of the the image
		@param image, must 1 channel RGB
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the top
		@return the y-coordinate of the top
	*/
	unsigned int findTop(cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);
	/*
		Finds the bottom of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the bottom
		@return the y-coordinate of the bottom
	*/
	unsigned int findBottom(cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the bottom of the the image
		@param image, must 1 channel RGB
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the bottom
		@return the y-coordinate of the bottom
	*/
	unsigned int findBottom(cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);
	
	/*
		Creates a profile of the left side of the image
		@param image, must be 3 channel RGB
		@param container, where the profile will be stored, as a vector of x coordinates of the left side
		@param background, the background color of the image
	*/
	void buildLeftProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the left side of the image
		@param image, must be 1 channel grayscale
		@param container, where the profile will be stored, as a vector of x coordinates of the left side
		@param background, the background gray of the image
	*/
	void buildLeftProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the right side of the image
		@param image, must be 3 channel RGB
		@param container, where the profile will be stored, as a vector of x coordinates of the right side
		@param background, the background color of the image
	*/
	void buildRightProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the right side of the image
		@param image, must be 1 channel grayscale
		@param container, where the profile will be stored, as a vector of x coordinates of the right side
		@param background, the background gray of the image
	*/
	void buildRightProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the top of the image
		@param image, must be 3 channel RGB
		@param container, where the profile will be stored, as a vector of y coordinates of the top
		@param background, the background color of the image
	*/
	void buildTopProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the top of the image
		@param image, must be 1 channel grayscale
		@param container, where the profile will be stored, as a vector of y coordinates of the top
		@param background, the background gray of the image
	*/
	void buildTopProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the bottom of the image
		@param image, must be 3 channel RGB
		@param container, where the profile will be stored, as a vector of y coordinates of the bottom
		@param background, the background color of the image
	*/
	void buildBottomProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the bottom of the image
		@param image, must be 1 channel grayscale
		@param container, where the profile will be stored, as a vector of y coordinates of the bottom
		@param background, the background gray of the image
	*/
	void buildBottomProfile(cimg_library::CImg<unsigned char>& image,::std::vector<unsigned int>& container,ImageUtils::Grayscale const background);
	/*
		Selects the outside (non-systems) of a score image
		@param image, must be 1 channel grayscale
		@param resultContainer, where the selected rectangles go
	*/
	void selectOutside(cimg_library::CImg<unsigned char>& image,::std::vector<::std::unique_ptr<ImageUtils::RectangleUINT>>& resultContainer);
	/*
		Cuts a specified score image into multiple smaller images
		@param image
		@param filename, the start of the filename that the images will be saved as
		@param padding, how much white space will be put at the top and bottom of the pages
		@return the number of images created
	*/
	unsigned int cutImage(cimg_library::CImg<unsigned char>& image,char const* filename);

	/*
		Finds the line that is the top of the score image
		@param image
		@return a line defining the top of the score image
	*/
	ImageUtils::line<unsigned int> findTopLine(cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the bottom of the score image
		@param image
		@return a line defining the bottom of the score image
	*/
	ImageUtils::line<unsigned int> findBottomLine(cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the left of the score image
		@param image
		@return a line defining the left of the score image
	*/
	ImageUtils::line<unsigned int> findLeftLine(cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the right of the score image
		@param image
		@return a line defining the right of the score image
	*/
	ImageUtils::line<unsigned int> findRightLine(cimg_library::CImg<unsigned char> const& image);
	/*
		Automatically levels the image
		@param image
	*/
	void autoRotate(cimg_library::CImg<unsigned char>& image);
	/*
		Automatically deskews the image
		@param image
	*/
	void autoSkew(cimg_library::CImg<unsigned char>& image);
	/*
		Automatically levels and deskews the image
		@param image
	*/
	void undistort(cimg_library::CImg<unsigned char>& image);

	/*
		Finds all pixels within or without certain tolerance of selected color
		@param image, must be 3 channel RGB
		@param resultContainer, where the selected pixels will go as a vector of rectangles
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param color, selected color
		@param ignoreWithinTolerance, whether selected pixels must be within tolerance
	*/
	void globalSelect(cimg_library::CImg<unsigned char> const& image,::std::vector<::std::shared_ptr<ImageUtils::RectangleUINT>>& resultContainer,float const tolerance,ImageUtils::ColorRGB const color,bool const ignoreWithinTolerance);
	/*
		Finds all pixels within or without certain tolerance of selected color
		@param image, must be 1 channel grayscale
		@param resultContainer, where the selected pixels will go as a vector of rectangles
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param gray, selected gray
		@param ignoreWithinTolerance, whether selected pixels must be within tolerance
	*/
	void globalSelect(cimg_library::CImg<unsigned char> const& image,::std::vector<::std::shared_ptr<ImageUtils::RectangleUINT>>& resultContainer,float const tolerance,ImageUtils::Grayscale const gray,bool const ignoreWithinTolerance);

	/*
		Flood selects from point
		@param image, must be 1 channel grayscale
		@param resultContainer, where the selected pixels will go as a vector of rectangles
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param gray, selected gray
		@param start, seed point of flood fill
	*/
	void floodSelect(cimg_library::CImg<unsigned char> const& image,::std::vector<::std::shared_ptr<ImageUtils::RectangleUINT>>& resultContainer,float const tolerance,ImageUtils::Grayscale const gray,ImageUtils::vec2_t<unsigned int> start);
	/*
	Paints over black clusters within certain size thresholds with white
	@param image
	@param lowerThreshold, clusters must be above this size to be cleared
	@param upperThreshold, clusters must be below this size to be cleared
	@param tolerance, the tolerance 0.0f-1.0f between colors
	@param ignoreWithinTolerance, whether selected pixels must be within tolerance
	@return 0 if worked, 2 if improper image
	*/
	int clusterClear(cimg_library::CImg<unsigned char>& image,unsigned int const lowerThreshold,unsigned int const upperThreshold,float const tolerance,bool const ignoreWithinTolerance);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
		@return 0 if worked, 2 if improper image
	*/
	int autoPadding(cimg_library::CImg<unsigned char>& image,int const paddingSize);
	unsigned int combinescores(::std::vector<::std::unique_ptr<::std::string>>& filenames,unsigned int const padding,unsigned int const optimalHeight);
	void compress(cimg_library::CImg<unsigned char>& image,int const minPadding,unsigned int const optimalHeight);
}
