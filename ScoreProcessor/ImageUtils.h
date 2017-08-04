#pragma once
#include <vector>
#include <memory>
#include <iostream>
namespace ImageUtils {
	typedef struct vec2_t {
		int x,y;
		vec2_t operator+(vec2_t const& other);
		vec2_t operator-(vec2_t const& other);
		vec2_t& operator+=(vec2_t const& other);
		vec2_t& operator-=(vec2_t const& other);
	}Point;
	struct line {
		Point a,b;
	};
	struct Rectangle {
		int left,right,top,bottom;
		bool operator<(Rectangle const& other) const;
		bool operator>(Rectangle const& other) const;
		int area() const;
		int perimeter() const;
		bool isBorderingVert(Rectangle const& other) const;
		bool overlapsX(Rectangle const& other) const;
		bool isBorderingHoriz(Rectangle const& other) const;
		bool overlapsY(Rectangle const& other) const;
		bool intersects(Rectangle const& other) const;
		line diagonal();
		friend std::ostream& operator<<(std::ostream& os,Rectangle const& rect);
	};
	void splitHoriz(Rectangle* orig,Rectangle* buffer,int numRects);
	void compressRectangles(std::vector<std::shared_ptr<Rectangle>>& container);

	struct ColorRGBA;
	struct ColorRGB;
	struct ColorHSV;
	struct ColorRGB {
		unsigned char r,g,b;
		ColorRGBA toRGBA() const;
		ColorHSV toHSV() const;
		unsigned char brightness() const;
	};
	inline unsigned char brightness(unsigned char r,unsigned char g,unsigned char b) {
		return static_cast<unsigned char>((unsigned short(r)+unsigned short(g)+unsigned short(b))/3);
	}
	extern const ColorRGB WHITE_RGB;
	extern const ColorRGB BLACK_RGB;
	struct ColorHSV {
		unsigned char h,s,v;
		ColorRGB toRGB() const;
	};
	extern const ColorHSV WHITE_HSV;
	extern const ColorHSV BLACK_HSV;
	struct ColorRGBA {
		unsigned char r,g,b,a;
		ColorRGB toRGB() const;
	};
	const float maxDif=255.0f*255.0f*3.0f;
	float RGBColorDiff(unsigned char r1,unsigned char g1,unsigned char b1,unsigned char r2,unsigned char g2,unsigned char b2);

	typedef unsigned char Grayscale;
	extern const Grayscale WHITE_GRAYSCALE;
	extern const Grayscale BLACK_GRAYSCALE;
	const float twofivefivesquared=255.0f*255.0f;
	float grayDiff(Grayscale g1,Grayscale g2);
}