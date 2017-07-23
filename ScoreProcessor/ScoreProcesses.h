#pragma once
#include "CImg.h"
#include "ImageUtils.h"
namespace ScoreProcessor {
	int copyShiftSelection(cimg_library::CImg<unsigned char>* const image,ImageUtils::Rectangle const* const selection,int const shiftx,unsigned int const shifty);
	int fillSelection(cimg_library::CImg<unsigned char>* const image,ImageUtils::Rectangle const* const selection,ImageUtils::ColorRGB const color);
	int autoCenterHoriz(cimg_library::CImg<unsigned char>* const image);
	int findLeft(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	int findRight(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	int autoCenterVert(cimg_library::CImg<unsigned char>* const image);
	int findTop(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	int findBottom(cimg_library::CImg<unsigned char>* const image,int* const where,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	int cutImage(cimg_library::CImg<unsigned char>* const image,char const* filename);
	int autoRotate(cimg_library::CImg<unsigned char>* const image);
	int autoSkew(cimg_library::CImg<unsigned char>* const image);
	int undistort(cimg_library::CImg<unsigned char>* const image);
	int clusterClear(cimg_library::CImg<unsigned char>* const image,unsigned int const tolerance,bool const mode);
}
