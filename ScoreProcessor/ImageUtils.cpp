#include "stdafx.h"
#include "ImageUtils.h"
#include <algorithm>
#include <iostream>
using namespace std;
namespace ImageUtils {
	bool Rectangle::operator<(Rectangle const& other) const {
		if(left<other.left) return true;
		if(left>other.left) return false;
		return top<other.top;
	}
	bool Rectangle::operator>(Rectangle const& other) const {
		if(left>other.left) return true;
		if(left<other.left) return false;
		return top>other.top;
	}
	int Rectangle::area() const {
		return (right-left)*(bottom-top);
	}
	int Rectangle::perimeter() const {
		return 2*(right-left+bottom-top);
	}
	bool Rectangle::isBorderingVert(Rectangle const& other) const {
		return (other.bottom==top||other.top==bottom)&&overlapsX(other);
	}
	bool Rectangle::overlapsX(Rectangle const& other) const {
		return (other.left>=left&&other.left<right)||
			(other.right>left&&other.right<=right)||
			(other.left<left&&other.right>right);
	}
	bool Rectangle::isBorderingHoriz(Rectangle const& other) const {
		return (other.left==right||other.right==left)&&overlapsY(other);
	}
	bool Rectangle::overlapsY(Rectangle const& other) const {
		return (other.top>=top&&other.top<bottom)||
			(other.bottom>top&&other.bottom<=bottom)||
			(other.top<top&&other.bottom>bottom);
	}
	bool Rectangle::intersects(Rectangle const& other) const {
		return overlapsX(other)&&overlapsY(other);
	}
	line Rectangle::diagonal() {
		return {{left,top},{right,bottom}};
	}
	ostream& operator<<(ostream& os,Rectangle const& rect) {
		return os<<"[("<<rect.left<<','<<rect.top<<"),("<<rect.right<<','<<rect.bottom<<")]";
	}
	void compressRectangles(vector<shared_ptr<Rectangle>>& container) {
		sort(container.begin(),container.end(),[](shared_ptr<Rectangle>& a,shared_ptr<Rectangle>& b) {return *a<*b;});
		for(unsigned int i=1;i<container.size();++i) {
			if(container[i]->left==container[i-1]->left&&
				container[i]->top==container[i-1]->bottom&&
				container[i]->right==container[i-1]->right) {
				Rectangle temp=*container[--i];
				container[i].reset();
				container[i]=make_shared<Rectangle>(Rectangle{temp.left,temp.right,temp.top,container[i+1]->bottom});
				container.erase(container.begin()+i+1);
			}
		}
		std::cout<<"Hi3\n";
	}
	vec2_t vec2_t::operator+(vec2_t const& other) {
		return {x+other.x,y+other.y};
	}
	vec2_t vec2_t::operator-(vec2_t const& other) {
		return {x-other.x,y-other.y};
	}
	vec2_t& vec2_t::operator+=(vec2_t const& other) {
		x+=other.x;
		y+=other.y;
		return *this;
	}
	vec2_t& vec2_t::operator-=(vec2_t const& other) {
		x-=other.y;
		y-=other.y;
		return *this;
	}
	ColorRGBA ColorRGB::toRGBA() const {
		return ColorRGBA{r,g,b,255};
	}
	ColorHSV ColorRGB::toHSV() const {
		ColorHSV ret;
		ret.h=max({r,g,b})-min({r,g,b});
		return ret;
	}
	unsigned char ColorRGB::brightness() const {
		unsigned short t=r+g+b;
		return static_cast<unsigned char>(t/3);
	}
	const ColorRGB WHITE_RGB={255,255,255};
	const ColorRGB BLACK_RGB={0,0,0};
	ColorRGB ColorHSV::toRGB() const {
		ColorRGB ret;
		return ret;
	}
	const ColorHSV WHITE_HSV={0,0,255};
	const ColorHSV BLACK_HSV={0,0,0};
	ColorRGB ColorRGBA::toRGB() const {
		return ColorRGB{r,g,b};
	}
	float RGBColorDiff(unsigned char r1,unsigned char g1,unsigned char b1,unsigned char r2,unsigned char g2,unsigned char b2) {
		float rdif=r1>r2?r1-r2:r2-r1;
		float gdif=g1>g2?g1-g2:g2-g1;
		float bdif=b1>b2?b1-b2:b2-b1;
		return (rdif*rdif+gdif*gdif+bdif*bdif)/maxDif;
	}
	const Grayscale WHITE_GRAYSCALE=255;
	const Grayscale BLACK_GRAYSCALE=0;
	float grayDiff(Grayscale g1,Grayscale g2) {
		float dif=(g1>g2?g1-g2:g2-g1);
		return dif*dif/twofivefivesquared;
	}
	void splitHoriz(Rectangle* orig,Rectangle* buffer,int numRects) {
		int height=(orig->bottom-orig->top)/numRects;
		--numRects;
		for(int i=0;i<numRects;) {
			buffer[i]={orig->left,orig->right,orig->top+height*i,orig->top+height*(++i)};
		}
		buffer[numRects]={orig->left,orig->right,orig->top+height*numRects,orig->bottom};
	}
}