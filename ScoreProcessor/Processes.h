#ifndef PROCESSES_H
#define PROCESSES_H
#include "ImageMath.h"
#include "ImageUtils.h"
#include "ScoreProcesses.h"
#include "ImageProcess.h"
namespace ScoreProcessor {

	class ChangeToGrayscale:public ImageProcess<> {
	public:
		bool process(Img& img) const override;
	};

	class FillTransparency:public ImageProcess<> {
		ImageUtils::ColorRGB background;
	public:
		inline FillTransparency(unsigned char r,unsigned char g,unsigned char b)
		{
			background={r,g,b};
		}
		bool process(Img& img) const override;
	};

	class RemoveBorderGray:public ImageProcess<> {
		float tolerance;
	public:
		inline RemoveBorderGray(float tolerance):tolerance(tolerance)
		{}
		bool process(Img& img) const override;
	};

	class FilterGray:public ImageProcess<> {
		unsigned char min;
		unsigned char max;
		unsigned char replacer;
	public:
		inline FilterGray(unsigned char min,unsigned char max,unsigned char replacer):min(min),max(max),replacer(replacer)
		{}
		bool process(Img& img) const override;
	};

	class FilterHSV:public ImageProcess<> {
		ImageUtils::ColorHSV start,end;
		ImageUtils::ColorRGB replacer;
	public:
		inline FilterHSV(ImageUtils::ColorHSV start,ImageUtils::ColorHSV end,ImageUtils::ColorRGB replacer):start(start),end(end),replacer(replacer)
		{}
		bool process(Img& img) const override;
	};

	class FilterRGB:public ImageProcess<> {
		ImageUtils::ColorRGB start,end;
		ImageUtils::ColorRGB replacer;
	public:
		inline FilterRGB(ImageUtils::ColorRGB start,ImageUtils::ColorRGB end,ImageUtils::ColorRGB replacer):start(start),end(end),replacer(replacer)
		{}
		bool process(Img& img) const override;
	};

	class PadBase:public ImageProcess<> {
	public:
		//fixed amount or proportion of width or height
		using pv=exlib::maybe_fixed<unsigned int>;
		enum bases:unsigned int {
			width,height
		};
	protected:
		exlib::maybe_fixed<unsigned int> side1,side2,tolerance;
		unsigned char background;
		inline PadBase(pv side1,pv side2,pv tol,unsigned char bg):side1(side1),side2(side2),tolerance(tol),background(bg)
		{}
	};

	class PadHoriz:public PadBase {
	public:
		using PadBase::pv;
		PadHoriz(pv left,pv right,pv tol,unsigned char bg):PadBase(left,right,tol,bg)
		{}
		bool process(Img& img) const override;
	};

	class PadVert:public PadBase {
	public:
		using PadBase::pv;
		PadVert(pv top,pv bottom,pv tol,unsigned char bg):PadBase(top,bottom,tol,bg)
		{}
		bool process(Img& img) const override;
	};

	class PadAuto:public ImageProcess<> {
		unsigned int vert,min_h,max_h;
		signed int hoff;
		float opt_rat;
	public:
		inline PadAuto(unsigned int vert,unsigned int min_h,unsigned int max_h,signed int hoff,float opt_rat)
			:vert(vert),min_h(min_h),max_h(max_h),hoff(hoff),opt_rat(opt_rat)
		{}
		bool process(Img& img) const override;
	};

	class PadCluster:public ImageProcess<> {
		unsigned int left,right,top,bottom;
		unsigned char bt;
	public:
		inline PadCluster(unsigned int left,unsigned int right,unsigned int top,unsigned int bottom,unsigned char bt):
			left(left),right(right),bottom(bottom),top(top),bt(bt)
		{}
		bool process(Img& img) const override;
	};

