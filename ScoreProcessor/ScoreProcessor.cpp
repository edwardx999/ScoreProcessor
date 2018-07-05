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
#include "support.h"
#include <unordered_map>
#include <map>
#include <optional>
#include <array>
#include <numeric>
#include <sstream>
#include <variant>
#include "parse.h"
using namespace cimg_library;
using namespace std;
using namespace ScoreProcessor;
using namespace ImageUtils;

#define make_perc_val(pv,valname,str,PercConstraints,ValConstraints)\
	if(##str##.back()=='%')\
	{\
		##pv##.is_perc=true;\
		##str##.back()='\0';\
		if(parse_str(##pv##.perc,##str##.c_str()))\
		{\
			return "Invalid " #valname " input";\
		}\
		if(##pv##.perc<0||##pv##.perc>100) return #valname " input must be in range [0,100]%";\
		##PercConstraints##\
	}\
	else\
	{\
		##pv##.is_perc=false;\
		if(parse_str(##pv##.val,##str##.c_str()))\
		{\
			return "Invalid" #valname " input";\
		}\
		##ValConstraints##\
	}

class ChangeToGrayscale:public ImageProcess<> {
public:
	bool process(Img& img) const
	{
		if(img._spectrum>=3)
		{
			img=get_grayscale_simple(img);
			return true;
		}
		return false;
	}
};
class FillTransparency:public ImageProcess<> {
	ImageUtils::ColorRGB background;
public:
	FillTransparency(unsigned char r,unsigned char g,unsigned char b)
	{
		background={r,g,b};
	}
	bool process(Img& img) const
	{
		if(img._spectrum>=4)
		{
			return fill_transparency(img,background);
		}
		return false;
	}
};
class RemoveBorderGray:public ImageProcess<> {
	float tolerance;
public:
	RemoveBorderGray(float tolerance):tolerance(tolerance)
	{}
	bool process(Img& img) const
	{
		if(img._spectrum==1)
		{
			return remove_border(img,0,tolerance);
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
	bool process(Img& img) const
	{
		if(img._spectrum<3)
		{
			return replace_range(img,min,max,replacer);
		}
		else
		{
			return replace_by_brightness(img,min,max,ColorRGB({replacer,replacer,replacer}));
		}
	}
};
class FilterHSV:public ImageProcess<> {
	ColorHSV start,end;
	ColorRGB replacer;
public:
	FilterHSV(ColorHSV start,ColorHSV end,ColorRGB replacer):start(start),end(end),replacer(replacer)
	{}
	bool process(Img& img) const override
	{
		if(img._spectrum>=3)
		{
			return replace_by_hsv(img,start,end,replacer);
		}
		else
		{
			throw std::invalid_argument("Color (3+ spectrum) image required");
		}
	}
};
class FilterRGB:public ImageProcess<> {
	ColorRGB start,end;
	ColorRGB replacer;
public:
	FilterRGB(ColorRGB start,ColorRGB end,ColorRGB replacer):start(start),end(end),replacer(replacer)
	{}
	bool process(Img& img) const override
	{
		if(img._spectrum>=3)
		{
			return replace_by_rgb(img,start,end,replacer);
		}
		else
		{
			throw std::invalid_argument("Color (3+ spectrum) image required");
		}
	}
};
class PadHoriz:public ImageProcess<> {
	unsigned int left,right;
	perc_or_val tol;
	unsigned char background;
public:
	PadHoriz(unsigned int const left,unsigned int const right,perc_or_val tol,unsigned char bg):left(left),right(right),tol(tol),background(bg)
	{}
	bool process(Img& img) const
	{
		return horiz_padding(img,left,right,tol(img._height),background);
	}
};
class PadVert:public ImageProcess<> {
	unsigned int top,bottom;
	perc_or_val tol;
	unsigned char background;
public:
	PadVert(unsigned int const top,unsigned int const bottom,perc_or_val tol,unsigned char bg):top(top),bottom(bottom),tol(tol),background(bg)
	{}
	bool process(Img& img) const
	{
		return vert_padding(img,top,bottom,tol(img._width),background);
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
	bool process(Img& img) const
	{
		return auto_padding(img,vert,max_h,min_h,hoff,opt_rat);
	}
};
class PadCluster:public ImageProcess<> {
	unsigned int left,right,top,bottom;
	unsigned char bt;
public:
	PadCluster(unsigned int left,unsigned int right,unsigned int top,unsigned int bottom,unsigned char bt):
		left(left),right(right),bottom(bottom),top(top),bt(bt)
	{}
	bool process(Img& img) const override
	{
		return cluster_padding(img,left,right,top,bottom,bt);
	}
};
class Rescale:public ImageProcess<> {
	double val;
	int interpolation;
public:
	enum rescale_mode {
		automatic=-2,
		raw_mem,
		boundary_fill,
		nearest_neighbor,
		moving_average,
		linear,
		grid,
		cubic,
		lanczos
	};
	Rescale(double val,int interpolation):
		val(val),
		interpolation(interpolation==automatic?(val>1?cubic:moving_average):interpolation)
	{}
	bool process(Img& img) const
	{
		img.resize(
			scast<int>(std::round(img._width*val)),
			scast<int>(std::round(img._height*val)),
			img._depth,
			img._spectrum,
			interpolation);
		return true;
	}
};

class ExtractLayer0NoRealloc:public ImageProcess<> {
public:
	bool process(Img& img) const override
	{
		if(img._spectrum>1U)
		{
			img._spectrum=1U;
			return true;
		}
		return false;
	}
};

class ClusterClearGray:public ImageProcess<> {
	unsigned int min,max;
	Grayscale background;
	float tolerance;
public:
	ClusterClearGray(unsigned int min,unsigned int max,Grayscale background,float tolerance):min(min),max(max),background(background),tolerance(tolerance)
	{}
	bool process(Img& img) const override
	{
		if(img._spectrum==1)
		{
			return clear_clusters(img,
				std::array<unsigned char,1>({background}),
				[this](std::array<unsigned char,1> v)
			{
				return gray_diff(v[0],background)<tolerance;
			},
				[this](Cluster const& cluster)
			{
				auto size=cluster.size();
				return size>=min&&size<=max;
			});
		}
		else
		{
			throw std::invalid_argument("Cluster Clear Grayscale requires grayscale image");
		}
	}
};
class ClusterClearGrayAlt:public ImageProcess<> {
	unsigned char required_min,required_max;
	unsigned int min_size,max_size;
	unsigned char sel_min;
	unsigned char sel_max;
	unsigned char background;
public:
	ClusterClearGrayAlt(unsigned char rcmi,unsigned char rcma,unsigned int mis,unsigned mas,unsigned char smi,unsigned char sma,unsigned char back):
		required_min(rcmi),required_max(rcma),min_size(mis),max_size(mas),sel_min(smi),sel_max(sma),background(back)
	{}
	bool process(Img& img) const
	{
		if(img._spectrum<3)
		{
			return clear_clusters(img,std::array<unsigned char,1>({background}),
				[this](std::array<unsigned char,1> v)
			{
				return v[0]>=sel_min&&v[0]<=sel_max;
			},
				[this,&img](Cluster const& c)
			{
				auto size=c.size();
				if(size>=min_size&&size<=max_size)
				{
					return true;
				}
				if(sel_min>=required_min&&sel_max<=required_max)
				{
					return false;
				}
				for(auto rect:c.get_ranges())
				{
					for(unsigned int y=rect.top;y<rect.bottom;++y)
					{
						auto row=img._data+y*img._width;
						for(unsigned int x=rect.left;x<rect.right;++x)
						{
							auto pix=*(row+x);
							if(pix>=required_min&&pix<=required_max)
							{
								return false;
							}
						}
					}
				}
				return true;
			});
		}
		else
		{
			return clear_clusters(img,std::array<unsigned char,3>({background,background,background}),
				[this](std::array<unsigned char,3> v)
			{
				auto const brightness=(float(v[0])+v[1]+v[2])/3.0f;
				return brightness>=sel_min&&brightness<=sel_max;
			},
				[this,&img](Cluster const& c)
			{
				auto size=c.size();
				if(size>=min_size&&size<=max_size)
				{
					return true;
				}
				if(sel_min>=required_min&&sel_max<=required_max)
				{
					return false;
				}
				unsigned int const img_size=img._width*img._height;
				for(auto rect:c.get_ranges())
				{
					for(unsigned int y=rect.top;y<rect.bottom;++y)
					{
						auto row=img._data+y*img._width;
						for(unsigned int x=rect.left;x<rect.right;++x)
						{
							auto pix=(row+x);
							float brightness=0;
							for(unsigned int s=0;s<3;++s)
							{
								brightness+=*(pix+s*img_size);
							}
							brightness/=3;
							if(brightness>=required_min&&brightness<=required_max)
							{
								return false;
							}
						}
					}
				}
				return true;
			});
		}
	}
};
class RescaleGray:public ImageProcess<> {
	unsigned char min,mid,max;
public:
	RescaleGray(unsigned char min,unsigned char mid,unsigned char max=255):min(min),mid(mid),max(max)
	{}
	bool process(Img& img) const
	{
		if(img._spectrum<3)
		{
			rescale_colors(img,min,mid,max);

		}
		else
		{
			rescale_colors(img,min,mid,max);
			rescale_colors(img.get_shared_channel(1),min,mid,max);
			rescale_colors(img.get_shared_channel(2),min,mid,max);
		}
		return true;
	}
};
class FillSelectionAbsGray:public ImageProcess<> {
public:
	enum origin_reference {
		top_left,top_middle,top_right,
		middle_left,middle,middle_right,
		bottom_left,bottom_middle,bottom_right
	};
private:
	ImageUtils::Rectangle<signed int> offsets;
	origin_reference origin;
	Grayscale color;
	static ImageUtils::Point<signed int> get_origin(origin_reference origin_code,int width,int height)
	{
		ImageUtils::Point<signed int> porigin;
		switch(origin_code%3)
		{
			case 0:
				porigin.x=0;
				break;
			case 1:
				porigin.x=width/2;
				break;
			case 2:
				porigin.x=width;
		}
		switch(origin_code/3)
		{
			case 0:
				porigin.y=0;
				break;
			case 1:
				porigin.y=height/2;
				break;
			case 2:
				porigin.y=height;
		}
		return porigin;
	}
public:
	FillSelectionAbsGray(ImageUtils::Rectangle<signed int> offsets,Grayscale g,origin_reference orgn):offsets(offsets),color(g),origin(orgn)
	{
		assert(origin>=top_left&&origin<=bottom_right);
	}
	bool process(Img& img) const override
	{
		auto const porigin=get_origin(origin,img.width(),img.height());
		ImageUtils::Rectangle<signed int> rect;
		rect.left=offsets.left+porigin.x;
		rect.right=offsets.right+porigin.x;
		rect.top=offsets.top+porigin.y;
		rect.bottom=offsets.bottom+porigin.y;
		if(rect.left<0)
		{
			rect.left=0;
		}
		if(rect.top<0)
		{
			rect.top=0;
		}
		if(rect.right>img.width())
		{
			rect.right=img.width();
		}
		if(rect.bottom>img.height())
		{
			rect.bottom=img.height();
		}
		unsigned char buffer[10];
		for(int i=0;i<img.spectrum();++i)
		{
			buffer[i]=color;
		}
		return fill_selection(
			img,
			{scast<uint>(rect.left),scast<uint>(rect.right),scast<uint>(rect.top),scast<uint>(rect.bottom)},
			buffer,check_fill_t());
	}
};
class Blur:public ImageProcess<> {
	float radius;
public:
	Blur(float radius):radius(radius)
	{}
	bool process(Img& img) const override
	{
		img.blur(radius);
		return true;
	}
};
class Straighten:public ImageProcess<> {
	double pixel_prec;
	unsigned int num_steps;
	double min_angle,max_angle;
	unsigned char boundary;
public:
	Straighten(double pixel_prec,double min_angle,double max_angle,double angle_prec,unsigned char boundary)
		:pixel_prec(pixel_prec),
		min_angle(M_PI_2+min_angle*DEG_RAD),max_angle(M_PI_2+max_angle*DEG_RAD),
		num_steps(std::ceil((max_angle-min_angle)/angle_prec)),
		boundary(boundary)
	{}
	bool process(Img& img) const override
	{
		return auto_rotate_bare(img,pixel_prec,min_angle,max_angle,num_steps,boundary)!=0;
	}
};
class Rotate:public ImageProcess<> {
	float angle;
public:
	Rotate(float angle):angle(angle)
	{}
	bool process(Img& img) const override
	{
		img.rotate(angle,2,1);
		return true;
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

class AmountLog:public Log {
private:
	std::atomic<size_t> count;//count of finished pages
	size_t amount;
	unsigned int num_digs;
	size_t buffer_length;
	std::unique_ptr<char[]> message_template;
	std::mutex mtx;
	bool begun;//atomicity not needed, double writing the first thing is ok
	static constexpr size_t const START_STRLEN=9/*=strlen("Finished ")*/;
	static constexpr size_t const BUFFER_SIZE=START_STRLEN+3+2*exlib::num_digits(size_t(-1));//enough to fit message and digits of max value of size_t
public:
	AmountLog(size_t amount):
		amount(amount),
		count(0),
		num_digs(exlib::num_digits(amount)),
		buffer_length(START_STRLEN+3+2*num_digs),
		message_template(new char[buffer_length]),
		begun(false)
	{
		memcpy(message_template.get(),"Finished ",START_STRLEN);
		for(auto it=message_template.get()+START_STRLEN;it<message_template.get()+START_STRLEN+num_digs-1;++it)
		{
			*it=' ';
		}
		message_template[START_STRLEN-1+num_digs]='0';
		message_template[START_STRLEN+num_digs]='/';
		std::to_chars(message_template.get()+START_STRLEN+num_digs+1,message_template.get()+START_STRLEN+2*num_digs+1,amount);
		message_template[START_STRLEN+1+2*num_digs]='\r';
		message_template[START_STRLEN+2+2*num_digs]='\0';
	}
private:
	void insert_message(char* place,size_t num)
	{
		memcpy(place,message_template.get(),buffer_length);
		char* it=place+START_STRLEN+num_digs-1;
		do
		{
			*it=num%10+'0';
			--it;
			num/=10;
		} while(num);
	}
public:
	void log(char const* msg,size_t) override
	{
		if(msg[0]=='S')
		{
			if(!begun)
			{
				begun=true;
				std::cout<<message_template.get();
			}
		}
		else
		{
			char buffer[BUFFER_SIZE];
			//if(msg[0]=='F')
			{
				auto c=++count;
				assert(c<=amount);
				{
					//if(c==count)
					{
						insert_message(buffer,c);
						if(c==count)
						{
							std::lock_guard lock(mtx);
							if(c==count)
							{
								std::cout<<buffer;
							}
						}
					}
				}
			}
		}
	}
	void log_error(char const* msg,size_t) override
	{
		auto const sl=strlen(msg);
		std::unique_ptr<char[]> buffer(new char[sl+buffer_length]);
		memcpy(buffer.get(),msg,sl);
		size_t const c=count;
		insert_message(buffer.get()+sl,c);
		auto disp_only_error=[&]()
		{
			buffer[sl]='\0';
			std::cout<<buffer.get();
		};
		if(c==count)
		{
			std::lock_guard lock(mtx);
			if(c==count)
			{
				std::cout<<buffer.get();
			}
			else
			{
				disp_only_error();
			}
		}
		else
		{
			disp_only_error();
		}
	}
};

class CommandMaker {
public:
	typedef std::vector<std::string>::iterator iter;
	struct delivery {
		SaveRules sr;
		unsigned int starting_index;
		bool do_move;
		ProcessList<unsigned char> pl;
		enum do_state {
			do_absolutely_nothing,
			do_nothing,
			do_single,
			do_cut,
			do_splice
		};
		do_state flag;
		enum log_type {
			unassigned_log,
			quiet,
			errors_only,
			count,
			full_message
		};
		log_type lt;
		struct {
			perc_or_val horiz_padding;
			perc_or_val optimal_padding;
			perc_or_val min_padding;
			perc_or_val optimal_height;
			float excess_weight;
			float padding_weight;
		} splice_args;
		struct {
			perc_or_val min_height,min_width,min_vert_space;
			float horiz_weight;
		} cut_args;
		std::regex rgx;
		enum regex_state {
			unassigned,normal,inverted
		};
		struct sel_boundary {
			std::string_view begin; //will always be present in the parameters list
			std::string_view end;
		};
		std::vector<sel_boundary> selections;
		regex_state rgxst;
		unsigned int num_threads;
		bool list_files;
		delivery():starting_index(1),flag(do_absolutely_nothing),num_threads(0),rgxst(unassigned),do_move(false),list_files(false),lt(unassigned_log)
		{}
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
		if(del.flag>del.do_single)
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

class FilterHSVMaker:public SingleCommandMaker {
	FilterHSVMaker():SingleCommandMaker(2,3,"Replaces colors found between HSV ranges with replacer. Hue is a circular range.","Filter HSV")
	{}
	static FilterHSVMaker const singleton;
	static char const* do_parse(char const* const errors[],std::array<unsigned char,3>& arr,std::string& str)
	{
		constexpr std::optional<unsigned char> none;
		auto no_constraints=[](auto thing)
		{
			return -1;
		};
		auto res=parse_range(arr,str,{none,none,none},no_constraints);
		if(res!=-1)
		{
			return errors[res];
		}
		return nullptr;
	}
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		static char const* const start_errors[]=
		{"Too few parameters for start HSV value",
		"Too many parameters for start HSV value",
		"Missing start hue value",
		"Missing start saturation value",
		"Missing start value value",
		"Invalid start hue value",
		"Invalid start saturation value",
		"Invalid start value value"};
		std::array<unsigned char,3> start_hsv;
		auto res=do_parse(start_errors,start_hsv,begin[0]);
		if(res) return res;

		static char const* const end_errors[]=
		{"Too few parameters for end HSV value",
			"Too many parameters for end HSV value",
			"Missing end hue value",
			"Missing end saturation value",
			"Missing end value value",
			"Invalid end hue value",
			"Invalid end saturation value",
			"Invalid end value value"};
		std::array<unsigned char,3> end_hsv;
		res=do_parse(end_errors,end_hsv,begin[1]);
		if(res) return res;

		if(end_hsv[1]<start_hsv[1])
		{
			return "Start saturation must be less than or equal to end saturation";
		}
		if(end_hsv[2]<start_hsv[2])
		{
			return "Start value must be less than or equal to end value";
		}

		decltype(end_hsv) replacer={255,255,255};
		if(n>2)
		{
			static char const* const repl_errors[]=
			{"Too few parameters for replacer RGB value",
				"Too many parameters for replacer RGB value",
				"Missing replacer red value",
				"Missing replacer green value",
				"Missing replacer blue value",
				"Invalid replacer red value"
				"Invalid replacer green value",
				"Invalid replacer blue value"};

			typedef std::optional<unsigned char> opt;
			auto resu=parse_range(replacer,begin[2],{opt(255),opt(255),opt(255)},[](auto thing)
			{
				return -1;
			});
			if(resu!=-1) return repl_errors[resu];
		}

		del.pl.add_process<FilterHSV>(
			ColorHSV({start_hsv[0],start_hsv[1],start_hsv[2]}),
			ColorHSV({end_hsv[0],end_hsv[1],end_hsv[2]}),
			ColorRGB({replacer[0],replacer[1],replacer[2]}));
		return nullptr;
	}
};
FilterHSVMaker const FilterHSVMaker::singleton;

class FilterRGBMaker:public SingleCommandMaker {
	FilterRGBMaker():SingleCommandMaker(2,3,"Replaces colors found between RGB ranges with replacer.","Filter RGB")
	{}
	static FilterRGBMaker const singleton;
	static char const* do_parse(char const* const errors[],std::array<unsigned char,3>& arr,std::string& str)
	{
		constexpr std::optional<unsigned char> none;
		auto no_constraints=[](auto thing)
		{
			return -1;
		};
		auto res=parse_range(arr,str,{none,none,none},no_constraints);
		if(res!=-1)
		{
			return errors[res];
		}
		return nullptr;
	}
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		static char const* const start_errors[]=
		{"Too few parameters for start RGB value",
			"Too many parameters for start RGB value",
			"Missing start red value",
			"Missing start green value",
			"Missing start blue value",
			"Invalid start red value",
			"Invalid start green value",
			"Invalid start blue value"};
		std::array<unsigned char,3> start_rgb;
		auto res=do_parse(start_errors,start_rgb,begin[0]);
		if(res) return res;

		static char const* const end_errors[]=
		{"Too few parameters for end RGB value",
			"Too many parameters for end RGB value",
			"Missing end red value",
			"Missing end green value",
			"Missing end blue value",
			"Invalid end red value",
			"Invalid end green value",
			"Invalid end blue value"};
		std::array<unsigned char,3> end_rgb;
		res=do_parse(end_errors,end_rgb,begin[1]);
		if(res) return res;

		if(end_rgb[0]<start_rgb[0])
		{
			return "Start red must be less than or equal to end red";
		}
		if(end_rgb[1]<start_rgb[1])
		{
			return "Start green must be less than or equal to end green";
		}
		if(end_rgb[2]<start_rgb[2])
		{
			return "Start blue must be less than or equal to end blue";
		}

		decltype(end_rgb) replacer={255,255,255};
		if(n>2)
		{
			static char const* const repl_errors[]=
			{"Too few parameters for replacer RGB value",
				"Too many parameters for replacer RGB value",
				"Missing replacer red value",
				"Missing replacer green value",
				"Missing replacer blue value",
				"Invalid replacer red value"
				"Invalid replacer green value",
				"Invalid replacer blue value"};

			typedef std::optional<unsigned char> opt;
			auto resu=parse_range(replacer,begin[2],{opt(255),opt(255),opt(255)},[](auto thing)
			{
				return -1;
			});
			if(resu!=-1) return repl_errors[resu];
		}

		del.pl.add_process<FilterRGB>(
			ColorRGB({start_rgb[0],start_rgb[1],start_rgb[2]}),
			ColorRGB({end_rgb[0],end_rgb[1],end_rgb[2]}),
			ColorRGB({replacer[0],replacer[1],replacer[2]}));
		return nullptr;
	}
};
FilterRGBMaker const FilterRGBMaker::singleton;

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

class ClusterClearAltMaker:public SingleCommandMaker {
	ClusterClearAltMaker()
		:SingleCommandMaker(2,4,
			"Pixels are selected that are in sel_range inclusive and clustered.\n"
			"Clusters that are in the bad_size_range inclusive\n"
			"or which do not contain a color in required_color_range inclusive\n"
			"are replaced by the background color",
			"Cluster Clear Grayscale")
	{}
	static ClusterClearAltMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		static char const* const rcr_errors[]={
			"Range not given for required_color_range",
			"Too many arguments for required_color_range",
			"Should not have happened",
			"Missing argument for upper bound of required_color_range",
			"Invalid argument for lower bound of required_color_range",
			"Invalid argument for upper bound of required_color_range",
			"Lower bound cannot be greater than upper bound of required_color_range"
		};
		std::array<unsigned char,2> required_color_range;
		constexpr std::array<std::optional<unsigned char>,2> const rcr_def={std::optional<unsigned char>(0),std::optional<unsigned char>()};
		auto ordered=[](std::array<unsigned char,2> const& arr)
		{
			if(arr[0]>arr[1])
			{
				return 6;
			}
			return -1;
		};
		auto res=parse_range(
			required_color_range,
			begin[0],
			rcr_def,
			ordered);
		if(res!=-1)
		{
			return rcr_errors[res];
		}
		static char const* const bsr_errors[]={
			"Range not given for bad_size_range",
			"Too many arguments for bad_size_range",
			"Should not have happened",
			"Missing argument for upper bound of bad_size_range",
			"Invalid argument for lower bound of bad_size_range",
			"Invalid argument for upper bound of bad_size_range",
			"Lower bound cannot be greater than upper bound of bad_size_range"
		};
		constexpr std::array<std::optional<unsigned int>,2> const bsr_def={std::optional<unsigned int>(0),std::optional<unsigned int>()};
		std::array<unsigned int,2> bad_size_range;
		auto bres=parse_range(bad_size_range,begin[1],bsr_def,[](decltype(bad_size_range) const& arr)
		{
			if(arr[0]>arr[1])
			{
				return 6;
			}
			return -1;
		});
		if(bres!=-1)
		{
			return bsr_errors[bres];
		}
		std::array<unsigned char,2> sel_range;
		if(n>2)
		{
			static char const* const sr_errors[]={
				"Range not given for sel_range",
				"Too many arguments for sel_range",
				"Should not have happened",
				"Should not have happened",
				"Invalid argument for lower bound of sel_range",
				"Invalid argument for upper bound of sel_range",
				"Lower bound cannot be greater than upper bound of sel_range"
			};
			std::array<std::optional<unsigned char>,2> sr_def={std::optional<unsigned char>(0),std::optional<unsigned char>(200)};
			auto sres=parse_range(sel_range,begin[2],sr_def,ordered);
			if(sres!=-1)
			{
				return sr_errors[sres];
			}
		}
		else
		{
			goto init_sel_range;
		}
		unsigned char background;
		if(n>3)
		{
			auto bres=parse_str(background,begin[3].c_str());
			if(bres)
			{
				return "Invalid argument for background";
			}
		}
		else
		{
			goto init_background;
		}
	end:
		del.pl.add_process<ClusterClearGrayAlt>(
			required_color_range[0],required_color_range[1],
			bad_size_range[0],bad_size_range[1],
			sel_range[0],sel_range[1],
			background);
		return nullptr;
	init_sel_range:
		sel_range[0]=0;
		sel_range[1]=200;
	init_background:
		background=255;
		goto end;
	}
};
ClusterClearAltMaker const ClusterClearAltMaker::singleton;

class HorizontalPaddingMaker:public SingleCommandMaker {
	HorizontalPaddingMaker():
		SingleCommandMaker(1,4,"Pads the left and right sides of the image with given number of pixels","Horizontal Padding")
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
#define padding_maker(type,first,second,match)\
			int amount;int a;perc_or_val tol(0.5f);unsigned char bg(128);\
			if(parse_str(amount,begin[0].c_str()))\
			{\
				return "Invalid " #first " input";\
			}\
			if(amount<-1) return "Input for " #first " must be non-negative";\
			if(n>1)\
			{\
				if(begin[1][0]==##match##)\
				{\
					a=amount;\
				}\
				else\
				{\
					if(parse_str(a,begin[1].c_str()))\
					{\
						return "Invalid " #second " input";\
					}\
					if(a<-1) return "Input for " #second " must be non-negative";\
				}\
				if(n>2)\
				{\
					make_perc_val(tol,tolerance,begin[2],,);\
					if(n>3)\
					{\
						if(parse_str(bg,begin[3].c_str())) return "Invalid tolerance input";\
					}\
				}\
			}\
			else\
			{\
				a=amount;\
			}\
			del.pl.add_process<##type##>(unsigned int(amount),unsigned int(a),tol,bg);\
			return nullptr;
		padding_maker(PadHoriz,left,right,'l');
	}
};
HorizontalPaddingMaker const HorizontalPaddingMaker::singleton;

class VerticalPaddingMaker:public SingleCommandMaker {
	VerticalPaddingMaker()
		:SingleCommandMaker(1,4,"Pads the top and bottom of the image with given number of pixels","Vertical Padding")
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
		padding_maker(PadVert,top,bottom,'t');
	}
};
VerticalPaddingMaker const VerticalPaddingMaker::singleton;

class ClusterPaddingMaker:public SingleCommandMaker {
	ClusterPaddingMaker()
		:SingleCommandMaker(4,5,
			"Pads the sides of the image with given number of pixels.\n"
			"The image is found to be the largest non-background cluster found.\n"
			"THIS HAS A VERY SPECIALIZED USE AND I RECOMMEND YOU DO NOT USE IT!","Cluster Padding")
	{}
	static ClusterPaddingMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		int left,right,top,bottom;
		unsigned char bt=254;
		int res;
#define judge(name,index)\
	res=parse_str(name,begin[index].c_str());\
		if(res) return "Invalid input for " #name;\
		if(name<-1) return "Padding for " #name " must be non-negative";
		judge(left,0);
		judge(right,1);
		judge(top,2);
		judge(bottom,3);
		if(n>4)
		{
			res=parse_str(bt,begin[4].c_str());
			if(res)
			{
				return "Invalid input for background";
			}
		}
		del.pl.add_process<PadCluster>(uint(left),uint(right),uint(top),uint(bottom),bt);
		return nullptr;
#undef judge
	}
};
ClusterPaddingMaker const ClusterPaddingMaker::singleton;

class OutputMaker:public CommandMaker {
	OutputMaker():
		CommandMaker(
			1,2,
			"Pattern templates:\n"
			"  %c copy whole filename (includes path)\n"
			"  %p copy path (does not include trailing slash)\n"
			"  %x copy extension (does not include dot)\n"
			"  %f copy filename (does not include path, dot, or extension)\n"
			"  %w copy whole name (filename with extension)\n"
			"  %0 any number from 0-9, index of file with specified number of padding\n"
			"  %% literal percent\n"
			"Anything else will be interpreted as a literal character\n"
			"Pattern is %w if no output is specified\n"
			"If you want the file to start with -, prepend an additional -\n"
			"  e.g.: \"-my-file.txt\" -> \"--my-file.txt\"\n"
			"This is only done for starting dashes\n"
			"Move is whether file should be moved or copied (ignored for multi)",
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
		if(begin->empty())
		{
			return "Output format cannot be empty";
		}
		if(del.sr.empty())
		{
			try
			{
				if((*begin).front()=='-'&&(*begin)[1]=='-')
				{
					del.sr.assign((*begin).c_str()+1);
				}
				else
				{
					del.sr.assign(*begin);
				}
			}
			catch(std::exception const&)
			{
				return "Invalid template";
			}
			if(n>1)
			{
				if(begin[1][0]=='0'||begin[1][0]=='f')
				{
					del.do_move=false;
				}
				else
				{
					del.do_move=true;
				}
			}
			else
			{
				del.do_move=false;
			}
		}
		else
		{
			return "Filename template already given";
		}
		if(del.flag<del.do_nothing)
		{
			del.flag=del.do_nothing;
		}
		return nullptr;
	}
};
OutputMaker const OutputMaker::singleton;

class StartingIndexMaker:public CommandMaker {
	StartingIndexMaker():CommandMaker(1,1,"The starting index to label template names and spliced pages","Starting Index")
	{}
	static StartingIndexMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
	char const* parse_command(iter begin,size_t n,delivery& del) const override
	{
		unsigned int num;
		auto err=ScoreProcessor::parse_str(num,begin[0].c_str());
		if(err)
		{
			return "Invalid starting index input";
		}
		del.starting_index=num;
		return nullptr;
	}
};
StartingIndexMaker const StartingIndexMaker::singleton;

class ListFilesMaker:public CommandMaker {
	ListFilesMaker():CommandMaker(0,0,"Makes program list out files","List Files")
	{}
	static ListFilesMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
	char const* parse_command(iter,size_t,delivery& del) const override
	{
		del.list_files=true;
		return nullptr;
	}
};
ListFilesMaker const ListFilesMaker::singleton;

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
		int vert,minh,maxh,hoff=0;
		float opt_rat=16.0f/9.0f;
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
		}
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
	static char const* const errors[3];
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t,delivery& del) const override
	{
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
char const* const RescaleGrayMaker::errors[3]={"Invalid argument for min value","Invalid argument for mid value","Invalid argument for max value"};

class CutMaker:public CommandMaker {
	CutMaker()
		:CommandMaker(0,4,
			"Cuts the image into separate systems\n"
			"Min dimensions are the heuristics used to determine whether\n"
			"a group of pixels is actually a system\n"
			"Percentages are taken as percentages of the respective dimension\n"
			"Horizontal weight is how much horizontal whitespace is weighted",
			"Cut")
	{}
	static CutMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command(iter begin,size_t n,delivery& del) const override
	{
		if(del.flag>del.do_nothing)
		{
			return "Cut can not be done along with other commands";
		}
		perc_or_val min_width(55.0f),min_height(8.0f),min_vert_space(0U);
		float hw=20;
		if(n>0)
		{

			int res;
			make_perc_val(min_width,min_width,begin[0],,);
			if(n>1)
			{
				make_perc_val(min_height,min_height,begin[1],,)
					if(n>2)
					{
						res=parse_str(hw,begin[2].c_str());
						if(res)
						{
							return "Invalid horiz_weight input";
						}
						if(n>3)
						{
							make_perc_val(min_vert_space,min_vert_space,begin[3],,);
						}
					}
			}
		}
#undef do_parse
		del.flag=delivery::do_cut;
		del.cut_args.min_width=min_width;
		del.cut_args.min_height=min_height;
		del.cut_args.horiz_weight=hw;
		del.cut_args.min_vert_space=min_vert_space;
		return nullptr;
	}
};
CutMaker const CutMaker::singleton;

class SpliceMaker:public CommandMaker {
	SpliceMaker():
		CommandMaker(
			0,6,
			"Splices the pages together assuming right alignment.\n"
			"Knuth algorithm that tries to minimize deviation from optimal height and optimal padding.\n"
			"Horizontal padding is the padding placed between elements of the page horizontally.\n"
			"Min padding is the minimal vertical padding between pages.\n"
			"Excess weight is the penalty weight applied to height deviation above optimal\n"
			"Pad weight is weight of pad deviation relative to height deviation\n"
			"Cost function is\n"
			"  if(height>opt_height)\n"
			"    (excess_weight*(height-opt_height)/opt_height)^3+\n"
			"    (pad_weight*abs_dif(padding,opt_padding)/opt_padding)^3\n"
			"  else\n"
			"    ((opt_height-height)/opt_height)^3+\n"
			"    (pad_weight*abs_dif(padding,opt_padding)/opt_padding)^3\n"
			"Percentages are taken as percentages of the 1st page width",
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
		if(del.flag>del.do_nothing)
		{
			return "Splice can not be done along with other commands";
		}
		del.flag=del.do_splice;
		perc_or_val hpad(3.0f),opad(5.0f),mpad(1.2f);
		perc_or_val oheight(55.0f);
		float excess_weight=10;
		float padding_weight=1;
		int res;
		if(n>0)
		{
			make_perc_val(hpad,horiz_pad,begin[0],,);
			if(n>1)
			{
				make_perc_val(opad,opt_pad,begin[1],,);
				if(n>2)
				{
					make_perc_val(mpad,min_pad,begin[2],,);
					if(n>3)
					{
						make_perc_val(oheight,opt_height,begin[3],,);
						if(n>4)
						{
							res=parse_str(excess_weight,begin[4].c_str());
							if(res) return "Invalid excs_weight input";
							if(excess_weight<0) return "excs_weight must be positive";
							if(n>5)
							{
								res=parse_str(padding_weight,begin[5].c_str());
								if(res) return "Invalid pad_weight input";
								if(padding_weight<0) return "pad_weight must be positive";
							}
						}
					}
				}
			}
		}
		del.flag=delivery::do_splice;
		del.splice_args.horiz_padding=hpad;
		del.splice_args.min_padding=mpad;
		del.splice_args.optimal_padding=opad;
		del.splice_args.optimal_height=oheight;
		del.splice_args.excess_weight=excess_weight;
		del.splice_args.padding_weight=padding_weight;
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
		if(radius>0)
		{
			del.pl.add_process<Blur>(radius);
		}
		return nullptr;
	}
};
BlurMaker const BlurMaker::singleton;

class StraightenMaker:public SingleCommandMaker {
	StraightenMaker()
		:SingleCommandMaker(
			0,5,
			"Straightens the image\n"
			"Min angle is minimum angle of range to consider rotation (in degrees)\n"
			"Max angle is maximum angle of range to consider rotation (in degrees)\n"
			"Angle precision is precision in measuring angle (in degrees)\n"
			"Pixel precision is precision when measuring distance from origin\n"
			"A pixel is considered an edge if there is a vertical transition across boundary",
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
		unsigned char boundary;
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
		if(n>4)
		{
			auto res=parse_str(boundary,begin[4].c_str());
			if(res)
			{
				return "Invalid input for boundary";
			}
		}
		else
		{
			goto init_boundary;
		}
	end:
		if(min_angle>max_angle)
		{
			return "Max angle must be greater than min angle";
		}
		if(max_angle-min_angle>180)
		{
			return "Difference between angles must be less than or equal to 180";
		}
		del.pl.add_process<Straighten>(pixel_prec,min_angle,max_angle,angle_prec,boundary);
		return nullptr;
	init_min_angle:
		min_angle=-5.0;
	init_max_angle:
		max_angle=5.0;
	init_angle_prec:
		angle_prec=0.1;
	init_pixel_prec:
		pixel_prec=1.0;
	init_boundary:
		boundary=128;
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
	RescaleMaker():SingleCommandMaker(1,2,
		"Rescales image by given factor\n"
		"Rescale modes are:\n"
		"  auto (moving average if downscaling, else cubic)\n"
		"  nearest neighbor\n"
		"  moving average\n"
		"  linear\n"
		"  grid\n"
		"  cubic\n"
		"  lanczos\n"
		"To specify mode, type as many letters as needed to unambiguously identify mode",
		"Rescale")
	{}
	static RescaleMaker const singleton;
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
			size_t idx;
			double factor=std::stod(*begin,&idx);
			if(factor<0)
			{
				return "Rescale factor must be non-negative";
			}
			auto end=(*begin)[idx];
			if(end=='%')
			{
				factor/=100;
			}
			else if(end!='\0'&&!std::isspace(end))
			{
				return "Invalid factor input";
			}
			if(factor==1)
			{
				return nullptr;
			}
			int mode=Rescale::automatic;
			if(n>1)
			{
				std::string const& mode_string=begin[1];
				switch(mode_string[0]) //thank you null-termination
				{
					case 'a':
						mode=Rescale::automatic;
						break;
					case 'n':
						mode=Rescale::nearest_neighbor;
						break;
					case 'm':
						mode=Rescale::moving_average;
						break;
					case 'l':
						switch(mode_string[1])
						{
							case 'i':
								mode=Rescale::linear;
								break;
							case 'a':
								mode=Rescale::lanczos;
								break;
							case '\0':
								return "Ambiguous mode starting with l";
								break;
							default:
								return "Mode does not exist";
						}
						break;
					case 'g':
						mode=Rescale::grid;
						break;
					case 'c':
						mode=Rescale::cubic;
						break;
					default:
						return "Mode does not exist";
				}
			}
			del.pl.add_process<Rescale>(factor,mode);
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
		:CommandMaker(1,1,"Changes verbosity of output: Silent=0=s, Errors-only=1=e, Count=2=c (default), Loud=3=l","Verbosity")
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
		char const* const invl="Invalid level";
		if(del.lt)
		{
			return "Verbosity level already given";
		}
#define read_second if(begin[0][1]!='\0') return invl
		switch(begin[0][0])
		{
			case '0':
				read_second;
			case 'S':
			case 's':
				del.lt=del.quiet;
				break;
			case '1':
				read_second;
			case 'e':
			case 'E':
				del.lt=del.errors_only;
				break;
			case '2':
				read_second;
			case 'c':
			case 'C':
				del.lt=del.count;
				break;
			case '3':
				read_second;
			case 'l':
			case 'L':
				del.lt=del.full_message;
				break;
			default:
				return invl;
		}
		return nullptr;
	}
#undef read_second
};
LogMaker const LogMaker::singleton;

class RegexMaker:public CommandMaker {
	RegexMaker():
		CommandMaker(
			1,2,
			"Filters the folder of files using a regex pattern\n"
			"Files that match are kept, unless inversion option is specified as true\n"
			"Giving a string starting with 0 or f is false, otherwise is true\n"
			"Does nothing if you give only a single file",
			"Filter Files")
	{}
	static RegexMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command(iter begin,size_t n,delivery& del) const override
	{
		if(del.rgxst)
		{
			return "Filter pattern already given";
		}
		try
		{
			del.rgx.assign(begin[0]);
			if(n>1)
			{
				if(begin[1][0]=='0'||begin[1][0]=='f')
				{
					del.rgxst=del.normal;
				}
				else
				{
					del.rgxst=del.inverted;
				}
			}
			else
			{
				del.rgxst=del.normal;
			}
		}
		catch(std::exception const&)
		{
			return "Invalid regex pattern";
		}
		return nullptr;
	}
};
RegexMaker const RegexMaker::singleton;

class BoundSelMaker:public CommandMaker {
	BoundSelMaker():CommandMaker(2,-1,"Files between the begin and end are included in the list of files to process","Boundary Select")
	{}
	static BoundSelMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command(iter begin,size_t n,delivery& del) const override
	{
		if(n&1)
		{
			return "Invalid number of arguments";
		}
		for(size_t i=0;i<n;i+=2)
		{
			delivery::sel_boundary b={begin[i],begin[i+1]};
			auto remove_start_dash=[](std::string_view& sv)
			{
				if(sv[0]=='-'&&sv[1]=='-')
				{
					sv.remove_prefix(1);
				}
			};
			remove_start_dash(b.begin);
			remove_start_dash(b.end);
			del.selections.push_back(b);
		}
		return nullptr;
	}
};
BoundSelMaker const BoundSelMaker::singleton;

class FillRectangleGrayMaker:public SingleCommandMaker {
	FillRectangleGrayMaker():SingleCommandMaker(4,6,
		"Fills given rectangle with given color\n"
		"Origin codes are:\n"
		"  tl  tm  tr\n"
		"  ml  m   mr\n"
		"  bl  bm  br",
		"Fill Rectangle Gray")
	{}
	static FillRectangleGrayMaker const singleton;
	static char const* const invalids[4];
#define fsag FillSelectionAbsGray
	static auto make_ref_map()
	{
		std::map<std::string,fsag::origin_reference> map;
		map.emplace("bl",fsag::bottom_left);
		map.emplace("bm",fsag::bottom_middle);
		map.emplace("br",fsag::bottom_right);
		map.emplace("ml",fsag::middle_left);
		map.emplace("m",fsag::middle);
		map.emplace("mm",fsag::middle);
		map.emplace("mr",fsag::middle_right);
		map.emplace("tl",fsag::top_left);
		map.emplace("tm",fsag::top_middle);
		map.emplace("tr",fsag::top_right);
		map.emplace("yaoi",fsag::bottom_left);
		//may add more in the future
#undef fsag
		return map;
	}
	static FillSelectionAbsGray::origin_reference get_origin(string const& str)
	{
		static auto const map=make_ref_map();
		auto sel=map.find(str);
		if(sel==map.end())
		{
			return FillSelectionAbsGray::origin_reference(-1);
		}
		return sel->second;
	}
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		int coords[4];
		for(size_t i=0;i<4;++i)
		{
			try
			{
				coords[i]=std::stoi(begin[i]);
			}
			catch(std::exception const&)
			{
				return invalids[i];
			}
		}
		int color=255;
		FillSelectionAbsGray::origin_reference origin=FillSelectionAbsGray::top_left;
		if(n>4)
		{
			try
			{
				color=std::stoi(begin[4]);
				if(color<0||color>255)
				{
					return "Color must be in range [0,255]";
				}
			}
			catch(std::exception const&)
			{
				return "Invalid argument for color";
			}
			if(n>5)
			{
				origin=get_origin(begin[5]);
				if(origin==-1)
				{
					return "Unknown origin code";
				}
			}
		}
		if(coords[0]>=coords[2])
		{
			return "Left must be less than right";
		}
		if(coords[1]>=coords[3])
		{
			return "Top must be less than bottom";
		}
		del.pl.add_process<FillSelectionAbsGray>(
			ImageUtils::Rectangle<signed int>(
				{
					coords[0],
					coords[2],
					coords[1],
					coords[3]
				}),
			Grayscale(color),
			origin
			);
		return nullptr;
	}
};
FillRectangleGrayMaker const FillRectangleGrayMaker::singleton;
#define minv(name) "Invalid argument for " #name
char const* const FillRectangleGrayMaker::invalids[4]={minv(left),minv(top),minv(right),minv(bottom)};
#undef minv

class CoverTransparencyMaker:public SingleCommandMaker {
	CoverTransparencyMaker():SingleCommandMaker(0,3,
		"Pixels are mixed with the specified background color according to their transparency",
		"Cover Transparency")
	{}
	static CoverTransparencyMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		unsigned char r=255,g=255,b=255;
		if(n>0)
		{
			if(parse_str(r,begin[0].c_str())) return "Invalid r input";
			if(n>1)
			{
				if(begin[1][0]=='r')
				{
					g=r;
				}
				else
				{
					if(parse_str(g,begin[1].c_str())) return "Invalid g input";
				}
				if(n>2)
				{
					if(begin[2][0]=='r')
					{
						b=r;
					}
					else if(begin[2][0]=='g')
					{
						b=g;
					}
					else
					{
						if(parse_str(b,begin[2].c_str())) return "Invalid b input";
					}
				}
			}
		}
		del.pl.add_process<FillTransparency>(r,g,b);
		return nullptr;
	}
};
CoverTransparencyMaker const CoverTransparencyMaker::singleton;

class ExtractLayerMaker:public SingleCommandMaker {
	ExtractLayerMaker():SingleCommandMaker(0,0,
		"Takes only the first layer from the image\n"
		"Will probably add more options",
		"Extract First Layer")
	{}
	static ExtractLayerMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		del.pl.add_process<ExtractLayer0NoRealloc>();
		return nullptr;
	}
};
ExtractLayerMaker const ExtractLayerMaker::singleton;

class RotateMaker:public SingleCommandMaker {
	RotateMaker():SingleCommandMaker(1,1,
		"Rotates the image by the specified amount in degrees\n"
		"Counterclockwise is positive",
		"Rotate")
	{}
	static RotateMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command_h(iter begin,size_t n,delivery& del) const override
	{
		float angle;
		auto res=ScoreProcessor::parse_str(angle,(*begin).c_str());
		if(res)
		{
			return "Invalid argument for angle input";
		}
		angle=fmod(angle,360);
		if(angle<0)
		{
			angle+=360;
		}
		if(angle!=0)
		{
			del.pl.add_process<Rotate>(-angle);
		}
		return nullptr;
	}
};
RotateMaker const RotateMaker::singleton;

class NumThreadMaker:public CommandMaker {
private:
	NumThreadMaker():
		CommandMaker(
			1,1,
			"Controls the number of CPU threads used when processing multiple images\n"
			"Defaults to max number supported by the CPU\n"
			"Number of threads created will not exceed number number of images"
			,"Number of Threads")
	{}
	static NumThreadMaker const singleton;
public:
	static CommandMaker const& get()
	{
		return singleton;
	}
protected:
	char const* parse_command(iter begin,size_t,delivery& del) const override
	{
		if(del.num_threads)
		{
			return "Number of threads already set";
		}
		try
		{
			int n=std::stoi(*begin);
			if(n<1)
			{
				return "Number of threads must be positive";
			}
			del.num_threads=n;
			return nullptr;
		}
		catch(std::exception const&)
		{
			return "Invalid argument given for number of threads";
		}
	}
};
NumThreadMaker const NumThreadMaker::singleton;

std::unordered_map<std::string,CommandMaker const*> const init_commands()
{
	std::unordered_map<std::string,CommandMaker const*> commands;
	commands.emplace("-fg",&FilterGrayMaker::get());

	commands.emplace("-cg",&ConvertGrayMaker::get());

	commands.emplace("-ccg",&ClusterClearMaker::get());

	commands.emplace("-ccga",&ClusterClearAltMaker::get());

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

	commands.emplace("-flt",&RegexMaker::get());

	commands.emplace("-fr",&FillRectangleGrayMaker::get());

	commands.emplace("-nt",&NumThreadMaker::get());

	commands.emplace("-si",&StartingIndexMaker::get());

	commands.emplace("-rot",&RotateMaker::get());
	commands.emplace("-rotate",&RotateMaker::get());

	commands.emplace("-list",&ListFilesMaker::get());

	commands.emplace("-fhsv",&FilterHSVMaker::get());
	commands.emplace("-frgb",&FilterRGBMaker::get());

	commands.emplace("-bsel",&BoundSelMaker::get());

	commands.emplace("-cp",&ClusterPaddingMaker::get());

	commands.emplace("-ct",&CoverTransparencyMaker::get());

	commands.emplace("-exl",&ExtractLayerMaker::get());
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
	return ret;
}
std::pair<std::string,std::string> pretty_date()
{
	std::pair<std::string,std::string> ret;
	char const* const date=__DATE__;
	ret.first.assign(date);
	if(ret.first[4]==' ')
	{
		ret.first[4]='0';
	}
	ret.second.assign(date+7,4);
	return ret;
}
void test()
{
	stop();
}
//help screen of single input
void info_output();
//does single processes
void do_single(CommandMaker::delivery const&,std::vector<std::string> const& files);
//does cut processes
void do_cut(CommandMaker::delivery const&,std::vector<std::string> const& files);
//does splice processes
void do_splice(CommandMaker::delivery const&,std::vector<std::string> const& files);
//finds end of input list and gets the strings from that list
CommandMaker::iter find_file_list(CommandMaker::iter begin,CommandMaker::iter end);

std::vector<std::string> get_files(CommandMaker::iter begin,CommandMaker::iter end);

//filters out files according to the regex pattern
void filter_out_files(std::vector<std::string>&,CommandMaker::delivery const&);
//lists out files
void list_files(std::vector<std::string> const&);

CommandMaker::delivery parse_commands(CommandMaker::iter begin,CommandMaker::iter end);

bool could_be_command(std::string const& str)
{
	if(str[0]=='-')
	{
		if(str[1]=='r'&&str[2]=='\0')
		{
			return false;
		}
		return str[1]>='a'&&str[1]<='z';
	}
	return false;
}
bool is_folder(std::string const& str)
{
	return str.back()=='\\'||str.back()=='/';
}

int main(int argc,char** argv)
{
#ifdef TESTING
	test();
#endif
	cimg::exception_mode(0);
	if(argc==1)
	{
		info_output();
		return 0;
	}
	auto args=conv_strings(argc,argv);
	auto const& arg1=args[1];
	if(could_be_command(arg1))
	{
		auto cmd=commands.find(arg1);
		if(cmd!=commands.end())
		{
			auto& m=cmd->second;
			std::cout<<m->name()<<":\n"<<m->help_message()<<'\n';
		}
		else
		{
			std::cout<<"Unknown command: "<<arg1<<'\n';
		}
		return 0;
	}
	auto arg_find=find_file_list(args.begin()+1,args.end());
	CommandMaker::delivery del;
	decltype(get_files(args.end(),args.end())) files;
	try
	{
		del=parse_commands(arg_find,args.end());
		files=get_files(args.begin()+1,arg_find);
		filter_out_files(files,del);
	}
	catch(std::exception const& ex)
	{
		std::cout<<ex.what()<<'\n';
		return 0;
	}
	if(del.list_files)
	{
		list_files(files);
	}
	if(del.num_threads==0)
	{
		del.num_threads=std::thread::hardware_concurrency();
		if(del.num_threads==0)
		{
			del.num_threads=2;
		}
	}
	using ui=decltype(del.num_threads);
	del.num_threads=std::min(
		del.num_threads,
		ui((std::min(size_t(std::numeric_limits<ui>::max()),files.size()))));
	std::optional<AmountLog> al;
	switch(del.lt)
	{
		case del.unassigned_log:
		case del.count:
			al.emplace(files.size());
			del.pl.set_log(&al.value());
			del.pl.set_verbosity(del.pl.loud);
			break;
		case del.errors_only:
			del.pl.set_log(&CoutLog::get());
			del.pl.set_verbosity(del.pl.errors_only);
			break;
		case del.quiet:
			del.pl.set_log(&CoutLog::get());
			del.pl.set_verbosity(del.pl.silent);
			break;
		case del.full_message:
			del.pl.set_log(&CoutLog::get());
			del.pl.set_verbosity(del.pl.loud);
	}
	switch(del.flag)
	{
		case del.do_absolutely_nothing:
			std::cout<<"No commands given\n";
			break;
		case del.do_nothing:
			[[fallthrough]];
		case del.do_single:
			do_single(del,files);
			break;
		case del.do_cut:
			do_cut(del,files);
			break;
		case del.do_splice:
			do_splice(del,files);
			break;
	}
#ifndef NDEBUG
	stop();
#endif
#ifdef STOP_CONSOLE
	stop();
#endif
	return 0;
}

void info_output()
{
	auto dates=pretty_date();
	std::cout<<
		"Version: "<<
		dates.first<<
		" "
		__TIME__
		" Copyright 2017-"<<
		dates.second<<
		" Edward Xie\n"
		"filename_or_folder... command params... ...\n"
		"If you want to recursively search a folder, type -r before it\n"
		"If a file starts with a dash, double the starting dash: \"-my-file.jpg\" -> \"--my-file.jpg\"\n"
		"parameters that require multiple values are notated with a comma\n"
		"ex: img0.png --image1.jpg my_folder/ -r rec_folder/ -fg 180 -ccga 20,50 ,30\n"
		"Type command alone to get readme\n"
		"Available commands:\n"
		"  Single Page Operations:\n"
		"    Convert to Grayscale:     -cg\n"
		"    Filter Brightness:        -fg min_value max_value=255 replacer=255\n"
		"    Filter HSV:               -fhsv start_hsv=,, end_hsv=,, replacer_rgb=255,255,255\n"
		"    Filter RGB:               -frgb start_rgb=,, end_rgb=,, replacer_rgb=255,255,255\n"
		"    Cluster Clear Gray:       -ccg max_size min_size=0 background_color=255 tolerance=0.042\n"
		"    Cluster Clear Gray Alt:   -ccga required_color_range=0, bad_size_range=0, sel_range=0,200 bg_color=255\n"
		"    Horizontal Padding:       -hp left right=left tolerance=0.5% background_threshold=128\n"
		"    Vertical Padding:         -vp top bottom=top tolerance=0.5% background_threshold=128\n"
		"    Cluster Padding:          -cp left right top bottom background_threshold=254\n"
		"    Auto Padding:             -ap vert_pad min_horiz_pad max_horiz_pad horiz_offset=0 opt_ratio=1.777778\n"
		"    Rescale Colors Grayscale: -rcg min mid max\n"
		"    Blur:                     -bl radius\n"
		"    Straighten:               -str min_angle=-5 max_angle=5 angle_prec=0.1 pixel_prec=1 boundary=128\n"
		"    Remove Border (DANGER):   -rb tolerance=0.5\n"
		"    Rescale:                  -rs factor interpolation_mode=auto\n"
		"    Fill Rectangle Gray:      -fr left top right bottom color=255 origin=tl\n"
		"    Cover Transparency        -ct r=255 g=r b=r\n"
		"    Extract First Layer       -exl\n"
		"    Rotate:                   -rot degrees\n"
		"  Multi Page Operations:\n"
		"    Cut:                      -cut min_width=66% min_height=8% horiz_weight=20 min_vert_space=0\n"
		"    Splice:                   -spl horiz_pad=3% opt_pad=5% min_vert_pad=1.2% opt_height=55% excs_weight=10 pad_weight=1\n"
		"  Options:\n"
		"    Output:                   -o format move=false\n"
		"    Verbosity:                -vb level\n"
		"    Filter Files:             -flt regex remove=false\n"
		"    Boundary Select:          -bsel first_file1 last_file1 ... first_filen last_filen\n"
		"    Number of Threads:        -nt num\n"
		"    Starting index:           -si num\n"
		"    List Files:               -list\n"
		"Multiple Single Page Operations can be done at once. They are performed in the order they are given.\n"
		"A Multi Page Operation can not be done with other operations.\n"
		;
}

void do_single(CommandMaker::delivery const& del,std::vector<std::string> const& files)
{
	del.pl.process(files,&del.sr,del.num_threads,del.starting_index,del.do_move);
}

void do_cut(CommandMaker::delivery const& del,std::vector<std::string> const& files)
{
	class CutProcess:public exlib::ThreadTaskA<SaveRules const*,int,perc_or_val,perc_or_val,perc_or_val,float,Log*> {
	private:
		std::string const* input;
		unsigned int index;
	public:
		CutProcess(std::string const* input,unsigned int index,CommandMaker::delivery const& del):
			input(input),
			index(index)
		{}
		void execute(SaveRules const* output,int verbosity,perc_or_val min_width,perc_or_val min_height,perc_or_val min_vert_space,float horiz_weight,Log* plog) override
		{
			try
			{
				if(verbosity>ProcessList<>::verbosity::errors_only)
				{
					std::string coutput("Starting ");
					coutput.append(*input);
					coutput.append(1,'\n');
					plog->log(coutput.c_str(),index);
				}
				auto out=output->make_filename(*input,index);
				auto ext=exlib::find_extension(out.begin(),out.end());
				auto s=supported(&*ext);
				if(s==support_type::no)
				{
					if(verbosity>ProcessList<>::verbosity::silent)
					{
						std::cout<<(std::string("Unsupported file type ").append(&*ext,std::distance(ext,out.end())).append(1,'\n'));
					}
					return;
				}
				CImg<unsigned char> in(input->c_str());
				cut_heuristics cut_args;
				cut_args.horizontal_energy_weight=horiz_weight;
				auto fix_perc=[](unsigned int val,perc_or_val pv)
				{
					if(pv.is_perc)
					{
						return unsigned int(std::round(pv.perc*val/100.0));
					}
					else
					{
						return pv.val;
					}
				};
				cut_args.min_height=fix_perc(in._height,min_height);
				cut_args.min_width=fix_perc(in._width,min_width);
				cut_args.minimum_vertical_space=fix_perc(in._height,min_vert_space);
				auto num_pages=ScoreProcessor::cut_page(in,out.c_str(),cut_args);
				if(verbosity>ProcessList<>::verbosity::errors_only)
				{
					std::string coutput("Finished ");
					coutput.append(*input);
					coutput.append(" and created ");
					coutput.append(std::to_string(num_pages));
					coutput.append(num_pages==1?" page\n":" pages\n");
					plog->log(coutput.c_str(),index);
				}
			}
			catch(std::exception const& ex)
			{
				if(verbosity>ProcessList<>::verbosity::silent)
				{
					std::string err(ex.what());
					err+='\n';
					plog->log_error(err.c_str(),index);
				}
			}
		}
	};
	exlib::ThreadPoolA<SaveRules const*,int,perc_or_val,perc_or_val,perc_or_val,float,Log*> tp(del.num_threads);
	for(size_t i=0;i<files.size();++i)
	{
		tp.add_task<CutProcess>(&files[i],i+del.starting_index,del);
	}
	tp.start(&del.sr,del.pl.get_verbosity(),del.cut_args.min_width,del.cut_args.min_height,del.cut_args.min_vert_space,del.cut_args.horiz_weight,del.pl.get_log());
}

void do_splice(CommandMaker::delivery const& del,std::vector<std::string> const& files)
{
	try
	{
		auto save=del.sr.make_filename(files[0],del.starting_index);
		auto ext=exlib::find_extension(save.begin(),save.end());
		if(supported(&*ext)==support_type::no)
		{
			std::cout<<std::string("Unsupported file type ")<<&*ext<<'\n';
			return;
		}
		auto num=del.num_threads<2?
			splice_pages_nongreedy(
				files,
				del.splice_args.horiz_padding,
				del.splice_args.optimal_height,
				del.splice_args.optimal_padding,
				del.splice_args.min_padding,
				save.c_str(),
				del.splice_args.excess_weight,
				del.splice_args.padding_weight,
				del.starting_index):
			splice_pages_nongreedy_parallel(
				files,
				del.splice_args.horiz_padding,
				del.splice_args.optimal_height,
				del.splice_args.optimal_padding,
				del.splice_args.min_padding,
				save.c_str(),
				del.splice_args.excess_weight,
				del.splice_args.padding_weight,
				del.starting_index,
				del.num_threads);
		std::cout<<"Created "<<num<<(num==1?" page\n":" pages\n");
	}
	catch(std::exception const& ex)
	{
		std::cout<<"Error(s):\n"<<ex.what()<<'\n';
	}
}

//finds end of input list
CommandMaker::iter find_file_list(CommandMaker::iter begin,CommandMaker::iter end)
{
	CommandMaker::iter pos;
	for(pos=begin;pos!=end;++pos)
	{
		if((*pos)[0]=='-')
		{
			char c=(*pos)[1];
			if(c=='r'&&(*pos)[2]=='\0')
			{
				continue;
			}
			else if(c>='a'&&c<='z')//could_be_command
			{
				return pos;
			}
			else if(c=='-')
			{
				(*pos).erase((*pos).begin());//I should have written this whole thing with string_view
			}
		}
	}
	return pos;
}

std::vector<std::string> get_files(CommandMaker::iter begin,CommandMaker::iter end)
{
	bool do_recursive=false;
	std::vector<std::string> files;
	for(auto pos=begin;pos!=end;++pos)
	{
		if(*pos=="-r")
		{
			if(do_recursive)
			{
				throw std::logic_error("Double recursive flag given");
			}
			do_recursive=true;
		}
		else if(is_folder(*pos))
		{
			auto fid=do_recursive?exlib::files_in_dir_rec(*pos):exlib::files_in_dir(*pos);
			files.reserve(files.size()+fid.size());
			if(*pos=="./"||*pos==".\\")
			{
				for(auto& str:fid)
				{
					files.emplace_back(std::move(str));
				}
			}
			else
			{
				for(auto const& str:fid)
				{
					files.emplace_back(*pos+str);
				}
			}
			do_recursive=false;
		}
		else
		{
			if(do_recursive)
			{
				throw std::logic_error("Cannot recursively search a non-folder");
			}
			files.emplace_back(std::move(*pos));
		}
	}
	if(do_recursive)
	{
		throw std::logic_error("Recursive flag given at end of input list");
	}
	return files;
}
//filters out files according to the regex pattern
void filter_out_files(std::vector<std::string>& files,CommandMaker::delivery const& del)
{
	if(!del.selections.empty())
	{
		std::vector<char> what_to_keep(files.size()); //not gonna take much space anyway, so I'm not using vector<bool>
		std::memset(what_to_keep.data(),0,what_to_keep.size());
		for(auto sr:del.selections)
		{
			auto const beg=files.begin(),ed=files.end();
			auto first=std::find_if(beg,ed,[b=sr.begin](auto const& str)
			{
				return b==std::string_view(str);
			});
			if(first==ed)
			{
				throw std::logic_error(std::string(sr.begin).append(" start boundary not found"));
			}
			auto last=std::find_if(first,ed,[e=sr.end](auto const& str)
			{
				return e==std::string_view(str);
			});
			if(last==ed)
			{
				throw std::logic_error(std::string(sr.end).append(" end boundary not found beyond ").append(sr.begin));
			}
			size_t start=first-files.begin();
			size_t const end=last-files.begin();
			for(;start<=end;++start)
			{
				what_to_keep[start]=1;
			}
		}
		std::vector<std::string> filtered;
		for(size_t i=0;i<files.size();++i)
		{
			if(what_to_keep[i])
			{
				filtered.emplace_back(std::move(files[i]));
			}
		}
		files=std::move(filtered);
	}
	if(del.rgxst)
	{
		files.erase(std::remove_if(files.begin(),files.end(),
			[&del,keep=del.rgxst==CommandMaker::delivery::normal](auto const& a)
		{
			return std::regex_match(a,del.rgx)!=keep;
		}),files.end());
	}
}

CommandMaker::delivery parse_commands(CommandMaker::iter arg_start,CommandMaker::iter end)
{
	CommandMaker::delivery del;
	if(arg_start!=end)
	{
		auto it=arg_start+1;
		for(;;++it)
		{
			if(it==end||could_be_command(*it))
			{
				auto cmd=commands.find(*arg_start);
				if(cmd==commands.end())
				{
					throw std::invalid_argument("Unknown command: "+*arg_start);
				}
				auto& m=cmd->second;
				if(auto res=m->make_command(arg_start+1,it,del))
				{
					throw std::invalid_argument(std::string(m->name())+" Error:\n"+res);
				}
				arg_start=it;
			}
			if(it==end)
			{
				break;
			}
		}
	}
	if(del.sr.empty())
	{
		del.sr.assign("%w");
	}
	return del;
}

void list_files(std::vector<std::string> const& files)
{
	if(files.empty())
	{
		std::cout<<"No files found\n";
	}
	else
	{
		std::cout<<'\n';
		for(auto const& file:files)
		{
			std::cout<<file<<'\n';
		}
	}
	std::cout<<'\n';
}
