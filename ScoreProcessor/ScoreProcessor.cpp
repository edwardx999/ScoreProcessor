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
	typedef std::vector<std::string>::const_iterator iter;
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
private:
	unsigned int min_args;
	unsigned int max_args;
	char const* _help_message;
	char const* _name;
protected:
	CommandMaker(unsigned int min_args,unsigned int max_args,char const* hm,char const* nm)
		:min_args(min_args),max_args(max_args),_help_message(hm),_name(nm)
	{}
	virtual char const* parse_command(iter begin,size_t num_args,delivery&) const=0;
public:
	static char const* const mci;
	virtual ~CommandMaker()=default;
	char const* help_message() const
	{
		return _help_message;
	}
	char const* name() const
	{
		return _name;
	}
	char const* make_command(iter begin,iter end,delivery& del) const
	{
		size_t n=std::distance(begin,end);
		if(n<min_args)
		{
			return "Too few parameters";
		}
		if(n>max_args)
		{
			return "Too many parameters";
		}
		return parse_command(begin,n,del);
	}

};

class SingleCommandMaker:public CommandMaker {
protected:
	virtual char const* parse_command_h(iter begin,size_t num_args,delivery&) const=0;
	SingleCommandMaker(unsigned int min_args,unsigned int max_args,char const* hm,char const* nm)
		:CommandMaker(min_args,max_args,hm,nm)
	{}
public:
	char const* const mci="Single Command cannot be done with a Multi Command";
	char const* parse_command(iter begin,size_t num_args,delivery& del) const override final
	{
		if(del.flag>1)
		{
			return mci;
		}
		del.flag=del.do_single;
		return parse_command_h(begin,num_args,del);
	}
};

class FilterGrayMaker:public SingleCommandMaker {
	static char const* assign_val(iter arg,int* hold)
	{
		try
		{
			*hold=std::stoi(*arg);
			if(*hold<0||*hold>255)
			{
				return "Values must be in range [0,255]";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid parameter given";
		}
		return nullptr;
	}
public:
	FilterGrayMaker():SingleCommandMaker(1,3,"Replaces all values between min and max value inclusive with replacer","Filter Gray")
	{}
	char const* parse_command_h(iter argb,size_t n,delivery& del) const override
	{
		int params[3];
		size_t i;
		for(i=0;i<n;++i)
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
		del.pl.add_process<FilterGray>(params[0],params[1],Grayscale(params[2]));
		return nullptr;
	}
};

class ConvertGrayMaker:public SingleCommandMaker {
public:
	ConvertGrayMaker():SingleCommandMaker(0,0,"Converts given image to Grayscale","Convert Gray")
	{}
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		del.pl.add_process<ChangeToGrayscale>();
		return nullptr;
	}
};

class ClusterClearMaker:public SingleCommandMaker {
public:
	ClusterClearMaker()
		:SingleCommandMaker(1,4,
			"All clusters of pixels that are outside of tolerance of background color\n"
			"and between min and max size are replaced by the background color",
			"Cluster Clear Grayscale")
	{}
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		int max_size,min_size=0;
		Grayscale background=255;
		float tolerance=0.042;
		try
		{
			max_size=std::stoi(begin[0]);
			if(min_size<0)
			{
				return "Maximum cluster size must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for maximum size";
		}
		if(n<2)
		{
			goto end;
		}
		try
		{
			min_size=std::stoi(begin[1]);
			if(min_size<0)
			{
				return "Minimum cluster size must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for minimum size";
		}
		if(max_size<min_size)
		{
			return "Max size must be greater than min size";
		}
		if(n<3)
		{
			goto end;
		}
		try
		{
			int bg=std::stoi(begin[2]);
			if(bg>255||bg<0)
			{
				return "Background color must be in range [0,255]";
			}
			background=bg;
		}
		catch(std::exception const&)
		{
			return "Invalid input for background color";
		}
		if(n<4)
		{
			goto end;
		}
		try
		{
			tolerance=std::stof(begin[3]);
			if(tolerance<0||tolerance>1)
			{
				return "Tolerance must be between 0 and 1";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for tolerance";
		}
	end:
		del.pl.add_process<ClusterClearGray>(min_size,max_size,Grayscale(background),tolerance);
		return nullptr;
	}
};

class HorizontalPaddingMaker:public SingleCommandMaker {
public:
	HorizontalPaddingMaker():
		SingleCommandMaker(1,1,"Pads the left and right sides of the image with given number of pixels","Horizontal Padding")
	{}
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		try
		{
			int amount=std::stoi(*begin);
			if(amount<0)
			{
				return "Padding must be non-negative";
			}
			del.pl.add_process<PadHoriz>(amount);
			return nullptr;
		}
		catch(std::exception const&)
		{
			return "Invalid input";
		}
	}
};

class VerticalPaddingMaker:public SingleCommandMaker {
public:
	VerticalPaddingMaker()
		:SingleCommandMaker(1,1,"Pads the top and bottom of the image with given number of pixels","Vertical Padding")
	{}
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		try
		{
			int amount=std::stoi(*begin);
			if(amount<0)
			{
				return "Amount must be non-negative";
			}
			del.pl.add_process<PadVert>(amount);
			return 0;
		}
		catch(std::exception const&)
		{
			return "Invalid input";
		}
	}
};

