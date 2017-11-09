#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H
//#include <stdint.h>
#include <vector>
#include <memory>
#include <iostream>
namespace ImageUtils {
	template<typename T>
	struct vec2_t {
		T x,y;
		vec2_t<T> operator+(vec2_t<T> const& other);
		vec2_t<T> operator-(vec2_t<T> const& other);
		vec2_t<T>& operator+=(vec2_t<T> const& other);
		vec2_t<T>& operator-=(vec2_t<T> const& other);
	};
	template<typename T>
	using Point=vec2_t<T>;
	using PointUINT=Point<unsigned int>;
	template<typename T>
	struct line {
		Point<T> a,b;
		template<typename U>
		friend ::std::ostream& operator<<(::std::ostream& os,line<U> const& aline);
	};
	template<typename T>
	struct line_norm {
		T theta;//the normal has this angle with the x-axis
		T r;//the line is r away from the origin
	};
	template<typename T>
	struct Rectangle {
		T left,right,top,bottom;
		bool operator<(Rectangle<T> const& other) const;
		bool operator>(Rectangle<T> const& other) const;
		T area() const;
		T perimeter() const;
		T height() const;
		T width() const;
		bool isBorderingVert(Rectangle<T> const& other) const;
		bool overlapsX(Rectangle<T> const& other) const;
		bool isBorderingHoriz(Rectangle<T> const& other) const;
		bool overlapsY(Rectangle<T> const& other) const;
		bool intersects(Rectangle<T> const& other) const;
		line<T> diagonal();
		template<typename U>
		friend ::std::ostream& operator<<(::std::ostream& os,Rectangle<U> const& rect);
	};
	template<typename T>
	void split_horiz(Rectangle<T>* orig,Rectangle<T>* buffer,unsigned int numRects);
	template<typename T>
	void compress_rectangles(::std::vector<Rectangle<T>,::std::allocator<Rectangle<T>>>& container);
	using RectangleUINT=Rectangle<unsigned int>;

	struct ColorRGBA;
	struct ColorRGB;
	struct ColorHSV;

	unsigned char brightness(ColorRGB color);
	struct ColorRGB {
		static ColorRGB const WHITE;
		static ColorRGB const BLACK;

		unsigned char r,g,b;
		ColorRGBA toRGBA() const;
		ColorHSV toHSV() const;
		unsigned char brightness() const;
		float colorDiff(ColorRGB other);
	};

	struct ColorHSV {
		unsigned char h,s,v;
		ColorRGB toRGB() const;
		static ColorHSV const WHITE;
		static ColorHSV const BLACK;
	};

	struct ColorRGBA {
		unsigned char r,g,b,a;
		ColorRGB toRGB() const;
	};

	float RGBColorDiff(ColorRGB color1,ColorRGB color2);

	struct Grayscale {
		unsigned char value;
		inline operator unsigned char() const { return value; }
		Grayscale(unsigned char val) { value=val; }
		Grayscale():Grayscale(0) {}
		inline Grayscale& operator=(unsigned char val) { value=val; }
		static Grayscale const WHITE;
		static Grayscale const BLACK;
	};

	float grayDiff(Grayscale g1,Grayscale g2);

	inline unsigned char brightness(Grayscale g) { return g; }
	inline unsigned char brightness(ColorRGB color) {
		return static_cast<unsigned char>((uint16_t(color.r)+uint16_t(color.g)+uint16_t(color.b))/3);
	}
	inline unsigned char ColorRGB::brightness() const {
		return ImageUtils::brightness(*this);
	}
}
#include "ImageUtilsTemplate.cpp"
#endif