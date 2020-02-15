#ifndef PROCESSES_H
#define PROCESSES_H
#include "ImageMath.h"
#include "ImageUtils.h"
#include "ScoreProcesses.h"
#include "ImageProcess.h"
#include "../NeuralNetwork/neural_scaler.h"
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
		using mark=std::optional<int>;
	private:
		int left,top;
		mark right,bottom;
	public:
		inline Crop(int l,int t,mark r,mark b):left(l),top(t),right(r),bottom(b)
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

	/*
	class ClusterClearGray:public ImageProcess<> {
		unsigned int min,max;
		unsigned char background;
		float tolerance;
		bool eight_way;
	public:
		inline ClusterClearGray(unsigned int min,unsigned int max,unsigned char background,float tolerance,bool eight_way):min(min),max(max),background(background),tolerance(tolerance),eight_way(eight_way)
		{}
		bool process(Img& img) const override;
	};
	*/

	class ClusterClearGrayAlt:public ImageProcess<> {
		unsigned char required_min,required_max;
		unsigned int min_size,max_size;
		unsigned char sel_min;
		unsigned char sel_max;
		unsigned char background;
		bool eight;
	public:
		inline ClusterClearGrayAlt(unsigned char rcmi,unsigned char rcma,unsigned int mis,unsigned mas,unsigned char smi,unsigned char sma,unsigned char back,bool eight):
			required_min(rcmi),required_max(rcma),min_size(mis),max_size(mas),sel_min(smi),sel_max(sma),background(back),eight(eight)
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
		FillRectangle(ImageUtils::Rectangle<signed int> offsets,std::array<unsigned char,4> color,unsigned int num_layers,origin_reference orgn):offsets(offsets),color(color),num_layers(num_layers),origin(orgn)
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
		bool use_horiz;
	public:
		inline Straighten(double pixel_prec,double min_angle,double max_angle,double angle_prec,unsigned char boundary,float gamma,bool use_horiz)
			:pixel_prec(pixel_prec),
			min_angle(M_PI_2+min_angle*DEG_RAD),max_angle(M_PI_2+max_angle*DEG_RAD),
			num_steps(std::ceil((max_angle-min_angle)/angle_prec)),
			boundary(boundary),gamma(gamma),use_horiz(use_horiz)
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
		Rotate(float angle,interp_mode mode):angle(angle),mode(mode)
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
		inline ThreadOverride(unsigned int const* num_threads):_num_threads(num_threads)
		{}
		inline unsigned int num_threads() const
		{
			return *_num_threads;
		}
	};

	class ShiftFixer:public ImageProcess<> {
	protected:
		bool side,direction;unsigned char background_threshold;
		inline ShiftFixer(bool side,bool direction,unsigned char bt):side(side),direction(direction),background_threshold(bt)
		{}
	};

	class HorizontalShift:public ShiftFixer {
	public:
		HorizontalShift(bool eval_right,bool from_bottom,unsigned char bt):ShiftFixer(eval_right,from_bottom,bt)
		{}
		bool process(Img&) const override;
	};

	class VerticalShift:public ShiftFixer {
	public:
		inline VerticalShift(bool eval_bottom,bool from_right,unsigned char bt):ShiftFixer(eval_bottom,from_right,bt)
		{}
		bool process(Img&) const override;
	};

	class RescaleAbsolute:public ImageProcess<> {
		unsigned int width;
		unsigned int height;
		float ratio;
		Rescale::rescale_mode mode;
	public:
		RescaleAbsolute(unsigned int width,unsigned int height,float ratio,Rescale::rescale_mode mode):width{width},height{height},ratio{ratio},mode{mode}{}
		bool process(Img&) const override;
	};

	class ChangeCanvasSize:public ImageProcess<> {
		int width;
		int height;
		FillRectangle::origin_reference origin;
		unsigned char fill;
	public:
		ChangeCanvasSize(int width, int height, FillRectangle::origin_reference origin, unsigned char fill = 255):width{width}, height{height}, origin{origin}, fill{fill}{}
		bool process(Img&) const override;
	};

	class MLAA:public ImageProcess<> {
		double gamma;
		unsigned char contrast_threshold;
	public:
		MLAA(double gamma,unsigned char contrast_threshold):gamma{gamma},contrast_threshold{contrast_threshold}{}
		bool process(Img&) const override;
	};

	class NeuralScale:public ThreadOverride {
		ScoreProcessor::neural_scaler scaler;
		float ratio;
	public:
		inline NeuralScale(float ratio,char const* network,unsigned int const* num_threads):ratio(ratio),scaler(network),ThreadOverride(num_threads)
		{}
		bool process(Img&) const override;
	};

	class TemplateMatchErase:public ImageProcess<> {
		cil::CImg<unsigned char> tmplt;
		float threshold;
	public:
		TemplateMatchErase(char const* filename,float threshold):tmplt(filename),threshold{threshold}{}
		bool process(Img&) const override;
	};
	class SlidingTemplateMatchEraseExact:public ImageProcess<> {
		std::vector<cil::CImg<unsigned char>> tmplts;
		unsigned int downscaling;
		float threshold;
		decltype(tmplts) downsized_tmplts;
		ImageUtils::Rectangle<signed int> offsets;
		FillRectangle::origin_reference origin;
		std::function<void(Img&,Img const&,ImageUtils::PointUINT)> replacer;
		static auto get_downsized(decltype(tmplts)& t,unsigned int scale)
		{
			decltype(tmplts) down;
			down.reserve(t.size());
			if(scale==1)
			{
				for(auto& tmplt:t)
				{
					down.push_back({tmplt,true});
				}
			}
			else
			{
				for(auto& tmplt:t)
				{
					down.push_back(integral_downscale(tmplt,scale));
				}
			}
			return down;
		}
	public:
		SlidingTemplateMatchEraseExact(decltype(tmplts) the_tmplts,unsigned int downscaling,float threshold,decltype(replacer) replacer,decltype(offsets) off,decltype(origin) or):
			tmplts(std::move(the_tmplts)),
			downscaling{downscaling},
			threshold{threshold},
			downsized_tmplts(get_downsized(tmplts,downscaling)),
			replacer{std::move(replacer)},
			offsets{off},
			origin{or}
		{
		}
		bool process(Img&) const override;
	};

	class PyramidTemplateErase:public ImageProcess<> {
		std::vector<cil::CImg<unsigned char>> tmplts;
		std::size_t num_images;
		std::vector<unsigned int> scales;
		float threshold;
		std::function<void(Img&,Img const&,ImageUtils::PointUINT)> replacer;
		void verify_scales()
		{
			if(scales.size()==0)
			{
				throw std::invalid_argument("At least 1 scale required");
			}
			scales.erase(std::unique(scales.begin(),scales.end()),scales.end());
			std::sort(scales.begin(),scales.end(),std::greater<>{});
			for(auto const scale:scales)
			{
				if(scale==0)
				{
					throw std::invalid_argument("Scale cannot be zero");
				}
			}
			for(std::size_t i=1;i<scales.size();++i)
			{
				if(scales[i-1]%scales[i])
				{
					throw std::invalid_argument("Scale must be multiple of next scale");
				}
			}
		}
		static cil::CImg<unsigned char> get_downscale(cil::CImg<unsigned char>& img,unsigned int downscale)
		{
			if(downscale==1)
			{
				return {img,true};
			}
			return integral_downscale(img,downscale);
		}
	public:
		PyramidTemplateErase(std::string_view const* tmplt_names,std::size_t n,decltype(scales) scale_factors,float threshold,decltype(replacer) replacer):
			scales(std::move(scale_factors)),num_images{n},threshold{threshold},replacer{std::move(replacer)}
		{
			verify_scales();
			tmplts.reserve(n*scales.size());
			std::unique_ptr<char[]> string_buffer(new char[std::max_element(tmplt_names,tmplt_names+n,[](auto a,auto b)
				{
					return a.size()<b.size();
				})->size()+1]);
			for(size_t i=0;i<n;++i)
			{
				auto const name=tmplt_names[i];
				std::memcpy(string_buffer.get(),name.data(),name.size());
				string_buffer[name.size()]=='\0';
				cil::CImg<unsigned char> orig(string_buffer.get());
				for(size_t j=0;j<scales.size();++j)
				{
					auto const scale=scales[j];
					tmplts.emplace_back(get_downscale(orig,scale));
				}
				tmplts.emplace_back(std::move(orig));
			}
		}
		bool process(Img&) const override;
	};

	class RemoveEmptyLines:public ImageProcess<> {
		unsigned int min_space;
		unsigned int max_presence;
		unsigned char background_threshold;
	public:
		RemoveEmptyLines(unsigned int ms,unsigned int mp,unsigned char bt):min_space{ms},max_presence{mp},background_threshold{bt}{}
		bool process(Img&) const override;
	 };

	class VertCompress:public ImageProcess<> {
		unsigned int min_vert_space;
		unsigned int min_horiz_space;
		unsigned int min_horizontal_protection;
		unsigned int max_vertical_protection;
		unsigned char background_threshold;
	public:
		VertCompress(unsigned int min_vert_space,unsigned int min_horiz_space,unsigned int min_horiz_prot,unsigned int max_vert_prot,unsigned char bt):
			min_vert_space{min_vert_space},
			min_horiz_space{min_horiz_space==-1?min_vert_space:min_horiz_space},
			min_horizontal_protection{min_horiz_prot},
			max_vertical_protection{max_vert_prot},
			background_threshold{bt}{}
		bool process(Img&) const override;
	};

	class ResizeToBound:public ImageProcess<> {
	protected:
		unsigned int width;
		unsigned int height;
		bool pad;
		unsigned char fill;
		Rescale::rescale_mode mode;
		float gamma;
	public:
		ResizeToBound(unsigned int width, unsigned int height, bool pad=false, unsigned char fill=255,Rescale::rescale_mode mode=Rescale::automatic, float gamma=2):
			width(width),height(height),pad(pad),fill(fill),mode(mode),gamma(gamma){}

		bool process(Img&) const override;
	};

	class SquishToBound:public ResizeToBound {
	public:
		using ResizeToBound::ResizeToBound;
		bool process(Img&) const override;
	};

	class ClusterWiden:public ImageProcess<> {
		unsigned char _lower_bound;
		unsigned char _upper_bound;
		unsigned int _widen_to;
		Rescale::rescale_mode _mode;
		float _gamma;
	public:
		ClusterWiden(unsigned char lower_bound, unsigned char upper_bound, unsigned int widen_to, Rescale::rescale_mode mode, float gamma) noexcept:
			_lower_bound(lower_bound),_upper_bound(upper_bound),_widen_to(widen_to),_mode(mode),_gamma(gamma)
		{}
		bool process(Img&) const override;
	};

}
#endif