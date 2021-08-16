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
#ifndef SCORE_PROCESSES_H
#define SCORE_PROCESSES_H
#include "CImg.h"
#include "ImageUtils.h"
#include <vector>
#include <memory>
#include "Cluster.h"
#include <assert.h>
#include <functional>
#include <array>
#include <mutex>
#include "lib/threadpool/thread_pool.h"
#include "../NeuralNetwork/neural_net.h"
#include <optional>
#ifdef _WIN64
#define USE_CAFFE
#endif
namespace ScoreProcessor {

#ifdef USE_CAFFE
	class CaffeFilterNet {
		class Impl;
		std::unique_ptr<Impl> _impl;
		static cil::CImg<unsigned char> cpu_run(Impl const* impl,cil::CImg<unsigned char> const& img);
		static cil::CImg<unsigned char> gpu_run(Impl const* impl,cil::CImg<unsigned char> const& img);
		static std::optional<cil::CImg<unsigned char>> try_gpu_run(Impl const* impl,cil::CImg<unsigned char> const& img);
	public:
		CaffeFilterNet(std::filesystem::path const& file);
		cil::CImg<unsigned char> cpu_run(cil::CImg<unsigned char> const& img) const
		{
			return cpu_run(_impl.get(),img);
		}
		cil::CImg<unsigned char> gpu_run(cil::CImg<unsigned char> const& img) const
		{
			return gpu_run(_impl.get(),img);
		}
		std::optional<cil::CImg<unsigned char>> try_gpu_run(cil::CImg<unsigned char> const& img) const
		{
			return try_gpu_run(_impl.get(),img);
		}
	};
#endif

	bool remove_empty_lines(cil::CImg<unsigned char>& img,unsigned char background_threshold,unsigned int min_space,unsigned int min_horizontal_protection);

	/*
		Compress the image vertical and return whether changes were made.
		background_threshold - pixels <= this are considered background
		min_space - if vertical spacing is greater than this it can be removed
		min_horiz_protection - regions of the largest cluster through which a horizontal path can be traced at least this length are protected
		max_vertical_protection - regions of the largest cluster through which a vertical can be be traced less at least this length are treated like background
		protection has higher authority over mark for removal
	*/
	bool compress_vertical(
		cil::CImg<unsigned char>& img,
		unsigned char background_threshold,
		unsigned int min_vertical_space,
		unsigned int min_horizontal_space,
		unsigned int min_horiz_protection,
		unsigned int max_vertical_protection,
		unsigned int min_height,
		bool only_straight_paths);

	template<typename T>
	cil::CImg<T> to_rgb(cil::CImg<T> const& img)
	{
		cil::CImg<T> ret(img._width,img._height,1,3);
		auto const layer_size=std::size_t(img._width)*img._height;
		switch(img._spectrum)
		{
		case 1:
		case 2:
			std::memcpy(ret.data(),img.data(),layer_size*sizeof(T));
			for(int i=1;i<3;++i)
			{
				std::memcpy(ret.data()+i*layer_size,ret.data(),layer_size*sizeof(T));
			}
			break;
		case 3:
		case 4:
			std::memcpy(ret.data(),img.data(),layer_size*sizeof(T)*3);
			break;
		default:
			throw std::invalid_argument("Invalid spectrum");
		}
		return ret;
	}

	template<typename T>
	cil::CImg<T> to_rgb(cil::CImg<T>&& img)
	{
		switch(img._spectrum)
		{
		case 4:
			img._spectrum=3;
		case 3:
			return std::move(img);
		}
		return to_rgb(img);
	}

	template<typename T>
	cil::CImg<T> integral_downscale(cil::CImg<T> const& image,unsigned int downscale,ImageUtils::RectangleUINT region,T boundary_fill=std::numeric_limits<T>::max())
	{
		if(downscale==1)
		{
			return image.get_crop(region.left,region.top,region.right-1,region.bottom-1,4);
		}
		auto const fwidth=ImageUtils::round_up(region.width(),downscale);
		auto const fheight=ImageUtils::round_up(region.height(),downscale);
		if(fheight==0||fwidth==0) return {};
		cil::CImg<T> ret(fwidth/downscale,fheight/downscale,image._depth,image._spectrum);
		auto const factor=downscale*downscale;
		for(unsigned int d=0;d<ret._depth;++d)
		{
			for(unsigned int s=0;s<ret._spectrum;++s)
			{
				auto const layer_start=&ret(0,0,d,s);
				auto const ilayer_start=&image(0,0,d,s);
				for(unsigned int y=0;y<ret._height;++y)
				{
					auto const row=layer_start+y*ret._width;
					for(unsigned int x=0;x<ret._width;++x)
					{
						auto const ix_start=x*downscale+region.left;
						auto const iy_start=y*downscale+region.top;
						auto const idata=ilayer_start+iy_start*image._width+ix_start;
						auto const y_to_go=std::min(region.bottom,iy_start+downscale)-iy_start;
						auto const x_to_go=std::min(region.right,ix_start+downscale)-ix_start;
						std::common_type_t<T,unsigned long long> sum=(factor-(x_to_go*y_to_go))*boundary_fill;
						for(unsigned int iy=0;iy<y_to_go;++iy)
						{
							auto const irow=idata+iy*image._width;
							for(unsigned int ix=0;ix<x_to_go;++ix)
							{
								sum+=irow[ix];
							}
						}
						row[x]=sum/factor;
					}
				}
			}
		}
		return ret;
	}

	template<typename T>
	cil::CImg<T> integral_downscale(cil::CImg<T> const& image,unsigned int downscale,T boundary_fill=std::numeric_limits<T>::max())
	{
		return integral_downscale(image,downscale,{0,image._width,0,image._height},boundary_fill);
	}

	template<typename T>
	cil::CImg<T> integral_downscale(cil::CImg<T>&& image,unsigned int downscale,T boundary_fill=std::numeric_limits<T>::max())
	{
		if(downscale==1)
		{
			return std::move(image);
		}
		return integral_downscale(std::as_const(image),downscale,boundary_fill);
	}

	template<typename T>
	cil::CImg<T> integral_downscale(cil::CImg<T>&& image,unsigned int downscale,ImageUtils::RectangleUINT region,T boundary_fill=std::numeric_limits<T>::max())
	{
		if(downscale==1&&region.left==0&&region.top==0&&region.right==image._width&&region.bottom==image._height)
		{
			return std::move(image);
		}
		return integral_downscale(std::as_const(image),downscale,region,boundary_fill);
	}

