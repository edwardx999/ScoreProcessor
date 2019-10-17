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
#include "ScoreProcesses.h"
#include <thread>
#include "ImageUtils.h"
#include "Cluster.h"
#include <iostream>
#include <string>
#include <stack>
#include <cmath>
#include <array>
#include "moreAlgorithms.h"
#include "shorthand.h"
#include <assert.h>
#include "ImageMath.h"
#include "lib/exstring/exmath.h"
#include "lib/exstring/exalg.h"
#include <atomic>
#include <mutex>
#include "lib/threadpool/thread_pool.h"
#include <numeric>
using namespace std;
using namespace ImageUtils;
using namespace cimg_library;
using namespace misc_alg;
namespace ScoreProcessor {

	//template<typename PixelDetector>
	bool remove_empty_lines_help(cil::CImg<unsigned char>& img,unsigned int min_space,unsigned int max_presence/*,PixelDetector detector*/)
	{
		unsigned int read_start=0;
		unsigned int write_head=0;
		unsigned int empty_line_count=0;
		auto const size=std::size_t{img._width}*img._height;
		bool changed=false;
		auto is_line_foreground=[&img,max_presence](unsigned int y)
		{
			auto const row=&img(0,y);
			unsigned int background_count=0;
			for(unsigned int x=0;x<img._width;++x)
			{
				if(/*detector(row+x)*/row[x]<128)
				{
					if(++background_count>max_presence)
					{
						return true;
					}
				}
			}
			return false;
		};
		auto in_empty_region=!is_line_foreground(0);
		unsigned int space;
		for(unsigned int read_head=1;read_head<img._height;++read_head)
		{
			auto const line_is_foreground=is_line_foreground(read_head);
			if(in_empty_region)
			{
				if(line_is_foreground)
				{
					in_empty_region=false;
					//std::cout<<read_start<<' '<<read_head<<'\n';
					space=read_head-read_start;
					if(space>min_space)
					{
						changed=true;
						auto const fore_padding=min_space/2;
						auto const aft_padding=min_space-fore_padding;
						auto const fore_size=std::size_t{fore_padding}*img._width;
						auto const aft_size=std::size_t{aft_padding}*img._width;
						for(unsigned int s=0;s<img._spectrum;++s)
						{
							std::memmove(
								&img(0,write_head,0,s),
								&img(0,read_start,0,s),
								fore_size);
							std::memmove(
								&img(0,write_head+fore_padding,0,s),
								&img(0,read_head-aft_padding,0,s),
								aft_size);
						}
						write_head+=min_space;
					}
					else
					{
						goto copy_unchanged;
					}
					read_start=read_head;
				}
			}
			else
			{
				if(!line_is_foreground)
				{
					//std::cout<<read_start<<' '<<read_head<<'\n';
					in_empty_region=true;
					space=read_head-read_start;
				copy_unchanged:
					if(read_start!=write_head)
					{
						for(unsigned int s=0;s<img._spectrum;++s)
						{
							std::memmove(
								&img(0,write_head,0,s),
								&img(0,read_start,0,s),
								std::size_t{space}*img._width);
						}
					}
					write_head+=space;
					read_start=read_head;
				}
			}
		}
		space=std::min(img._height-read_start,min_space);
		for(unsigned int s=0;s<img._spectrum;++s)
		{
			std::memmove(
				&img(0,write_head,0,s),
				&img(0,read_start,0,s),
				std::size_t{space}*img._width);
		}
		write_head+=space;
		if(img._spectrum==1)
		{
			img.crop(0,0,img._width-1,write_head-1);
		}
		else
		{
			img._height=write_head;
		}
		return changed;
	}

