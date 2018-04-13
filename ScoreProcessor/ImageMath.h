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
#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923
#define M_PI_4	0.78539816339744830962
#define M_PI_6	0.52359877559829887308
#define RAD_DEG 180.0/M_PI
#define DEG_RAD M_PI/180.0
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

	void remove_transparency(::cimg_library::CImg<unsigned char>& img,unsigned char threshold,ImageUtils::ColorRGB replacer);

	class HoughArray:public CImg<unsigned int> {
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
		unsigned int& operator()(double theta,double r);
		double angle() const;
		std::vector<ImageUtils::line_norm<double>> top_lines(size_t num) const;
	};
}
#endif // !IMAGE_MATH_H
