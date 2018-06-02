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
#include "stdafx.h"
#include "ImageUtils.h"
#include <algorithm>
#include "moreAlgorithms.h"
namespace ImageUtils {
#pragma region ColorRGB

	float ColorRGB::difference(ColorRGB other) {
		float
			rdif=misc_alg::abs_dif(r,other.r),
			gdif=misc_alg::abs_dif(g,other.g),
			bdif=misc_alg::abs_dif(b,other.b);
		return (rdif*rdif+gdif*gdif+bdif*bdif)/(255.0f*255.0f*3.0f);
	}

	float ColorRGB::color_diff(unsigned char const* c1,unsigned char const* c2) {
		float
			rdif=misc_alg::abs_dif(c1[0],c2[0]),
			gdif=misc_alg::abs_dif(c1[1],c2[1]),
			bdif=misc_alg::abs_dif(c1[2],c2[2]);
		return (rdif*rdif+gdif*gdif+bdif*bdif)/(255.0f*255.0f*3.0f);
	}
	ColorRGB const ColorRGB::WHITE={255,255,255};
	ColorRGB const ColorRGB::BLACK={0,0,0};
#pragma endregion
#pragma region Grayscale
	float const max_dif_gs=255.0f;
	float Grayscale::color_diff(unsigned char const* c1,unsigned char const* c2) {
		float dif=misc_alg::abs_dif(*c1,*c2);
		return dif/max_dif_gs;
	}
	float const twofivefivesquared=255.0f*255.0f;
	float gray_diff(Grayscale g1,Grayscale g2) {
		float dif=(g1>g2?g1-g2:g2-g1);
		return dif*dif/twofivefivesquared;
	}

	Grayscale const Grayscale::WHITE=255;
	Grayscale const Grayscale::BLACK=0;

	float Grayscale::difference(Grayscale other) {
		return static_cast<float>(misc_alg::abs_dif(*this,other))/255.0f;
	}
#pragma endregion
	ColorRGB ColorRGBA::toRGB() const {
		return ColorRGB{r,g,b};
	}

	ColorHSV const ColorHSV::WHITE={0,0,255};
	ColorHSV const ColorHSV::BLACK={0,0,0};
}