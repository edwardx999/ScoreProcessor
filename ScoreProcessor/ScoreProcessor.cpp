// ScoreProcessor.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "CImg.h"
#include "ImageUtils.h"
#include "ScoreProcesses.h"
#include "Cluster.h"
#include <vector>
#include <memory>
#include <iostream>
#include "moreAlgorithms.h"
using namespace cimg_library;
using namespace std;
using namespace ScoreProcessor;
using namespace ImageUtils;

void stop() {
	cout<<"Stopping\n";
	int e;
	cin>>e;
}
void test() {
	cimg::imagemagick_path("C:\\\"Program Files\"\\ImageMagick-7.0.6-Q16\\convert.exe");
	string filename=";_Page_31.jpg";
	CImg<unsigned char> image(filename.c_str());
	CImg<float> map(image._width,image._height);
	map.fill(100.0f);
	for(unsigned int x=0;x<image._width;++x) {
		unsigned int nodeStart;
		bool nodeFound=grayDiff(WHITE_GRAYSCALE,image(x,0))<.2f;
		if(nodeFound) {
			nodeStart=0;
		}
		unsigned int mid=(image._height)>>1;
		unsigned int y;
		float const value=100.0f;
		for(y=1;y<image._height;++y) {
			if(nodeFound) {
				if(!(nodeFound=grayDiff(WHITE_GRAYSCALE,image(x,y))<.2f)) {
					unsigned int mid=(nodeStart+y)>>1;
					unsigned int valueDivider=2U;
					unsigned int nodeY;
					for(nodeY=nodeStart;nodeY<mid;++nodeY) {
						map(x,nodeY)=value/(++valueDivider);
					}
					for(;nodeY<y;++nodeY) {
						map(x,nodeY)=value/(--valueDivider);
					}

				}
			}
			else {
				if(nodeFound=grayDiff(WHITE_GRAYSCALE,image(x,y))<.2f) {
					nodeStart=y;
				}
			}
		}
		if(nodeFound) {
			unsigned int mid=(nodeStart+y)>>1;
			unsigned int valueDivider=2U;
			unsigned int nodeY;
			for(nodeY=nodeStart;nodeY<mid;++nodeY) {
				map(x,nodeY)=value/(++valueDivider);
			}
			for(;nodeY<y;++nodeY) {
				map(x,nodeY)=value/(--valueDivider);
			}
		}
	}
	minEnergyToRight(map);
	map.display();
}

int main() {
	test();
	return 0;
}

