// ScoreProcessor.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <vector>
#include <memory>
#include <iostream>
#include <string>
//#define cimg_use_jpeg
#include "allAlgorithms.h"
#include "threadpool/ThreadPool.h"
#include "shorthand.h"
using namespace cimg_library;
using namespace std;
using namespace ScoreProcessor;
using namespace ImageUtils;
using namespace concurrent;
void stop() {
	cout<<"Stopping\n";
	int e;
	cin>>e;
}
void test() {
	class PageTask:public ThreadTask {
	private:
		std::string filename;
	public:
		PageTask(string& filename):filename(filename) {}
		void execute() {
			try
			{
				CImg<unsigned char> page;
				page.load(filename.c_str());
			//if(static_cast<signed long>(filename.find("garbage"))>0) return;
				string disp="Starting "+filename+"\n";
				cout<<disp;
				//page.display();
				replace_by_brightness(page,180);
				replace_by_chroma(page,50);
				page=get_grayscale(page);
				page.save(filename.c_str());
				disp="Done with "+filename+"\n";
				cout<<disp;
			}
			catch(CImgIOException&)
			{
				return;
			}
		}
	};
	string temp="C:\\Users\\edwar\\Videos\\Score\\m3liturgies\\";
	string end=".jpg";
	/*for(unsigned int i=0;i<8;++i)
	{
		pool.add_task<PageTask>(temp+"garbage.jpg");
	}*/
	ThreadPool pool(8);
	for(unsigned int i=0;i<230;++i)
	{
		if(i==52||i==53) continue;
		pool.add_task<PageTask>(temp+to_string(i)+end);
	}
	pool.start();
}

int main() {
	//cimg::imagemagick_path("C:\\\"Program Files\"\\ImageMagick-7.0.6-Q16\\convert.exe");
	//cout<<string("gawgarbage").find("garbage")<<'\n'<<static_cast<signed int>(string("gaw").find("e"));
	//Sleep(1000000);
	test();
	stop();
	return 0;
}

