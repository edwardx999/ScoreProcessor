#include "neural_scaler.h"
#include "../ScoreProcessor/lib/threadpool/ThreadPool.h"
namespace ScoreProcessor {
	cil::CImg<unsigned char> neural_scaler::get_smart_scale(cil::CImg<unsigned char> const& img,float scale,unsigned int num_threads) const
	{
		if(scale<1)
		{
			return img.get_resize(
				static_cast<unsigned int>(std::round(img._width*scale)),
				static_cast<unsigned int>(std::round(img._height*scale)),
				img._depth,
				img._spectrum,
				2);
		}
		if(scale==1)
		{
			return img;
		}
		auto const i=input_dim();
		auto const o=output_dim();
		auto const s=scale_factor();
		using Img=cil::CImg<unsigned char>;
		Img orig(img,true);//shared view of image
		unsigned int desired_width=static_cast<unsigned int>(std::round(img._width*scale));
		unsigned int desired_height=static_cast<unsigned int>(std::round(img._height*scale));
		while(true)
		{
			Img upscaled(orig._width*s,orig._height*s);
			using Task=exlib::ThreadTaskA<Img*,Img const*,unsigned int,neural_scaler const*>;
			struct Scaler:Task {
			protected:
				unsigned int const output_x;
				unsigned int const input_x;
				unsigned int scale_factor;
				unsigned int const input_dim;
				unsigned int const output_dim;
				std::unique_ptr<float[]> input;
				std::unique_ptr<float[]> output;
			public:
				Scaler(unsigned int output_x,unsigned int scale_factor,unsigned int input_dim,unsigned int output_dim):
					output_x(output_x),
					input_x(output_x/scale_factor),
					scale_factor(scale_factor),
					input_dim(input_dim),
					output_dim(output_dim),
					input(new float[input_dim*input_dim]),
					output(new float[output_dim*output_dim])
				{}
			private:
				bool all_white() const
				{
					size_t const limit=size_t{input_dim}*input_dim;
					for(size_t i=0;i<limit;++i)
					{
						if(input[i]!=1.0f)
						{
							return false;
						}
					}
					return true;
				}
				void write_to_img(Img* const out,size_t const y) const
				{
					auto const out_width=out->_width;
					auto out_row=out->_data+out->_width*y;
					auto in_row=output.get();
					size_t const c_limit=std::min<size_t>(out->_width-output_x,output_dim);
					size_t const r_limit=std::min<size_t>(out->_height-y,output_dim);
					auto const in_limit=in_row+r_limit*output_dim;
					for(;in_row<in_limit;in_row+=output_dim,out_row+=out_width)
					{
						for(size_t c=0;c<c_limit;++c)
						{
							out_row[c]=static_cast<unsigned char>(std::round(in_row[c]*255));
						}
					}
				}
				void scale(Img* const out,Img const* const in,unsigned int const padding,neural_scaler const* const ns)
				{
					//begin and end are boundaries of box to take values from
					//start and finish are valid values to take values from
					using st=std::ptrdiff_t;
					st const out_height=out->_height;
					st const in_height=in->_height;
					st const in_width=in->_width;
					st x_begin=input_x-padding;
					st x_end=input_x+input_dim-padding;
					st x_start=std::max<st>(0,x_begin);
					st x_finish=std::min<st>(in_width,x_end);
					for(st y=0;y<out_height;y+=output_dim)
					{
						st y_input=y/scale_factor;
						st y_begin=y_input-padding;
						st y_end=y_input+input_dim-padding;
						st y_start,y_finish;
						if(y_begin<0)
						{
							y_start=0;
							std::fill_n(input.get(),-y_begin*input_dim,1.0f);
						}
						else
						{
							y_start=y_begin;
						}
						if(y_end>in_height)
						{
							y_end=in_height;
							std::fill_n(input.get()+(y_end-y_begin)*input_dim,y_end-in_height,1.0f);
						}
						else
						{
							y_finish=y_end;
						}
						for(st y_in=y_start;y_in<y_finish;++y_in)
						{
							auto const img_row=in->_data+y_in*in_width;
							auto const in_row=input.get()+(y_in-y_begin)*input_dim;
							for(st x=x_start;x<x_end;++x)
							{
								in_row[x]=img_row[x]/255.0f;
							}
						}
						if(x_start!=x_begin)
						{
							auto const s=y_start-y_begin;
							auto const l=y_finish-y_start;
							auto const count=x_start-x_begin;
							for(st y=s;s<l;++y)
							{
								std::fill_n(input.get()+s*input_dim,count,1.0f);
							}
						}
						if(x_finish!=x_end)
						{
							auto const s=y_start-y_begin;
							auto const l=y_finish-y_start;
							auto const offset=input.get()+x_finish-x_begin;
							auto const count=x_end-x_finish;
							for(st y=s;s<l;++y)
							{
								std::fill_n(offset+s*input_dim,count,1.0f);
							}
						}
						if(!all_white())
						{
							ns->feed(output.get(),input.get());
							write_to_img(out,y);
						}
					}
				}
			public:
				void execute(Img* out,Img const* in,unsigned int padding,neural_scaler const* ns) override
				{
					scale(out,in,padding,ns);
				}
			};
			exlib::ThreadPoolA<Img*,Img const*,unsigned int,neural_scaler const*> pool(num_threads);
			unsigned int padding=(o/s-i)/2;
			for(unsigned int x=0;x<upscaled._width;x+=o)
			{
				pool.add_task<Scaler>(x,s,i,o);
			}
			pool.start(&upscaled,&orig,padding,this);
			pool.wait();
			if(upscaled._width>=desired_width)
			{
				return upscaled.resize(desired_width,desired_height);
			}
			orig=std::move(upscaled);
		}
	}
}