class OutputMaker:public CommandMaker {
public:
	OutputMaker():
		CommandMaker(
			1,1,
			"Pattern templates:\n"
			"  %c copy whole filename\n"
			"  %p copy path\n"
			"  %x copy extension\n"
			"  %f copy filename\n"
			"  %0 any number from 0-9, index of file with specified number of padding\n"
			"  %% literal percent\n"
			"Anything else will be interpreted as a literal character",
			"Output Pattern")
	{}
	char const* parse_command(iter begin,size_t n,delivery& del) const override
	{
		if(del.sr.empty())
		{
			try
			{
				del.sr.assign(*begin);
			}
			catch(std::exception const&)
			{
				return "Invalid filename template";
			}
		}
		else
		{
			return "Filename template already given";
		}
		return nullptr;
	}
};

class AutoPaddingMaker:public SingleCommandMaker {
public:
	AutoPaddingMaker()
		:SingleCommandMaker(
			3,5,
			"Attempts to make the image fit the desired ratio.\n"
			"Top and bottom are padded by vertical padding.\n"
			"Left is padded by somewhere between min padding and max padding.\n"
			"Right is padded by left padding plus horizontal offset.",
			"Auto Padding")
	{}
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		int vert,minh,maxh,hoff;
		float opt_rat;
		try
		{
			vert=std::stoul(begin[0]);
			if(vert<0)
			{
				return "Vertical padding must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid argument given for vertical padding";
		}
		try
		{
			minh=std::stoi(begin[1]);
			if(minh<0)
			{
				return "Minimum horizontal padding must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for minimum horizontal padding";
		}
		try
		{
			maxh=std::stoi(begin[2]);
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
		if(n>3)
		{
			try
			{
				hoff=std::stoi(begin[3]);
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
		if(n>4)
		{
			try
			{
				opt_rat=std::stof(begin[4]);
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
		del.pl.add_process<PadAuto>(vert,minh,maxh,hoff,opt_rat);
		return nullptr;
	}
};

class RescaleGrayMaker:public SingleCommandMaker {
public:
	RescaleGrayMaker()
		:SingleCommandMaker(3,3,
			"Colors are scaled such that values less than or equal to min become 0,\n"
			"and values greater than or equal to max becomes 255.\n"
			"They are scaled based on their distance from mid.",
			"Rescale Gray")
	{}
	char const* parse_command_h(iter begin,size_t,delivery& del) const override
	{
		char const* const errors[]=
		{"Invalid argument for min value",
			"Invalid argument for mid value",
			"Invalid argument for max value"};
		int params[3];
		for(size_t i=0;i<3;++i)
		{
			try
			{
				params[i]=std::stoi(begin[i]);
				if(params[i]<0||params[i]>255)
				{
					return "Values must be in range [0,255]";
				}
			}
			catch(std::exception const&)
			{
				return errors[i];
			}
		}
		if(params[0]>params[1])
		{
			return "Min value must be less than or equal to mid value";
		}
		if(params[1]>params[2])
		{
			return "Mid value must be less than or equal to max value";
		}
		del.pl.add_process<RescaleGray>(params[0],params[1],params[2]);
		return nullptr;
	}
};

class CutMaker:public CommandMaker {
public:
	CutMaker()
		:CommandMaker(0,0,"Cuts the image into separate systems","Cut")
	{}
	char const* parse_command(iter begin,size_t,delivery& del) const override
	{
		if(del.flag)
		{
			return "Cut can not be done along with other commands";
		}
		del.flag=delivery::do_cut;
		return nullptr;
	}
};

class SpliceMaker:public CommandMaker {
public:
	SpliceMaker():
		CommandMaker(
			3,4,
			"Splices the pages together assuming right alignment.\n"
			"Greedy algorithm that tries to minimize deviation from optimal height and optimal padding.\n"
			"Horizontal padding is the padding placed between elements of the page horizontally.\n"
			"Min padding is the minimal vertical padding between pages.",
			"Splice")
	{}
	char const* parse_command(iter begin,size_t n,delivery& del) const override
	{
		if(del.flag)
		{
			return "Splice can not be done along with other commands";
		}
		del.flag=del.do_splice;
		int hpadding,opadding,mpadding,oheight;
		try
		{
			hpadding=std::stoi(begin[0]);
			if(hpadding<0)
			{
				return "Horizontal padding must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for horizontal padding";
		}
		try
		{
			opadding=std::stoi(begin[1]);
			if(opadding<0)
			{
				return "Optimal padding must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for optimal padding";
		}
		try
		{
			mpadding=std::stoi(begin[2]);
			if(mpadding<0)
			{
				return "Minimum padding must be non-negative";
			}
		}
		catch(std::exception const&)
		{
			return "Invalid input for minimum padding";
		}
		if(n<4)
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
					return "Optimal height must be non-negative";
				}
			}
			catch(std::exception const&)
			{
				return "Invalid input for optimal height";
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

class BlurMaker:public SingleCommandMaker {
public:
	BlurMaker()
		:SingleCommandMaker(1,1,"Gaussian blur of given standard deviation","Blur")
	{}
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
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
		del.pl.add_process<Blur>(radius);
		return nullptr;
	}
};

class RemoveBorderMaker:public SingleCommandMaker {
public:
	RemoveBorderMaker()
		:SingleCommandMaker(0,0,"Removes border of image (SUPER BETA VERSION)","Remove Border")
	{}
	char const* parse_command_h(iter begin,size_t,delivery& del) const override
	{
		del.pl.add_process<RemoveProcess>();
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
	commands.emplace("-rb",make_unique<RemoveBorderMaker>());
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
std::string pretty_date()
{
	std::string ret(__DATE__);
	if(ret[4]==' ')
	{
		ret[4]='0';
	}
	return ret;
}
void test()
{

}
int main(int argc,char** argv)
{
	cimg::exception_mode(0);
	if(argc==1)
	{
		std::cout<<
			"Version: "<<
			pretty_date()<<
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
			"    Auto Padding:             -ap vert_padding min_horiz_padding max_horiz_padding horiz_offset=0 optimal_ratio=1.777778\n"
			"    Rescale Colors Grayscale: -rcg min mid max\n"
			"    Blur:                     -bl radius\n"
			"  Multi Page Operations:\n"
			"    Cut:                      -cut\n"
			"    Splice:                   -spl horiz_padding optimal_padding min_vert_padding optimal_height=(4/7 width of first page)\n"
			"  Options:\n"
			"    Output:                   -o format\n"
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
		auto& m=cmd->second;
		std::cout<<m->name()<<":\n"<<m->help_message()<<'\n';
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
			auto& m=cmd->second;
			if(auto res=m->make_command(arg_start+1,it,del))
			{
				std::cout<<m->name()<<" Error:\n"<<res<<'\n';
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
	auto num_threads=[]()
	{
		auto nt=std::thread::hardware_concurrency();
		return nt==0?2:nt;
	};
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
					processes.process(files,&output,num_threads());
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
				exlib::ThreadPool tp(num_threads());
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
