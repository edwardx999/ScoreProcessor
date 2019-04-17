#include "stdafx.h"
#include "Splice.h"
#include "ScoreProcesses.h"
namespace ScoreProcessor {

	template<typename TreatPixel>//function (tog_pixel_pointer,tog_size,current_img,x,y)
	cil::CImg<unsigned char>& splice_images_h(Splice::page const* imgs,size_t num,unsigned int padding,cil::CImg<unsigned char>& tog,TreatPixel tp)
	{
		unsigned int ypos=padding;
		auto const size=size_t{tog._width}*tog._height;
		for(size_t i=0;i<num;++i)
		{
			auto const& current=imgs[i];
			unsigned int begin,end;
			if(ypos<current.top)
			{
				begin=current.top;
			}
			else
			{
				begin=0;
			}
			if(ypos+current.img._height>tog._height+current.top)
			{
				end=tog._height+current.top-ypos;
			}
			else
			{
				end=current.img._height;
			}
			for(unsigned int y=begin;y<end;++y)
			{
				auto yabs=ypos+y-current.top;
				for(unsigned int x=1;x<=current.img._width;++x)
				{
					tp(&tog(tog._width-x,yabs),size,current.img,current.img._width-x,y);
				}
			}
			ypos+=padding+current.true_height();
		}
		return tog;
	}

	cil::CImg<unsigned char> splice_images(Splice::page const* imgs,size_t num,unsigned int padding)
	{
		unsigned int height=0;
		unsigned int width=0;
		unsigned int spectrum=0;
		for(size_t i=0;i<num;++i)
		{
			height+=imgs[i].true_height();
			if(imgs[i].img._width>width)
			{
				width=imgs[i].img._width;
			}
			if(imgs[i].img._spectrum>spectrum)
			{
				spectrum=imgs[i].img._spectrum;
			}
		}
		height+=padding*(num+1);
		cil::CImg<unsigned char> tog(width,height,1,spectrum);
		tog.fill(255);
		switch(spectrum)
		{
			case 1:
			case 2:
				return splice_images_h(imgs,num,padding,tog,[](unsigned char* pixel,size_t const size,cil::CImg<unsigned char> const& current,unsigned int x,unsigned int y)
				{
					*pixel=std::min(*pixel,current(x,y));
				});
				/*return splice_images_h(imgs,num,padding,tog,[](unsigned char* pixel,size_t const size,cil::CImg<unsigned char> const& current,unsigned int x,unsigned int y)
				{
					double dark=(255U-*pixel)*(pixel[size]/255.0);
					double cdark;
					auto const cpixel=&current(x,y);
					size_t csize;
					switch(current._spectrum)
					{
						case 1:
							cdark=255U-*cpixel;
							break;
						case 2:
							cdark=(255U-*cpixel)*(cpixel[csize=current._width*current._height]/255.0);
					}
					if(cdark>dark)
					{
						switch(current._spectrum)
						{
							case 1:
								*pixel=*cpixel;
								pixel[size]=255;
								break;
							case 2:
								*pixel=*cpixel;
								pixel[size]=cpixel[csize];
						}
					}
				});*/
			case 3:
				using pinfo=uint_fast32_t;
				return splice_images_h(imgs,num,padding,tog,[](unsigned char* pixel,size_t const size,cil::CImg<unsigned char> const& current,unsigned int x,unsigned int y)
				{
					pinfo const br=(pinfo{pixel[0]}+pixel[size]+pixel[2*size]);
					size_t csize;
					pinfo cbr;
					auto const cpixel=&current(x,y);
					switch(current._spectrum)
					{
						case 1:
							cbr=*cpixel*3U;
							break;
						case 3:
							csize=size_t{current._height}*current._width;
							cbr=pinfo{cpixel[0]}+cpixel[csize]+cpixel[2*csize];
					}
					if(cbr<br)
					{
						switch(current._spectrum)
						{
							case 1:
								pixel[0]=pixel[size]=pixel[2*size]=cpixel[0];
								break;
							case 3:
								pixel[0]=cpixel[0];
								pixel[size]=cpixel[csize];
								pixel[2*size]=cpixel[2*csize];
						}
					}
				});
			case 4:
				return splice_images_h(imgs,num,padding,tog,[](unsigned char* pixel,size_t const size,cil::CImg<unsigned char> const& current,unsigned int x,unsigned int y)
				{
					constexpr pinfo max=3U*255U;
					pinfo drk=(max-(pinfo{pixel[0]}+pixel[size]+pixel[2*size]))*pixel[3*size];
					size_t csize;
					pinfo cdrk;
					auto const cpixel=&current(x,y);
					switch(current._spectrum)
					{
						case 1:
							cdrk=(max-(*cpixel*3U))*255U;
							break;
						case 3:
							csize=size_t{current._height}*current._width;
							cdrk=(max-(pinfo{cpixel[0]}+cpixel[csize]+cpixel[2*csize]))*255U;
							break;
						case 4:
							csize=size_t{current._height}*current._width;
							cdrk=(max-(pinfo{cpixel[0]}+cpixel[csize]+cpixel[2*csize]))*cpixel[3*csize];
							break;
					}
					if(cdrk>drk)
					{
						switch(current._spectrum)
						{
							case 1:
								pixel[0]=pixel[size]=pixel[2*size]=cpixel[0];
								pixel[3*size]=255;
								break;
							case 3:
								pixel[0]=cpixel[0];
								pixel[size]=cpixel[csize];
								pixel[2*size]=cpixel[2*csize];
								pixel[3*size]=255;
								break;
							case 4:
								pixel[0]=cpixel[0];
								pixel[size]=cpixel[csize];
								pixel[2*size]=cpixel[2*csize];
								pixel[3*size]=cpixel[3*csize];
						}
					}
				});
		}
	}