	namespace detail {
		template<typename CountType,typename InputType,typename ComparisonFunc,std::size_t... Layers>
		void template_match(cil::CImg<CountType>& counts,cil::CImg<InputType> const& img,cil::CImg<InputType> const& tmplt,ImageUtils::PointUINT const point,ComparisonFunc comp,CountType initial_count,std::index_sequence<Layers...>)
		{
			auto const theight=tmplt._height;
			auto const twidth=tmplt._width;
			auto const layer_depth=std::size_t{img._width}*img._height;
			auto const tlayer_depth=std::size_t{theight}*twidth;
			auto const y_max=std::min(counts._height,point.y+theight)-point.y;
			auto const x_max=std::min(counts._width,point.x+twidth)-point.x;
			auto& pixel=(counts(point.x,point.y)=initial_count);
			for(unsigned int y1=0;y1<y_max;++y1)
			{
				for(unsigned int x1=0;x1<x_max;++x1)
				{
					std::array<InputType,sizeof...(Layers)> const i{*(&img(point.x+x1,point.y+y1)+Layers*layer_depth)...};
					decltype(i) t{*(&tmplt(x1,y1)+Layers*tlayer_depth)...};
					pixel+=comp(i,t);
				}
			}
		}
		template<typename CountType,typename InputType,typename ComparisonFunc,std::size_t... Layers>
		void sliding_template_match(cil::CImg<CountType>& counts,cil::CImg<InputType> const& img,cil::CImg<InputType> const& tmplt,ComparisonFunc comp,std::index_sequence<Layers...>)
		{
			auto const cwidth=counts._width;
			auto const cheight=counts._height;
			auto const theight=tmplt._height;
			auto const twidth=tmplt._width;
			auto const ilayer_depth=std::size_t{img._width}*img._height;
			auto const tlayer_depth=std::size_t{theight}*twidth;
			for(unsigned int ty=0;ty<theight;++ty)
			{
				for(unsigned int cy=0;cy<cheight;++cy)
				{
					for(unsigned int cx=0;cx<cwidth;++cx)
					{
						auto& cpixel=counts(cx,cy);
						auto const istart=&img(cx,cy+ty);
						auto const tstart=&tmplt(0,ty);
						for(unsigned int x=0;x<twidth;++x)
						{
							std::array<InputType,sizeof...(Layers)> const i{*(istart+x+Layers*ilayer_depth)...};
							decltype(i) t{*(tstart+x+Layers*tlayer_depth)...};
							cpixel+=comp(i,t);
						}
					}
				}
			}
		}
	}

	template<std::size_t Layers,typename CountType,typename InputType,typename ComparisonFunc>
	void template_match(cil::CImg<CountType>& counts,cil::CImg<InputType> const& img,cil::CImg<InputType> const& tmplt,ImageUtils::PointUINT const point,ComparisonFunc comp,CountType initial=0)
	{
		return detail::template_match(counts,img,tmplt,point,comp,initial,std::make_index_sequence<Layers>{});
	}

	template<std::size_t Layers,typename CountType,typename InputType,typename ComparisonFunc>
	cil::CImg<CountType> sliding_template_match(cil::CImg<InputType> const& img,cil::CImg<InputType> const& tmplt,ComparisonFunc comp,CountType initial=0)
	{
		if(img._width<tmplt._width||img._height<tmplt._height)
		{
			return {};
		}
		cil::CImg<CountType> counts(img._width-tmplt._width+1,img._height-tmplt._height+1,1,Layers);
		counts.fill(initial);
		detail::sliding_template_match(counts,img,tmplt,comp,std::make_index_sequence<Layers>{});
		return counts;
	}

	unsigned int sliding_template_match_erase_exact(cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,float threshold,std::array<unsigned char,4> color);

	unsigned int sliding_template_match_erase_scale_compare(cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,float threshold,float scale,std::array<unsigned char,4> color);

	unsigned int cluster_template_match_erase(cil::CImg<unsigned char>& img,std::vector<ScoreProcessor::Cluster> const& rects,cil::CImg<unsigned char> const& tmplt,float match_threshold);

	template<typename T>
	class vertical_iterator {
		T* _top;
		size_t _width;
	public:
		using difference_type=std::ptrdiff_t;
		using value_type=T;
		using pointer=T*;
		using reference=T&;
		using iterator_category=std::random_access_iterator_tag;
		vertical_iterator(cil::CImg<T> const& img,unsigned int column,unsigned int spectrum=0) noexcept:_top{img.data()+column+size_t{spectrum}*img._width*img._height},_width{img._width}{}
		vertical_iterator(cil::CImg<T>& img,unsigned int column,unsigned int spectrum=0) noexcept:_top{img.data()+column+size_t{spectrum}*img._width*img._height},_width{img._width}{}
		vertical_iterator(T* top,unsigned int width) noexcept:_top{top},_width{width}{}
		T& operator[](size_t s) const noexcept
		{
			return *(_top+s*_width);
		}
		T& operator*() const noexcept
		{
			return *_top;
		}
		T* operator->() const noexcept
		{
			return &**this;
		}
		vertical_iterator& operator++() noexcept
		{
			_top+=_width;
			return *this;
		}
		vertical_iterator& operator--() noexcept
		{
			_top-=_width;
			return *this;
		}
		vertical_iterator operator++(int) noexcept
		{
			auto copy{*this};
			++(*this);
			return copy;
		}
		vertical_iterator operator--(int) noexcept
		{
			auto copy{*this};
			--(*this);
			return copy;
		}
		vertical_iterator& operator+=(std::ptrdiff_t d) noexcept
		{
			_top+=d*_width;
			return *this;
		}
		vertical_iterator& operator-=(std::ptrdiff_t d) noexcept
		{
			_top-=d*_width;
			return *this;
		}
		friend vertical_iterator operator+(vertical_iterator v,std::ptrdiff_t t) noexcept
		{
			v+=t;
			return v;
		}
		friend vertical_iterator operator+(std::ptrdiff_t t,vertical_iterator v) noexcept
		{
			v+=t;
			return v;
		}
		friend vertical_iterator operator-(vertical_iterator v,std::ptrdiff_t t) noexcept
		{
			v-=t;
			return v;
		}
		friend std::ptrdiff_t operator-(vertical_iterator const& v,vertical_iterator const& v2) noexcept
		{
			assert(v._width==v2._width);
			return (v._top-v2._top)/v._width;
		}
#define vertical_operator_comp_op(op)\
		template<typename T, typename U>\
		friend bool operator op(vertical_iterator<T> const& a,vertical_iterator<U> const& b) noexcept\
		{\
			assert(a._width==b._width);\
			return a._top op b._top;\
		}
		vertical_operator_comp_op(==)
			vertical_operator_comp_op(<)
			vertical_operator_comp_op(>)
			vertical_operator_comp_op(<=)
			vertical_operator_comp_op(>=)
			vertical_operator_comp_op(!=)
#undef vertical_operator_comp_op
	};

	template<typename T>
	vertical_iterator(cil::CImg<T> const&,unsigned int,unsigned int)->vertical_iterator<T const>;

	template<typename T>
	vertical_iterator(cil::CImg<T>&,unsigned int,unsigned int)->vertical_iterator<T>;

	class ExclusiveThreadPool {
	public:
		exlib::thread_pool& pool() const;
		void set_thread_count(unsigned int nt);
		ExclusiveThreadPool(unsigned int num_threads=std::thread::hardware_concurrency());
		~ExclusiveThreadPool();
	};