	bool compress_vertical(cil::CImg<unsigned char>& img,unsigned char background_threshold,unsigned int min_vert_space,unsigned int min_horiz_space,unsigned int min_horiz_protection,unsigned int max_vert_protection,unsigned int optimal_height)
	{
		using count_t=short;
		constexpr auto dim_lim=std::numeric_limits<count_t>::max();
		if(img._height<exlib::multi_max(3U,min_vert_space)||img._width<std::max(3U,min_horiz_space)||img._height>dim_lim||img._width>dim_lim)
		{
			return false;
		}
		enum safety {
			safe=-1,
			unsafe=0,
			candidate_path_mark,
			real_path_mark,
		};
		cil::CImg<char> safe_points;
		{
			auto const clusters=[&img,background_threshold]()
			{
				auto const rects=global_select<1>(img,[=](std::array<unsigned char,1> color)
					{
						return color[0]<background_threshold;
					});
				return Cluster::cluster_ranges_8way(rects);
			}();
			if(clusters.size()==0)
			{
				return false;
			}
			auto const it_kill=std::max_element(clusters.begin(),clusters.end(),[](auto const& a,auto const& b)
				{
					return a.size()<b.size();
				});
			auto& kill=*it_kill;
			using CountImg=cil::CImg<count_t>;
			std::vector<Point<unsigned short>> pixels;
			//for(auto const& kill:clusters)
			{
				for(auto const rect:kill.get_ranges())
				{
					using us=unsigned short;
					for(unsigned int y=rect.top;y<rect.bottom;++y)
					{
						for(unsigned int x=rect.left;x<rect.right;++x)
						{
							pixels.push_back({us(x),us(y)});
						}
					}
				}
			}
			std::sort(pixels.begin(),pixels.end(),[](auto a,auto b)
				{
					return std::make_pair(a.x,a.y)<std::make_pair(b.x,b.y);
				});
			CountImg path_counts_raw(img._width,img._height+2);
			CountImg path_counts(path_counts_raw,true);
			path_counts._data+=path_counts._width;
			path_counts._height-=2;
			// steps mark white space as 0
			// protect small clusters (mark as -1)
			// protect portions of kill that you can draw a sufficiently large horizontal path through 
			// mark portions of kill that you can draw a sufficiently large vertical path through as 0
			std::memset(path_counts_raw.begin(),0,path_counts_raw.size()*sizeof(count_t));
			{ //finding horiz_paths
				auto ipix=pixels.begin();
				auto const ipix_end=pixels.end();
				while(true)
				{
					if(ipix==ipix_end)
					{
						goto end_horiz_trace;
					}
					if(ipix->x>0)
					{
						break;
					}
					path_counts(0,ipix->y)=1;
					++ipix;
				}
				do
				{
					auto const pixx=ipix->x;
					auto const pixy=ipix->y;
					auto const val=exlib::multi_max(
						path_counts(pixx-1,pixy-1),
						path_counts(pixx-1,pixy),
						path_counts(pixx-1,pixy+1));
					path_counts(pixx,pixy)=val+1;
					++ipix;
				} while(ipix!=ipix_end);
			end_horiz_trace:
				for(auto it=pixels.rbegin();it!=pixels.rend();++it)
				{
					auto const pix_loc=*it;
					path_counts(pix_loc.x,pix_loc.y)=exlib::multi_max(
						path_counts(pix_loc.x+1,pix_loc.y-1),
						path_counts(pix_loc.x+1,pix_loc.y),
						path_counts(pix_loc.x+1,pix_loc.y+1),
						path_counts(pix_loc.x,pix_loc.y)
					);
				}
				pixels.erase(std::remove_if(pixels.begin(),pixels.end(),[&](auto pix)
					{
						if(path_counts(pix.x,pix.y)>=min_horiz_protection)
						{
							path_counts(pix.x,pix.y)=safe;
							return true;
						}
						else
						{
							path_counts(pix.x,pix.y)=unsafe;
							return false;
						}
					}),pixels.end());
			}
			//std::cout<<"Horizontal protection\n";
			//path_counts.display();
			//if(false)
			{ // protected small clusters
				auto protect_rect=[&path_counts](auto const rect)
				{
					auto row=&path_counts(rect.left,rect.top);
					auto const end=row+std::size_t(rect.height())*path_counts._width;
					auto const width=rect.width()*sizeof(count_t);
					for(;row<end;row+=path_counts._width)
					{
						std::memset(row,safe,width);
					}
				};
				auto protect_cluster=[&,left_threshold=it_kill->bounding_box().left](auto const& cluster)
				{
					for(auto const rect:cluster.get_ranges())
					{
						protect_rect(rect);
					}
				};
				for(auto it=clusters.begin();it!=it_kill;++it)
				{
					protect_cluster(*it);
				}
				for(auto it=std::next(it_kill);it!=clusters.end();++it)
				{
					protect_cluster(*it);
				}
			}
			//std::cout<<"Small protection\n";
			//path_counts.display();
			{ //marking kill sufficiently large vertical paths
				std::sort(pixels.begin(),pixels.end(),[](auto a,auto b)
					{
						return std::make_pair(a.y,a.x)<std::make_pair(b.y,b.x);
					});
				for(auto const pix_loc:pixels)
				{
					path_counts(pix_loc.x,pix_loc.y)=exlib::multi_max(
						path_counts(pix_loc.x-1,pix_loc.y-1),
						path_counts(pix_loc.x,pix_loc.y-1),
						path_counts(pix_loc.x+1,pix_loc.y-1)
					)+1;
				}
				for(auto it=pixels.rbegin();it!=pixels.rend();++it)
				{
					auto const pix_loc=*it;
					path_counts(pix_loc.x,pix_loc.y)=exlib::multi_max(
						path_counts(pix_loc.x-1,pix_loc.y+1),
						path_counts(pix_loc.x,pix_loc.y+1),
						path_counts(pix_loc.x+1,pix_loc.y+1),
						path_counts(pix_loc.x,pix_loc.y)
					);
				}
				//std::cout<<"Vertical paths\n";
				//path_counts.display();
				for(auto const pix_loc:pixels)
				{
					auto& pix=path_counts(pix_loc.x,pix_loc.y);
					// need to trace back
					if(pix<max_vert_protection)
					{
						pix=safe;
					}
					else
					{
						pix=unsafe;
					}
				}
			}
			//std::cout<<"Vertical protection\n";
			//path_counts.display();
			cil::CImg<char> safe_points_raw(path_counts._width,path_counts._height);
			{
				// now -1 is safe, 0 is unsafe, mark pixels safe due to min_width; they can only descend from the bottom of clusters
				auto const tail_space=min_vert_space/2;
				auto const head_space=min_vert_space-tail_space;
				std::memset(path_counts.data(),-1,size_t{head_space}*path_counts._width*sizeof(count_t));
				{
					auto const tail_space_d=size_t{tail_space}*path_counts._width;
					std::memset(path_counts.end()-tail_space_d,-1,tail_space_d*sizeof(count_t));
				}
				auto const statuses=path_counts_raw.data();
				for(unsigned int y=0;y<path_counts._height;++y)
				{
					exlib::get_fatten(&path_counts(0,y),&path_counts(0,y+1),min_horiz_space-min_horiz_space/2,&safe_points_raw(0,y));
				}
				{
					//decltype(safe_points_raw) layer0(safe_points_raw,true);
					//layer0._spectrum=1;
					//layer0.display();
				}
				safe_points_raw.rotate(-90); // for cache coherency
				safe_points.resize(path_counts._height,path_counts._width);
				for(unsigned int x=0;x<path_counts._width;++x)
				{
					auto const begin=&safe_points_raw(0,x);
					exlib::get_fatten(begin,begin+path_counts._height,head_space,&safe_points(0,x));
				}
			}
		}
		//std::cout<<"Min space protection\n";
		//safe_points.display();
		{
			// hug left path tracer
			auto const width=count_t(safe_points._width);
			auto const height=count_t(safe_points._height);
			auto const last_row=height-1;
			//std::vector<count_t> path(safe_points._height);
			std::unique_ptr<count_t[]> path(new count_t[safe_points._height]);
			auto trace_path_down=[&safe_points,&path,width,height,last_row](count_t x)
			{
				{
					auto& start=safe_points(x,0);
					if(start!=unsafe)
					{
						return;
					}
					start=candidate_path_mark;
				}
				path[0]=x;
				decltype(x) y=0;
				while(true)
				{
					count_t furthest_left=x;
					auto const current_row=&safe_points(0,y);
					auto const next_row=current_row+safe_points._width;
					for(;;)
					{
						auto const cand=furthest_left-1;
						if(current_row[cand]!=unsafe)
						{
							break;
						}
						furthest_left=cand;
					}
					{
						bool found_in_left=false;
						for(;furthest_left<=x;++furthest_left)
						{
							auto const pix=next_row[furthest_left];
							if(pix==unsafe)
							{
								found_in_left=true;
								break;
							}
						}
						if(!found_in_left)
						{
							for(;;++furthest_left)
							{
								auto const level_pix=current_row[furthest_left];
								if(level_pix!=unsafe)
								{
									std::fill(current_row+x+1,current_row+furthest_left,candidate_path_mark);
									return;
								}
								auto const next_pix=next_row[furthest_left];
								if(next_pix==unsafe)
								{
									break;
								}
							}
						}
					}
					for(;;)
					{
						auto const cand=furthest_left-1;
						if(next_row[cand]!=unsafe)
						{
							break;
						}
						furthest_left=cand;
					}
					++y;
					next_row[furthest_left]=candidate_path_mark;
					path[y]=furthest_left;
					if(y==last_row)
					{
						for(count_t r=0;r<safe_points._height;++r)
						{
							safe_points(path[r],r)=real_path_mark;
						}
						break;
					}
					x=furthest_left;
				}
			};
			for(count_t x_top=0;x_top<width;++x_top)
			{
				trace_path_down(x_top);
				if(x_top%100==0)
				{
					//safe_points.display();
				}
			}
			//safe_points.display();
		}
		img.rotate(-90);
		//img.display();
		unsigned int new_width=0;
		for(unsigned int y=0;y<img._height;++y)
		{
			auto* img_row=&img(0,y);
			auto const* safe_row=&safe_points(0,y);
			unsigned int write_head=0;
			auto const width=img._width;
			for(unsigned int read_head=0;read_head<width;++read_head)
			{
				if(safe_row[read_head]!=real_path_mark)
				{
					img_row[write_head]=img_row[read_head];
					++write_head;
				}
			}
			new_width=std::max(new_width,write_head);
		}
		img.crop(0,new_width-1);
		//img.display();
		img.rotate(90);
		//img.display();
		return true;
	}

	bool remove_empty_lines(cil::CImg<unsigned char>& img,unsigned char background_threshold,unsigned int min_space,unsigned int max_presence)
	{
		if(img._height==0)
		{
			return false;
		}
		switch(img._spectrum)
		{
		case 1:
		case 2:
		{
			return remove_empty_lines_help(img,min_space,max_presence/*,[background_threshold](unsigned char const* pixel)
				{
					return *pixel<background_threshold;
				} */);
		}
		case 3:
		case 4:
		{
			auto s=std::size_t{img._height}*img._width;
			return remove_empty_lines_help(img,min_space,max_presence/*,[t=3*background_threshold,s,s2=2*s](unsigned char const* pixel)
				{
					return pixel[0]+pixel[s]+pixel[s2]<t;
				}*/);
		}
		default:
			throw std::invalid_argument("Invalid spectrum");
		}
	}


	template<typename CountType>
	unsigned int sliding_template_match_erase_exact_help(cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,float threshold,std::array<unsigned char,4> color)
	{
		if(img._width<tmplt._width||img._height<tmplt._height)
		{
			throw std::invalid_argument("Template smaller than image");
		}
		cil::CImg<CountType> counts(img._width,img._height);
		CountType converted_threshold=tmplt._height*tmplt._width*threshold;
		for(unsigned int y=0;y<counts._height;++y)
		{
			auto const y_max=std::min(img._height,y+tmplt._height)-y;
			for(unsigned int x=0;x<counts._width;++x)
			{
				auto const x_max=std::min(img._width,x+tmplt._width)-x;
				auto& pixel=(counts(x,y)=0);
				for(unsigned int y1=0;y1<y_max;++y1)
				{
					for(unsigned int x1=0;x1<x_max;++x1)
					{
						pixel+=img(x+x1,y+y1)==tmplt(x1,y1);
					}
				}
			}
		}
		auto const y_last=counts._height-1;
		auto const x_last=counts._width-1;
		unsigned int erase_count=0;
		for(unsigned int y=1;y<y_last;++y)
		{
			for(unsigned int x=1;x<x_last;++x)
			{
				auto const count=counts(x,y);
				if(count>=converted_threshold&&
					count>=counts(x-1,y)&&
					count>=counts(x+1,y)&&
					count>=counts(x,y-1)&&
					count>=counts(x,y+1))
				{
					++erase_count;
					ImageUtils::RectangleUINT to_erase{x,std::min(counts._width,x+tmplt._width),y,std::min(counts._height,y+tmplt._height)};
					fill_selection(img,to_erase,color);
				}
			}
		}
		return erase_count;
	}

