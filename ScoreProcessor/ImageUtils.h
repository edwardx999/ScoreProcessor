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
#include <algorithm>
#include <type_traits>
namespace ImageUtils {
	struct perc_or_val {
		bool is_perc;
		union {
			float perc;
			unsigned int val;
		};
		inline perc_or_val(float perc):is_perc(true),perc(perc)
		{}
		inline perc_or_val(unsigned int val):is_perc(false),val(val)
		{}
		inline perc_or_val()
		{}
		unsigned int operator()(unsigned int base_val) const
		{
			if(is_perc)
			{
				return std::round(perc/100.0*base_val);
			}
			return val;
		}
		unsigned int value(unsigned int base_val) const
		{
			return operator()(base_val);
		}
		void fix_value(unsigned int base_val)
		{
			if(is_perc)
			{
				is_perc=false;
				val=std::round(perc/100.0*base_val);
			}
		}
	};
	/*
		A cartesian vertical line.
	*/
	template<typename T=unsigned int>
	struct vertical_line {
		T x,top,bottom;
	};
	/*
		A cartesian horizontal line.
	*/
	template<typename T=unsigned int>
	struct horizontal_line {
		T y,left,right;
	};
	/*
		A 2 dimensional vector.
	*/
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
	/*
		A cartesian line in 2 dimensions.
	*/
	template<typename T=unsigned int>
	struct line {
		Point<T> a,b;
		template<typename U>
		friend ::std::ostream& operator<<(::std::ostream& os,line<U> const& aline);
	};
	/*
		A polar line in 2 dimensions.
	*/
	template<typename T=float>
	struct line_norm {
		T theta;//the normal has this angle with the x-axis
		T r;//the line is r away from the origin
	};
	/*
		A rectangle.
	*/
	template<typename T=unsigned int>
	struct Rectangle {
		T left,right,top,bottom;
		/*
			Compares value of left side.
			If equal, value of tops.
		*/
		bool operator<(Rectangle<T> const& other) const;
		/*
			Compares value of left side.
			If equal, value of tops.
		*/
		bool operator>(Rectangle<T> const& other) const;

		bool operator==(Rectangle<T> const& other) const
		{
			return left==other.left&&right==other.right&&top==other.top&&bottom==other.bottom;
		}
		/*
			Calculates the area.
		*/
		template<typename U=T>
		U area() const;
		/*
			Calculates the perimeter.
		*/
		template<typename U=T>
		U perimeter() const;
		/*
			Returns the height.
		*/
		template<typename U=T>
		U height() const;
		/*
			Returns the width.
		*/
		template<typename U=T>
		U width() const;
		template<typename U=T>
		/*
			Returns the center of the rectangle.
		*/
		Point<U> center() const;
		/*
			Returns true if this rectangle is bordering the top or bottom of the other.
		*/
		bool is_bordering_vert(Rectangle<T> const& other) const;
		/*
			Returns true if the x-coordinates overlap.
		*/
		bool overlaps_x(Rectangle<T> const& other) const;
		/*
			Returns true if this rectangle is bordering the left or right of the other.
		*/
		bool is_bordering_horiz(Rectangle<T> const& other) const;
		/*
			Returns true if the y-coordinates overlap.
		*/
		bool overlaps_y(Rectangle<T> const& other) const;
		/*
			Returns true if the rectangles intersect.
		*/
		bool intersects(Rectangle<T> const& other) const;
		/*
			Returns the diagonal of this rectangle.
		*/
		line<T> diagonal();
		/*
			Console output
		*/
		template<typename U>
		friend ::std::ostream& operator<<(::std::ostream& os,Rectangle<U> const& rect);
	};

	/*
	template<typename T>
	void split_horiz(Rectangle<T>* orig,Rectangle<T>* buffer,unsigned int numRects);
	*/

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
		operator ColorRGBA() const;
		operator ColorHSV() const;
		operator Grayscale() const;
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
		inline operator unsigned char() const
		{
			return value;
		}
		Grayscale(unsigned char const val):value(val)
		{}
		Grayscale()
		{}

		inline Grayscale& operator=(unsigned char const val)
		{
			value=val; return *this;
		}
		static Grayscale const WHITE;
		static Grayscale const BLACK;
		float difference(Grayscale other);

		static float color_diff(unsigned char const*,unsigned char const*);
	};

	float gray_diff(Grayscale g1,Grayscale g2);

	inline unsigned char brightness(Grayscale g)
	{
		return g;
	}
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

	/*
		Converts rectangles that can be represented together as one rectangle into one rectangle.
		Also sorts the container.
		Returns end of new range.
	*/
	template<typename Iter>
	Iter compress_rectangles(Iter begin,Iter end)
	{
		if(begin==end)
		{
			return end;
		}
		std::sort(begin,end,[](auto const& rect1,auto const& rect2)
			{
				if(rect1.left<rect2.left)
				{
					return true;
				}
				if(rect1.left>rect2.left)
				{
					return false;
				}
				return rect1.top<rect2.top;
			});
		auto write_head=begin;
		auto read_head=std::next(write_head);
		for(;read_head!=end;++read_head)
		{
			auto& parent=*write_head;
			auto const& child=*read_head;
			if(parent.left==child.left&&
				parent.right==child.right&&
				parent.bottom==child.top)
			{
				parent.bottom=child.bottom;
			}
			else
			{
				++write_head;
				*write_head=std::move(*read_head);
			}
		}
		return std::next(write_head);
	}

	/*
		Converts rectangles that can be represented together as one rectangle into one rectangle.
		Also sorts the container.
	*/
	template<typename Container>
	void compress_rectangles(Container& container)
	{
		auto iter=compress_rectangles(container.begin(),container.end());
		container.erase(iter,container.end());
	}

	inline ColorRGB::operator ColorHSV() const
	{
		ColorHSV ret;
		float r=this->r,b=this->b,g=this->g;
		auto min=std::min(r,std::min(g,b));
		auto max=std::max(r,std::max(g,b));
		ret.v=unsigned char(max);
		auto delta=max-min;
		if(max!=0)
		{
			ret.s=unsigned char(std::round(delta/max*255));
		}
		else
		{
			ret.s=0;
			//doesn't matter what hue is
			return ret;
		}
		float hue;
		if(r==max)
		{
			hue=(g-b)/delta;
		}
		else if(g==max)
		{
			hue=2+(b-r)/delta;
		}
		else
		{
			hue=4+(r-g)/delta;
		}
		hue*=(256.0f/360*60);
		if(hue<0)
		{
			hue+=256;
		}
		ret.h=unsigned char(std::round(hue));
		return ret;
	}

	inline ColorRGB::operator ColorRGBA() const
	{
		return ColorRGBA{r,g,b,255};
	}

	inline ColorRGB::operator Grayscale() const
	{
		return static_cast<Grayscale>((r*0.2126f+g*0.7152f+b*0.0722f));
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
	/*
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
	*/
}
#endif