// ScoreProcessor.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <vector>
#include <memory>
#include <iostream>
#include <string>
//#define cimg_use_jpeg
#include "allAlgorithms.h"
#include "lib/threadpool/ThreadPool.h"
#include "exstring.h"
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
			string disp="Starting "+filename+"\n";
			cout<<disp;
			CImg<uchar> page;
			page.load(filename.c_str());
			if(page._spectrum==3)
			{
				page=get_grayscale(page);
			}
			//remove_border(page,Grayscale::BLACK,0.95f);
			//binarize(page,150);
			//replace_range(page,0,60,0);
			vert_padding(page,80);
			/*if(page._height>3500)
			{
				horiz_padding(page,300);
			}*/
			page.save(filename.c_str());
			//cut_image(page,filename.c_str());
			disp="Done with "+filename+"\n";
			cout<<disp;
		}
	};
	string path="C:\\Users\\edwar\\Videos\\Score\\moiseauxexotiques\\";
	//string end=".jpg";
	ThreadPool pool(8);
	HANDLE hFind;
	WIN32_FIND_DATA fdata;
	hFind=FindFirstFile(L"C:\\Users\\edwar\\Videos\\Score\\moiseauxexotiques\\*.*",&fdata);
	if(hFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			wcout<<fdata.cFileName<<'\n';
			if(*fdata.cFileName==L';')
			{
				string filename=exstring::string_cast<string>(wstring(fdata.cFileName));
				string filepath=path+filename;
				pool.add_task<PageTask>(filepath);
			}
		} while(FindNextFile(hFind,&fdata));
	}
	else
	{
		cout<<"Failed";
	}
	pool.start();
}

int main() {
	cout<<"hi\n";
	CImg<uchar> ct("cuttest.jpg");
	if(ct._spectrum==3)
	{
		ct=get_grayscale(ct);
	}
	ct.display();
	cout<<cut_image(ct,"ct.jpg")<<'\n';
	//CImg<unsigned char> tp("C:\\Users\\edwar\\Videos\\Score\\mascension\\;_Page_19.jpg");
	//if(tp._spectrum==3)
	//{
	//	tp=get_grayscale(tp);
	//}
	//remove_border(tp,Grayscale::BLACK,0.95f);
	//clear_clusters(tp,rcast<ucharcpc>(&ColorRGB::BLACK),ColorRGB::color_diff,0.95f,false,0,200,rcast<ucharcpc>(&ColorRGB::WHITE));
	//tp.display();
	//test();
	stop();
	return 0;
}

