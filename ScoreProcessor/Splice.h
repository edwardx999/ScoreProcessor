#ifndef SPLICE_H
#define SPLICE_H
#include "CImg.h"
#include <mutex>
#include <vector>
#include <string>
#include "lib/threadpool/thread_pool.h"
#include "lib/exstring/exmath.h"
#include <array>
namespace ScoreProcessor {

	//Anything in namespace Splice, except standard_heurstics, you should not access directly
	namespace Splice {

		//class used to manage when files are open and closed by splice
		class manager {
		private:
			cil::CImg<unsigned char> _img;
			char const* filename;
			unsigned int times_used=0;
			std::mutex guard;
		public:
			[[nodiscard]]
			inline cil::CImg<unsigned char> const& img() const
			{
				return _img;
			}
			[[nodiscard]]
			inline char const* fname() const
			{
				return filename;
			}
			inline void fname(char const* fn)
			{
				filename=fn;
			}
			inline void load()
			{
				std::lock_guard<std::mutex> locker(guard);
				if(_img._data==0)
				{
					_img.load(filename);
					if(_img._spectrum==2)
					{
						cil::CImg<unsigned char> temp(_img._width,_img._height,1,4);
						size_t const size=size_t{temp._width}*temp._height;
						std::memcpy(temp.data(),_img.data(),size);
						std::memcpy(temp.data()+size,_img.data(),size);
						std::memcpy(temp.data()+2*size,_img.data(),size);
						std::memset(temp.data()+3*size,255,size);
						_img=std::move(temp);
					}
				}
			}
			inline void finish()
			{
				std::lock_guard<std::mutex> locker(guard);
				++times_used;
				if(times_used==2)
				{
					delete[] _img._data;
					_img._data=0;
				}
			}
		};

		//descriptor of a page's padding and height
		struct page_layout {
			unsigned int padding;
			unsigned int height;
		};

		//descriptor of an image's true top and bottom (where actual image data is) as opposed to its raw size
		struct page {
			cil::CImg<unsigned char> img;
			unsigned int top;
			unsigned int bottom;
			unsigned int true_height() const
			{
				return bottom-top;
			}
		};

		//the top or bottom of a page as found by kerning images together or plainly finding where the image data starts
		struct edge {
			unsigned int raw;
			unsigned int kerned;
		};

		//top and bottom edges of a page
		struct page_desc {
			edge top;
			edge bottom;
		};

		//
		struct page_break {
			size_t index;
			unsigned int padding;
		};

		//template class used to find the approprate edges of tops and bottoms
		//and descriptors between middle pages
		template<typename Top,typename Middle,typename Bottom>
		class PageEval:private Top,private Middle,private Bottom {
		public:
			PageEval(Top t,Middle m,Bottom b):Top(t),Middle(m),Bottom(b)
			{}
			edge eval_top(cil::CImg<unsigned char> const& img) const
			{
				return Top::operator()(img);
			}
			page_desc eval_middle(cil::CImg<unsigned char> const& top,cil::CImg<unsigned char> const& bottom) const
			{
				return Middle::operator()(top,bottom);
			}
			edge eval_bottom(cil::CImg<unsigned char> const& img) const
			{
				return Bottom::operator()(img);
			}
		};
		template<typename Top,typename Middle,typename Bottom>
		PageEval(Top,Middle,Bottom)->PageEval<Top,Middle,Bottom>;
	}

	//splices together the pages pointed to by imgs, padded apart by padding
	cil::CImg<unsigned char> splice_images(Splice::page const* imgs,size_t num,unsigned int padding);

	//returns breaks in backwards order
	//determines where page breaks should go using Knuth word-wrap algorithm, based on given cost function, and
	//way of layout out pages
	template<typename PageDescIter,typename CreateLayout,typename Cost>
	std::vector<Splice::page_break> nongreedy_break(PageDescIter begin,PageDescIter end,CreateLayout cl,Cost cost)
	{
		struct node {
#ifdef _WIN64
			double
#else 
			float
#endif
				cost;
			Splice::page_layout layout;
			size_t previous;
		};
		size_t const c=end-begin;
		std::vector<node> nodes(c+1);
		nodes[0].cost=0;
		for(size_t i=1;i<=c;++i)
		{
			nodes[i].cost=INFINITY;
			for(size_t j=i-1;;)
			{
				auto layout=cl(&begin[j],i-j);
				auto local_cost=cost(layout);
				auto total_cost=local_cost+nodes[j].cost;
				if(total_cost<=nodes[i].cost)
				{
					nodes[i].cost=total_cost;
					nodes[i].previous=j;
					nodes[i].layout=layout;
				}
				else
				{
					break;
				}
				if(j==0)
				{
					break;
				}
				--j;
			}
		}
		std::vector<Splice::page_break> breaks;
		breaks.reserve(c);
		size_t index=c;
		do
		{
			breaks.push_back(Splice::page_break{index,nodes[index].layout.padding});
			index=nodes[index].previous;
		} while(index);
		return breaks;
	}

