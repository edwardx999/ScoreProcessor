#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H
//#include <stdint.h>
#include <vector>
#include <memory>
#include <iostream>
namespace ImageUtils {
	template<typename T=unsigned int>
	struct vertical_line {
		T x,top,bottom;
	};
	template<typename T=unsigned int>
	struct horizontal_line {
		T y,left,right;
	};
	template<typename T=unsigned int>
	struct vec2_t {
		T x,y;
		/*template<typename U>
		vec2_t(vec2_t<U> const& other);
		template<typename U>
		vec2_t<T>& operator=(vec2_t<U> const& other);*/
		vec2_t<T> operator+(vec2_t<T> const& other);
		vec2_t<T> operator-(vec2_t<T> const& other);
		vec2_t<T>& operator+=(vec2_t<T> const& other);
		vec2_t<T>& operator-=(vec2_t<T> const& other);
	};
	template<typename T>
	using Point=vec2_t<T>;
	using PointUINT=Point<unsigned int>;
	template<typename T=unsigned int>
	struct line {
		Point<T> a,b;
		template<typename U>
		friend ::std::ostream& operator<<(::std::ostream& os,line<U> const& aline);
	};
	template<typename T=float>
	struct line_norm {
		T theta;//the normal has this angle with the x-axis
		T r;//the line is r away from the origin
	};
	template<typename T=unsigned int>
	struct Rectangle {
		T left,right,top,bottom;
		bool operator<(Rectangle<T> const& other) const;
		bool operator>(Rectangle<T> const& other) const;
		template<typename U=T>
		U area() const;
		template<typename U=T>
		U perimeter() const;
		template<typename U=T>
		U height() const;
		template<typename U=T>
		U width() const;
		template<typename U=T>
		Point<U> center() const;
		bool is_bordering_vert(Rectangle<T> const& other) const;
		bool overlaps_x(Rectangle<T> const& other) const;
		bool is_bordering_horiz(Rectangle<T> const& other) const;
		bool overlaps_y(Rectangle<T> const& other) const;
		bool intersects(Rectangle<T> const& other) const;
		line<T> diagonal();
		template<typename U>
		friend ::std::ostream& operator<<(::std::ostream& os,Rectangle<U> const& rect);
	};
	template<typename T>
	void split_horiz(Rectangle<T>* orig,Rectangle<T>* buffer,unsigned int numRects);
	template<typename T,typename alloc>
	void compress_rectangles(::std::vector<Rectangle<T>,alloc>& container);
	using RectangleUINT=Rectangle<unsigned int>;

	struct ColorRGBA;
	struct ColorRGB;
	struct ColorHSV;
	struct Grayscale;

	unsigned char brightness(ColorRGB color);
	struct ColorRGB {
		static ColorRGB const WHITE;
		static ColorRGB const BLACK;

		unsigned char r,g,b;
		explicit operator ColorRGBA() const;
		explicit operator ColorHSV() const;
		explicit operator Grayscale() const;
		unsigned char brightness() const;
		//float color_diff(ColorRGB const other);

		static float color_diff(unsigned char const* const,unsigned char const* const);
	};

	struct ColorHSV {
		unsigned char h,s,v;

		explicit operator ColorRGBA() const;
		explicit operator ColorRGB() const;
		explicit operator Grayscale() const;
		static ColorHSV const WHITE;
		static ColorHSV const BLACK;
	};

	struct ColorRGBA {
		unsigned char r,g,b,a;
		ColorRGB toRGB() const;
	};

	struct Grayscale {
		unsigned char value;
		inline operator unsigned char() const { return value; }
		Grayscale(unsigned char const val):value(val) {}
		Grayscale():Grayscale(0) {}

		inline Grayscale& operator=(unsigned char const val) { value=val; return *this; }
		static Grayscale const WHITE;
		static Grayscale const BLACK;

		static float color_diff(unsigned char const* const,unsigned char const* const);
	};

	float gray_diff(Grayscale g1,Grayscale g2);

	inline unsigned char brightness(Grayscale g) { return g; }
	inline unsigned char brightness(ColorRGB color) {
		return static_cast<unsigned char>(color.r*0.2126f+color.g*0.7152f+color.b*0.0722f);
	}
	inline unsigned char ColorRGB::brightness() const {
		return ImageUtils::brightness(*this);
	}
	/*
	inline float ColorRGB::color_diff(ColorRGB const other) {
		return ColorRGB::color_diff(reinterpret_cast<unsigned char const* const>(this),
			reinterpret_cast<unsigned char const* const>(&other));
	}
	*/
}
#include "ImageUtilsTemplate.cpp"
#endif