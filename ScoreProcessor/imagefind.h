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