	unsigned int sliding_template_match_erase_exact(cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,float threshold,std::array<unsigned char,4> color)
	{
		if(img._width<tmplt._width||img._height<tmplt._height)
		{
			throw std::invalid_argument("Template smaller than image");
		}
		if(tmplt._width*tmplt._height<=std::numeric_limits<unsigned short>::max())
		{
			return sliding_template_match_erase_exact_help<unsigned short>(img,tmplt,threshold,color);
		}
		else
		{
			return sliding_template_match_erase_exact_help<unsigned int>(img,tmplt,threshold,color);
		}
	}

	unsigned int cluster_template_match_erase(cil::CImg<unsigned char>& img,std::vector<ScoreProcessor::Cluster> const& clusters,cil::CImg<unsigned char> const& tmplt,float match_threshold)
	{
		auto const total_size=float(tmplt._width)*tmplt._height;
		unsigned int count=0;
		auto const inverted_threshold=(1-match_threshold)*total_size;
		for(auto& cluster:clusters)
		{
			auto const bb=cluster.bounding_box();
			auto const ltx=bb.left;
			auto const lty=bb.top;
			//std::cout<<ltx<<' '<<lty<<'\n';
			float match=0;
			auto const y_max=std::min(img._height,lty+tmplt._height);
			auto const x_max=std::min(img._width,ltx+tmplt._width);
			auto const y_end=y_max-lty;
			auto const x_end=x_max-ltx;
			for(unsigned int y=0;y<y_end;++y)
			{
				for(unsigned int x=0;x<x_end;++x)
				{
					//std::cout<<int(img(ltx+x,lty+y))<<' '<<int(tmplt(x,y))<<'\n';
					match+=ImageUtils::Grayscale::color_diff(&img(ltx+x,lty+y),&tmplt(x,y));
				}
			}
			//std::cout<<match<<'\n';
			if(match<inverted_threshold)
			{
				for(auto& erase:clusters)
				{
					bool contains=false;
					for(auto& rect:erase.get_ranges())
					{
						if(rect.intersects({ltx,ltx+tmplt._width,lty,lty+tmplt._height}))
						{
							contains=true;
							++count;
							break;
						}
					}
					if(contains)
					{
						unsigned char white_buffer[]={255,255,255,255};
						for(auto& rect:erase.get_ranges())
						{
							fill_selection(img,rect,white_buffer);
						}
					}
				}
			}
		}
		return count;
	}

	struct guarded_pool {
		exlib::thread_pool pool;
		std::mutex lock;
		explicit guarded_pool(unsigned int nt):pool{std::max(1U,nt)}
		{}
	};

	auto& init_exclusive_pool(unsigned int num_threads)
	{
		static guarded_pool pool{std::max(1U,num_threads)};
		return pool;
	}

	ExclusiveThreadPool::ExclusiveThreadPool(unsigned int num_threads)
	{
		init_exclusive_pool(num_threads).lock.lock();
	}

	exlib::thread_pool& ExclusiveThreadPool::pool() const
	{
		return init_exclusive_pool(0).pool;
	}

	ExclusiveThreadPool::~ExclusiveThreadPool()
	{
		init_exclusive_pool(0).lock.unlock();
	}

	void ExclusiveThreadPool::set_thread_count(unsigned int nt)
	{
		assert(nt!=0);
		init_exclusive_pool(0).pool.num_threads(nt);
	}

