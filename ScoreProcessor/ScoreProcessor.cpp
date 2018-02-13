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
#include "lib/exstring/exstdstring.h"
#include "convert.h"
#include "shorthand.h"
using namespace cimg_library;
using namespace std;
using namespace ScoreProcessor;
using namespace ImageUtils;
using namespace exlib;
class ImageProcess {
protected:
	ImageProcess()=default;
public:
	typedef CImg<unsigned char> Img;
	~ImageProcess()=default;
	virtual void process(Img& image)=0;
};
class ChangeToGrayscale:public ImageProcess {
public:
	void process(Img& image)
	{
		if(image._spectrum==3||image._spectrum==4)
		{
			image=get_grayscale_simple(image);
		}
	}
};
class RemoveTransparency:public ImageProcess {
public:
	void process(Img& img)
	{
		if(img._spectrum==4)
		{
			remove_transparency(img,150,ColorRGB::WHITE);
		}
	}
};
class RemoveProcess:public ImageProcess {
public:
	void process(Img& image)
	{
		remove_border(image);
	}
};
class FilterGray:public ImageProcess {
	unsigned char val;
public:
	FilterGray(unsigned char val):val(val) {}
	void process(Img& image)
	{
		replace_range(image,val);
	}
};
class PadHoriz:public ImageProcess {
	unsigned int val;
public:
	PadHoriz(unsigned int val):val(val) {}
	void process(Img& image)
	{
		horiz_padding(image,val);
	}
};
class PadVert:public ImageProcess {
	unsigned int val;
public:
	PadVert(unsigned int val):val(val) {}
	void process(Img& img)
	{
		vert_padding(img,val);
	}
};
class PadAuto:public ImageProcess {
	unsigned int vert,min_h,max_h;
	signed int hoff;
public:
	PadAuto(unsigned int vert,unsigned int min_h,unsigned int max_h,signed int hoff)
		:vert(vert),min_h(min_h),max_h(max_h),hoff(hoff) {}
	void process(Img& img)
	{
		auto_padding(img,vert,max_h,min_h,hoff);
	}
};
class Resize:public ImageProcess {
	double val;
public:
	Resize(double val):val(val) {}
	void process(Img& img)
	{
		img.resize(
			scast<int>(std::round(img._width*val)),
			scast<int>(std::round(img._height*val)),
			1,
			1,
			2);
	}
};
class ClusterClear:public ImageProcess {
	unsigned int min,max;
public:
	ClusterClear(unsigned int min,unsigned int max):min(min),max(max) {}
	void process(Img& img)
	{
		clear_clusters(img,rcast<ucharcp>(&ImageUtils::Grayscale::WHITE),
			ImageUtils::Grayscale::color_diff,0.042,true,min,max,rcast<ucharcp>(&ImageUtils::Grayscale::WHITE));
	}
};
class RescaleColors:public ImageProcess {
	unsigned char min,mid,max;
public:
	RescaleColors(unsigned char min,unsigned char mid,unsigned char max=255):min(min),mid(mid),max(max) {}
	void process(Img& img)
	{
		rescale_colors(img,min,mid,max);
	}
};
class FillSelection:public ImageProcess {
	ImageUtils::Rectangle<unsigned int> rect;
public:
	FillSelection(ImageUtils::Rectangle<unsigned int> rect):rect(rect) {}
	void process(Img& img) override
	{
		fill_selection(img,rect,Grayscale::WHITE);
	}
};
template<typename Task,typename... extra_args>
int add_files_to_task_and_exec(unsigned int num_threads,string const& path,extra_args... args)
{
	wstring wpath=exlib::string_cast<wstring>(path+"*.*");
	ThreadPool pool(num_threads);
	HANDLE hFind;
	WIN32_FIND_DATA fdata;
	hFind=FindFirstFile(wpath.c_str(),&fdata);
	string end=".jpg";
	string end_alt=".png";
	if(hFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			//wcout<<fdata.cFileName<<L'\n';
			std::string filename=exlib::string_cast<std::string>(wstring(fdata.cFileName));
			if(find_end(filename.begin(),filename.end(),end.begin(),end.end())!=filename.end()||
				find_end(filename.begin(),filename.end(),end_alt.begin(),end_alt.end())!=filename.end())
			{
				std::string filepath=path+filename;
				cout<<filepath<<'\n';
				pool.add_task<Task>(std::move(filepath),args...);
			}
		} while(FindNextFile(hFind,&fdata));
	}
	else
	{
		return 1;
	}
	FindClose(hFind);
	pool.start();
	return 0;
}
void stop()
{
	cout<<"Done\n";
	Sleep(60000);
}
void cut(string const& path)
{
	class CutTask:public ThreadTask {
	private:
		std::string filename;
		std::string savename;
	public:
		CutTask(std::string& filename,std::string& savename):filename(filename),savename(savename) {}
		void execute()
		{
			string disp="Starting "+filename+"\n";
			cout<<disp;
			try
			{
				CImg<uchar> page(filename.c_str());
				if(page._spectrum==3)
				{
					page=get_grayscale(page);
				}
				cut_page(page,savename.c_str());
				disp="Done with "+filename+"\n";
				cout<<disp;
			}
			catch(CImgException&)
			{
				disp="Failed to load page "+filename+'\n';
				cout<<disp;
			}

		}
	};
	string folder=path+"cut\\";
	wstring wpath=exlib::string_cast<wstring>(path+"*.*");
	CreateDirectory(exlib::string_cast<wstring>(folder).c_str(),NULL);
	ThreadPool pool(8);
	HANDLE hFind;
	WIN32_FIND_DATA fdata;
	hFind=FindFirstFile(wpath.c_str(),&fdata);
	cout<<sizeof(CutTask)<<'\n';
	if(hFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			//wcout<<fdata.cFileName<<L'\n';
			if(*fdata.cFileName==L';')
			{
				std::string filename=exlib::string_cast<std::string>(wstring(fdata.cFileName));
				std::string filepath=path+filename;
				std::string savename=folder+filename;
				cout<<filepath<<'\n';
				pool.add_task<CutTask>(std::move(filepath),std::move(savename));
			}
		} while(FindNextFile(hFind,&fdata));
	}
	else
	{
		cout<<"Failed";
	}
	FindClose(hFind);
	pool.start();
}
void splice(string const& path)
{
	string savepath=path+"together\\";
	CreateDirectory(exlib::string_cast<wstring>(path).c_str(),nullptr);
	savepath+=";page_.jpg";
	wstring wpath=exlib::string_cast<wstring>(path+"*.*");
	HANDLE hFind;
	WIN32_FIND_DATA fdata;
	hFind=FindFirstFile(wpath.c_str(),&fdata);
	vector<string> filenames;
	if(hFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			//wcout<<fdata.cFileName<<L'\n';
			std::string filename=exlib::string_cast<std::string>(wstring(fdata.cFileName));
			if(filename.find(".jpg")==string::npos&&filename.find(".png")==string::npos)
			{
				continue;
			}
			filename=path+filename;
			cout<<filename<<'\n';
			filenames.emplace_back(std::move(filename));
		} while(FindNextFile(hFind,&fdata));
	}
	else
	{
		cout<<"Failed";
	}
	FindClose(hFind);
	splice_pages_greedy(savepath,filenames,2800);
}

