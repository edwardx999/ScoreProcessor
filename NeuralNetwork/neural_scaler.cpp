#include "neural_scaler.h"
#include <assert.h>
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
		struct info {
			unsigned int padding;
			unsigned int input_dim;
			unsigned int output_dim;
			unsigned int scale_factor;
		};
		info inf;
		inf.input_dim=input_dim();
		inf.output_dim=output_dim();
		inf.scale_factor=scale_factor();
		inf.padding=padding();
		auto const i=input_dim();
		auto const o=output_dim();
		auto const s=scale_factor();
		using Img=cil::CImg<unsigned char>;
		Img orig(img,true);//shared view of image
		unsigned int desired_width=static_cast<unsigned int>(std::round(img._width*scale));
		unsigned int desired_height=static_cast<unsigned int>(std::round(img._height*scale));

		exlib::ThreadPoolA<Img*,Img const*,neural_scaler const*,info const*> pool(num_threads);
		while(true)
		{
			Img upscaled(orig._width*s,orig._height*s);
			using Task=exlib::ThreadTaskA<Img*,Img const*,neural_scaler const*,info const*>;
			struct Scaler:Task {
			protected:
				unsigned int output_x;
			public:
				Scaler(unsigned int output_x):
					output_x(output_x)
				{}
			private:
				bool all_white(float const* input,size_t const limit) const
				{
					for(size_t i=0;i<limit;++i)
					{
						if(input[i]!=1.0f)
						{
							return false;
						}
					}
					return true;
				}
				void write_to_img(Img& out,float const* in_row,size_t const y,size_t const output_dim) const
				{
					auto const out_width=out._width;
					auto out_row=out._data+out_width*y+output_x;
					size_t const c_limit=std::min<size_t>(out._width-output_x,output_dim);
					size_t const r_limit=std::min<size_t>(out._height-y,output_dim);
					auto const in_limit=in_row+r_limit*output_dim;
					for(;in_row<in_limit;in_row+=output_dim,out_row+=out_width)
					{
						for(size_t c=0;c<c_limit;++c)
						{
							auto const val=in_row[c];
							if(val>=1.0f)
							{
								out_row[c]=255;
							}
							else if(val<=0.0f)
							{
								out_row[c]=0;
							}
							else
							{
								out_row[c]=static_cast<unsigned char>(std::round(val*255));
							}
						}
					}
				}
				void write_to_img(Img& out,size_t const y,size_t const output_dim) const
				{
					auto const out_width=out._width;
					auto out_row=out._data+out_width*y+output_x;
					size_t const c_limit=std::min<size_t>(out._width-output_x,output_dim);
					size_t const r_limit=std::min<size_t>(out._height-y,output_dim);
					auto const out_limit=out_row+r_limit*out_width;
					for(;out_row<out_limit;out_row+=out_width)
					{
						for(size_t c=0;c<c_limit;++c)
						{
							out_row[c]=255;
						}
					}
				}
				void scale(Img& out,Img const& in,neural_scaler const& ns,info const& inf)
				{
					//begin and end are boundaries of box to take values from
					//start and finish are valid values to take values from
					using st=std::ptrdiff_t;
					st const input_x=output_x/inf.scale_factor;
					st const input_dim=inf.input_dim;
					st const output_dim=inf.output_dim;
					st const scale_factor=inf.scale_factor;
					st const padding=inf.padding;
					size_t const input_area=size_t{inf.input_dim}*inf.input_dim;
					size_t const output_area=size_t{inf.output_dim}*inf.output_dim;
					std::unique_ptr<float[]> input(new float[input_area]);
					std::unique_ptr<float[]> output(new float[output_area]);
					st const out_height=out._height;
					st const in_height=in._height;
					st const in_width=in._width;
					st const x_begin=static_cast<st>(input_x)-padding;
					st const x_end=x_begin+input_dim;
					st const x_start=std::max<st>(0,x_begin);
					st const x_finish=std::min<st>(in_width,x_end);
					if(x_begin<0)
					{
						auto const amount=-x_begin;
						for(st y=0;y<input_dim;++y)
						{
							std::fill_n(input.get()+y*input_dim,amount,1.0f);
						}
					}
					if(x_end>in_width)
					{
						auto const offset=input.get()+in_width-x_begin;
						auto const count=x_end-in_width;
						for(st y=0;y<input_dim;++y)
						{
							std::fill_n(offset+y*input_dim,count,1.0f);
						}
					}
					for(st y=0;y<out_height;y+=output_dim)
					{
						st const y_input=y/scale_factor;
						st const y_begin=y_input-padding;
						st const y_end=y_begin+input_dim;
						st y_start,y_finish;
						if(y_begin<0)
						{
							y_start=0;
							st amount=-y_begin*input_dim;
							assert(amount<input_dim*input_dim);
							std::fill_n(input.get(),amount,1.0f);
						}
						else
						{
							y_start=y_begin;
						}
						if(y_end>in_height)
						{
							y_finish=in_height;
							st offset=(in_height-y_begin)*input_dim;
							st amount=(y_end-in_height)*input_dim;
							assert(offset+amount==input_area);
							std::fill_n(input.get()+offset,amount,1.0f);
						}
						else
						{
							y_finish=y_end;
						}
						for(st y_in=y_start;y_in<y_finish;++y_in)
						{
							auto const img_row=in._data+y_in*in_width;
							auto const in_row=input.get()+(y_in-y_begin)*input_dim-x_begin;
							for(st x=x_start;x<x_finish;++x)
							{
								assert(in_row+x<input.get()+input_dim*input_dim);
								assert(img_row+x<in._data+in._height*in._width);
								in_row[x]=img_row[x]/255.0f;
							}
						}
						if(!all_white(input.get(),input_area))
						{
							ns.feed(output.get(),input.get());
							write_to_img(out,output.get(),y,output_dim);
						}
						else
						{
							write_to_img(out,y,output_dim);
						}
					}
				}
			public:
				void execute(Img* out,Img const* in,neural_scaler const* ns,info const* inf) override
				{
					scale(*out,*in,*ns,*inf);
				}
			};
			for(unsigned int x=0;x<upscaled._width;x+=inf.output_dim)
			{
				pool.add_task<Scaler>(x);
			}
			pool.start(&upscaled,&orig,this,&inf);
			pool.join();
			if(upscaled._width>=desired_width)
			{
				return upscaled.resize(desired_width,desired_height);
			}
			orig=std::move(upscaled);
		}
	}
}