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
int main() {
	cimg::imagemagick_path("C:\\\"Program Files\"\\ImageMagick-7.0.6-Q16\\convert.exe");
	string filename="C:\\Users\\edwar\\Videos\\Score\\Holst Cloud Video\\;_Page_04.jpg";
	filename="C:\\Eddie\\Pictures and Videos\\50.jpg";
	CImg<unsigned char> templateImage(filename.c_str());
	get_grayscale(templateImage).display();
	try {
		//ScoreProcessor::autoCenterHoriz(templateImage.get());
		//ScoreProcessor::autoCenterVert(templateImage.get());
		
		//clusterClear(templateImage,0,150,0.001f,true);
		ColorRGB midColor={180,180,180};//averageColor(templateImage);
		vector<unsigned int> left;
		buildLeftProfile(templateImage,left,255);
		//templateImage.rotate(7.0f,0,0,2,1);
		//templateImage.display();
		for(unsigned int i=0;i<left.size();++i) {
			std::cout<<i<<":\t"<<left[i]<<'\n';
		}
	}
	catch(CImgIOException& e) {
		std::cout<<e._message<<endl;
	}
	stop();
	vector<string> imageNames;
	return 0;
}