void process(string const& path,vector<uptr<ImageProcess>>* pprocesses,unsigned int num_threads=8)
{
	class ProcessTask:public ThreadTask {
		vector<uptr<ImageProcess>>* processes;
		string filename;
	public:
		ProcessTask(string& filename,vector<uptr<ImageProcess>>* processes):filename(filename),processes(processes) {}
		void execute()
		{
			string disp="Starting "+filename+"\n";
			cout<<disp;
			CImg<unsigned char> image(filename.c_str());
			for(auto& process:*processes)
			{
				process->process(image);
			}
			image.save(filename.c_str());
			disp="Done with "+filename+"\n";
			cout<<disp;
		}
	};
	if(add_files_to_task_and_exec<ProcessTask>(num_threads,path,pprocesses))
	{
		cout<<"Failed\n";
	}
}
int main()
{
	cout<<"hi\n";
	//pad_vert();
	//splice();
	//Grayscale near_white=200;
	//cout<<Grayscale::color_diff(rcast<ucharcpc>(&near_white),rcast<ucharcpc>(&Grayscale::BLACK))<<'\n';
	string path="C:\\Users\\edwar\\Videos\\Score\\hbenimora\\";
	//cut(path);
	//path+=string("cut\\");
	vector<uptr<ImageProcess>> ips;
	ips.emplace_back(make_unique<RemoveTransparency>());
	ips.emplace_back(make_unique<ChangeToGrayscale>());
	ips.emplace_back(make_unique<PadAuto>(100,100,200,50));
	//ips.emplace_back(make_unique<Resize>(0.5));
	//ips.emplace_back(make_unique<FillSelection>(ImageUtils::Rectangle<unsigned int>{3385,3385+226,9337,9337+80}));
	//ips.emplace_back(make_unique<FillSelection>(ImageUtils::Rectangle<unsigned int>{6395,6395+150,512,512+85}));

	//ips.emplace_back(make_unique<FilterGray>(190));
	//ips.emplace_back(make_unique<RescaleColors>(40,255));
	//ips.emplace_back(make_unique<Resize>(0.5));
	//ips.emplace_back(make_unique<ClusterClear>(20'000'000,~0));
	//2ips.emplace_back(make_unique<RemoveProcess>());
	//ips.emplace_back(make_unique<Resize>(2.0/3.0));
	//ips.emplace_back(make_unique<PadHoriz>(200));
	//ips.emplace_back(make_unique<PadVert>(200));
	//ips.emplace_back(make_unique<Resize>(2000.0/2500.0));
	process(path,&ips,8);
	//splice(path);

	//ips.emplace_back(make_unique<ChangeToGrayscale>());
	//ips.emplace_back(make_unique<RemoveProcess>());
	//ips.emplace_back(make_unique<FilterGray>(180));
	/*ips.emplace_back(make_unique<PadHoriz>(800));
	ips.emplace_back(make_unique<PadVert>(600));
	ips.emplace_back(make_unique<ChangeToGrayscale>());
	ips.emplace_back(make_unique<Resize>(2.0/3.0));*/
	//process(path,&ips);
	stop();
	return 0;
}

