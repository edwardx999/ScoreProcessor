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
using namespace cimg_library;
using namespace std;
using namespace ScoreProcessor;
using namespace ImageUtils;

void stop() {
	int e;
	cin>>e;
}
void test() {
	cimg::imagemagick_path("C:\\\"Program Files\"\\ImageMagick-7.0.6-Q16\\convert.exe");
	string filename=";_Page_32.jpg";
	//filename="outside.jpg";
	//filename="C:\\Eddie\\Pictures and Videos\\50.jpg";
	CImg<unsigned char> templateImage(filename.c_str());
	if(templateImage._spectrum==3) {
		templateImage=get_grayscale(templateImage);
	}
	//vector<unique_ptr<RectangleUINT>> outside;
	cout<<cutImage(templateImage,"ll.jpg")<<'\n';
	stop();
	vector<unique_ptr<string>> imageNames;
}

int main() {
	test();
	return 0;
}

