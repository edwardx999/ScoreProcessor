#ifndef IMAGE_MATH_H
#define IMAGE_MATH_H
#include "CImg.h"
#include "ImageUtils.h"
#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923
namespace cimg_library {
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
	ImageUtils::ColorRGB averageColor(::cimg_library::CImg<unsigned char const>& image);
	/*
	Returns the average grayness of the image
	@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale averageGray(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the darkest color in the image
	@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB darkestColor(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the darkest gray in the image
	@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale darkestGray(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the brightest color in the image
	@param image, must be 3 channel RGB
	*/
	ImageUtils::ColorRGB brightestColor(::cimg_library::CImg<unsigned char> const& image);
	/*
	Returns the brightest gray in the image
	@param image, must be 1 channel grayscale
	*/
	ImageUtils::Grayscale brightestGray(::cimg_library::CImg<unsigned char> const& image);
	/*
	Converts image to a 1 channel grayscale image
	@param image, must be 3 channel RGB
	*/
	::cimg_library::CImg<unsigned char> get_grayscale(::cimg_library::CImg<unsigned char> const& image);
	/*
	*/
	::cimg_library::CImg<unsigned char> get_grayscale_simple(::cimg_library::CImg<unsigned char> const& image);
	void remove_transparency(::cimg_library::CImg<unsigned char>& img,unsigned char threshold,ImageUtils::ColorRGB replacer);
	/*
		Performs a HoughTransform
		Add pi/2 to the hough theta to get the line horizontal theta
		By default, 1 pixel precision, tests vertical lines from angle ~-pi/4 to ~pi/4
	*/
	::cimg_library::CImg<unsigned int> get_hough(::cimg_library::CImg<unsigned char> const& gradImage,float lowerAngle=-0.8f,float upperAngle=0.8f,float angleStep=0.05f,unsigned int precision=1U);
	float findHoughAngle(::cimg_library::CImg<unsigned int>& hough,float lowerAngle,float angleStep,unsigned int avgHowMany=10U);
}
#endif // !IMAGE_MATH_H