	void binarize(CImg<unsigned char>& image,ColorRGB const middleColor,ColorRGB const lowColor,ColorRGB const highColor)
	{
		assert(image._spectrum==3);
		unsigned char midBrightness=middleColor.brightness();
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(brightness({image(x,y,0),image(x,y,1),image(x,y,2)})>midBrightness)
				{
					image(x,y,0)=highColor.r;
					image(x,y,1)=highColor.g;
					image(x,y,2)=highColor.b;
				}
				else
				{
					image(x,y,0)=lowColor.r;
					image(x,y,1)=lowColor.g;
					image(x,y,2)=lowColor.b;
				}
			}
		}
	}
	void binarize(CImg<unsigned char>& image,Grayscale const middleGray,Grayscale const lowGray,Grayscale const highGray)
	{
		assert(image._spectrum==1);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(image(x,y)>middleGray)
				{
					image(x,y)=highGray;
				}
				else
				{
					image(x,y)=lowGray;
				}
			}
		}
	}
	void binarize(CImg<unsigned char>& image,Grayscale const middleGray,float scale)
	{
		assert(image._spectrum==1);
		for(unsigned int x=0;x<image._width;++x)
		{
			for(unsigned int y=0;y<image._height;++y)
			{
				if(image(x,y)>middleGray)
				{
					image(x,y)=255-(255-image(x,y))/scale;
				}
				else
				{
					image(x,y)/=scale;
				}
			}
		}
	}
	bool replace_range(CImg<unsigned char>& image,Grayscale const lower,ImageUtils::Grayscale const upper,Grayscale const replacer)
	{
		bool edited=false;
		unsigned char* const limit=image.end();
		for(auto it=image.begin();it!=limit;++it)
		{
			if(*it>=lower&&*it<=upper)
			{
				if(*it!=replacer)
				{
					edited=true;
					*it=replacer;
				}
			}
		}
		return edited;
	}

	bool replace_by_brightness(CImg<unsigned char>& image,unsigned char lowerBrightness,unsigned char upperBrightness,ColorRGB replacer)
	{
		assert(image._spectrum>=3);
		bool edited=false;
		auto repl=std::array<unsigned char,3>({replacer.r,replacer.g,replacer.b});
		map_if<3U>(image,[=](auto color)
			{
				return repl;;
			},[=,&edited](auto color)
			{
				if(color==repl)
				{
					return false;
				}
				auto const brightness=(float(color[0])+color[1]+color[2])/3.0f;
				if(brightness>=lowerBrightness&&brightness<=upperBrightness)
				{
					edited=true;
					return true;
				}
				return false;
			});
		return edited;
	}
	bool replace_by_hsv(::cimg_library::CImg<unsigned char>& image,ImageUtils::ColorHSV start,ImageUtils::ColorHSV end,ImageUtils::ColorRGB replacer)
	{
		assert(image._spectrum>=3);
		bool edited=false;
		std::array<unsigned char,3> const& repl=*reinterpret_cast<std::array<unsigned char,3>*>(&replacer);
		map_if<3U>(image,[=](auto color)
			{
				return repl;
			},[=,&edited](auto color)
			{
				if(color==repl)
				{
					return false;
				}
				ColorHSV hsv=*reinterpret_cast<ColorRGB*>(&color);
				if(hsv.s>=start.s&&hsv.s<=end.s&&hsv.v>=start.v&&hsv.v<=end.v)
				{
					if(start.h<end.h)
					{
						if(hsv.h>=start.h&&hsv.h<=end.h)
						{
							edited=true;
							return true;
						}
					}
					else if(hsv.h>=start.h||hsv.h<=end.h)
					{
						edited=true;
						return true;
					}
				}
				return false;
			});
		return edited;
	}
	bool replace_by_rgb(::cil::CImg<unsigned char>& image,ImageUtils::ColorRGB start,ImageUtils::ColorRGB end,ImageUtils::ColorRGB replacer)
	{
		assert(image._spectrum>=3);
		bool edited=false;
		std::array<unsigned char,3> const& repl=*reinterpret_cast<std::array<unsigned char,3>*>(&replacer);
		map_if<3U>(image,[=](auto color)
			{
				return repl;
			},[=,&edited](auto color)
			{
				if(color==repl)
				{
					return false;
				}
				if(color[0]>=start.r&&color[0]<=end.r&&
					color[1]>=start.g&&color[1]<=end.g&&
					color[2]>=start.b&&color[2]<=end.b)
				{
					edited=true;
					return true;
				}
				return false;
			});
		return edited;
	}
	bool auto_center_horiz(CImg<unsigned char>& image)
	{
		bool isRgb=image._spectrum>=3;
		unsigned int left,right;
		unsigned int tolerance=image._height>>4;
		if(isRgb)
		{
			left=find_left(image,ColorRGB::WHITE,tolerance);
			right=find_right(image,ColorRGB::WHITE,tolerance);
		}
		else
		{
			left=find_left(image,Grayscale::WHITE,tolerance);
			right=find_right(image,Grayscale::WHITE,tolerance);
		}

		unsigned int center=image._width/2;
		int shift=static_cast<int>(center-((left+right)/2));
		if(0==shift)
		{
			return false;
		}
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0)
		{
			toFill={0,static_cast<unsigned int>(shift),0,image._height};
		}
		else if(shift<0)
		{
			toFill={static_cast<unsigned int>(image._width+shift),image._width,0,image._height};
		}
		copy_shift_selection(image,toShift,shift,0);
		if(isRgb)
		{
			fill_selection(image,toFill,ColorRGB::WHITE);
		}
		else
		{
			fill_selection(image,toFill,Grayscale::WHITE);
		}
		return false;
	}
	unsigned int find_left(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=0;x<image._width;++x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return image._width-1;
	}
	unsigned int find_left(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=0;x<image._width;++x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				if(gray_diff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return image._width-1;
	}
	unsigned int find_right(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=image._width-1;x<image._width;--x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return 0;
	}
	unsigned int find_right(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int x=image._width-1;x<image._width;--x)
		{
			//num=0;
			for(unsigned int y=0;y<image._height;++y)
			{
				if(gray_diff(image(x,y),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return x;
			}
		}
		return 0;
	}

	bool auto_center_vert(CImg<unsigned char>& image)
	{
		bool isRgb=image._spectrum>=3;
		int top,bottom;
		unsigned int tolerance=image._height>>4;
		if(isRgb)
		{
			top=find_top(image,ColorRGB::WHITE,tolerance);
			bottom=find_bottom(image,ColorRGB::WHITE,tolerance);
		}
		else
		{
			top=find_top(image,Grayscale::WHITE,tolerance);
			bottom=find_bottom(image,Grayscale::WHITE,tolerance);
		}
		unsigned int center=image._height/2;
		int shift=static_cast<int>(center-((top+bottom)/2));
		if(0==shift)
		{
			return false;
		}
		RectangleUINT toShift,toFill;
		toShift={0,image._width,0,image._height};
		if(shift>0)
		{
			toFill={0,image._width,0,static_cast<unsigned int>(shift)};
		}
		else if(shift<0)
		{
			toFill={0,image._width,static_cast<unsigned int>(image._height+shift),image._height};
		}
		copy_shift_selection(image,toShift,0,shift);
		if(isRgb)
		{
			fill_selection(image,toFill,ColorRGB::WHITE);
		}
		else
		{
			fill_selection(image,toFill,Grayscale::WHITE);
		}
		return true;
	}
	unsigned int find_top(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int y=0;y<image._height;++y)
		{
			//num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return image._height-1;
	}
	unsigned int find_top(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		auto const data=image.data();
		auto const width=image._width;
		auto const height=image._height;
		for(unsigned int y=0;y<height;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				if(gray_diff(*(row+x),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return image._height-1;
	}
	unsigned int find_bottom(CImg<unsigned char> const& image,ColorRGB const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		for(unsigned int y=image._height-1;y<image._height;--y)
		{
			//num=0;
			for(unsigned int x=0;x<image._width;++x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					++num;
				}
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return 0;
	}
	unsigned int find_bottom(CImg<unsigned char> const& image,Grayscale const background,unsigned int const tolerance)
	{
		unsigned int num=0;
		auto const data=image.data();
		auto const width=image._width;
		auto const height=image._height;
		for(unsigned int y=height-1;y<height;--y)
		{
			auto const row=data+width*y;
			for(unsigned int x=0;x<image._width;++x)
			{
				if(gray_diff(*(row+x),background)>.5f)
					++num;
			}
			if(num>tolerance)
			{
				return y;
			}
		}
		return 0;
	}
	std::vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_left_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=0;x<limit;++x)
			{
				if(gray_diff(image(x,y),background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		container.resize(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;--x)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(ColorRGB::color_diff(
					rcast<unsigned char const*>(&pixel),
					rcast<unsigned char const*>(&background))>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_right_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		unsigned int limit=image._width/2;
		std::vector<unsigned int> container(image._height);
		container.resize(image._height);
		for(unsigned int y=0;y<image._height;++y)
		{
			container[y]=limit;
			for(unsigned int x=image._width-1;x>=limit;--x)
			{
				if(gray_diff(image(x,y),background)>0.5f)
				{
					container[y]=x;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3||image._spectrum==4);
		std::vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		unsigned int color=static_cast<unsigned int>(background.r)+background.g+background.b;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(static_cast<unsigned int>(pixel.r)+pixel.g+pixel.b<=color)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_top_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		std::vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=0;y<limit;++y)
			{
				if(image(x,y)<=background)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,ColorRGB const background)
	{
		assert(image._spectrum==3||image._spectrum==4);
		std::vector<unsigned int> container(image._width);
		unsigned int color=static_cast<unsigned int>(background.r)+background.g+background.b;
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>=limit;--y)
			{
				ColorRGB pixel={image(x,y,0),image(x,y,1),image(x,y,2)};
				if(static_cast<unsigned int>(pixel.r)+pixel.g+pixel.b<=color)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}
	std::vector<unsigned int> build_bottom_profile(CImg<unsigned char> const& image,Grayscale const background)
	{
		assert(image._spectrum==1);
		std::vector<unsigned int> container(image._width);
		unsigned int limit=image._height/2;
		for(unsigned int x=0;x<image._width;++x)
		{
			container[x]=limit;
			for(unsigned int y=image._height-1;y>limit;--y)
			{
				if(image(x,y)<background)
				{
					container[x]=y;
					break;
				}
			}
		}
		return container;
	}

	::cimg_library::CImg<float> create_vertical_energy(::cimg_library::CImg<unsigned char> const& refImage,float const vec,unsigned int min_vertical_space,unsigned char background);

	::cimg_library::CImg<float> create_compress_energy(::cimg_library::CImg<unsigned char> const& refImage,unsigned int const min_padding)
	{
		throw std::runtime_error("Not implemented");
	}
	CImg<unsigned char> hpixelator(CImg<unsigned char> const& img)
	{
		throw std::runtime_error("Not implemented");
	}

	//basically a flood fill that can't go left, assumes top row is completely clear
	std::vector<unique_ptr<RectangleUINT>> select_outside(CImg<unsigned char> const& image)
	{
		assert(image._spectrum==1);
		std::vector<unique_ptr<RectangleUINT>> resultContainer;
		CImg<bool> safePoints(image._width,image._height,1,1);
		safePoints.fill(false);
		for(unsigned int x=0;x<image._width;++x)
		{
			safePoints(x,0)=true;
		}
		struct scanRange {
			unsigned int left,right,y;
			int direction;
		};
		stack<scanRange> scanRanges;
		scanRanges.push({0U,image._width,0U,1});
		unsigned int xL;
		//unsigned int xR;
		scanRange r;
		unsigned int sleft;
		unsigned int sright;
		float const tolerance=0.2f;
		while(!scanRanges.empty())
		{
			r=scanRanges.top();
			scanRanges.pop();
			sleft=r.left;
			safePoints(sleft,r.y)=true;
			//scan right
			for(sright=r.right;sright<image._width&&!safePoints(sright,r.y)&&gray_diff(Grayscale::WHITE,image(sright,r.y))<=tolerance;++sright)
			{
				safePoints(sright,r.y)=true;
			}
			resultContainer.push_back(make_unique<RectangleUINT>(RectangleUINT{sleft,sright,r.y,r.y+1U}));

			//scan in same direction vertically
			bool rangeFound=false;
			unsigned int rangeStart=0;
			unsigned int newy=r.y+r.direction;
			if(newy<image._height)
			{
				xL=sleft;
				while(xL<sright)
				{
					for(;xL<sright;++xL)
					{
						if(!safePoints(xL,newy)&&gray_diff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||gray_diff(Grayscale::WHITE,image(xL,newy))>tolerance)
						{
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound)
					{
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,r.direction});
					}
				}
			}

			//scan opposite direction vertically
			newy=r.y-r.direction;
			if(newy<image._height)
			{
				xL=sleft;
				while(xL<r.left)
				{
					for(;xL<r.left;++xL)
					{
						if(!safePoints(xL,newy)&&gray_diff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<r.left;++xL)
					{
						if(safePoints(xL,newy)||gray_diff(Grayscale::WHITE,image(xL,newy))>tolerance)
						{
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound)
					{
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
				xL=r.right+1;
				while(xL<sright)
				{
					for(;xL<sright;++xL)
					{
						if(!safePoints(xL,newy)&&gray_diff(Grayscale::WHITE,image(xL,newy))<=tolerance)
						{
							safePoints(xL,newy)=true;
							rangeFound=true;
							rangeStart=xL++;
							break;
						}
					}
					for(;xL<sright;++xL)
					{
						if(safePoints(xL,newy)||gray_diff(Grayscale::WHITE,image(xL,newy))>tolerance)
						{
							break;
						}
						safePoints(xL,newy)=true;
					}
					if(rangeFound)
					{
						rangeFound=false;
						scanRanges.push({rangeStart,xL,newy,-r.direction});
					}
				}
			}
		}
		return resultContainer;
	}

	void add_horizontal_energy(CImg<unsigned char> const& ref,CImg<float>& map,float const hec,unsigned char bg)
	{
		for(unsigned int y=0;y<map._height;++y)
		{
			unsigned int x=0;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&,bg]()
			{
				return node_found=ref(x,y)>bg;
			};
			auto place_values=[&]()
			{
				unsigned int mid=(node_start+x)/2;
				float val_div=2.0f;
				unsigned int node_x;
				for(node_x=node_start;node_x<mid;++node_x)
				{
					++val_div;
					map(node_x,y)+=hec/(val_div*val_div*val_div);
				}
				for(;node_x<x;++node_x)
				{
					--val_div;
					map(node_x,y)+=hec/(val_div*val_div*val_div);
				}
			};
			if(assign_node_found())
			{
				node_start=0;
			}
			for(x=1;x<ref._width;++x)
			{
				if(node_found)
				{
					if(!assign_node_found())
					{
						place_values();
					}
				}
				else
				{
					if(assign_node_found())
					{
						node_start=x;
					}
				}
			}
			if(node_found)
			{
				place_values();
			}
		}
	}
	unsigned int cut_page(CImg<unsigned char> const& image,char const* filename,cut_heuristics const& ch,int quality)
	{
		auto const support=validate_path(filename);
		/*
		bool isRGB;
		switch(image._spectrum)
		{
			case 1:
				isRGB=false;
				break;
			case 3:
				isRGB=true;
				//return 0;
				break;
			default:
				return 0;
		}
		*/
		std::vector<std::vector<unsigned int>> paths;
		{
			struct line {
				unsigned int top,bottom,right;
			};
			float const VEC=100.0f;
			CImg<float> map=create_vertical_energy(image,VEC,ch.minimum_vertical_space,ch.background);
			if(ch.horizontal_energy_weight!=0)
				add_horizontal_energy(image,map,ch.horizontal_energy_weight*VEC,ch.background);
			min_energy_to_right(map);
#ifndef NDEBUG
			map.display();
#endif

			auto selector=[](std::array<float,1> color)
			{
				return color[0]==INFINITY;
			};
			std::vector<line> boxes;
			{
				auto const selections=global_select<1U>(map,selector);
				auto const clusters=Cluster::cluster_ranges(selections);
				auto heuristic_filter=
					[=]
				(auto box)
				{
					return (box.width()>=ch.min_width&&box.height()>=ch.min_height);
				};
				for(auto c=clusters.cbegin();c!=clusters.cend();++c)
				{
					//if(clusters[i]->size()>threshold)
					auto box=c->bounding_box();
					if(heuristic_filter(box))
					{
						boxes.push_back(line{box.top,box.bottom,box.right});
					}
				}
			}
			std::sort(boxes.begin(),boxes.end(),[](line a,line b)
				{
					return a.top<b.top;
				});
			unsigned int last_row=map._width-1;
			for(size_t i=1;i<boxes.size();++i)
			{
				auto const& current=boxes[i-1];
				auto const& next=boxes[i];
				paths.emplace_back(trace_seam(map,(current.bottom+next.top)/2,(current.right+next.right)/2-1));
			}
		}
		if(paths.size()==0)
		{
			cil::save_image(image,filename,support,quality);
			return 1;
		}
		/*std::sort(paths.begin(),paths.end(),[](auto const& a,auto const& b)
		{
			assert(a.size()==b.size());
			for(size_t i=a.size();i-->0;)
			{
				if(a[i]<b[i])
				{
					return true;
				}
				if(a[i]>b[i])
				{
					return false;
				}
			}
			throw std::runtime_error("Duplicate paths found (a bug), aborting");
		});*/
		unsigned int num_images=0;
		unsigned int bottom_of_old=0;
		for(unsigned int path_num=0;path_num<paths.size();++path_num)
		{
			auto const& path=paths[path_num];
			unsigned int lowest_in_path=path[0],highest_in_path=path[0];
			for(unsigned int x=1;x<path.size();++x)
			{
				unsigned int node_y=path[x];
				if(node_y>lowest_in_path)
				{
					lowest_in_path=node_y;
				}
				else if(node_y<highest_in_path)
				{
					highest_in_path=node_y;
				}
			}
			unsigned int height;
			height=lowest_in_path-bottom_of_old;
			assert(height<image._height);
			CImg<unsigned char> new_image(image._width,height);
			if(path_num==0)
			{
				for(unsigned int x=0;x<new_image._width;++x)
				{
					unsigned int y=0;
					unsigned int y_stage2=paths[path_num][x];
					for(;y<y_stage2;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=image(x,y);
					}
					for(;y<new_image._height;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=Grayscale::WHITE;
					}
				}
			}
			else
			{
				for(unsigned int x=0;x<new_image._width;++x)
				{
					unsigned int y=0;
					unsigned int y_stage1=paths[path_num-1][x]-bottom_of_old;
					unsigned int y_stage2=paths[path_num][x]-bottom_of_old;
					for(;y<y_stage1;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=Grayscale::WHITE;
					}
					for(;y<y_stage2;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=image(x,y+bottom_of_old);
					}
					for(;y<new_image._height;++y)
					{
						if(y<new_image._height)
							new_image(x,y)=Grayscale::WHITE;
					}
				}
			}
			auto const save_name=cil::number_filename(filename,++num_images,3U);
			cil::save_image(new_image,save_name.c_str(),support,quality);
			bottom_of_old=highest_in_path;
		}
		CImg<unsigned char> new_image(image._width,image._height-bottom_of_old);
		unsigned int y_max=image._height-bottom_of_old;
		for(unsigned int x=0;x<new_image._width;++x)
		{
			unsigned int y=0;
			unsigned int y_stage1=paths.back()[x]-bottom_of_old;
			for(;y<y_stage1;++y)
			{
				new_image(x,y)=Grayscale::WHITE;
			}
			for(;y<new_image._height;++y)
			{
				new_image(x,y)=image(x,y+bottom_of_old);
			}
		}
		auto const save_name=cil::number_filename(filename,++num_images,3U);
		cil::save_image(new_image,save_name.c_str(),support,quality);
		return num_images;
	}

	float auto_rotate(CImg<unsigned char>& image,double pixel_prec,double min_angle,double max_angle,double angle_prec,unsigned char boundary)
	{
		assert(angle_prec>0);
		assert(pixel_prec>0);
		assert(min_angle<max_angle);
		return RAD_DEG*auto_rotate_bare(image,pixel_prec,min_angle*DEG_RAD+M_PI_2,max_angle*DEG_RAD+M_PI_2,(max_angle-min_angle)/angle_prec+1,boundary);
	}

	float find_angle_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary)
	{
		assert(angle_steps>0);
		assert(pixel_prec>0);
		assert(min_angle<max_angle);
		if(img._height==0||img._width==0)
		{
			return 0;
		}
		if(img._spectrum<3)
		{
			auto selector=[&img,boundary](unsigned int x,unsigned int y)
			{
				auto const top=img(x,y);
				auto const bottom=img(x,y+1);
				return
					//(top<=boundary&&bottom>boundary)||
					(top>boundary&& bottom<=boundary);
			};
			HoughArray<unsigned short> ha(selector,img._width,img._height-1,min_angle,max_angle,angle_steps,pixel_prec);
			return M_PI_2-ha.angle();
		}
		else
		{
			auto selector=[&img,boundary=3U*boundary,size=size_t(img._width)*img._height](unsigned int x,unsigned int y)
			{
				auto const ptop=&img(x,y);
				auto const pbottom=&img(x,y+1);
				auto const top=unsigned int(*ptop)+*(ptop+size)+*(ptop+2*size);
				auto const bottom=unsigned int(*pbottom)+*(pbottom+size)+*(pbottom+2*size);
				return
					//(top<=boundary&&bottom>boundary)||
					(top>boundary&& bottom<=boundary);
			};
			HoughArray<unsigned short> ha(selector,img._width,img._height-1,min_angle,max_angle,angle_steps,pixel_prec);
			return M_PI_2-ha.angle();
		}
	}

	float auto_rotate_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary)
	{
		auto angle=find_angle_bare(img,pixel_prec,min_angle,max_angle,angle_steps,boundary);
		img.rotate(angle*RAD_DEG,2,1);
		return angle;
	}

	bool auto_skew(CImg<unsigned char>& image)
	{
		return false;
	}
	line<unsigned int> find_top_line(CImg<unsigned char> const& image)
	{
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_bottom_line(CImg<unsigned char> const& image)
	{
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_left_line(CImg<unsigned char> const& image)
	{
		return {{0,0},{0,0}};
	}
	line<unsigned int> find_right_line(CImg<unsigned char> const& image)
	{
		std::vector<unsigned int> rightProfile;
		switch(image._spectrum)
		{
		case 1:
			rightProfile=build_right_profile(image,Grayscale::WHITE);
			break;
		case 3:
			rightProfile=build_right_profile(image,ColorRGB::WHITE);
			break;
		default:
			return {{0,0},{0,0}};
		}
		struct LineCandidate {
			PointUINT start;
			PointUINT last;
			unsigned int numPoints;
			int error;//dx/dy
			float derror_dy;
		};
		std::vector<LineCandidate> candidates;
		unsigned int const limit=image._width/2+1;
		unsigned int lastIndex=0;
		for(unsigned int pi=0;pi<rightProfile.size();++pi)
		{
			if(rightProfile[pi]<=limit)
			{
				continue;
			}
			PointUINT test{rightProfile[pi],pi};
			unsigned int ci;
			for(ci=lastIndex;ci<candidates.size();++ci)
			{
#define determineLine() \
				int error=static_cast<signed int>(test.x-candidates[ci].start.x);\
				float derror_dy=static_cast<float>(error-candidates[ci].error)/(test.y-candidates[ci].last.y);\
				if(abs_dif(derror_dy,candidates[ci].derror_dy)<1.1f) {\
					candidates[ci].error=error;\
					candidates[ci].derror_dy=derror_dy;\
					candidates[ci].last=test;\
					++candidates[ci].numPoints;\
					lastIndex=ci;\
					goto skipCreatingNewCandidate;\
				}
				determineLine();
			}
			for(ci=0;ci<lastIndex;++ci)
			{
				determineLine();
			}
#undef determineLine
			candidates.push_back({test,test,1,0,0.0f});
			lastIndex=candidates.size()-1;
		skipCreatingNewCandidate:;
		}
		if(candidates.size()==0)
			return {{0,0},{0,0}};
		unsigned int maxIndex=0;
		unsigned int maxPoints=candidates[maxIndex].numPoints;
		for(unsigned int ci=0;ci<candidates.size();++ci)
		{
			if(candidates[ci].numPoints>maxPoints)
			{
				maxIndex=ci;
				maxPoints=candidates[ci].numPoints;
			}
		}
		//cout<<candidates[maxIndex].numPoints<<'\n';
		return {candidates[maxIndex].start,candidates[maxIndex].last};
	}
	bool undistort(CImg<unsigned char>& image)
	{}

	std::vector<ImageUtils::Rectangle<unsigned int>> flood_select(CImg<unsigned char> const& image,float const tolerance,Grayscale const color,Point<unsigned int> start)
	{
		std::vector<bool> checked(image._width*image._height,false);
		return flood_select(image,tolerance,color,start,checked);
	}
	std::vector<ImageUtils::Rectangle<unsigned int>> flood_select(CImg<unsigned char> const& image,float const tolerance,Grayscale const color,Point<unsigned int> start,std::vector<bool>& checked)
	{
		assert(image._spectrum==1);
		std::vector<ImageUtils::Rectangle<unsigned int>> result_container;
		flood_operation(image,start,[&](PointUINT point)
			{
				auto const coord=point.y*image._width+point.x;
				std::vector<bool>::reference ref=checked[coord];
				if(ref)
				{
					return false;
				}
				ref=true;
				return gray_diff(image(point.x,point.y),color)<tolerance;
			},[&](horizontal_line<> line)
			{
				result_container.push_back({line.left,line.right,line.y,line.y+1});
			});
		return result_container;
	}
	bool auto_padding(CImg<unsigned char>& image,unsigned int const vertical_padding,unsigned int const horizontal_padding_max,unsigned int const horizontal_padding_min,signed int horiz_offset,float optimal_ratio,unsigned int tolerance,unsigned char background)
	{
		unsigned int left,right,top,bottom;
		switch(image._spectrum)
		{
		case 1:
		case 2:
		{
			auto selector=[=](auto color)
			{
				return color[0]<background;
			};
			left=find_left<3>(image,tolerance,selector);
			right=find_right<3>(image,tolerance,selector)+1;
			top=find_top<3>(image,tolerance,selector);
			bottom=find_bottom<3>(image,tolerance,selector)+1;
			break;
		}
		default:
		{
			auto selector=[bg=3*float(background)](auto color)
			{
				return float(color[0])+color[1]+color[2]<bg;
			};
			left=find_left<3>(image,tolerance,selector);
			right=find_right<3>(image,tolerance,selector)+1;
			top=find_top<3>(image,tolerance,selector);
			bottom=find_bottom<3>(image,tolerance,selector)+1;
			break;
		}
		}
		if(left>right) return false;
		if(top>bottom) return false;

		unsigned int true_height=bottom-top;
		unsigned int optimal_width=optimal_ratio*(true_height+2*vertical_padding);
		unsigned int horizontal_padding;
		unsigned int actual_width=right-left;
		if(optimal_width>actual_width)
		{
			horizontal_padding=(optimal_width-actual_width)/2;
			if(horizontal_padding>horizontal_padding_max)
			{
				horizontal_padding=horizontal_padding_max;
			}
			else if(horizontal_padding<horizontal_padding_min)
			{
				horizontal_padding=horizontal_padding_min;
			}
		}
		else
		{
			horizontal_padding=horizontal_padding_min;
		}
		signed int x1=left-horizontal_padding;
		signed int y1=top-vertical_padding;
		signed int x2=right+horizontal_padding-1+horiz_offset;
		signed int y2=bottom+vertical_padding-1;
		if(x1==0&&y1==0&&x2==image.width()-1&&y2==image.height()-1)
		{
			return false;
		}
		image=get_crop_fill(
			image,
			{x1,x2,y1,y2},
			unsigned char(255)
		);
		return true;
	}
	bool horiz_padding(CImg<unsigned char>& image,unsigned int const left)
	{
		return horiz_padding(image,left,left);
	}
	bool horiz_padding(CImg<unsigned char>& image,unsigned int const left_pad,unsigned int const right_pad,unsigned int tolerance,unsigned char background)
	{
		signed int x1,x2;
		switch(image._spectrum)
		{
		case 1:
		case 2:
		{
			auto selector=[=](auto color)
			{
				return color[0]<=background;
			};
			x1=left_pad==-1?0:find_left<1>(image,tolerance,selector)-left_pad;
			x2=right_pad==-1?image.width()-1:find_right<1>(image,tolerance,selector)+right_pad;
			break;
		}
		default:
		{
			auto selector=[bg=3U*unsigned int(background)](auto color)
			{
				return unsigned int(color[0])+color[1]+color[2]<=bg;
			};
			x1=left_pad==-1?0:find_left<3>(image,tolerance,selector)-left_pad;
			x2=right_pad==-1?image.width()-1:find_right<3>(image,tolerance,selector)+right_pad;
			break;
		}
		}
		if(x1>x2)
		{
			std::swap(x1,x2);
		}
		if(x1==0&&x2==image.width()-1)
		{
			return false;
		}
		image=get_crop_fill(image,{x1,x2,0,image.height()-1});
		return true;
	}
	bool vert_padding(CImg<unsigned char>& image,unsigned int const p)
	{
		return vert_padding(image,p,p);
	}
	bool vert_padding(CImg<unsigned char>& image,unsigned int const tp,unsigned int const bp,unsigned int tolerance,unsigned char background)
	{
		signed int y1,y2;
		switch(image._spectrum)
		{
		case 1:
		case 2:
		{
			auto selector=[=](auto color)
			{
				return color[0]<=background;
			};
			y1=tp==-1?0:find_top<1>(image,tolerance,selector)-tp;
			y2=bp==-1?image.height()-1:find_bottom<1>(image,tolerance,selector)+bp;
			break;
		}
		default:
		{
			auto selector=[bg=3U*unsigned int(background)](auto color)
			{
				return unsigned int(color[0])+color[1]+color[2]<=bg;
			};
			y1=tp==-1?0:find_top<3>(image,tolerance,selector)-tp;
			y2=bp==-1?image.height()-1:find_bottom<3>(image,tolerance,selector)+bp;
			break;
		}
		}
		if(y1>y2)
		{
			std::swap(y1,y2);
		}
		if(y1==0&&y2==image.height()-1)
		{
			return false;
		}
		image=get_crop_fill(image,{0,image.width()-1,y1,y2});
		return true;
	}

	bool cluster_padding(
		::cil::CImg<unsigned char>& img,
		unsigned int const lp,
		unsigned int const rp,
		unsigned int const tp,
		unsigned int const bp,
		unsigned char bt)
	{
		auto selections=img._spectrum>2?
			global_select<3>(img,[threshold=3U*unsigned short(bt)](auto color)
		{
			return unsigned short(color[0])+color[1]+color[2]<=threshold;
		}):
			global_select<1>(img,[bt](auto color)
				{
					return color[0]<=bt;
				});
		auto clusters=Cluster::cluster_ranges(selections);
		if(clusters.size()==0)
		{
			return false;
		}
		unsigned int top_size=0;
		Cluster const* top_cluster;
		for(auto const& cluster:clusters)
		{
			auto size=cluster.size();
			if(size>top_size)
			{
				top_size=size;
				top_cluster=&cluster;
			}
		}
		auto com=top_cluster->bounding_box().center<double>();
		ImageUtils::PointUINT tl,tr,bl,br;
		double tld=0,trd=0,bld=0,brd=0;
		auto switch_if_bigger=[com](double& dist,PointUINT& contender,PointUINT cand)
		{
			double d=hypot(com.x-double(cand.x),com.y-double(cand.y));
			if(d>dist)
			{
				dist=d;
				contender=cand;
			}
		};
		for(auto const rect:top_cluster->get_ranges())
		{
			PointUINT cand;
			cand={rect.left,rect.top};
			if(cand.x<=com.x&&cand.y<=com.y) switch_if_bigger(tld,tl,cand);
			cand.x=rect.right;
			if(cand.x>=com.x&&cand.y<=com.y) switch_if_bigger(trd,tr,cand);
			cand.y=rect.bottom;
			if(cand.x>=com.x&&cand.y>=com.y) switch_if_bigger(brd,br,cand);
			cand.x=rect.left;
			if(cand.x<=com.x&&cand.y>=com.y) switch_if_bigger(bld,bl,cand);
		}
		int left=lp==-1?0:int(std::min(tl.x,bl.x))-int(lp);
		int right=rp==-1?img.width():int(std::max(tr.x,br.x))+int(rp);
		int top=tp==-1?0:int(std::min(tl.y,tr.y))-int(tp);
		int bottom=bp==-1?img.height():int(std::max(bl.y,br.y))+int(bp);
		if(left==0&&right==img.width()&&top==0&&bottom==img.height())
		{
			return false;
		}
		img=get_crop_fill(img,{left,right-1,top,bottom-1});
		return true;
	}

	void add_horizontal_energy(cimg_library::CImg<unsigned char> const& ref,cimg_library::CImg<float>& map,float const hec,unsigned char bg);

	void compress(
		CImg<unsigned char>& image,
		unsigned int const min_padding,
		unsigned int const optimal_height,
		unsigned char background_threshold)
	{
		auto copy=hpixelator(image);
		CImg<float> energy_map=create_compress_energy(copy,min_padding);
		if(image._spectrum==1)
		{
			clear_clusters(copy,std::array<unsigned char,1>({255}),[](auto val)
				{
					return val[0]<220;
				},[threshold=2*copy._width/3](Cluster const& c){
					return c.bounding_box().width()<threshold;
				});
		}
		else
		{
			throw "Bad spectrum";
		}
		copy.display();
		/*
		uint num_seams=image._height-optimal_height;
		for(uint i=0;i<num_seams;++i)
		{
			auto map=create_compress_energy(image);
			min_energy_to_right(map);
			auto seam=create_seam(map);
			for(auto s=0U;s<map._width;++s)
			{
				ImageUtils::Rectangle<uint> to_shift{map._width-s-1,map._width-s,seam[s]+1,map._height};
				copy_shift_selection(image,to_shift,0,-1);
				copy_shift_selection(map,to_shift,0,-1);
			}
			image.crop(0,0,image._width-1,image._height-2);
		}
		*/
	}
	CImg<float> create_vertical_energy(CImg<unsigned char> const& ref,float const vec,unsigned int min_vert_space,unsigned char background)
	{
		CImg<float> map(ref._width,ref._height);
		map.fill(INFINITY);
		for(unsigned int x=0;x<map._width;++x)
		{
			unsigned int y=0;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&,background]()
			{
				return node_found=ref(x,y)>background;
			};
			auto place_values=[&,vec,min_vert_space](unsigned int begin,unsigned int end)
			{
				if(end-begin>min_vert_space)
				{
					unsigned int mid=(node_start+y)/2;
					float val_div=2.0f;
					unsigned int y;
					for(y=begin;y<mid;++y)
					{
						++val_div;
						map(x,y)=vec/(val_div*val_div*val_div);
					}
					for(;y<end;++y)
					{
						--val_div;
						map(x,y)=vec/(val_div*val_div*val_div);
					}
				}
			};
			if(assign_node_found())
			{
				node_start=0;
			}
			for(y=1;y<ref._height;++y)
			{
				if(node_found)
				{
					if(!assign_node_found())
					{
						place_values(node_start,y);
					}
				}
				else
				{
					if(assign_node_found())
					{
						node_start=y;
					}
				}
			}
			if(node_found)
			{
				place_values(node_start,y);
			}
		}
		return map;
	}
	float const COMPRESS_HORIZONTAL_ENERGY_CONSTANT=1.0f;
	CImg<float> create_compress_energy(CImg<unsigned char> const& ref)
	{
		auto map=create_vertical_energy(ref,100.0f,0,127);
		for(auto y=0U;y<map._height;++y)
		{
			auto x=0U;
			unsigned int node_start;
			bool node_found;
			auto assign_node_found=[&]()
			{
				return (node_found=map(x,y)==INFINITY);
			};
			auto place_values=[&]()
			{
				unsigned int multiplier=(x-node_start)/2;
				for(auto node_x=node_start;node_x<x;++node_x)
				{
					map(node_x,y)=COMPRESS_HORIZONTAL_ENERGY_CONSTANT*multiplier;
				}
			};
			if(assign_node_found())
			{
				node_start=0;
			}
			for(x=1;x<map._width;++x)
			{
				if(node_found)
				{
					if(!assign_node_found())
					{
						place_values();
					}
				}
				else
				{
					if(assign_node_found())
					{
						node_start=x;
					}
				}
			}
			if(node_found)
			{
				place_values();
			}
		}
		return map;
	}
	bool remove_border(CImg<unsigned char>& image,Grayscale const color,float const tolerance)
	{
		uint right=image._width-1;
		uint bottom=image._height-1;
		bool edited=false;
		std::vector<bool> buffer(image._width*image._height,false);
		for(uint x=0;x<=right;++x)
		{
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{x,0},buffer);
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{x,bottom},buffer);
		}
		for(uint y=1;y<bottom;++y)
		{
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{0,y},buffer);
			edited|=flood_fill(image,tolerance,color,Grayscale::WHITE,{right,y},buffer);
		}
		return edited;
	}
	bool flood_fill(CImg<unsigned char>& image,float const tolerance,Grayscale const color,Grayscale const replacer,Point<unsigned int> point,std::vector<bool>& buffer)
	{
		auto rects=flood_select(image,tolerance,color,point,buffer);
		if(rects.empty())
		{
			return false;
		}
		for(auto rect:rects)
		{
			fill_selection(image,rect,replacer);
		}
		return true;
	}
	bool flood_fill(CImg<unsigned char>& image,float const tolerance,Grayscale const color,Grayscale const replacer,Point<unsigned int> point)
	{
		auto rects=flood_select(image,tolerance,color,point);
		if(rects.empty())
		{
			return false;
		}
		for(auto rect:rects)
		{
			fill_selection(image,rect,replacer);
		}
		return true;
	}
	void rescale_colors(CImg<unsigned char>& img,unsigned char min,unsigned char mid,unsigned char max)
	{
		assert(min<mid);
		assert(mid<max);
		double const scale_up=scast<double>(255-mid)/scast<double>(max-mid);
		double const scale_down=scast<double>(mid)/scast<double>(mid-min);
		unsigned char* const limit=img.begin()+size_t{img._width}*img._height;
		for(unsigned char* it=img.begin();it!=limit;++it)
		{
			unsigned char& pixel=*it;
			if(pixel<=min)
			{
				pixel=0;
			}
			else if(pixel>=max)
			{
				pixel=255;
			}
			else
			{
				if(pixel>mid)
				{
					pixel=mid+(pixel-mid)*scale_up;
					assert(pixel>mid);
				}
				else
				{
					pixel=mid-(mid-pixel)*scale_down;
					assert(pixel<=mid);
				}
			}
		}
	}

	void horizontal_shift(cil::CImg<unsigned char>& img,bool eval_side,bool eval_direction,unsigned char background_threshold)
	{
		std::vector<int> shifts(img._height);
		if(eval_side)
		{
			if(eval_direction)
			{
				unsigned int x,y;
				for(x=img._width;x>0;)
				{
					--x;
					for(y=img._height;y>0;)
					{
						--y;
						if(img(x,y)<background_threshold)
						{
							goto end_loop1;
						}
					}
				}
			end_loop1:
				int shift=img._width-1-x;
				for(unsigned int y_f=img._height;y_f>y;)
				{
					--y_f;
					shifts[y_f]=shift;
				}
				for(unsigned int y_f=y;y_f>0;)
				{
					--y_f;
					if(img(x,y_f)>=background_threshold)
					{
						--x;
					}
					shifts[y_f]=img._width-x-1;
				}
			}
			else
			{
				unsigned int x,y;
				for(x=img._width;x>0;)
				{
					--x;
					for(y=0;y<img._height;++y)
					{
						if(img(x,y)<background_threshold)
						{
							goto end_loop2;
						}
					}
				}
			end_loop2:
				int shift=img._width-1-x;
				for(unsigned int y_f=0;y_f<=y;++y_f)
				{
					shifts[y_f]=shift;
				}
				for(unsigned int y_f=y+1;y_f<img._height;++y_f)
				{
					if(img(x,y_f)>=background_threshold)
					{
						--x;
					}
					shifts[y_f]=img._width-x-1;
				}
			}
		}
		else
		{
			if(eval_direction)
			{
				unsigned int x,y;
				for(x=0;x<img._width;++x)
				{
					for(y=img._height;y>0;)
					{
						--y;
						if(img(x,y)<background_threshold)
						{
							goto end_loop3;
						}
					}
				}
			end_loop3:
				for(unsigned int y_f=img._height;y_f>y;)
				{
					--y_f;
					shifts[y_f]=--x;
				}
				for(unsigned int y_f=y;y_f>0;)
				{
					--y_f;
					if(img(x,y_f)>=background_threshold)
					{
						++x;
					}
					shifts[y_f]=--x;
				}
			}
			else
			{
				unsigned int x,y;
				for(x=0;x<img._width;++x)
				{
					for(y=0;y<img._height;++y)
					{
						if(img(x,y)<background_threshold)
						{
							goto end_loop4;
						}
					}
				}
			end_loop4:
				for(unsigned int y_f=0;y_f<=y;++y_f)
				{
					shifts[y_f]=--x;
				}
				for(unsigned int y_f=y+1;y_f<img._height;++y_f)
				{
					if(img(x,y_f)>=background_threshold)
					{
						++x;
					}
					shifts[y_f]=-x;
				}
			}
		}
		for(unsigned int y=0;y<img._height;++y)
		{
			auto data=&img(0,y);
			auto shift=shifts[y];
			if(shift<0)
			{
				std::memmove(data,data+(-shift),img._width+shift);
			}
			else if(shifts[y]>0)
			{
				std::memmove(data+shift,data,img._width-shift);
			}
		}
	}
	void vertical_shift(cil::CImg<unsigned char>& img,bool eval_bottom,bool from_right,unsigned char background_threshold)
	{

		std::vector<int> shifts(img._width);
		if(eval_bottom)
		{
			if(from_right)
			{
				unsigned int x,y;
				for(y=img._height;y>0;)
				{
					--y;
					for(x=img._width;x>0;)
					{
						--x;
						if(img(x,y)<background_threshold)
						{
							goto end_loop1;
						}
					}
				}
			end_loop1:
				int shift=img._height-1-y;
				for(unsigned int x_f=img._width;x_f>x;)
				{
					--x_f;
					shifts[x_f]=shift;
				}
				for(unsigned int x_f=x;x_f>0;)
				{
					--x_f;
					if(img(x_f,y)>=background_threshold)
					{
						--y;
					}
					shifts[x_f]=img._height-y-1;
				}
			}
			else
			{
				unsigned int x,y;
				for(y=img._height;y>0;)
				{
					--y;
					for(x=0;x<img._width;++x)
					{
						if(img(x,y)<background_threshold)
						{
							goto end_loop2;
						}
					}
				}
			end_loop2:
				int shift=img._height-1-y;
				for(unsigned int x_f=0;x_f<=x;++x_f)
				{
					shifts[x_f]=shift;
				}
				for(unsigned int x_f=x+1;x_f<img._width;++x_f)
				{
					if(img(x_f,y)>=background_threshold)
					{
						--y;
					}
					shifts[x_f]=img._height-y-1;
				}
			}
		}
		else
		{
			if(from_right)
			{
				unsigned int x,y;
				for(y=0;y<img._height;++y)
				{
					for(x=img._width;x>0;)
					{
						--x;
						if(img(x,y)<background_threshold)
						{
							goto end_loop3;
						}
					}
				}
			end_loop3:
				for(unsigned int x_f=img._width;x_f>x;)
				{
					--x_f;
					shifts[x_f]=-y;
				}
				for(unsigned int x_f=x;x_f>0;)
				{
					--x_f;
					if(img(x_f,y)>=background_threshold)
					{
						++y;
					}
					shifts[x_f]=-y;
				}
			}
			else
			{
				unsigned int x,y;
				for(y=0;y<img._height;++y)
				{
					for(x=0;y<img._width;++x)
					{
						if(img(x,y)<background_threshold)
						{
							goto end_loop4;
						}
					}
				}
			end_loop4:
				for(unsigned int x_f=0;x_f<=x;++x_f)
				{
					shifts[x_f]=-y;
				}
				for(unsigned int x_f=x+1;x_f<img._width;++x_f)
				{
					if(img(x_f,y)>=background_threshold)
					{
						++y;
					}
					shifts[x_f]=-y;
				}
			}
		}
		for(unsigned int x=0;x<img._width;++x)
		{
			auto shift=shifts[x];
			if(shift<0)
			{
				auto limit=img._height+shift;
				for(unsigned int y=0;y<limit;++y)
				{
					img(x,y)=img(x,y-shift);
				}
			}
			else
			{
				for(unsigned int y=img._height;y>shift;)
				{
					--y;
					img(x,y)=img(x,y-shift);
				}
			}
		}
	}

}