	struct spacing {
		unsigned int bottom_sg;
		unsigned int top_sg;
	};

	spacing find_spacing(std::vector<unsigned int> const& bottom_of_top,unsigned int size_top,std::vector<unsigned int> const& top_of_bottom)
	{
		auto b=bottom_of_top.rbegin();
		auto t=top_of_bottom.rbegin();
		auto end=b+std::min(bottom_of_top.size(),top_of_bottom.size());
		unsigned int min_spacing=std::numeric_limits<unsigned int>::max();
		spacing ret;
		for(;b!=end;++b,++t)
		{
			unsigned int cand=size_top-*b+*t;
			if(cand<min_spacing)
			{
				min_spacing=cand;
				ret.bottom_sg=*b;
				ret.top_sg=*t;
			}
		}
		return ret;
	}

	unsigned int splice_pages_parallel(
		std::vector<std::string> const& filenames,
		char const* output,
		unsigned int starting_index,
		unsigned int num_threads,
		Splice::standard_heuristics const& sh,
		int quality)
	{
		if(filenames.size()<2)
		{
			throw std::invalid_argument("Need multiple pages to splice");
		}
		std::vector<Splice::manager> managers(filenames.size());
		for(size_t i=0;i<filenames.size();++i)
		{
			managers[i].fname(filenames[i].c_str());
		}
		managers[0].load();
		unsigned int horiz_padding,min_pad,opt_pad,opt_height;
		std::array<unsigned int,2> bases{managers[0].img()._width,managers[0].img()._height};
		horiz_padding=sh.horiz_padding(bases);
		min_pad=sh.min_padding(bases);
		opt_pad=sh.optimal_padding(bases);
		opt_height=sh.optimal_height(bases);
		using Img=cil::CImg<unsigned char>;
		auto find_top=[bg=sh.background_color](Img const& img){
			if(img._spectrum<3)
			{
				return build_top_profile(img,bg);
			}
			else
			{
				return build_top_profile(img,ImageUtils::ColorRGB({bg,bg,bg}));
			}
		};
		auto find_bottom=[bg=sh.background_color](Img const& img){
			if(img._spectrum<3)
			{
				return build_bottom_profile(img,bg);
			}
			else
			{
				return build_bottom_profile(img,ImageUtils::ColorRGB({bg,bg,bg}));
			}
		};
		Splice::PageEval pe([bg=sh.background_color](Img const& img)
		{
			unsigned int min=img._height/2;
			if(img._spectrum<3)
			{
				for(unsigned int x=0;x<img._width;++x)
				{
					for(unsigned int y=0;y<min;++y)
					{
						if(img(x,y)<=bg)
						{
							min=y;
							break;
						}
					}
				}
			}
			else
			{
				unsigned int limit=3U*bg;
				for(unsigned int x=0;x<img._width;++x)
				{
					for(unsigned int y=0;y<min;++y)
					{
						if(static_cast<unsigned int>(img(x,y))+img(x,y,1)+img(x,y,2)<=limit)
						{
							min=y;
							break;
						}
					}
				}
			}
			return Splice::edge{min,min};
		},[=](Img const& t,Img const& b)
		{
			auto top=exlib::fattened_profile(find_bottom(t),horiz_padding,[](auto a,auto b)
			{
				return a>b;
			});
			auto bot=exlib::fattened_profile(find_top(b),horiz_padding,[](auto a,auto b)
			{
				return a<b;
			});
			auto top_max=*std::max_element(top.begin(),top.end());
			auto bot_min=*std::min_element(bot.begin(),bot.end());
			auto spacing=find_spacing(top,top_max,bot);
			Splice::page_desc ret;
			ret.bottom.kerned=spacing.bottom_sg;
			ret.bottom.raw=top_max;
			ret.top.kerned=spacing.top_sg;
			ret.top.raw=bot_min;
			return ret;
		},[bg=sh.background_color](Img const& img)
		{
			unsigned int max=img._height/2;
			if(img._spectrum<3)
			{
				for(unsigned int x=0;x<img._width;++x)
				{
					for(unsigned int y=img._height-1;y>max;--y)
					{
						if(img(x,y)<=bg)
						{
							max=y;
							break;
						}
					}
				}
			}
			else
			{
				unsigned int limit=3U*bg;
				for(unsigned int x=0;x<img._width;++x)
				{
					for(unsigned int y=img._height-1;y>max;--y)
					{
						if(static_cast<unsigned int>(img(x,y))+img(x,y,1)+img(x,y,2)<=limit)
						{
							max=y;
							break;
						}
					}
				}
			}
			return Splice::edge{max,max};
		});
		auto create_layout=[=](Splice::page_desc const* const items,size_t const n)
		{
			assert(n!=0);
			unsigned int total_height;
			if(n==1)
			{
				total_height=items[0].bottom.raw-items[0].top.raw;
			}
			else
			{
				total_height=items[0].bottom.kerned-items[0].top.raw;
				for(size_t i=1;i<n-1;++i)
				{
					total_height+=items[i].bottom.kerned-items[i].top.kerned;
				}
				total_height+=items[n-1].bottom.raw-items[n-1].top.kerned;
			}
			unsigned int minned=total_height+(n+1)*min_pad;
			if(minned>=opt_height)
			{
				return Splice::page_layout{min_pad,minned};
			}
			else
			{
				return Splice::page_layout{unsigned int((opt_height-total_height)/(n+1)),opt_height};
			}
		};
		auto cost=[=](Splice::page_layout const p)
		{
			float numer;
			if(p.height>opt_height)
			{
				numer=sh.excess_weight*(p.height-opt_height);
			}
			else
			{
				numer=opt_height-p.height;
			}
			float height_cost=numer/opt_height;
			height_cost=height_cost*height_cost*height_cost;
			float padding_cost=sh.padding_weight*exlib::abs_dif(float(p.padding),opt_pad)/opt_pad;
			padding_cost=padding_cost*padding_cost*padding_cost;
			return height_cost+padding_cost;
		};
		return splice_pages_parallel(managers,output,starting_index,num_threads,pe,create_layout,cost,quality);
	}
}