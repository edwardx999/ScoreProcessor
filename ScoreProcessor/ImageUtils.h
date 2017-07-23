#pragma once
#include <vector>
namespace ImageUtils {
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
	};
	void splitHoriz(Rectangle* orig,Rectangle* buffer,int numRects);
	void compressRectangles(std::vector<Rectangle*> container);
	struct ColorRGB {
		unsigned char r,g,b;
	};
	struct ColorRGBA {
		unsigned char r,g,b,a;
	};
	const float maxDif=255.0f*255.0f*3.0f;
	float RGBColorDiff(unsigned char r1,unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2);
}