	//splices together the non-greedily and multi-threadedly, based on the given page descriptors and evaluators
	//returns the number of pages spliced together
	template<typename EvalPage,typename CreateLayout,typename Cost>
	unsigned int splice_pages_parallel(
		std::vector<Splice::manager>& files,
		char const* output,
		unsigned int starting_index,
		unsigned int num_threads,
		EvalPage ep,
		CreateLayout cl,
		Cost cost,
		int quality)
	{
		auto const c=files.size();
		if(c<2)
		{
			throw std::invalid_argument("Need multiple pages to splice");
		}
		auto const extension=exlib::find_extension(output,output+std::strlen(output));
		validate_extension(extension);
		std::string error_log;
		std::mutex error_mutex;
		std::vector<Splice::page_desc> page_descs(c);
		exlib::thread_pool pool(num_threads);
		using parent_ref=typename decltype(pool)::parent_ref;
		auto send_error=[&error_mutex,&error_log](auto const& err,auto parent,auto filename)
		{
			{
				std::lock_guard lock{error_mutex};
				error_log.append(filename).append(": ").append(err.what()).append("\n");
			}
			parent.stop();
		};
		pool.push_back([work=files.data(),output=page_descs.data(),ep,send_error](parent_ref parent) noexcept{
			try
			{
				work->load();
				Splice::edge res=ep.eval_top(work->img());
				output->top=res;
				work->finish();
			}
			catch(std::exception const& ex)
			{
				send_error(ex,parent,work->fname());
			}
		});
		for(size_t i=1;i<c;++i)
		{
			pool.push_back([work=files.data()+i,output=page_descs.data()+i,ep,send_error](parent_ref parent) noexcept
			{
				
				try
				{
					(work-1)->load();
				}
				catch(std::exception const& err)
				{
					send_error(err,parent,(work-1)->fname());
					return;
				}
				try
				{
					work->load();
				}
				catch(std::exception const& err)
				{
					send_error(err,parent,work->fname());
					return;
				}
				try
				{
					Splice::page_desc res=ep.eval_middle((work-1)->img(),work->img());
					(output-1)->bottom=res.bottom;
					(output)->top=res.top;
					(work-1)->finish();
					work->finish();
				}
				catch(std::exception const& err)
				{
					std::string names=(work-1)->fname();
					names+=" and ";
					names+=work->fname();
					send_error(err,parent,names.c_str());
				}
			});
		}
		pool.push_back([work=files.data()+c-1,output=page_descs.data()+c-1,ep,send_error](parent_ref parent)noexcept{
			try
			{
				work->load();
				Splice::edge res=ep.eval_bottom(work->img());
				output->bottom=res;
				work->finish();
			}
			catch(std::exception const& ex)
			{
				send_error(ex,parent,work->fname());
			}
		});
		pool.wait();
		if(!error_log.empty())
		{
			throw std::logic_error(error_log);
		}

		std::vector<Splice::page_break> breaks=nongreedy_break(page_descs.begin(),page_descs.end(),cl,cost);
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
				[index=num_imgs,
				num_digs,
				output,
				fbegin=files.data()+start,
				ibegin=page_descs.data()+start,
				num_pages=s,
				padding=breaks[i].padding,
				send_error,
				quality](parent_ref parent) noexcept{
				try
				{
					std::vector<Splice::page> imgs(num_pages);
					for(size_t i=0;i<num_pages;++i)
					{
						imgs[i].img.load(fbegin[i].fname());
						imgs[i].top=ibegin[i].top.kerned;
						imgs[i].bottom=ibegin[i].bottom.kerned;
					}
					imgs[0].top=ibegin[0].top.raw;
					auto const last=num_pages-1;
					imgs[last].bottom=ibegin[last].bottom.raw;
					auto const support=supported_path(output);
					auto const filename=cil::number_filename(output,index,num_digs);
					cil::save_image(splice_images(imgs.data(),num_pages,padding),filename.c_str(),support,quality);
				}
				catch(std::exception const& ex)
				{
					std::string names{fbegin[0].fname()};
					names.append(" to ").append(fbegin[num_pages-1].fname());
					send_error(ex,parent,names.data());
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
			throw std::logic_error(error_log);
		}
		return num_imgs;
	}

	template<typename EvalPage,typename CreateLayout,typename Cost>
	unsigned int splice_pages_parallel(
		std::vector<std::string> const& filenames,
		char const* output,
		unsigned int starting_index,
		unsigned int num_threads,
		EvalPage ep,
		CreateLayout cl,
		Cost c,
		int quality)
	{
		std::vector<Splice::manager> managers(filenames.size());
		for(size_t i=0;i<filenames.size();++i)
		{
			managers[i].fname(filenames[i].c_str());
		}
		return splice_pages_parallel(managers,output,starting_index,num_threads,ep,cl,c,quality);
	}

	template<typename EvalPage,typename CreateLayout,typename Cost>
	unsigned int splice_pages(
		std::vector<std::string> const& filenames,
		char const* output,
		unsigned int starting_index,
		unsigned int num_threads,
		EvalPage ep,
		CreateLayout cl,
		Cost c)
	{
		auto const c=files.size();
		if(c<2)
		{
			throw std::invalid_argument("Need multiple pages to splice");
		}
		std::vector<Splice::page_desc> page_descs(c);
		page_descs.front().top=ep.eval_middle()
	}

	namespace Splice {
		using pv=exlib::maybe_fixed<unsigned int>;
		struct standard_heuristics {
			unsigned char background_color;
			pv horiz_padding;
			pv optimal_height;
			pv optimal_padding;
			pv min_padding;
			float excess_weight;
			float padding_weight;
		};
	}

	//splices together images using the standard heuristics and dif^3 cost algorithm
	//cost is 
	unsigned int splice_pages_parallel(
		std::vector<std::string> const& filenames,
		char const* output,
		unsigned int starting_index,
		unsigned int num_threads,
		Splice::standard_heuristics const&,
		int quality);

}
#endif