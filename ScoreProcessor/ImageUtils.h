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
#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H
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
		float difference(ColorRGB other);

		static float color_diff(unsigned char const*,unsigned char const*);
	};

	struct ColorHSV {
		unsigned char h,s,v;

		operator ColorRGBA() const;
		operator ColorRGB() const;
		operator Grayscale() const;
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
		float difference(Grayscale other);

		static float color_diff(unsigned char const*,unsigned char const*);
	};

	float gray_diff(Grayscale g1,Grayscale g2);

	inline unsigned char brightness(Grayscale g) { return g; }
	inline unsigned char brightness(ColorRGB color)
	{
		return static_cast<unsigned char>(color.r*0.2126f+color.g*0.7152f+color.b*0.0722f);
	}
	inline unsigned char ColorRGB::brightness() const
	{
		return ImageUtils::brightness(*this);
	}
}
namespace ImageUtils {
	template<typename T>
	bool Rectangle<T>::operator<(Rectangle<T> const& other) const
	{
		if(left<other.left) return true;
		if(left>other.left) return false;
		return top<other.top;
	}
	template<typename T>
	bool Rectangle<T>::operator>(Rectangle<T> const& other) const
	{
		if(left>other.left) return true;
		if(left<other.left) return false;
		return top>other.top;
	}
	template<typename T> template<typename U>
	U Rectangle<T>::area() const
	{
		return (right-left)*(bottom-top);
	}
	template<typename T> template<typename U>
	U Rectangle<T>::perimeter() const
	{
		return 2*(right-left+bottom-top);
	}
	template<typename T> template<typename U>
	U Rectangle<T>::height() const
	{
		return bottom-top;
	}
	template<typename T> template<typename U>
	U Rectangle<T>::width() const
	{
		return right-left;
	}
	template<typename T> template<typename U>
	Point<U> Rectangle<T>::center() const
	{
		return {U(left+right)/2,U(top+bottom)/2};
	}
	template<typename T>
	bool Rectangle<T>::is_bordering_vert(Rectangle<T> const& other) const
	{
		return (other.bottom==top||other.top==bottom)&&overlaps_x(other);
	}
	template<typename T>
	bool Rectangle<T>::overlaps_x(Rectangle<T> const& other) const
	{
		return (left<other.right)&&(right>other.left);
	}
	template<typename T>
	bool Rectangle<T>::is_bordering_horiz(Rectangle<T> const& other) const
	{
		return (other.left==right||other.right==left)&&overlaps_y(other);
	}
	template<typename T>
	bool Rectangle<T>::overlaps_y(Rectangle<T> const& other) const
	{
		return (top<other.bottom)&&(bottom>other.top);
	}
	template<typename T>
	bool Rectangle<T>::intersects(Rectangle<T> const& other) const
	{
		return overlaps_x(other)&&overlaps_y(other);
	}
	template<typename T>
	line<T> Rectangle<T>::diagonal()
	{
		return {{left,top},{right,bottom}};
	}
	template<typename T>
	std::ostream& operator<<(std::ostream& os,Rectangle<T> const& rect)
	{
		return os<<"[("<<rect.left<<','<<rect.top<<"),("<<rect.right<<','<<rect.bottom<<")]";
	}
	template<typename U>
	std::ostream& operator<<(::std::ostream& os,line<U> const& aline)
	{
		return os<<"[("<<aline.a.x<<','<<aline.a.y<<"),("<<aline.b.x<<','<<aline.b.y<<")]";
	}
	template<typename T,typename alloc>
	void compress_rectangles(std::vector<Rectangle<T>,alloc>& container)
	{
		sort(container.begin(),container.end());
		for(unsigned int i=1;i<container.size();++i)
		{
			Rectangle<T>& a=container[i],b=container[i-1];
			if(a.left==b.left&&
				a.top==b.bottom&&
				a.right==b.right)
			{
				Rectangle<T> temp=container[i-1];
				container[i-1]=Rectangle<T>{temp.left,temp.right,temp.top,container[i].bottom};
				container.erase(container.begin()+i);
				--i;
			}
		}
	}

	/*template<typename T> template<typename U>
	vec2_t<T>::vec2_t(vec2_t<U> const& other):x(other.x),y(other.y) {}
	template<typename T> template<typename U>
	vec2_t<T>& vec2_t<T>::operator=(vec2_t<U> const& other) {
	x=other.x;
	y=other.y;
	return *this;
	}
	*/
	template<typename T>
	vec2_t<T> vec2_t<T>::operator+(vec2_t<T> const& other)
	{
		return {x+other.x,y+other.y};
	}
	template<typename T>
	vec2_t<T> vec2_t<T>::operator-(vec2_t<T> const& other)
	{
		return {x-other.x,y-other.y};
	}
	template<typename T>
	vec2_t<T>& vec2_t<T>::operator+=(vec2_t<T> const& other)
	{
		x+=other.x;
		y+=other.y;
		return *this;
	}
	template<typename T>
	vec2_t<T>& vec2_t<T>::operator-=(vec2_t<T> const& other)
	{
		x-=other.y;
		y-=other.y;
		return *this;
	}
	template<typename T>
	void split_horiz(Rectangle<T>* orig,Rectangle<T>* buffer,unsigned int numRects)
	{
		int height=(orig->bottom-orig->top)/numRects;
		--numRects;
		for(int i=0;i<numRects;)
		{
			buffer[i]={orig->left,orig->right,orig->top+height*i,orig->top+height*(++i)};
		}
		buffer[numRects]={orig->left,orig->right,orig->top+height*numRects,orig->bottom};
	}
}
#endif