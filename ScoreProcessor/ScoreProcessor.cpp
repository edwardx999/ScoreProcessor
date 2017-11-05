// ScoreProcessor.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <vector>
#include <memory>
#include <iostream>
#include <string>
//#define cimg_use_jpeg
#include "allAlgorithms.h"
#include "ThreadPool.h"
#include "shorthand.h"
using namespace cimg_library;
using namespace std;
using namespace ScoreProcessor;
using namespace ImageUtils;
using namespace Concurrent;
void stop() {
	cout<<"Stopping\n";
	int e;
	cin>>e;
}
void test() {
	cimg::imagemagick_path("C:\\\"Program Files\"\\ImageMagick-7.0.6-Q16\\convert.exe");
	ThreadPool pool(4);
	class Simple:public ThreadTask {
		void execute() {
			std::cout<<"Hi\n";
		}
	};
	pool.add_task<Simple>();
	pool.start();
}

int main() {
	test();
	stop();
	return 0;
}

