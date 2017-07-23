#include "stdafx.h"
#include "ImageUtils.h"
#include <algorithm>
namespace ImageUtils {
	bool Rectangle::operator<(Rectangle const& other) const{
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
	bool Rectangle::isBorderingVert(Rectangle const& other) const{
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
	void compressRectangles(std::vector<Rectangle*> container) {
		std::sort(container.begin(),container.end(),[](Rectangle* a,Rectangle* b) {return *a<*b;});
		for(int i=1;i<container.size();++i) {
			if(container[i]->left==container[i-1]->left&&
				container[i]->top==container[i-1]->bottom&&
				container[i]->right==container[i-1]->right) {
				*container[--i]={container[i]->left,container[i]->right,container[i]->top,container[i+1]->bottom};
				delete container[i+1];
				container.erase(container.begin()+1);
			}
		}
	}
	float RGBColorDiff(unsigned char r1,unsigned char g1,unsigned char b1,unsigned char r2,unsigned char g2,unsigned char b2) {
		float rdif=r1>r2?r1-r2:r2-r1;
		float gdif=g1>g2?g1-g2:g2-g1;
		float bdif=b1>b2?b1-b2:b2-b1;
		return (rdif*rdif+gdif*gdif+bdif*bdif)/maxDif;
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