/*
Copyright(C) 2017-2018 Edward Xie

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "stdafx.h"
#include <vector>
#include <memory>
#include <iostream>
#include <string>
//#define cimg_use_jpeg
#include "allAlgorithms.h"
#include "lib/threadpool/ThreadPool.h"
#include "lib/exstring/exfiles.h"
#include "shorthand.h"
#include "ImageProcess.h"
#include <algorithm>
#include <unordered_map>
using namespace cimg_library;
using namespace std;
using namespace ScoreProcessor;
using namespace ImageUtils;
class ChangeToGrayscale:public ImageProcess<> {
public:
	void process(Img& image)
	{
		if(image._spectrum==3||image._spectrum==4)
		{
			image=get_grayscale_simple(image);
		}
	}
};
class RemoveTransparency:public ImageProcess<> {
public:
	void process(Img& img)
	{
		if(img._spectrum==4)
		{
			remove_transparency(img,150,ColorRGB::WHITE);
		}
	}
};
class RemoveProcess:public ImageProcess<> {
public:
	void process(Img& image)
	{
		if(image._spectrum==1)
		{
			remove_border(image);
		}
		else
		{
			throw std::invalid_argument("Remove process requires Grayscale image");
		}
	}
};
class FilterGray:public ImageProcess<> {
	unsigned char min;
	unsigned char max;
	Grayscale replacer;
public:
	FilterGray(unsigned char min,unsigned char max,Grayscale replacer):min(min),max(max),replacer(replacer)
	{}
	void process(Img& image)
	{
		if(image._spectrum==1)
		{
			replace_range(image,min,max,replacer);
		}
		else
		{
			throw std::invalid_argument("Filter Gray requires grayscale image");
		}
	}
};
class PadHoriz:public ImageProcess<> {
	unsigned int val;
public:
	PadHoriz(unsigned int val):val(val)
	{}
	void process(Img& image)
	{
		horiz_padding(image,val);
	}
};
class PadVert:public ImageProcess<> {
	unsigned int val;
public:
	PadVert(unsigned int val):val(val)
	{}
	void process(Img& img)
	{
		vert_padding(img,val);
	}
};
class PadAuto:public ImageProcess<> {
	unsigned int vert,min_h,max_h;
	signed int hoff;
	float opt_rat;
public:
	PadAuto(unsigned int vert,unsigned int min_h,unsigned int max_h,signed int hoff,float opt_rat)
		:vert(vert),min_h(min_h),max_h(max_h),hoff(hoff),opt_rat(opt_rat)
	{}
	void process(Img& img)
	{
		auto_padding(img,vert,max_h,min_h,hoff,opt_rat);
	}
};
class Resize:public ImageProcess<> {
	double val;
public:
	Resize(double val):val(val)
	{}
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
class ClusterClearGray:public ImageProcess<> {
	unsigned int min,max;
	Grayscale background;
	float tolerance;
public:
	ClusterClearGray(unsigned int min,unsigned int max,Grayscale background,float tolerance):min(min),max(max),background(background),tolerance(tolerance)
	{}
	void process(Img& img)
	{
		if(img._spectrum==1)
		{
			clear_clusters(img,rcast<ucharcp>(&background),
				ImageUtils::Grayscale::color_diff,tolerance,true,min,max,rcast<ucharcp>(&background));
		}
		else
		{
			throw std::invalid_argument("Cluster Clear Grayscale requires grayscale image");
		}
	}
};
class RescaleGray:public ImageProcess<> {
	unsigned char min,mid,max;
public:
	RescaleGray(unsigned char min,unsigned char mid,unsigned char max=255):min(min),mid(mid),max(max)
	{}
	void process(Img& img)
	{
		if(img._spectrum==1)
		{
			rescale_colors(img,min,mid,max);
		}
		else
		{
			throw std::invalid_argument("Rescale Gray requires grayscale image");
		}
	}
};
class FillSelection:public ImageProcess<> {
	ImageUtils::Rectangle<unsigned int> rect;
public:
	FillSelection(ImageUtils::Rectangle<unsigned int> rect):rect(rect)
	{}
	void process(Img& img) override
	{
		fill_selection(img,rect,Grayscale::WHITE);
	}
};
class Blur:public ImageProcess<> {
	float radius;
public:
	Blur(float radius):radius(radius)
	{}
	void process(Img& img) override
	{
		img.blur(radius);
	}
};

void stop()
{
	cout<<"Done\n";
	Sleep(60000);
}


class CommandMaker {
public:
	struct delivery {
		SaveRules sr;
		ProcessList<unsigned char> pl;
		enum do_state:unsigned char {
			do_nothing,
			do_single,
			do_cut,
			do_splice
		};
		do_state flag;
		struct {
			unsigned int horiz_padding;
			unsigned int optimal_padding;
			unsigned int min_padding;
			unsigned int optimal_height;
		} splice_args;
	};
	static char const* const mci;
	typedef std::vector<std::string>::const_iterator iter;
	virtual ~CommandMaker()=default;
	virtual char const* help_message() const=0;
	virtual char const* make_command(iter args,iter end,delivery&) const=0;
};
char const* const CommandMaker::mci="Multi command incompatibility";;
class FilterGrayMaker:public CommandMaker {
	static char const* assign_val(iter arg,int* hold)
	{
		try
		{
			*hold=std::stoi(*arg);
			if(*hold<0||*hold>255)
			{
				return "Values must be in range [0,255] for Filter Gray";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid parameter given for Filter Gray";
		}
		return nullptr;
	}
public:
	char const* help_message() const override
	{
		return "Replaces all values between min_value and max_value inclusive with replacer";
	}
	char const* make_command(iter argb,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}

		if(argb==end)
		{
			return "Too few parameters for Filter Gray";
		}
		int params[3];
		size_t i;
		for(i=0;argb+i!=end;++i)
		{
			if(auto res=assign_val(argb+i,params+i))
			{
				return res;
			}
		}
		for(;i<3;++i)
		{
			params[i]=255;
		}
		del.pl.add_process<FilterGray>(*params,params[1],Grayscale(params[2]));
		del.flag=delivery::do_single;
		return nullptr;
	}
};

class ConvertGrayMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return "Converts given image to Grayscale";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}
		del.flag=delivery::do_single;
		del.pl.add_process<ChangeToGrayscale>();
		return nullptr;
	}
};

class ClusterClearMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return "All clusters of pixels that are outside of tolerance of background color and between min and max size are replaced by the background color";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}
		if(begin==end)
		{
			return "Too few arguments for Cluster Clear";
		}
		int max_size,min_size=0;
		Grayscale background=255;
		float tolerance=0.042;
		try
		{
			max_size=std::stoi(*begin);
			if(min_size<0)
			{
				return "Maximum cluster size must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for maximum size of cluster clear";
		}
		++begin;
		if(begin==end)
		{
			goto end;
		}
		try
		{
			min_size=std::stoi(*begin);
			if(min_size<0)
			{
				return "Minimum cluster size must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for minimum size of cluster clear";
		}
		if(max_size<min_size)
		{
			return "Max size must be greater than min size";
		}
		++begin;
		if(begin==end)
		{
			goto end;
		}
		try
		{
			int bg=std::stoi(*begin);
			if(bg>255||bg<0)
			{
				return "Background color for Cluster Clear must be in range [0,255]";
			}
			background=bg;
		}
		catch(std::exception const&)
		{
			return "Invalid input for background color for Cluster Clear";
		}
		++begin;
		if(begin==end)
		{
			goto end;
		}
		try
		{
			tolerance=std::stof(*begin);
			if(tolerance<0||tolerance>1)
			{
				return "Tolerance for Cluster Clear must be between 0 and 1";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for tolerance for Cluster Clear";
		}
	end:
		del.flag=delivery::do_single;
		del.pl.add_process<ClusterClearGray>(min_size,max_size,Grayscale(background),tolerance);
		return nullptr;
	}
};

class HorizontalPaddingMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return "Pads the left and right sides of the image with given number of pixels";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}
		if(begin==end)
		{
			return "Too few arguments to horizontal padding";
		}
		try
		{
			int amount=std::stoi(*begin);
			if(amount<0)
			{
				return "Value for horizontal padding must be non-negative";
			}
			del.flag=delivery::do_single;
			del.pl.add_process<PadHoriz>(amount);
			return nullptr;
		}
		catch(std::exception const&)
		{
			return "Invalid input for padding amount";
		}
	}
};

class VerticalPaddingMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return "Pads the top and bottom of the image with given number of pixels";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}
		if(begin==end)
		{
			return "Too few arguments to vertical padding";
		}
		try
		{
			int amount=std::stoi(*begin);
			if(amount<0)
			{
				return "Value for vertical padding must be non-negative";
			}
			del.flag=delivery::do_single;
			del.pl.add_process<PadVert>(amount);
			return 0;
		}
		catch(std::exception const&)
		{
			return "Invalid input for padding amount";
		}
	}
};

class OutputMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return
			"Assign output pattern:\n"
			" %c copy whole filename\n"
			" %p copy path\n"
			" %x copy extension\n"
			" %f copy filename\n"
			" %0 any number from 0-9, index of file with specified number of padding\n"
			" %% literal percent";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{

		if(begin==end)
		{
			return "Missing filename params";
		}
		try
		{
			del.sr.assign(*begin);
		}
		catch(std::exception const&)
		{
			return "Invalid filename template";
		}
		return nullptr;
	}
};

class AutoPaddingMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return
			"Attempts to make the image fit the desired ratio.\n"
			"Top and bottom are padded by vertical padding.\n"
			"Left is padded by somewhere between min padding and max padding.\n"
			"Right is padded by left padding plus horizontal offset.";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}
		if(std::distance(begin,end)<3)
		{
			return "Too few arguments for Auto Padding";
		}
		int vert,minh,maxh,hoff;
		float opt_rat;
		try
		{
			vert=std::stoul(*begin);
			if(vert<0)
			{
				return "Vertical padding must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid argument given for vertical padding";
		}
		++begin;
		try
		{
			minh=std::stoi(*begin);
			if(minh<0)
			{
				return "Minimum horizontal padding must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for minimum horizontal padding";
		}
		++begin;
		try
		{
			maxh=std::stoi(*begin);
			if(maxh<0)
			{
				return "Maximum horizontal padding must be non-negative";
			}
			if(maxh<minh)
			{
				return "Maximum horizontal padding must be greater than minimum";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid argument given for max horizontal padding";
		}
		++begin;
		if(begin!=end)
		{
			try
			{
				hoff=std::stoi(*begin);
				if(hoff<0&&-hoff>minh)
				{
					return "Negative horizontal offset must be less than minimum horizontal padding";
				}
			}
			catch(std::exception const&)
			{
				return "Invalid argument given for horizontal offset";
			}
		}
		else
		{
			hoff=0;
			goto end;
		}
		++begin;
		if(begin!=end)
		{
			try
			{
				opt_rat=std::stof(*begin);
				if(opt_rat<0)
				{
					return "Optimal ratio must be non-negative";
				}
			}
			catch(std::exception const&)
			{
				return "Invalid argument given for optimal ratio";
			}
		}
		else
		{
			opt_rat=16.0f/9.0f;
		}
	end:
		del.flag=delivery::do_single;
		del.pl.add_process<PadAuto>(vert,minh,maxh,hoff,opt_rat);
		return nullptr;
	}
};

class RescaleGrayMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return
			"Colors are scaled such that values less than or equal to min become 0,\n"
			"and values greater than or equal to max becomes 255.\n"
			"They are scaled based on their distance from mid.";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}
		if(std::distance(begin,end)<3)
		{
			return "Too few arguments for Rescale Gray";
		}
		char const* const errors[]=
		{"Invalid argument for min value of Rescale Gray",
			"Invalid argument for mid value of Rescale Gray",
			"Invalid argument for max value of Rescale Gray"};
		int params[3];
		for(size_t i=0;i<3;++i)
		{
			try
			{
				params[i]=std::stoi(begin[i]);
				if(params[i]<0||params[i]>255)
				{
					return "Values for Rescale Gray must be in range [0,255]";
				}
			}
			catch(std::exception const&)
			{
				return errors[i];
			}
		}
		del.flag=del.do_single;
		del.pl.add_process<RescaleGray>(params[0],params[1],params[2]);
		return nullptr;
	}
};

class CutMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return "Cuts the image into separate systems";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag)
		{
			return "Cut can not be done along with other commands";
		}
		del.flag=delivery::do_cut;
		return 0;
	}
};

class SpliceMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return
			"Splices the pages together assuming right alignment.\n"
			"Greedy algorithm that tries to minimize deviation from optimal height and optimal padding.\n"
			"Horizontal padding is the padding placed between elements of the page horizontally.\n"
			"Min padding is the minimal vertical padding between pages."
			;
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag)
		{
			return "Splice can not be done along with other commands";
		}
		del.flag=delivery::do_splice;
		if(std::distance(begin,end)<3)
		{
			return "Too few arguments for splice";
		}
		int hpadding,opadding,mpadding,oheight;
		try
		{
			hpadding=std::stoi(*begin);
			if(hpadding<0)
			{
				return "Horizontal padding of splice must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for horizontal padding of splice";
		}
		++begin;
		try
		{
			opadding=std::stoi(*begin);
			if(opadding<0)
			{
				return "Optimal padding of splice must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for optimal padding of splice";
		}
		++begin;
		try
		{
			mpadding=std::stoi(*begin);
			if(mpadding<0)
			{
				return "Minimum padding of splice must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for minimum padding of splice";
		}
		++begin;
		if(begin==end)
		{
			oheight=-1;
		}
		else
		{
			try
			{
				oheight=std::stoi(*begin);
				if(oheight<0)
				{
					return "Optimal height of splice must be non-negative";
				}
			}
			catch(std::exception const&)
			{
				return "Invalid input for optimal height of splice";
			}
		}
		del.flag=delivery::do_splice;
		del.splice_args.horiz_padding=hpadding;
		del.splice_args.min_padding=mpadding;
		del.splice_args.optimal_height=oheight;
		del.splice_args.optimal_padding=opadding;
		return nullptr;
	}
};

class BlurMaker:public CommandMaker {
public:
	char const* help_message() const override
	{
		return "Does a Gaussian blur of given standard deviation";
	}
	char const* make_command(iter begin,iter end,delivery& del) const override
	{
		if(del.flag>1)
		{
			return mci;
		}
		if(begin==end)
		{
			return "Too few arguments for blur";
		}
		float radius;
		try
		{
			radius=std::stof(*begin);
			if(radius<0)
			{
				return "Blur radius must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid arguments given for blur";
		}
		del.flag=del.do_single;
		del.pl.add_process<Blur>(radius);
		return nullptr;
	}
};
std::unordered_map<std::string,std::unique_ptr<CommandMaker>> init_commands()
{
	std::unordered_map<std::string,std::unique_ptr<CommandMaker>> commands;
	commands.emplace("-fg",make_unique<FilterGrayMaker>());
	commands.emplace("-cg",make_unique<ConvertGrayMaker>());
	commands.emplace("-ccg",make_unique<ClusterClearMaker>());
	commands.emplace("-hp",make_unique<HorizontalPaddingMaker>());
	commands.emplace("-vp",make_unique<VerticalPaddingMaker>());
	commands.emplace("-o",make_unique<OutputMaker>());
	commands.emplace("-ap",make_unique<AutoPaddingMaker>());
	commands.emplace("-cut",make_unique<CutMaker>());
	commands.emplace("-spl",make_unique<SpliceMaker>());
	commands.emplace("-bl",make_unique<BlurMaker>());
	commands.emplace("-rcg",make_unique<RescaleGrayMaker>());
	return commands;
}
std::unordered_map<std::string,std::unique_ptr<CommandMaker>> const commands=init_commands();
std::vector<std::string> conv_strings(int argc,char** argv)
{
	std::vector<std::string> ret;
	ret.reserve(argc+1);
	for(int i=0;i<argc;++i)
	{
		ret.emplace_back(argv[i]);
	}
	ret.emplace_back("-duMmMwMd");
	return ret;
}
std::vector<std::string> images_in_path(std::string const& path)
{
	auto ret=exlib::files_in_dir(path);
	for(auto& f:ret)
	{
		f=path+f;
	}
	ret.erase(std::remove_if(ret.begin(),ret.end(),[](auto& a)
	{
		return a.find('.')==std::string::npos;
	}),
		ret.end());
	return ret;
}
void png_stress_test()
{
	auto files=images_in_path("C:\\Users\\edwar\\Videos\\Score\\mcant\\cut\\");
	struct page:public CImg<uchar> {
		page(char const* pg):CImg(pg)
		{}
		uint top,bottom;
	};
	std::vector<page> t;
	size_t count=0;
	for(auto it=files.begin();it!=files.end();++it,++count)
	{
		t.emplace_back(it->c_str());
		t.back().save((*it+"copy.png").c_str());
		if(count%5==4)
		{
			t.erase(t.begin(),t.end()-1);
		}
	}
}
int main(int argc,char** argv)
{
	cimg::exception_mode(0);
	if(argc==1)
	{
		std::cout<<
			"Version: "
			__DATE__
			" "
			__TIME__
			" Copyright 2017-2018 Edward Xie"
			"\n"
			"filename_or_folder command params... ...\n"
			"Type command alone to get readme\n"
			"Available commands:\n"
			"  Single Page Operations:\n"
			"    Convert to Grayscale:     -cg\n"
			"    Filter Gray:              -fg min_value max_value=255 replacer=255\n"
			"    Cluster Clear Grayscale:  -ccg max_size min_size=0 background_color=255 tolerance=0.042\n"
			"    Horizontal Padding:       -hp amount\n"
			"    Vertical Padding:         -vp amount\n"
			"    Auto Padding:             -ap vert_padding min_horiz_padding max_horiz_padding horiz_offset=0 optimal_ratio=1.777777\n"
			"    Rescale Colors Grayscale: -rcg min mid max\n"
			"    Blur:                     -bl radius\n"
			"  Multi Page Operations:\n"
			"    Cut:                      -cut\n"
			"    Splice:                   -spl horiz_padding optimal_padding min_vert_padding optimal_height=(4/7 width of first page)\n"
			"Options\n"
			"  Output: -o format\n"
			"Any parameters given beyond the number requested are ignored.\n"
			"Multiple Single Page Operations can be done at once. They are performed in the order they are given.\n"
			"A Multi Page Operation can not be done with other operations.\n"
			;
		return 0;
	}
	auto args=conv_strings(argc,argv);
	typedef decltype(commands.end()) entry;

	std::string& arg1=args[1];
	if(arg1.front()=='-')
	{
		entry cmd=commands.find(arg1);
		if(cmd==commands.end())
		{
			std::cout<<"Unknown command\n";
			return 1;
		}
		std::cout<<cmd->second->help_message()<<'\n';
		return 0;
	}
	bool is_folder=arg1.back()=='\\'||arg1.back()=='/';

	CommandMaker::delivery del;
	del.flag=del.do_nothing;
	auto& output=del.sr;
	auto& processes=del.pl;

	entry cmdpair;
	typedef decltype(args.cbegin()) iter;
	iter arg_start=args.cbegin()+2;
	iter it=arg_start+1;
	for(it=arg_start+1;it!=args.cend();++it)
	{
		if(it->front()=='-'&&it->size()>1&&(*it)[1]>='a'&&(*it)[1]<='z')
		{
			entry cmd=commands.find(*arg_start);
			if(cmd==commands.end())
			{
				std::cout<<"Unknown command: "<<*arg_start<<'\n';
				return 1;
			}
			if(auto res=cmd->second->make_command(arg_start+1,it,del))
			{
				std::cout<<res<<'\n';
				return 1;
			}
			arg_start=it;
		}
	}

	if(output.empty())
	{
		if(del.flag==del.do_nothing)
		{
			std::cout<<"No commands given\n";
			return 0;
		}
		output.assign("%c");
	}
	unsigned int num_threads=std::thread::hardware_concurrency();
	if(num_threads==0)
	{
		num_threads=2;
	}
	switch(del.flag)
	{
		case del.do_nothing:
		case del.do_single:
		{
			if(is_folder)
			{
				auto files=images_in_path(arg1);
				try
				{
					processes.process(files,&output,num_threads);
				}
				catch(std::exception const& ex)
				{
					std::cout<<ex.what()<<'\n';
				}
			}
			else
			{
				try
				{
					processes.process(arg1.c_str(),&output);
				}
				catch(std::exception const& ex)
				{
					std::cout<<ex.what()<<'\n';
				}
			}
			break;
		}
		case del.do_cut:
		{
			if(is_folder)
			{
				auto files=images_in_path(arg1);
				class CutProcess:public exlib::ThreadTask {
				private:
					std::string const* input;
					SaveRules const* output;
				public:
					CutProcess(std::string const* input,SaveRules const* output):input(input),output(output)
					{}
					void execute() override
					{
						try
						{
							CImg<unsigned char> in(input->c_str());
							auto out=output->make_filename(*input);
							cut_page(in,out.c_str());
						}
						catch(std::exception const& ex)
						{
							std::cout<<ex.what()<<'\n';
						}
					}
				};
				exlib::ThreadPool tp(num_threads);
				for(auto const& f:files)
				{
					tp.add_task<CutProcess>(&f,&output);
				}
				tp.start();
			}
			else
			{
				auto out=output.make_filename(arg1);
				try
				{
					cut_page(CImg<unsigned char>(arg1.c_str()),out.c_str());
				}
				catch(std::exception const& ex)
				{
					std::cout<<ex.what()<<'\n';
					return 1;
				}
			}
			break;
		}
		case del.do_splice:
		{
			if(is_folder)
			{
				auto files=images_in_path(arg1);
				auto save=output.make_filename(arg1);
				try
				{
					splice_pages(
						files,
						del.splice_args.horiz_padding,
						del.splice_args.optimal_padding,
						del.splice_args.min_padding,
						del.splice_args.optimal_height,
						save.c_str());
				}
				catch(std::exception const& ex)
				{
					std::cout<<ex.what()<<'\n';
					return 1;
				}
				return 0;
			}
			else
			{
				std::cout<<"Splice requires folder input\n";
				return 1;
			}
		}
		//stop();
		return 0;
	}
}
