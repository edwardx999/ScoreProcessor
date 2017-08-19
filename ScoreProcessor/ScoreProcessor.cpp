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
	string filename=";_Page_05.jpg";
	CImg<unsigned char> image(filename.c_str());
	vector<unsigned int> right;
	if(image._spectrum==3) {
		image=move(get_grayscale(image));
	}
	CImg<float> gradient=get_gradient(image);
	gradient.display();
	stop();
}

int main() {
	test();
	return 0;
}