	/*
		Fast approximate anti-aliasing
	*/
	template<typename T>
	bool fxaa(cil::CImg<T>& img,std::common_type_t<T,short> contrast_threshold,std::common_type_t<float,T> gamma,std::common_type_t<float,T> subpixel_blending=1)
	{
		if(img.width()<3||img.height()<3||img.spectrum()<1||img.depth()<1)
		{
			return false;
		}
		cil::CImg<T> copy{img._width,img._height,img._depth,img._spectrum};
		//cheap using first layer as luminance
		auto const width=img._width;
		auto const height=img._height;
		using p=decltype(contrast_threshold);
		for(unsigned int y=0;y<width;++y)
		{
			for(unsigned int x=0;x<height;++x)
			{
				p nw=img._atXY(x-1,y-1);
				p n=img._atXY(x,y-1);
				p ne=img._atXY(x+1,y-1);
				p w=img._atXY(x-1,y);
				p m=img(x,y);
				p e=img._atXY(x+1,y);
				p sw=img._atXY(x-1,y+1);
				p s=img._atXY(x,y+1);
				p se=img._atXY(x,y+1);
				auto minmax=std::minmax({n,s,e,w});
				p contrast=p{minmax.second}-p{minmax.first};
				if(contrast>contrast_threshold) //found an edge 
				{
					using f=std::common_type_t<float,p>;
					constexpr f weight=1.41421356237;
					f blend=(weight*(n+e+s+w)+nw+ne+sw+se)/(4*weight+4);
					f filter=std::clamp<f>(std::abs(blend-m)/contrast,0,1);
					filter=filter*filter*(3-2*x);
					f hcontrast=weight*std::abs(n+s-2*m)+std::abs(ne+se-2*e)+std::abs(nw+sw-2*w);
					f vcontrast=weight*std::abs(e+w-2*m)+std::abs(ne+nw-2*n)+std::abs(se+sw-2*s);
					bool horizontal=hcontrast>=vcontrast;
					auto [pgradient,ngradient]=horizontal?std::make_tuple(f{s}-m,f{n}-m):std::make_tuple(f{e}-m,f{w}-m);
					auto blend_factor=blend*blend*subpixel_blending;
					if(pgradient>ngradient)
					{
						auto gradient_threshold=pgradient*0.5;
						if(horizontal) //only need to look to left if horizontal
						{
							auto edge_luminance=f{s}+m;
							unsigned int x_s=x;
							for(;x_s<width;++x_s)
							{
								if(img(x_s,y)-edge_luminance>=gradient_threshold) break;
							}
						}
						else
						{
							auto edge_luminance=f{e}+m;
						}
					}
					else
					{
						auto gradient_threshold=ngradient*0.5;
						if(horizontal)
						{
							auto edge_luminance=f{n}+m;
						}
						else
						{
							auto edge_luminance=f{w}+m;
						}
					}
				}
			}
		}
		img=std::move(copy);
		return true;
	}

	namespace mlaa_det {
		enum class orientation:char {
			up=1,flat=2,down=3
		};
		template<typename T>
		auto operator*(orientation o,T val) noexcept
		{
			return static_cast<char>(o)*val;
		}
		template<typename T>
		auto operator*(T val,orientation o) noexcept
		{
			return static_cast<char>(o)*val;
		}
		struct edge_t {
			using orientation=mlaa_det::orientation;
			unsigned int begin;
			unsigned int end;
			orientation begin_orientation;
			orientation end_orientation;
		};
		template<typename Iter,typename U>
		edge_t find_edge(Iter row,Iter next_row,unsigned int start,unsigned int end,U contrast_threshold) noexcept
		{
			auto detect_orientation=[row,next_row,contrast_threshold](unsigned int x,orientation& orient)
			{
				auto const upper=std::abs(U{row[x]}-U(row[x-1]));
				auto const lower=std::abs(U{next_row[x]}-U(next_row[x-1]));
				if(upper>contrast_threshold||lower>contrast_threshold)
				{
					if(upper>lower)
					{
						orient=orientation::up;
					}
					else
					{
						orient=orientation::down;
					}
				}
				else
				{
					orient=orientation::flat;
				}
			};
			for(;start<end;++start)
			{
				if(std::abs(U{row[start]}-U{next_row[start]})>contrast_threshold)
				{
					edge_t edge;
					edge.begin=start;
					++start;
					for(;start<end;++start)
					{
						if(std::abs(U{row[start]}-U{next_row[start]})<=contrast_threshold)
						{
							edge.end=start;
							if(start+1<end)
							{
								detect_orientation(start,edge.end_orientation);
							}
							else
							{
								edge.end_orientation=orientation::flat;
							}
							break;
						}
					}
					if(start>=end)
					{
						edge.end_orientation=orientation::flat;
						edge.end=end;
					}
					if(edge.begin>0)
					{
						detect_orientation(edge.begin,edge.begin_orientation);
					}
					else
					{
						edge.begin_orientation=orientation::flat;
					}
					return edge;
				}
			}
			return {-1U,-1U};
		}

		template<typename Iter>
		void blend(Iter row,Iter next_row,double const x_start,double const y_start,double const x_end,double const y_end,double const gamma) noexcept
		{
			auto m=(y_end-y_start)/(x_end-x_start);
			if(y_start==1)
			{
				m=std::abs(m);
			}
			else
			{
				m=-std::abs(m);
			}
			bool const upper=y_start<1||y_end<1;
			auto const area_adjustment=y_start==1?0:0.5;
			auto write_row=upper?row:next_row;
			auto read_row=upper?next_row:row;
			auto x=x_start;
			auto mix=[gamma](auto a,auto b,auto area)
			{
				return std::round(std::pow(area*std::pow(a,gamma)+(1-area)*std::pow(b,gamma),1/gamma));
			};
			if(std::floor(x)==x)
			{
				for(;x<x_end-0.5;++x) //half integers should be exact
				{
					auto area=m*(x-x_start+0.5)+area_adjustment;
					size_t x_coord=x;
					write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				}
				if(x<x_end)
				{
					auto area=(area_adjustment+m*(x-x_start))/4;
					size_t x_coord=x;
					write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				}
			}
			else
			{
				auto area=(area_adjustment+m*0.5)/2;
				size_t x_coord=x;
				write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				x+=0.5;
				for(;x<x_end;++x) //half integers should be exact
				{
					auto area=m*(x-x_start+0.5)+area_adjustment;
					if(area>0.5) area=1-area;
					size_t x_coord=x;
					write_row[x_coord]=mix(read_row[x_coord],write_row[x_coord],area);
				}
			}
		}
	}

