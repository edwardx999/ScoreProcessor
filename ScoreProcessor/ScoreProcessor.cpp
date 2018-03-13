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
class RemoveBorderGray:public ImageProcess<> {
	float tolerance;
public:
	RemoveBorderGray(float tolerance):tolerance(tolerance)
	{}
	void process(Img& image)
	{
		if(image._spectrum==1)
		{
			remove_border(image,0,tolerance);
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
class Rescale:public ImageProcess<> {
	double val;
public:
	Rescale(double val):val(val)
	{}
	void process(Img& img)
	{
		img.resize(
			scast<int>(std::round(img._width*val)),
			scast<int>(std::round(img._height*val)),
			1,
			1,
			val>1?5:2);
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

class Straighten:public ImageProcess<> {
	double pixel_prec;
	unsigned int num_steps;
	double min_angle,max_angle;
public:
	Straighten(double pixel_prec,double min_angle,double max_angle,double angle_prec)
		:pixel_prec(pixel_prec),
		min_angle(M_PI_2+min_angle*DEG_RAD),max_angle(M_PI_2+max_angle*DEG_RAD),
		num_steps(std::ceil((max_angle-min_angle)/angle_prec))
	{}
	void process(Img& img) override
	{
		auto_rotate_bare(img,pixel_prec,min_angle,max_angle,num_steps);
	}
};

void stop()
{
	cout<<"Stopped\n";
	std::this_thread::sleep_for(std::chrono::seconds(1000));
}
class CoutLog:public Log {
	CoutLog()
	{}
	static CoutLog singleton;
public:
	static Log& get()
	{
		return singleton;
	}
	void log(char const* msg,size_t) override
	{
		std::cout<<msg;
	}
	void log_error(char const* msg,size_t) override
	{
		std::cout<<msg;
	}
};
CoutLog CoutLog::singleton;

class CommandMaker {
public:
	typedef std::vector<std::string>::const_iterator iter;
	struct delivery {
		SaveRules sr;
		ProcessList<unsigned char> pl;
		enum do_state {
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
	virtual ~CommandMaker()=default;
	CommandMaker(CommandMaker const&)=delete;
	CommandMaker(CommandMaker&&)=delete;
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
	static char const* const mci;
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
char const* const SingleCommandMaker::mci="Single Command cannot be done with a Multi Command";

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
	FilterGrayMaker():SingleCommandMaker(1,3,"Replaces all values between min and max value inclusive with replacer","Filter Gray")
	{}
	static FilterGrayMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
FilterGrayMaker const FilterGrayMaker::singleton;

class ConvertGrayMaker:public SingleCommandMaker {
	ConvertGrayMaker():SingleCommandMaker(0,0,"Converts given image to Grayscale","Convert Gray")
	{}
	static ConvertGrayMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		del.pl.add_process<ChangeToGrayscale>();
		return nullptr;
	}
};
ConvertGrayMaker const ConvertGrayMaker::singleton;

class ClusterClearMaker:public SingleCommandMaker {
	ClusterClearMaker()
		:SingleCommandMaker(1,4,
			"All clusters of pixels that are outside of tolerance of background color\n"
			"and between min and max size are replaced by the background color",
			"Cluster Clear Grayscale")
	{}
	static ClusterClearMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		int max_size,min_size;
		Grayscale background;
		float tolerance;
		try
		{
			max_size=std::stoi(begin[0]);
			if(max_size<0)
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
			goto def_min_size;
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
			goto def_background;
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
			goto def_tolerance;
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
		del.pl.add_process<ClusterClearGray>(min_size,max_size,background,tolerance);
		return nullptr;
	def_min_size:
		min_size=0;
	def_background:
		background=255;
	def_tolerance:
		tolerance=0.042f;
		goto end;
	}
};
ClusterClearMaker const ClusterClearMaker::singleton;

class HorizontalPaddingMaker:public SingleCommandMaker {
	HorizontalPaddingMaker():
		SingleCommandMaker(1,1,"Pads the left and right sides of the image with given number of pixels","Horizontal Padding")
	{}
	static HorizontalPaddingMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
HorizontalPaddingMaker const HorizontalPaddingMaker::singleton;

class VerticalPaddingMaker:public SingleCommandMaker {
	VerticalPaddingMaker()
		:SingleCommandMaker(1,1,"Pads the top and bottom of the image with given number of pixels","Vertical Padding")
	{}
	static VerticalPaddingMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
VerticalPaddingMaker const VerticalPaddingMaker::singleton;

class OutputMaker:public CommandMaker {
	OutputMaker():
		CommandMaker(
			1,1,
			"Pattern templates:\n"
			"  %c copy whole filename\n"
			"  %p copy path (includes trailing slash)\n"
			"  %x copy extension (does not include dot)\n"
			"  %f copy filename (does not include path, dot, or extension)\n"
			"  %0 any number from 0-9, index of file with specified number of padding\n"
			"  %% literal percent\n"
			"Anything else will be interpreted as a literal character\n"
			"Pattern is %c if no output is specified",
			"Output Pattern")
	{}
	static OutputMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
OutputMaker const OutputMaker::singleton;

class AutoPaddingMaker:public SingleCommandMaker {
	AutoPaddingMaker()
		:SingleCommandMaker(
			3,5,
			"Attempts to make the image fit the desired ratio.\n"
			"Top and bottom are padded by vertical padding.\n"
			"Left is padded by somewhere between min padding and max padding.\n"
			"Right is padded by left padding plus horizontal offset.",
			"Auto Padding")
	{}
	static AutoPaddingMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
AutoPaddingMaker const AutoPaddingMaker::singleton;

class RescaleGrayMaker:public SingleCommandMaker {
	RescaleGrayMaker()
		:SingleCommandMaker(3,3,
			"Colors are scaled such that values less than or equal to min become 0,\n"
			"and values greater than or equal to max becomes 255.\n"
			"They are scaled based on their distance from mid.",
			"Rescale Gray")
	{}
	static RescaleGrayMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
RescaleGrayMaker const RescaleGrayMaker::singleton;

class CutMaker:public CommandMaker {
	CutMaker()
		:CommandMaker(0,0,"Cuts the image into separate systems","Cut")
	{}
	static CutMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
CutMaker const CutMaker::singleton;

class SpliceMaker:public CommandMaker {
	SpliceMaker():
		CommandMaker(
			3,4,
			"Splices the pages together assuming right alignment.\n"
			"Greedy algorithm that tries to minimize deviation from optimal height and optimal padding.\n"
			"Horizontal padding is the padding placed between elements of the page horizontally.\n"
			"Min padding is the minimal vertical padding between pages.",
			"Splice")
	{}
	static SpliceMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
		if(n==3)
		{
			oheight=-1;
		}
		else
		{
			try
			{
				oheight=std::stoi(begin[3]);
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
		del.splice_args.optimal_padding=opadding;
		del.splice_args.optimal_height=oheight;
		return nullptr;
	}
};
SpliceMaker const SpliceMaker::singleton;

class BlurMaker:public SingleCommandMaker {
	BlurMaker()
		:SingleCommandMaker(1,1,"Gaussian blur of given standard deviation","Blur")
	{}
	static BlurMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
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
BlurMaker const BlurMaker::singleton;

class StraightenMaker:public SingleCommandMaker {
	StraightenMaker()
		:SingleCommandMaker(
			0,4,
			"Straightens the image\n"
			"Min angle is minimum angle of range to consider rotation (in degrees)\n"
			"Max angle is maximum angle of range to consider rotation (in degrees)\n"
			"Angle precision is the precision in measuring angle (in degrees)\n"
			"Pixel precision is precision when measuring distance from origin",
			"Straighten")
	{}
	static StraightenMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		double pixel_prec,min_angle,max_angle,angle_prec;
	#define assign_val(val,index,cond,check_statement,error_statement)\
		if(n>##index##)\
		{\
			try\
			{\
				##val##=std::stod(begin[##index##]);\
				if(##cond##)\
				{\
					return ##check_statement##;\
				}\
			}\
			catch(std::exception const&)\
			{\
				return "Invalid input for " ## error_statement ##;\
			}\
		}\
		else\
		{\
			goto init_##val##;\
		}
		assign_val(min_angle,0,false,"","minimum angle");
		assign_val(max_angle,1,false,"","maximum angle");
		assign_val(angle_prec,2,angle_prec<=0,"Angle precision must be positive","angle precision");
		assign_val(pixel_prec,3,pixel_prec<=0,"Pixel precision must be positive","pixel precision");
	#undef assign_val
		;
	end:
		if(min_angle>max_angle)
		{
			return "Max angle must be greater than min angle";
		}
		if(max_angle-min_angle>180)
		{
			return "Difference between angles must be less than or equal to 180";
		}
		del.pl.add_process<Straighten>(pixel_prec,min_angle,max_angle,angle_prec);
		return nullptr;
	init_min_angle:
		min_angle=-5.0;
	init_max_angle:
		max_angle=5.0;
	init_angle_prec:
		angle_prec=0.1;
	init_pixel_prec:
		pixel_prec=1.0;
		goto end;
	}
};
StraightenMaker const StraightenMaker::singleton;

class RemoveBorderMaker:public SingleCommandMaker {
	RemoveBorderMaker()
		:SingleCommandMaker(0,1,"Removes border of image (SUPER BETA VERSION)","Remove Border")
	{}
	static RemoveBorderMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		float tolerance;
		if(n>0)
		{
			try
			{
				tolerance=std::stof(*begin);
				if(tolerance<0||tolerance>1)
				{
					return "Tolerance must be in range [0,1]";
				}
			}
			catch(std::exception const&)
			{
				return "Invalid input for tolerance";
			}
		}
		else
		{
			tolerance=0.5;
		}
		del.pl.add_process<RemoveBorderGray>(tolerance);
		return nullptr;
	}
};
RemoveBorderMaker const RemoveBorderMaker::singleton;

class RescaleMaker:public SingleCommandMaker {
	RescaleMaker():SingleCommandMaker(1,1,"Rescales image by given factor","Rescale")
	{}
	static RescaleMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t,delivery& del) const override
	{
		try
		{
			double factor=std::stod(*begin);
			if(factor<0)
			{
				return "Rescale factor must be non-negative";
			}
			del.pl.add_process<Rescale>(factor);
		}
		catch(std::exception const&)
		{
			return "Invalid input for rescale factor";
		}
		return nullptr;
	}
};
RescaleMaker const RescaleMaker::singleton;

class LogMaker:public CommandMaker {
	LogMaker()
		:CommandMaker(1,1,"Changes verbosity of output: Silent=0, Errors-only=1 (default), Loud=2","Verbosity")
	{}
	static LogMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command(iter begin,size_t,delivery& del) const override
	{
		if(del.pl.get_log())
		{
			return "Verbosity level already given";
		}
		try
		{
			int lvl=std::stoi(*begin);
			if(lvl<0||lvl>2)
			{
				return "Invalid level";
			}
			del.pl.set_log(&CoutLog::get());
			del.pl.set_verbosity(decltype(del.pl)::verbosity(lvl));
			return nullptr;
		}
		catch(std::exception const&)
		{
			return "Invalid input for level";
		}
	}
};
LogMaker const LogMaker::singleton;

std::unordered_map<std::string,CommandMaker const*> const init_commands()
{
	std::unordered_map<std::string,CommandMaker const*> commands;
	commands.emplace("-fg",&FilterGrayMaker::get());

	commands.emplace("-cg",&ConvertGrayMaker::get());

	commands.emplace("-ccg",&ClusterClearMaker::get());

	commands.emplace("-hp",&HorizontalPaddingMaker::get());

	commands.emplace("-vp",&VerticalPaddingMaker::get());

	commands.emplace("-o",&OutputMaker::get());

	commands.emplace("-ap",&AutoPaddingMaker::get());

	commands.emplace("-cut",&CutMaker::get());

	commands.emplace("-spl",&SpliceMaker::get());
	commands.emplace("-splice",&SpliceMaker::get());

	commands.emplace("-bl",&BlurMaker::get());
	commands.emplace("-blur",&BlurMaker::get());

	commands.emplace("-rcg",&RescaleGrayMaker::get());

	commands.emplace("-rb",&RemoveBorderMaker::get());

	commands.emplace("-vb",&LogMaker::get());
	commands.emplace("-verbosity",&LogMaker::get());

	commands.emplace("-str",&StraightenMaker::get());
	commands.emplace("-straighten",&StraightenMaker::get());

	commands.emplace("-rs",&RescaleMaker::get());
	commands.emplace("-rescale",&RescaleMaker::get());

	return commands;
}

auto commands=init_commands();

std::vector<std::string> conv_strings(int argc,char** argv)
{
	std::vector<std::string> ret;
	ret.reserve(argc+1);
	for(int i=0;i<argc;++i)
	{
		ret.emplace_back(argv[i]);
	}
	ret.emplace_back("-duMmMwMd");//dummy used to trigger analyzing the last function input
	return ret;
}
std::vector<std::string> images_in_path(std::string const& path)
{
	auto ret=exlib::files_in_dir(path);
	for(auto& f:ret)
	{
		f=path+f;
	}
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
	CImg<unsigned char> test_img("cuttest.jpg");
	CImg<float> map=create_vertical_energy(test_img);
	add_horizontal_energy(test_img,map);
	map.display();
	stop();
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
			"    Straighten:               -str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1\n"
			"    Remove Border (DANGER):   -rb tolerance=0.5\n"
			"    Rescale:                  -rs factor\n"
			"  Multi Page Operations:\n"
			"    Cut:                      -cut\n"
			"    Splice:                   -spl horiz_padding optimal_padding min_vert_padding optimal_height=(6/11 width of first page)\n"
			"  Options:\n"
			"    Output:                   -o format\n"
			"    Verbosity:                -vb level\n"
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
	if(!del.pl.get_log())
	{
		del.pl.set_log(&CoutLog::get());
		del.pl.set_verbosity(decltype(del.pl)::errors_only);
	}
	auto num_threads=[]()
	{
		auto nt=std::thread::hardware_concurrency();
		return nt==0?2U:nt;
	};
	switch(del.flag)
	{
		case del.do_nothing:
		case del.do_single:
		{
			if(is_folder)
			{
				auto files=images_in_path(arg1);
				if(files.empty())
				{
					std::cout<<"No files found\n";
					return 0;
				}
				processes.process(files,&output,num_threads());
			}
			else
			{
				processes.process(arg1.c_str(),&output);
			}
			break;
		}
		case del.do_cut:
		{
			if(is_folder)
			{
				auto files=images_in_path(arg1);
				if(files.empty())
				{
					std::cout<<"No files found\n";
					return 0;
				}
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
				if(files.empty())
				{
					std::cout<<"No files found\n";
					return 0;
				}
				auto save=output.make_filename(files[0]);
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
