#include "stdafx.h"
#include "ImageUtils.h"
#include <algorithm>
#include "moreAlgorithms.h"
namespace ImageUtils {
#pragma region ColorRGB
	ColorRGB::operator ColorRGBA() const {
		return ColorRGBA{r,g,b,255};
	}
	ColorRGB::operator ColorHSV() const {
		ColorHSV ret;
		ret.h=std::max({r,g,b})-std::min({r,g,b});
		return ret;
	}
	ColorRGB::operator Grayscale() const {
		return static_cast<Grayscale>((r*0.2126f+g*0.7152f+b*0.0722f));
	}
	float const max_dif_rgb=255.0f*255.0f*3.0f;
	float ColorRGB::color_diff(unsigned char const* const c1,unsigned char const* const c2) {
		float
			rdif=misc_alg::abs_dif(c1[0],c2[0]),
			gdif=misc_alg::abs_dif(c1[1],c2[1]),
			bdif=misc_alg::abs_dif(c1[2],c2[2]);
		return (rdif*rdif+gdif*gdif+bdif*bdif)/max_dif_rgb;
	}
	ColorRGB const ColorRGB::WHITE={255,255,255};
	ColorRGB const ColorRGB::BLACK={0,0,0};
#pragma endregion
#pragma region Grayscale
	float const max_dif_gs=255.0f;
	float Grayscale::color_diff(unsigned char const* const c1,unsigned char const* const c2) {
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
#pragma endregion
	ColorRGB ColorRGBA::toRGB() const {
		return ColorRGB{r,g,b};
	}

	ColorHSV const ColorHSV::WHITE={0,0,255};
	ColorHSV const ColorHSV::BLACK={0,0,0};
}