	/*
		Morphological Antialiasing
	*/
	template<typename T>
	bool mlaa(cil::CImg<T>& img,std::common_type_t<T,short> contrast_threshold,double gamma)
	{
		auto const height=img._height;
		auto const width=img._width;
		if(height<2||width<2) return false;
		auto const hm1=height-1;
		auto const wm1=width-1;
		using p=decltype(contrast_threshold);
		using namespace mlaa_det;
		bool did_something=false;
		auto const size=size_t{height}*width;
		auto const spectrum=img._spectrum;
		auto copy{img};
		for(unsigned int y=0;y<hm1;++y) //scan for horizontal edge
		{
			T* const row=img.data()+y*width;
			T* const next_row=row+width;
			edge_t edge=find_edge(row,next_row,0,width,contrast_threshold);
			for(;edge.begin!=-1;edge=find_edge(row,next_row,edge.end,width,contrast_threshold))
			{
				auto do_blend=[row=copy.data()+y*width,width,gamma,spectrum,size](double x_start,double y_start,double x_end,double y_end)
				{
					for(unsigned int i=0;i<spectrum;++i)
					{
						auto const layer_row=row+i*size;
						mlaa_det::blend(layer_row,layer_row+width,x_start,y_start,x_end,y_end,gamma);
					}
				};
				if(edge.end-edge.begin==1||(edge.begin_orientation==orientation::flat&&edge.end_orientation==orientation::flat))
				{
					continue;
				}
				did_something=true;
				switch(edge.begin_orientation)
				{
				case orientation::flat:
					do_blend(edge.begin,1,edge.end,0.5*edge.end_orientation);
					break;
				case orientation::down:
				case orientation::up:
					switch(edge.end_orientation)
					{
					case orientation::down:
					case orientation::up:
					{
						auto const mid_dist=(double(edge.end)-edge.begin)/2;
						auto const mid=edge.begin+mid_dist;
						do_blend(edge.begin,0.5*edge.begin_orientation,mid,1);
						do_blend(mid,1,edge.end,0.5*edge.end_orientation);
					}
					break;
					case orientation::flat:
						do_blend(edge.begin,0.5*edge.begin_orientation,edge.end,1);
					}
					break;
				}
			}
		}
		for(unsigned int x=0;x<wm1;++x) //scan for vertical edge
		{
			vertical_iterator column{img,x};
			vertical_iterator next_column{img,x+1};
			edge_t edge=find_edge(column,next_column,0,height,contrast_threshold);
			for(;edge.begin!=-1;edge=find_edge(column,next_column,edge.end,height,contrast_threshold))
			{
				auto do_blend=[&copy,x,gamma,spectrum,size](double x_start,double y_start,double x_end,double y_end)
				{
					for(unsigned int i=0;i<spectrum;++i)
					{
						vertical_iterator column{copy,x,i};
						vertical_iterator next{copy,x+1,i};
						mlaa_det::blend(column,next,x_start,y_start,x_end,y_end,gamma);
					}
				};
				if(edge.begin_orientation==orientation::flat&&edge.end_orientation==orientation::flat)
				{
					continue;
				}
				did_something=true;
				switch(edge.begin_orientation)
				{
				case orientation::flat:
					do_blend(edge.begin,1,edge.end,0.5*edge.end_orientation);
					break;
				case orientation::down:
				case orientation::up:
					switch(edge.end_orientation)
					{
					case orientation::down:
					case orientation::up:
					{
						auto const mid_dist=(double(edge.end)-edge.begin)/2;
						auto const mid=edge.begin+mid_dist;
						do_blend(edge.begin,0.5*edge.begin_orientation,mid,1);
						do_blend(mid,1,edge.end,0.5*edge.end_orientation);
					}
					break;
					case orientation::flat:
						do_blend(edge.begin,0.5*edge.begin_orientation,edge.end,1);
					}
					break;
				}
			}
		}
		img=std::move(copy);
		return did_something;
	}