	class Crop:public ImageProcess<> {
	public:
		struct mark {
			enum class basis {
				fixed,width,height
			};
			basis base;
			union {
				int value;
				float part;
			};
			inline constexpr int operator ()(int width,int height) const
			{
				switch(base)
				{
					case basis::fixed:
						return value;
					case basis::width:
						return std::round(part*width);
					case basis::height:
						return std::round(part*height);
				}
				assert(false);
			}
		};
	private:
		mark left,top,right,bottom;
	public:
		inline Crop(mark l,mark t,mark r,mark b):left(l),top(t),right(r),bottom(b)
		{}
		bool process(Img& img) const override;
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
		inline Rescale(double val,int interpolation):
			val(val),
			interpolation(interpolation==automatic?(val>1?cubic:moving_average):interpolation)
		{}
		bool process(Img& img) const override;
	};

	class ExtractLayer0NoRealloc:public ImageProcess<> {
	public:
		bool process(Img& img) const override;
	};

	class ClusterClearGray:public ImageProcess<> {
		unsigned int min,max;
		unsigned char background;
		float tolerance;
	public:
		inline ClusterClearGray(unsigned int min,unsigned int max,unsigned char background,float tolerance):min(min),max(max),background(background),tolerance(tolerance)
		{}
		bool process(Img& img) const override;
	};

	class ClusterClearGrayAlt:public ImageProcess<> {
		unsigned char required_min,required_max;
		unsigned int min_size,max_size;
		unsigned char sel_min;
		unsigned char sel_max;
		unsigned char background;
	public:
		inline ClusterClearGrayAlt(unsigned char rcmi,unsigned char rcma,unsigned int mis,unsigned mas,unsigned char smi,unsigned char sma,unsigned char back):
			required_min(rcmi),required_max(rcma),min_size(mis),max_size(mas),sel_min(smi),sel_max(sma),background(back)
		{}
		bool process(Img& img) const override;
	};

	class RescaleGray:public ImageProcess<> {
		unsigned char min,mid,max;
	public:
		inline RescaleGray(unsigned char min,unsigned char mid,unsigned char max=255):min(min),mid(mid),max(max)
		{}
		bool process(Img& img) const override;
	};

	class FillRectangle:public ImageProcess<> {
	public:
		enum origin_reference {
			top_left,top_middle,top_right,
			middle_left,middle,middle_right,
			bottom_left,bottom_middle,bottom_right
		};
	private:
		ImageUtils::Rectangle<signed int> offsets;
		origin_reference origin;
		std::array<unsigned char,4> color;
		unsigned int num_layers;
	public:
		inline FillRectangle(ImageUtils::Rectangle<signed int> offsets,std::array<unsigned char,4> color,unsigned int num_layers,origin_reference orgn):offsets(offsets),color(color),num_layers(num_layers),origin(orgn)
		{
			assert(origin>=top_left&&origin<=bottom_right);
		}
		bool process(Img& img) const override;
	};

	class Blur:public ImageProcess<> {
		float radius;
	public:
		inline Blur(float radius):radius(radius)
		{}
		bool process(Img& img) const override;
	};

	class Straighten:public ImageProcess<> {
		double pixel_prec;
		unsigned int num_steps;
		double min_angle,max_angle;
		unsigned char boundary;
		float gamma;
	public:
		inline Straighten(double pixel_prec,double min_angle,double max_angle,double angle_prec,unsigned char boundary,float gamma)
			:pixel_prec(pixel_prec),
			min_angle(M_PI_2+min_angle*DEG_RAD),max_angle(M_PI_2+max_angle*DEG_RAD),
			num_steps(std::ceil((max_angle-min_angle)/angle_prec)),
			boundary(boundary),gamma(gamma)
		{}
		bool process(Img& img) const override;
	};

	class Rotate:public ImageProcess<> {
	public:
		enum interp_mode {
			nearest_neighbor,
			linear,
			cubic
		};
		float angle;
		interp_mode mode;
	public:
		inline Rotate(float angle,interp_mode mode):angle(angle),mode(mode)
		{}
		bool process(Img& img) const override;
	};

	class Gamma:public ImageProcess<> {
		float gamma;
	public:
		inline Gamma(float g):gamma(g)
		{}
		bool process(Img& img) const override;
	};

	class ThreadOverride:public ImageProcess<> {
		unsigned int const* _num_threads;
	protected:
		ThreadOverride(unsigned int const* num_threads):_num_threads(num_threads){}
		inline unsigned int num_threads() const
		{
			return *_num_threads;
		}
	};
}
#endif