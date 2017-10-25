// ScoreProcessor.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <vector>
#include <memory>
#include <iostream>
//#define cimg_use_jpeg
#include "allAlgorithms.h"
#include "shorthand.h"
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
	//string filename="llsk.jpg";
	//CImg<unsigned char> image(filename.c_str());
	//CImg<unsigned char> straight("ll_001.jpg");
	//if(image._spectrum==3) {
	//	image=get_grayscale(image);
	//}
	//if(straight._spectrum==3) {
	//	straight=get_grayscale(straight);
	//}
	//auto gradient=get_absolute_gradient(image);
	//binarize(gradient,80,0,255);
	//auto straightgradient=get_absolute_gradient(straight);
	//binarize(straightgradient,80,0,255);
	//gradient.display();
	//auto hough=get_hough(gradient,0.8f,2.3f,0.01f,2U);
	//auto houghStraight=get_hough(straightgradient,0.8f,2.3f,0.01f,2U);
	///*for(unsigned int x=0;x<hough._width;++x) {
	//	for(unsigned int y=0;y<hough._height;++y) {
	//		if(hough(x,y)<800)
	//			hough(x,y)=0;
	//	}
	//}*/
	///*hough.display();
	//houghStraight.display();*/
	//cout<<findHoughAngle(hough,0.8f,0.01f)<<endl;
	//cout<<findHoughAngle(houghStraight,0.8f,0.01f)<<endl;
	CImg<uchar> wide("ll_001.jpg");
	if(wide._spectrum==3)
	{
		wide=get_grayscale(wide);
	}
	/*auto vert_map=create_vertical_energy(wide);
	auto wide_map=create_compress_energy(wide);
	auto copy_wide=wide_map;
	min_energy_to_right(copy_wide);
	vert_map.display();
	wide_map.display();
	copy_wide.display();*/
	compress(wide,0,2000);
	wide.display();
	/*copy_shift_selection(wide,{0,wide._width,0,wide._height},-500,-500);
	wide.display();*/
}

int main() {
	test();
	stop();
	return 0;
}