	//BackgroundFinder:: returns true if a pixel is NOT part of the background
	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_left(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf,bool cumulative=true)
	{
		unsigned int num=0;
		auto const size=img._width*img._height;
		for(unsigned int x=0;x<img._width;++x)
		{
			for(unsigned int y=0;y<img._height;++y)
			{
				auto pix=&img(x,y);
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return x;
			}
			if(!cumulative)
			{
				num=0;
			}
		}
		return img._width-1;
	}
	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_right(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf,bool cumulative=true)
	{
		unsigned int num=0;
		auto const size=img._width*img._height;
		for(unsigned int x=img._width-1;x<img._width;--x)
		{
			for(unsigned int y=0;y<img._height;++y)
			{
				auto pix=&img(x,y);
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return x;
			}
			if(!cumulative)
			{
				num=0;
			}
		}
		return 0;
	}

	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_top(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf,bool cumulative=true)
	{
		unsigned int num=0;
		auto const width=img._width;
		auto const height=img._height;
		auto const size=width*height;
		auto const data=img._data;
		for(unsigned int y=0;y<height;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				auto pix=row+x;
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return y;
			}
			if(!cumulative)
			{
				num=0;
			}
		}
		return height-1;
	}

	template<unsigned int NumLayers,typename T,typename BackgroundFinder>
	unsigned int find_bottom(cil::CImg<T> const& img,unsigned int tolerance,BackgroundFinder bf,bool cumulative=true)
	{
		unsigned int num=0;
		auto const width=img._width;
		auto const height=img._height;
		auto const size=width*height;
		auto const data=img._data;
		for(unsigned int y=height-1;y<height;--y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				auto pix=row+x;
				std::array<T,NumLayers> pixel;
				for(unsigned int s=0;s<NumLayers;++s)
				{
					pixel[s]=*(pix+s*size);
				}
				if(bf(pixel))
				{
					++num;
				}
			}
			if(num>=tolerance)
			{
				return y;
			}
			if(!cumulative)
			{
				num=0;
			}
		}
		return 0;
	}

	template<typename T>
	cil::CImg<T> get_crop_fill(cil::CImg<T> const& img,ImageUtils::Rectangle<signed int> region,T fill=255)
	{
		auto const width=img.width();
		auto const height=img.height();
		auto ret=img.get_crop(region.left,region.top,region.right,region.bottom,4);
		T buffer[10];
		for(unsigned int s=0;s<img._spectrum;++s)
		{
			buffer[s]=fill;
		}
		if(region.left<0)
		{
			fill_selection(ret,{0,static_cast<unsigned int>(-region.left),0,ret._height},buffer);
		}
		if(region.right>=width)
		{
			fill_selection(ret,{static_cast<unsigned int>(width-region.left),ret._width,0,ret._height},buffer);
		}
		if(region.top<0)
		{
			fill_selection(ret,{0,ret._width,0,static_cast<unsigned int>(-region.top)},buffer);
		}
		if(region.bottom>=height)
		{
			fill_selection(ret,{0,ret._width,static_cast<unsigned int>(height-region.top),ret._height},buffer);
		}
		return ret;
	}

	// bg: std::array<T,Layers> pixel -> U brightness
	// repl: std::array<T,Layers>& pixel, ThresholdCalcType threshold -> void
	template<std::size_t Layers,typename ThresholdCalcType=float,typename T,typename BrightnessGetter,typename Replacer>
	void local_sauvola(cil::CImg<T>& img,unsigned int window_radius,BrightnessGetter bg,Replacer repl,ThresholdCalcType k,ThresholdCalcType max_standard_deviation=128)
	{
		auto const integral=cil::integral_image<Layers>(img,[bg](auto input)
			{
				return std::array{bg(input)};
			});
		auto const integral_squared=cil::integral_image<Layers>(img,[bg](auto input)
			{
				decltype(auto) res=bg(input);
				return std::array{res*res};
			});
		std::size_t const wradius=window_radius;
		std::size_t const width=img._width;
		std::size_t const height=img._height;
		auto const area=width*height;
		for(std::size_t y=0;y<height;++y)
		{
			auto const y_min=wradius>y?0:y-wradius;
			auto const y_max=std::min(y+wradius,height);
			auto const y_dist=y_max-y_min;
			for(std::size_t x=0;x<width;++x)
			{
				auto const x_min=wradius>x?0:x-wradius;
				auto const x_max=std::min(x+wradius,width);
				auto const area=y_dist*(x_max-m_min);
				auto get_point=[&](auto& img)
				{
					return ThresholdCalcType(img(x_max-1,y_max-1)-img(x_min,y_min))/area;
				};
				ThresholdCalcType const mean=get_point(integral);
				ThresholdCalcType const mean_of_squared=get_point(integral_squared);
				auto const std_dev=sqrt(mean_of_squared-mean*mean);
				auto const threshold=mean*(1+k*(std_dev/max_standard_deviation-1));
				repl(cil::pixel_reference<Layers>(&img(x,y),area),threshold);
			}
		}
	}

	/*
		Reduces colors to two colors
		@param image, must be a 3 channel RGB image
		@param middleColor, pixels darker than this color will be set to lowColor, brighter to highColor
		@param lowColor
		@param highColor
	*/
	void binarize(::cimg_library::CImg<unsigned char>& image,ImageUtils::ColorRGB const middleColor,ImageUtils::ColorRGB const lowColor,ImageUtils::ColorRGB const highColor);
	/*
		Reduces colors to two colors
		@param image, must be a 1 channel grayscale image
		@param middleGray, pixels darker than this gray will be set to lowGray, brighter to highGray
		@param lowGray
		@param highGray
	*/
	void binarize(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const middleGray,ImageUtils::Grayscale const lowGray,ImageUtils::Grayscale const highGray);
	/*
		@param image, must be a 1 channel grayscale image
		@param middleGray, pixels darker than this gray will be become more black by a factor of scale, higher whited by a factor of scale
	*/
	void binarize(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const middleGray,float const scale=2.0f);
	/*
		Replaces grays in a range with another gray
		@param image, must be 1 channel grayscale image
	*/
	bool replace_range(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const lower,ImageUtils::Grayscale const upper=255,ImageUtils::Grayscale const replacer=ImageUtils::Grayscale::WHITE);
	/*
		Replaces certainly bright pixels with a color
		@param image, must be 3 channel RGB
		@param lowerBrightness
		@param upperBrightness
		@param replacer
	*/
	bool replace_by_brightness(::cimg_library::CImg<unsigned char>& image,unsigned char lowerBrightness,unsigned char upperBrightness=255,ImageUtils::ColorRGB replacer=ImageUtils::ColorRGB::WHITE);
	/*
		Replaces particularly chromatic pixels with a color
		@param image, must be 3 channel RGB
		@param lowerChroma
		@param upperChroma
		@param replacer
	*/
	bool replace_by_hsv(::cimg_library::CImg<unsigned char>& image,ImageUtils::ColorHSV startbound,ImageUtils::ColorHSV end,ImageUtils::ColorRGB replacer=ImageUtils::ColorRGB::WHITE);
	bool replace_by_rgb(::cil::CImg<unsigned char>& image,ImageUtils::ColorRGB start,ImageUtils::ColorRGB end,ImageUtils::ColorRGB replacer);

	/*
		Shifts selection over while leaving rest unchanged
		@param image
		@param selection, the rectangle that will be shifted
		@param shiftx, the number of pixels the selection will be translated in the x direction
		@param shiftx, the number of pixels the selection will be translated in the y direction
	*/
	template<typename T>
	void copy_shift_selection(::cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> selection,int const shiftx,int const shifty);
	/*
		Fills selection with a certain color
		@param image, must be 3 channel RGB
		@param selection, the rectangle that will be filled
		@param color, the color the selection will be filled with
	*/
	void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::ColorRGB const color);
	/*
		Fills selection with a certain color
		@param image, must be 1 channel grayscale
		@param selection, the rectangle that will be filled
		@param gray, the gray the selection will be filled with
	*/
	void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::Grayscale const gray);
	/*
			Automatically centers the image horizontally
			@param image
			@return 0 if successful shift, 1 if no shift, 2 if invalid image
		*/
	bool auto_center_horiz(::cimg_library::CImg<unsigned char>& image);
	/*
		Finds the left side of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the left side
		@return the x-coordinate of the left side
	*/
	unsigned int find_left(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the left side of the the image
		@param image, must 1 channel grayscale
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the left side
		@return the x-coordinate of the left side
	*/
	unsigned int find_left(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);
	/*
		Finds the right side of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the right side
		@return the x-coordinate of the right side
	*/
	unsigned int find_right(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the left side of the the image
		@param image, must 1 channel grayscale
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a column as the right side
		@return the x-coordinate of the right side
	*/
	unsigned int find_right(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);

	/*
		Automatically centers the image vertically
		@param image
		@return 0 if successful shift, 1 if no shift, 2 if invalid image
	*/
	bool auto_center_vert(::cimg_library::CImg<unsigned char>& image);
	/*
		Finds the top of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the top
		@return the y-coordinate of the top
	*/
	unsigned int find_top(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the top of the the image
		@param image, must 1 channel RGB
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the top
		@return the y-coordinate of the top
	*/
	unsigned int find_top(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);
	/*
		Finds the bottom of the the image
		@param image, must 3 channel RGB
		@param background, the color of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the bottom
		@return the y-coordinate of the bottom
	*/
	unsigned int find_bottom(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background,unsigned int const tolerance);
	/*
		Finds the bottom of the the image
		@param image, must 1 channel RGB
		@param background, the gray of the background
		@param tolerance, the number of non-background pixels that qualifies a row as the bottom
		@return the y-coordinate of the bottom
	*/
	unsigned int find_bottom(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background,unsigned int const tolerance);

	/*
		Creates a profile of the left side of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of x coordinates of the left side
	*/
	::std::vector<unsigned int> build_left_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the left side of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return where the profile will be stored, as a vector of x coordinates of the left side
	*/
	::std::vector<unsigned int> build_left_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the right side of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of x coordinates of the right side
	*/
	::std::vector<unsigned int> build_right_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the right side of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return where the profile will be stored, as a vector of x coordinates of the right side
	*/
	::std::vector<unsigned int> build_right_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the top of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of y coordinates of the top
	*/
	::std::vector<unsigned int> build_top_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the top of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return where the profile will be stored, as a vector of y coordinates of the top
	*/
	::std::vector<unsigned int> build_top_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Creates a profile of the bottom of the image
		@param image, must be 3 channel RGB
		@param background, the background color of the image
		@return where the profile will be stored, as a vector of y coordinates of the bottom
	*/
	::std::vector<unsigned int> build_bottom_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::ColorRGB const background);
	/*
		Creates a profile of the bottom of the image
		@param image, must be 1 channel grayscale
		@param background, the background gray of the image
		@return container, where the profile will be stored, as a vector of y coordinates of the bottom
	*/
	::std::vector<unsigned int> build_bottom_profile(::cimg_library::CImg<unsigned char> const& image,ImageUtils::Grayscale const background);
	/*
		Selects the outside (non-systems) of a score image
		@param image, must be 1 channel grayscale
		@return where the selected rectangles go
	*/
	::std::vector<::std::unique_ptr<ImageUtils::Rectangle<unsigned int>>> select_outside(::cimg_library::CImg<unsigned char> const& image);
	struct cut_heuristics {
		unsigned int min_width;
		unsigned int min_height;
		float horizontal_energy_weight;
		unsigned int minimum_vertical_space;
		unsigned int minimum_horizontal_space;
		unsigned int horizontal_cut_through;
		unsigned char background;
	};
	/*
		Cuts a specified score page into multiple smaller images
		@param image
		@param filename, the start of the filename that the images will be saved as
		@param padding, how much white space will be put at the top and bottom of the pages
		@return the number of images created
	*/
	unsigned int cut_page(::cimg_library::CImg<unsigned char> const& image,char const* filename,cut_heuristics const& ch,int quality=100);

	/*
		Finds the line that is the top of the score image
		@param image
		@return a line defining the top of the score image
	*/
	ImageUtils::line<unsigned int> find_top_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the bottom of the score image
		@param image
		@return a line defining the bottom of the score image
	*/
	ImageUtils::line<unsigned int> find_bottom_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the left of the score image
		@param image
		@return a line defining the left of the score image
	*/
	ImageUtils::line<unsigned int> find_left_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Finds the line that is the right of the score image
		@param image
		@return a line defining the right of the score image
	*/
	ImageUtils::line<unsigned int> find_right_line(::cimg_library::CImg<unsigned char> const& image);
	/*
		Automatically levels the image
		@param image
		@param pixel_prec precision when measuring distance from origin
		@param min_angle minimum angle of range to consider rotation (degrees), is angle of line from x-axis
		@param max_angle maximum angle of range to consider rotation (degrees), is angle of line from x-axis
		@param angle_prec precision when measuring angle
		@return rotation in degrees
	*/
	float auto_rotate(::cimg_library::CImg<unsigned char>& image,double pixel_prec,double min_angle,double max_angle,double angle_prec,unsigned char boundary=128);

	float find_angle_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary=128,bool use_horizontal_transitions=true);
	/*
		Automatically levels the image.
		@param image
		@param pixel_prec precision when measuring distance from origin
		@param min_angle minimum angle of range to consider rotation (radians), is angle of Hesse normal form
		@param max_angle maximum angle of range to consider rotation (radians), is angle of Hesse normal form
		@param angle_steps number of angle quantization steps between min and max angle
		@return rotation in radians
	*/
	float auto_rotate_bare(::cimg_library::CImg<unsigned char>& img,double pixel_prec,double min_angle,double max_angle,unsigned int angle_steps,unsigned char boundary=128);
	/*
		Automatically deskews the image
		@param image
	*/
	bool auto_skew(::cimg_library::CImg<unsigned char>& image);
	/*
		Automatically levels and deskews the image
		@param image
	*/
	bool undistort(::cimg_library::CImg<unsigned char>& image);
	/*
		Flood selects from point
		@param image, must be 1 channel grayscale
		@param tolerance, the tolerance 0.0f-1.0f between colors
		@param gray, selected gray
		@param start, seed point of flood fill
		@return where the selected pixels will go as a vector of rectangles pointers
	*/
	::std::vector<ImageUtils::Rectangle<unsigned int>> flood_select(::cimg_library::CImg<unsigned char> const& image,float const tolerance,ImageUtils::Grayscale const gray,ImageUtils::Point<unsigned int> start);

	::std::vector<ImageUtils::Rectangle<unsigned int>> flood_select(::cimg_library::CImg<unsigned char> const& image,float const tolerance,ImageUtils::Grayscale const gray,ImageUtils::Point<unsigned int> start,std::vector<bool>& buffer);

	/*
		Removes border of the image.
	*/
	bool remove_border(::cimg_library::CImg<unsigned char>& image,ImageUtils::Grayscale const color=ImageUtils::Grayscale::BLACK,float const tolerance=0.5);

	/*
		Does a flood fill.
	*/
	bool flood_fill(::cimg_library::CImg<unsigned char>& image,float const tolerance,ImageUtils::Grayscale const color,ImageUtils::Grayscale const replacer,ImageUtils::Point<unsigned int> start);

	/*
		Does a flood fill.
	*/
	bool flood_fill(::cimg_library::CImg<unsigned char>& image,float const tolerance,ImageUtils::Grayscale const color,ImageUtils::Grayscale const replacer,ImageUtils::Point<unsigned int> start,std::vector<bool>& buffer);

	namespace detail {

		struct scan_range {
			unsigned int left,right,y;
			int direction;
		};

		template<typename Img,typename Selector,typename DoWithLine>
		void flood_operation(Img& img,ImageUtils::PointUINT start,Selector& selector,DoWithLine& dwl,std::vector<scan_range>& scan_ranges)
		{
			using point=ImageUtils::PointUINT;
			using line=ImageUtils::horizontal_line<unsigned int>;
			if(!selector(start))
			{
				return;
			}
			unsigned int left=start.x-1;
			while(left<start.x&&selector(point{left,start.y}))
			{
				--left;
			}
			++left;
			unsigned int right=start.x+1;
			while(right<img._width&&selector(point{right,start.y}))
			{
				++right;
			}
			dwl(line{left,right,start.y});
			scan_ranges.push_back({left,right,start.y,+1});
			scan_ranges.push_back({left,right,start.y,-1});
			while(!scan_ranges.empty())
			{
				auto const current_range=scan_ranges.back();
				scan_ranges.pop_back();
				//scan left
				for(left=current_range.left-1;
					left<img._width&&selector(point{left,current_range.y});
					--left)
				{
				}
				++left;

				//scan right
				for(right=current_range.right;
					right<img._width&&selector(point{right,current_range.y});
					++right)
				{
				}
				dwl(line{left,right,current_range.y});

				//scan in same direction vertically
				bool range_found=false;
				unsigned int range_start;
				unsigned int newy=current_range.y+current_range.direction;
				unsigned int sx=left;
				if(newy<img._height)
				{
					while(sx<right)
					{
						for(;sx<right;++sx)
						{
							if(selector(point{sx,newy}))
							{
								range_found=true;
								range_start=sx++;
								break;
							}
						}
						for(;sx<right;++sx)
						{
							if(!selector(point{sx,newy}))
							{
								break;
							}
						}
						if(range_found)
						{
							range_found=false;
							scan_ranges.push_back({range_start,sx,newy,current_range.direction});
						}
					}
				}

				//scan opposite direction vertically
				newy=current_range.y-current_range.direction;
				if(newy<img._height)
				{
					while(left<current_range.left)
					{
						for(;left<current_range.left;++left)
						{
							if(selector(point{left,newy}))
							{
								range_found=true;
								range_start=left++;
								break;
							}
						}
						for(;left<current_range.left;++left)
						{
							if(!selector(point{left,newy}))
								break;
						}
						if(range_found)
						{
							range_found=false;
							scan_ranges.push_back({range_start,left,newy,-current_range.direction});
						}
					}
					left=current_range.right;
					while(left<right)
					{
						for(;left<right;++left)
						{
							if(selector(point{left,newy}))
							{
								range_found=true;
								range_start=left++;
								break;
							}
						}
						for(;left<right;++left)
						{
							if(!selector(point{left,newy}))
							{
								break;
							}
						}
						if(range_found)
						{
							range_found=false;
							scan_ranges.push_back({range_start,left,newy,-current_range.direction});
						}
					}
				}
			}
		}
	}

	/*
		Does a scanline flood operation from a seed (DOES NOT KEEP TRACK OF SELECTED PIXELS ITSELF)
		img - img to flood op on
		start - where flood starts from
		selector - function that takes a PointUINT and returns whether it is a valid point, may do other stuff too
		dwl - takes in a horizontal_line<> that has been selected
	*/
	template<typename Img,typename Selector,typename DoWithLine>
	void flood_operation(Img&& img,ImageUtils::PointUINT start,Selector&& selector,DoWithLine&& dwl)
	{
		std::vector<detail::scan_range> scan_ranges;
		return detail::flood_operation(img,start,selector,dwl,scan_ranges);
	}

	/*
		Does a flood fill one 1 layer, writing in place assuming the replacer is not a valid point of the selector
	*/
	bool flood_fill_distinct_1layer(::cimg_library::CImg<unsigned char>& image,float const tolerance,ImageUtils::Grayscale const color,ImageUtils::Grayscale const replacer,ImageUtils::Point<unsigned int> start);

	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param vertical_padding, size in pixels of padding on top and bottom
		@param max_horizontal_padding
		@param min_horizontal_padding
		@param optimal_ratio
	*/
	bool auto_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const vertical_padding,unsigned int const max_horizontal_padding,unsigned int const min_horizontal_padding,signed int horiz_offset,float optimal_ratio=16.0f/9.0f,unsigned int tolerance=5,unsigned char background=200);
	
	bool cluster_padding(
		::cil::CImg<unsigned char>& img,
		unsigned int const left,
		unsigned int const right,
		unsigned int const top,
		unsigned int const bottom,
		unsigned char background_threshold);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
	*/
	bool horiz_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const padding);
	bool horiz_padding(::cimg_library::CImg<unsigned char>& img,unsigned int const left,unsigned int const right,unsigned int tolerance=5,unsigned char background=200,bool cumulative=true);
	/*
		Adds or removes paddings from all sides of the image
		@param image
		@param paddingSize, size in pixels of padding
	*/
	bool vert_padding(::cimg_library::CImg<unsigned char>& image,unsigned int const padding);
	bool vert_padding(::cimg_library::CImg<unsigned char>& img,unsigned int const top,unsigned int const bottom,unsigned int tolerance=5,unsigned char background=200,bool cumulative=true);

	void compress(
		::cimg_library::CImg<unsigned char>& image,
		unsigned int const min_padding,
		unsigned int const optimal_height,
		unsigned char background_threshold=254);

	/*

	*/
	void rescale_colors(::cimg_library::CImg<unsigned char>&,unsigned char min,unsigned char mid,unsigned char max=255);
}
template<typename T>
void ScoreProcessor::copy_shift_selection(cimg_library::CImg<T>& image,ImageUtils::Rectangle<unsigned int> selection,int const shiftx,int const shifty)
{
	if(shiftx==0&&shifty==0) return;
	selection.left=selection.left+shiftx;if(selection.left>image._width) selection.left=0;
	selection.right=selection.right+shiftx;if(selection.right>image._width) selection.right=image._width;
	selection.top=selection.top+shifty;if(selection.top>image._height) selection.top=0;
	selection.bottom=selection.bottom+shifty;if(selection.bottom>image._height) selection.bottom=image._height;
	unsigned int numChannels=image._spectrum;
	if(shiftx>0)
	{
		if(shifty>0) //shiftx>0 and shifty>0
		{
			for(unsigned int s=0;s<numChannels;++s)
			{
				for(unsigned int x=selection.right;x-->selection.left;)
				{
					for(unsigned int y=selection.bottom;y-->selection.top;)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else //shiftx>0 and shifty<=0
		{
			for(unsigned int s=0;s<numChannels;++s)
			{
				for(unsigned int x=selection.right;x-->selection.left;)
				{
					for(unsigned int y=selection.top;y<selection.bottom;++y)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
	else
	{
		if(shifty>0) //shiftx<=0 and shifty>0
		{
			for(unsigned int s=0;s<numChannels;++s)
			{
				for(unsigned int x=selection.left;x<selection.right;++x)
				{
					for(unsigned int y=selection.bottom;y-->selection.top;)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
		else //shiftx<=0 and shifty<=0
		{
			for(unsigned int s=0;s<numChannels;++s)
			{
				for(unsigned int x=selection.left;x<selection.right;++x)
				{
					for(unsigned int y=selection.top;y<selection.bottom;++y)
					{
						image(x,y,s)=image(x-shiftx,y-shifty,s);
					}
				}
			}
		}
	}
}
namespace ScoreProcessor {

	struct padding_pack {
		unsigned int left,right,top,bottom;
	};

	template<typename T,typename FindLeft,typename FindRight,typename FindTop,typename FindBottom>
	bool padding(::cil::CImg<T>& img,padding_pack pp,FindLeft fl,FindRight fr,FindTop ft,FindBottom fb)
	{
		signed int left,right,top,bottom;
		if(pp.left==-1)
		{
			left=0;
		}
		else
		{
			left=fl(img)-pp.left;
		}
		if(pp.right=-1)
		{
			right=img.width()-1;
		}
		else
		{
			right=fr(img)+pp.right;
		}
		if(pp.top==-1)
		{
			top=0;
		}
		else
		{
			top=ft(img)-pp.top;
		}
		if(pp.bottom=-1)
		{
			bottom=img.height()-1;
		}
		else
		{
			bottom=fr(img)+pp.bottom;
		}
		if(left>right)
		{
			std::swap(left,right);
		}
		if(top>bottom)
		{
			std::swap(top,bottom);
		}
		if(left==0&&top==0&&right==img.width()-1&&bottom==img.height()-1)
		{
			return false;
		}
		img=cil::get_crop_fill(img,ImageUtils::Rectangle<int>({left,right,top,bottom}));
		return true;
	}

	template<typename T>
	void fill_selection(::cimg_library::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,T const* color)
	{
		unsigned int const width=img._width;
		unsigned int const area=img._height*width;
		unsigned int const spectrum=img._spectrum;
		T* const data=img._data;
		for(unsigned int s=0;s<spectrum;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					row[x]=color[s];
				}
			}
		}
	}

	template<typename T,size_t N>
	void fill_selection(::cimg_library::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,std::array<T,N> color)
	{
		assert(img._spectrum>=N);
		assert(sel.right<img._width);
		assert(sel.bottom<img._height);
		unsigned int const width=img._width;
		unsigned int const area=width*img._height;
		T* const data=img._data;
		for(unsigned int s=0;s<N;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					row[x]=color[s];
				}
			}
		}
	}
	struct check_fill_t {};

	template<typename T,size_t N>
	bool fill_selection(::cil::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,std::array<T,N> color,check_fill_t)
	{
		assert(img._spectrum>=N);
		assert(sel.right<img._width);
		assert(sel.bottom<img._height);
		unsigned int const width=img._width;
		unsigned int const area=width*img._height;
		T* const data=img._data;
		bool edited=false;
		for(unsigned int s=0;s<N;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					if(row[x]!=color[s])
					{
						row[x]=color[s];
						edited=true;
					}
				}
			}
		}
		return edited;
	}

	template<typename T>
	bool fill_selection(::cimg_library::CImg<T>& img,ImageUtils::Rectangle<unsigned int> const sel,T const* color,check_fill_t)
	{
		unsigned int const width=img._width;
		unsigned int const area=img._height*width;
		unsigned int const spectrum=img._spectrum;
		T* const data=img._data;
		bool edited=false;
		for(unsigned int s=0;s<spectrum;++s)
		{
			T* const layer=data+s*area;
			for(unsigned int y=sel.top;y<sel.bottom;++y)
			{
				T* const row=layer+y*width;
				for(unsigned int x=sel.left;x<sel.right;++x)
				{
					if(row[x]!=color[s])
					{
						row[x]=color[s];
						edited=true;
					}
				}
			}
		}
		return edited;
	}

	inline void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::ColorRGB const color)
	{
		fill_selection(image,selection,std::array<unsigned char,3>({color.r,color.g,color.b}));
	}
	inline void fill_selection(::cimg_library::CImg<unsigned char>& image,ImageUtils::Rectangle<unsigned int> const selection,ImageUtils::Grayscale const gray)
	{
		fill_selection(image,selection,std::array<unsigned char,1>({gray}));
	}

	/*
		Copies a selection from the first image to the location of the second image
		Returns whether a copy-paste was done
	*/
	template<typename T>
	bool copy_paste(::cimg_library::CImg<T>& dest,::cimg_library::CImg<T> const& src,ImageUtils::Rectangle<unsigned int> select,ImageUtils::Point<signed int> destloc)
	{
		if(dest._spectrum<src._spectrum)
		{
			throw std::invalid_argument("Cannot copy to lower channel image");
		}
		if(destloc.x<0)
		{
			select.left-=destloc.x;
			destloc.x=0;
		}
		if(destloc.y<0)
		{
			select.top-=destloc.y;
			destloc.y=0;
		}
		if(unsigned int(destloc.x)+select.width()>=dest._width)
		{
			select.right=dest._width;
		}
		if(unsigned int(destloc.y)+select.height()>=dest._height)
		{
			select.bottom=dest._height;
		}
		if(select.right<=select.left||select.bottom<=select.top)
		{
			return false;
		}
		std::size_t const sheight=select.height();
		std::size_t const swidth=select.width();
		std::size_t const dwidth=dest._width;
		std::size_t const srcwidth=src._width;
		if(src._spectrum==dest._spectrum)
		{
			for(unsigned int s=0;s<src._spectrum;++s)
			{
				auto const dhead=&dest(destloc.x,destloc.y,0,s);
				auto const shead=&src(select.left,select.top,0,s);
				for(std::size_t y=0;y<sheight;++y)
				{
					std::memcpy(dhead+y*dwidth,shead+y*srcwidth,swidth*sizeof(T));
				}
			}
		}
		switch(src._spectrum)
		{
		case 1:
			switch(dest._spectrum)
			{
			case 4:
			{
				auto const dhead=&dest(destloc.x,destloc.y,0,3);
				for(std::size_t y=0;y<sheight;++y)
				{
					std::fill_n(dhead+y*dwidth,swidth,std::numeric_limits<T>::max());
				}
			}
			case 3:
			{
				auto const shead=&src(select.left,select.top);
				for(unsigned int s=0;s<dest._spectrum;++s)
				{
					auto const dhead=&dest(destloc.x,destloc.y,0,s);
					for(std::size_t y=0;y<sheight;++y)
					{
						std::memcpy(dhead+y*dwidth,shead+y*srcwidth,swidth*sizeof(T));
					}
				}
				return true;
			}
			}
		case 3:
			if(dest._spectrum==4)
			{
				for(std::size_t s=0;s<3;++s)
				{
					auto const dhead=&dest(destloc.x,destloc.y,0,s);
					auto const shead=&src(select.left,select.top,0,s);
					for(std::size_t y=0;y<sheight;++y)
					{
						std::memcpy(dhead+y*dwidth,shead+y*srcwidth,swidth*sizeof(T));
					}
				}
				auto const dhead=&dest(destloc.x,destloc.y,0,3);
				for(std::size_t y=0;y<sheight;++y)
				{
					std::fill_n(dhead+y*dwidth,swidth,std::numeric_limits<T>::max());
				}
			}
			return true;
		}
		throw std::invalid_argument("Unsupported spectrum conversion");
	}

	template<unsigned int num_layers,typename T,typename Selector>
	std::vector<ImageUtils::Rectangle<unsigned int>> global_select(
		::cil::CImg<T> const& image,
		Selector keep,bool compress=true)
	{
		static_assert(num_layers>0,"Positive number of layers required");
		assert(image._spectrum>=num_layers);
		std::array<T,num_layers> color;
		unsigned int range_found=0,range_start=0,range_end=0;
		auto const height=image._height;
		auto const width=image._width;
		size_t const size=height*width;
		auto const data=image._data;
		std::vector<ImageUtils::Rectangle<unsigned int>> container;
		for(unsigned int y=0;y<height;++y)
		{
			auto const row=data+y*width;
			for(unsigned int x=0;x<width;++x)
			{
				auto const pix=row+x;
				for(unsigned int i=0;i<num_layers;++i)
				{
					color[i]=*(pix+i*size);
				}
				switch(range_found)
				{
				case 0:
				{
					if(keep(color))
					{
						range_found=1;
						range_start=x;
					}
					break;
				}
				case 1:
				{
					if(!keep(color))
					{
						range_found=2;
						range_end=x;
					}
					else
					{
						break;
					}
				}
				case 2:
				{
					container.push_back(ImageUtils::Rectangle<unsigned int>{range_start,range_end,y,y+1});
					range_found=0;
					break;
				}
				}
			}
			if(1==range_found)
			{
				container.push_back(ImageUtils::Rectangle<unsigned int>{range_start,image._width,y,y+1});
				range_found=0;
			}
		}
		if(compress)
		{
			ImageUtils::compress_rectangles(container);
		}
		return container;
	}

	template<typename T,size_t NL,typename PixelSelectorArrayNLToBool,typename ClusterToTrueIfClear>
	bool clear_clusters(
		::cil::CImg<T>& img,
		std::array<T,NL> replacer,
		PixelSelectorArrayNLToBool ps,
		ClusterToTrueIfClear cl)
	{
		assert(img._spectrum>=NL);
		auto rects=global_select<NL>(img,ps);
		auto clusters=ScoreProcessor::Cluster::cluster_ranges(rects);
		bool edited=false;
		for(auto it=clusters.cbegin();it!=clusters.cend();++it)
		{
			if(cl(*it))
			{
				edited=true;
				for(auto rect:it->get_ranges())
				{
					ScoreProcessor::fill_selection(img,rect,replacer);
				}
			}
		}
		return edited;
	}

	template<typename T,size_t NL,typename PixelSelectorArrayNLToBool,typename ClusterToTrueIfClear>
	bool clear_clusters_8way(
		::cil::CImg<T>& img,
		std::array<T,NL> replacer,
		PixelSelectorArrayNLToBool ps,
		ClusterToTrueIfClear cl)
	{
		assert(img._spectrum>=NL);
		auto rects=global_select<NL>(img,ps);
		auto clusters=ScoreProcessor::Cluster::cluster_ranges_8way(rects);
		bool edited=false;
		for(auto it=clusters.cbegin();it!=clusters.cend();++it)
		{
			if(cl(*it))
			{
				edited=true;
				for(auto rect:it->get_ranges())
				{
					ScoreProcessor::fill_selection(img,rect,replacer);
				}
			}
		}
		return edited;
	}

	//a function specific to fixing a problem with my scanner
	//eval_side: left is false, true is right
	//eval_direction: from top is false, true is from bottom
	void horizontal_shift(cil::CImg<unsigned char>& img,bool eval_side,bool eval_direction,unsigned char background_threshold);

	void vertical_shift(cil::CImg<unsigned char>& img,bool eval_bottom,bool from_right,unsigned char background_threshold);

}
#endif // !1
