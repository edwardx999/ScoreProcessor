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

	unsigned int splice_find_top(cil::CImg<unsigned char> const& img,unsigned char bg)
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
		return min;
	}

	unsigned int splice_find_bottom(cil::CImg<unsigned char> const& img,unsigned char bg)
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
		return max;
	}

	void get_optimal_values(Splice::standard_heuristics const& sh,cil::CImg<unsigned char> const& ref,
		unsigned int& horiz_padding,unsigned int& min_pad,unsigned int& opt_pad,unsigned int& opt_height)
	{
		std::array<unsigned int,2> bases{ref._width,ref._height};
		horiz_padding=sh.horiz_padding(bases);
		min_pad=sh.min_padding(bases);
		opt_pad=sh.optimal_padding(bases);
		opt_height=sh.optimal_height(bases);
	}

	float layout_cost(Splice::page_layout p,Splice::standard_heuristics const& sh,unsigned int horiz_padding,unsigned int opt_pad,unsigned int opt_height)
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

	unsigned int splice_pages_parallel(
		std::vector<std::string> const& filenames,
		SaveRules const& output_rule,
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
		get_optimal_values(sh,managers[0].img(),horiz_padding,min_pad,opt_pad,opt_height);
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
			auto min=splice_find_top(img,bg);
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
			auto max=splice_find_bottom(img,bg);
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
			return layout_cost(p,sh,horiz_padding,opt_pad,opt_height);
		};
		auto saver = [quality](auto const& image, char const* name)
		{
			auto support = supported_path(name);
			if(support==decltype(support)::no)
			{
				support=decltype(support)::png;
			}
			//auto save=image.get_rotate(-90);
			return cil::save_image(image, name, support, quality);
		};
		return splice_pages_parallel(managers,output_rule,starting_index,num_threads,pe,create_layout,cost,&splice_images, saver);
	}

	unsigned int splice_pages_parallel(
		std::vector<std::string> const& filenames,
		SaveRules const& output_rule,
		unsigned int starting_index,
		unsigned int num_threads,
		Splice::standard_heuristics const& sh,
		cil::CImg<unsigned char> const& divider,
		int quality)
	{
		auto const c=filenames.size();
		if(c<2)
		{
			throw std::invalid_argument("Need multiple pages to splice");
		}
		// auto const extension=exlib::find_extension(output,output+std::strlen(output));
		// validate_extension(extension);
		Splice::page divider_desc{{divider,true}};
		std::vector<Splice::page> descriptions(filenames.size());
		exlib::thread_pool pool{num_threads};
		std::mutex error_lock;
		std::string error_log;
		unsigned int horiz_padding,min_pad,opt_pad,opt_height;
		auto get_dims=[bg=sh.background_color](Splice::page& page){
			page.top=splice_find_top(page.img,bg);
			page.bottom=splice_find_bottom(page.img,bg);
		};
		pool.push_back([get_dims,&divider_desc]() noexcept
		{
			get_dims(divider_desc);
		});
		pool.push_back([&,&desc=descriptions[0],&name=filenames[0],bg=get_dims](decltype(pool)::parent_ref parent) noexcept
		{
			try
			{
				desc.img.load(name.c_str());
				get_dims(desc);
				get_optimal_values(sh,desc.img,horiz_padding,min_pad,opt_pad,opt_height);
				desc.img=cil::CImg<unsigned char>{};
			}
			catch(std::exception const& err)
			{
				parent.stop();
				std::lock_guard guard{error_lock};
				error_log.append(name).append(": ").append(err.what()).append("\n");
			}
		}
		);
		for(std::size_t i=1;i<descriptions.size();++i)
		{
			pool.push_back([&,&desc=descriptions[i],&name=filenames[i],get_dims](decltype(pool)::parent_ref parent) noexcept
			{
				try
				{
					desc.img.load(name.c_str());
					get_dims(desc);
					desc.img=cil::CImg<unsigned char>{};
				}
				catch(std::exception const& err)
				{
					parent.stop();
					std::lock_guard guard{error_lock};
					error_log.append(name).append(": ").append(err.what()).append("\n");
				}
			});
		}
		pool.wait();
		if(!error_log.empty())
		{
			throw std::runtime_error(error_log);
		}
		auto create_layout=[&](Splice::page const* const items,size_t const n)
		{
			assert(n!=0);
			unsigned int total_height=(n-1)*divider_desc.true_height();
			for(size_t i=0;i<n;++i)
			{
				total_height+=items[i].true_height();
			}
			unsigned int minned=total_height+(2*n)*min_pad;
			if(minned>=opt_height)
			{
				return Splice::page_layout{min_pad,minned};
			}
			else
			{
				return Splice::page_layout{unsigned int((opt_height-total_height)/(2*n)),opt_height};
			}
		};
		auto breaks=nongreedy_break(
			descriptions.begin(),descriptions.end(),
			create_layout,
			[=](Splice::page_layout const p)
			{
				return layout_cost(p,sh,horiz_padding,opt_pad,opt_height);
			});
		unsigned int num_digs=exlib::num_digits(breaks.size()+starting_index);
		num_digs=num_digs<3?3:num_digs;
		unsigned int num_imgs=0;
		auto start=0;
		for(size_t i=breaks.size()-1;;)
		{
			++num_imgs;
			auto const end=breaks[i].index;
			auto const s=end-start;
			pool.push_back(
				[&,
				filename_index=start+starting_index,
				fbegin=filenames.data()+start,
				ibegin=descriptions.data()+start,
				num_pages=s,
				padding=breaks[i].padding,
				quality](decltype(pool)::parent_ref parent) noexcept{
				try
				{
					std::vector<Splice::page> imgs(num_pages*2-1);
					imgs[0].img.load(fbegin->c_str());
					imgs[0].top=ibegin->top;
					imgs[0].bottom=ibegin->bottom;
					for(size_t i=1;i<num_pages;++i)
					{
						imgs[2*i-1].img=cil::CImg{divider,true};
						imgs[2*i-1].top=divider_desc.top;
						imgs[2*i-1].bottom=divider_desc.bottom;
						imgs[2*i].img.load(fbegin[i].c_str());
						imgs[2*i].top=ibegin[i].top;
						imgs[2*i].bottom=ibegin[i].bottom;
					}
					auto const output=output_rule.make_filename(fbegin[0],filename_index);
					auto support=supported_path(output.c_str());
					if(support==decltype(support)::no)
					{
						support=decltype(support)::png;
					}
					auto const& filename=output;
					cil::save_image(splice_images(imgs.data(),imgs.size(),padding),filename.c_str(),support,quality);
				}
				catch(std::exception const& ex)
				{
					std::string names{fbegin[0]};
					names.append(" to ").append(fbegin[num_pages-1]);
					parent.stop();
					std::lock_guard guard(error_lock);
					error_log.append(names);
				}
			});
			if(i==0)
			{
				break;
			}
			--i;
			start=end;
		}
		pool.join();
		if(!error_log.empty())
		{
			throw std::runtime_error(error_log);
		}
		return num_imgs;
	}
}