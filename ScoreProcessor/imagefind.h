#ifndef IMAGEFIND_H
#define IMAGEFIND_H
#include "CImg.h"
#include <assert.h>
namespace ScoreProcessor {
	template<typename T>
	void replace(cimg_library::CImg<T>& img,cimg_library::CImg<T> const& target,cimg_library::CImg<T> const& replacer);

	template<typename T>
	void replace(cimg_library::CImg<T>& img,cimg_library::CImg<T> const& target,cimg_library::CImg<T> const& replacer)
	{
		assert(replacer._width==target._width);
		assert(replacer._height==target._height);
		assert(img._spectrum==target._spectrum);
		
	}
}
#endif