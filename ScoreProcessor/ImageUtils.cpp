#include "stdafx.h"
#include "ImageUtils.h"
#include <algorithm>
namespace ImageUtils {
	ColorRGBA ColorRGB::toRGBA() const {
		return ColorRGBA{r,g,b,255};
	}
	float ColorRGB::colorDiff(ColorRGB other) {
		return RGBColorDiff(*this,other);
	}
	ColorHSV ColorRGB::toHSV() const {
		ColorHSV ret;
		ret.h=std::max({r,g,b})-std::min({r,g,b});
		return ret;
	}
	unsigned char ColorRGB::brightness() const {
		return ImageUtils::brightness(*this);
	}
	ColorRGB ColorHSV::toRGB() const {
		ColorRGB ret;
		return ret;
	}
	ColorRGB ColorRGBA::toRGB() const {
		return ColorRGB{r,g,b};
	}
	float const maxDif=255.0f*255.0f*3.0f;
	float RGBColorDiff(ColorRGB color1,ColorRGB color2) {
		float rdif=color1.r>color2.r?color1.r-color2.r:color2.r-color1.r;
		float gdif=color1.g>color2.g?color1.g-color2.g:color2.g-color1.g;
		float bdif=color1.b>color2.b?color1.b-color2.b:color2.b-color1.b;
		return (rdif*rdif+gdif*gdif+bdif*bdif)/maxDif;
	}
	float const twofivefivesquared=255.0f*255.0f;
	float grayDiff(Grayscale g1,Grayscale g2) {
		float dif=(g1>g2?g1-g2:g2-g1);
		return dif*dif/twofivefivesquared;
	}
}