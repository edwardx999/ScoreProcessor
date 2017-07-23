// ScoreProcessor.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "CImg.h"
#include "ImageUtils.h"
#include "ScoreProcesses.h"
#include <vector>
#include <memory>
#include <iostream>
using namespace cimg_library;
using namespace std;

typedef unsigned char ubyte;
int main() {
	cimg::imagemagick_path("C:\\\"Program Files\"\\ImageMagick-7.0.6-Q16\\convert.exe");
	string filename="TestPic.jpg";
	unique_ptr<CImg<ubyte>> templateImage(new CImg<ubyte>(filename.c_str()));
	//ImageUtils::Rectangle whole{0,templateImage->width(),0,templateImage->height()};
	//ScoreProcessor::copyShiftSelection(templateImage.get(),&whole,0,10);
	//ImageUtils::Rectangle moved{0,templateImage->width(),0,10};
	//ImageUtils::ColorRGB white{255,255,255};
	//ScoreProcessor::fillSelection(templateImage.get(),&moved,white);
	try {
		ScoreProcessor::autoCenterHoriz(templateImage.get());
		ScoreProcessor::autoCenterVert(templateImage.get());
	}
	catch(CImgIOException& e) {
		std::cout<<e._message<<'\n';
	}
	templateImage->display();
	int numberOfImages;
	vector<string*> imageNames;
	return 0;
}

