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
#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H
#include "CImg.h"
#include <vector>
#include <memory>
#include <utility>
#include <string>
#include "lib\threadpool\ThreadPool.h"
#include "lib\exstring\exstring.h"
#include <stdexcept>
#include <regex>
#include <functional>
#include "lib\exstring\exmath.h"
#include "lib\exstring\exfiles.h"
#include <fstream>
#include <filesystem>
#include <string_view>
#include "support.h"
namespace ScoreProcessor {
	template<typename T=unsigned char>
	/*
		Represents processes done to an image.
	*/
	class ImageProcess {
	public:
		typedef cimg_library::CImg<T> Img;
		virtual ~ImageProcess()
		{};
		//returns true if the image has been modified
		virtual bool process(Img&) const=0;
	};
	/*
		Logs to some output.
	*/
	class Log {
	public:
		virtual ~Log()=default;
		virtual void log(char const* msg,size_t msg_len,size_t id)=0;
		virtual void log_error(char const* msg,size_t msg_len,size_t id)=0;
		inline void log(std::string_view sv,size_t id)
		{
			log(sv.data(),sv.length(),id);
		}
		inline void log_error(std::string_view sv,size_t id)
		{
			log_error(sv.data(),sv.length(),id);
		}
	};

	/*
		Template for creating an output name based on an input name.
	*/
	class SaveRules {
	private:
		enum template_symbol {
			string=-1,i=0,padding_min=0,padding_max=9,x=10,f,p,c,w
		};
		struct part {
			char* data;
			union {
				size_t size;
				unsigned int tmplt;
			} info;
			part(part&& o):data(o.data),info(o.info)
			{
				o.data=0;
			}
			part(unsigned int tmplt):data(0)
			{
				info.tmplt=tmplt;
			}
			part(exlib::string& str):
				data(str.data())
			{
				info.size=str.size();
				str.release();
			}
			~part()
			{
				delete[] data;
			}
		};
		std::vector<part> parts;
	public:
		/*
			Makes a string out of the input, based on the current template.
		*/
		template<typename String>
		std::string make_filename(String const& input,unsigned int index=1) const;

		std::string make_filename(char const* input,unsigned int index=1) const;
		/*
			%f to match filename
			%x to match extension
			%p to match path
			%c to match entirety of input
			%0 for index, with any number 0-9 for amount of padding
		*/
		template<typename String>
		SaveRules(String const& tmplt);
		SaveRules(char const* tmplt);
		SaveRules();

		/*
			%f to match filename
			%x to match extension
			%p to match path
			%c to match entirety of input
			%0 for index, with any number 0-9 for amount of padding
		*/
		template<typename String>
		void assign(String const& tmplt);
		void assign(char const* tmplt);

		/*
			Whether the current template is empty.
		*/
		bool empty() const;

		/*
			Whether template contains given templating.
		*/
		bool contains(template_symbol ts) const;

		bool contains_indexing() const;

		template<typename String>
		bool contains(String const& str) const;

	};

	/*
		A series of processes to be done onto an image.
	*/
	template<typename T=unsigned char>
	class ProcessList:public std::vector<std::unique_ptr<ImageProcess<T>>> {
	public:
		enum verbosity {
			silent,
			errors_only,
			loud
		};
	private:

		class ProcessTaskImg:public exlib::ThreadTask {
		private:
			cimg_library::CImg<T>* pimg;
			ProcessList<T> const* pparent;
			SaveRules const* output;
			unsigned int index;
		public:
			ProcessTaskImg(cimg_library::CImg<T>* pimg,ProcessList<T> const* pparent,SaveRules const* output,unsigned int index):
				pimg(pimg),pparent(pparent),output(output),index(index)
			{}
			void execute() override
			{
				pparent->process(*pimg,output,index);
			}
		};
		class ProcessTaskFName:public exlib::ThreadTask {
		private:
			char const* fname;
			ProcessList<T> const* pparent;
			SaveRules const* output;
			unsigned int index;
			bool move;
		public:
			ProcessTaskFName(char const* fname,ProcessList<T> const* pparent,SaveRules const* output,unsigned int index,bool move):
				fname(fname),pparent(pparent),output(output),index(index),move(move)
			{}
			void execute() override
			{
				pparent->process(fname,output,index,move);
			}
		};
		Log* plog;
		verbosity vb;
	public:
		ProcessList(Log* log,verbosity vb):plog(log),vb(vb)
		{}
		ProcessList(Log* log):ProcessList(log,1)
		{}
		ProcessList():ProcessList(nullptr,silent)
		{}

		void set_log(Log* log)
		{
			plog=log;
		}

		Log* get_log() const
		{
			return plog;
		}

		verbosity get_verbosity() const
		{
			return vb;
		}

		void set_verbosity(verbosity vb)
		{
			this->vb=vb;
		}
		/*
			Adds a process to the list.
		*/
		template<typename U,typename... Args>
		void add_process(Args&&... args);

		void process_unsafe(cimg_library::CImg<T>& img,char const* output) const;
		void process_unsafe(char const* input,char const* output,bool move=false) const;
		/*
			Processes an image.
		*/
		void process(cimg_library::CImg<T>& img) const;
		/*
			Processes an image and saves it to the output.
		*/
		void process(cimg_library::CImg<T>& img,char const* output) const;
		/*
			Processes an image and saves it to the output, based on the SaveRules.
			As this method has no information about the input, it passes an empty string to the SaveRules.
		*/
		void process(cimg_library::CImg<T>& img,SaveRules const* psr,unsigned int index=1) const;

		/*
			Processes an image at the given filename, and saves it in place.
		*/
		void process(char const* filename) const;
		/*
			Processes an image at the given filename, and saves it to the output.
		*/
		void process(char const* filename,char const* output,bool move=false) const;
		/*
			Processes an image at the given filename, and saves it based on the SaveRules.
			Pass nullptr to SaveRules if you do not want it saved (useless, but ok).
		*/
		void process(char const* filename,SaveRules const* psr,unsigned int index=1,bool move=false) const;

		/*
			Processes all the images in the vector.
			Pass nullptr to psr if you do not want the imgs to be saved.
			Might add iterator version one day.
		*/
		void process(std::vector<cimg_library::CImg<T>>& files,
			SaveRules const* psr,
			unsigned int const num_threads=std::thread::hardware_concurrency(),
			unsigned int const starting_index=1) const;

		/*
			Processes all the images in the vector.
			Might add iterator version one day.
		*/
		template<typename String>
		void process(std::vector<String> const& filenames,
			SaveRules const* psr,
			unsigned int const num_threads=std::thread::hardware_concurrency(),
			unsigned int const starting_index=1,
			bool move=false) const;

		/*
			Processes all the images in the vector.
			Might add iterator version one day.
		*/
		void process(std::vector<char*> const& filenames,
			SaveRules const* psr,
			unsigned int const num_threads=std::thread::hardware_concurrency(),
			unsigned int const starting_index=1,
			bool move=false) const;
	};
	typedef ProcessList<unsigned char> IPList;

	template<typename T>
	template<typename U,typename... Args>
	void ProcessList<T>::add_process(Args&&... args)
	{
		emplace_back(std::make_unique<U>(std::forward<Args>(args)...));
	}

	template<typename T>
	void ProcessList<T>::process(cimg_library::CImg<T>& img) const
	{
		process(img,nullptr);
	}

	template<typename T>
	void ProcessList<T>::process_unsafe(cimg_library::CImg<T>& img,char const* output) const
	{
		for(auto& pprocess:*this)
		{
			pprocess->process(img);
		}
		if(output!=nullptr)
		{
			img.save(output);
		}
	}

	template<typename T>
	void ProcessList<T>::process(cimg_library::CImg<T>& img,char const* output) const
	{
		try
		{
			process_unsafe(img,output);
		}
		catch(std::exception const& ex)
		{
			if(plog)
			{
				if(vb)
				{
					std::string log(ex.what());
					log.push_back('\n');
					plog->log_error(log,0);
				}
			}
			else
			{
				throw ex;
			}
		}
	}

	template<typename T>
	void ProcessList<T>::process(cimg_library::CImg<T>& img,SaveRules const* psr,unsigned int index) const
	{
		if(psr==nullptr)
		{
			process(img);
		}
		else
		{
			auto outname=psr->make_filename("",index);
			process(img,outname.c_str());
		}
	}

	template<typename T>
	void ProcessList<T>::process(char const* fname) const
	{
		process(fname,fname);
	}

	template<typename T>
	void ProcessList<T>::process_unsafe(char const* fname,char const* output,bool do_move) const
	{
		using namespace std::experimental::filesystem;
		path in(fname),out(output);
		if(!exists(in))
		{
			throw std::runtime_error(std::string("Failed to open ").append(fname,in.native().size()));
		}
		auto const& instr=in.native();
		auto in_ext=exlib::find_extension(instr.cbegin(),instr.cend());
		auto const& outstr=out.native();
		auto out_ext=exlib::find_extension(outstr.cbegin(),outstr.cend());
		auto support=[in_ext,out_ext,in_end=instr.end(),out_end=outstr.end()]()
		{
			auto make_ext_string=[](auto begin,auto end)
			{
				size_t shorts=std::distance(begin,end);
				std::string ext;ext.resize(shorts);
				auto eit=ext.begin();
				for(auto it=begin;it!=end;++eit,++it)
				{
					*eit=*it;
				}
				return ext;
			};
			auto sout=supported(&*out_ext);
			auto sin=supported(&*in_ext);
			if(sout==support_type::no)
			{
				throw std::invalid_argument(std::string("Unsupported file type ")+make_ext_string(out_ext,out_end));
			}
			if(sin==support_type::no)
			{
				throw std::invalid_argument(std::string("Unsupported file type ")+make_ext_string(in_ext,in_end));
			}
			return std::make_pair(sin,sout);
		};
		auto copy_or_move=[do_move,&in,&out,fname,output]()
		{
			if(std::experimental::filesystem::equivalent(in,out))
			{
				return;
			}
			if(do_move)
			{
				try
				{
					rename(in,out);
				}
				catch(std::exception const&)
				{
					throw std::runtime_error(std::string(std::string("Failed to move to ").append(output,out.native().size())));
				}
			}
			else
			{
				std::ifstream src(fname,std::ios::binary);
				if(!src)
				{
					throw std::runtime_error(std::string("Failed to open ").append(fname,in.native().size()));
				}
				std::ofstream dst(output,std::ios::binary);
				if(!dst)
				{
					throw std::runtime_error(std::string("Failed to copy to ").append(output,out.native().size()));
				}
				dst<<src.rdbuf();
			}
		};
		if(empty())
		{
			if(!exlib::strncmp_nocase(in_ext,out_ext))
			{
				copy_or_move();
			}
			else
			{
				auto s=support();
				if(s.first==s.second)
				{
					copy_or_move();
				}
				else
				{
					cil::CImg<T> img;
					switch(s.first)
					{
						case support_type::bmp:
							img.load_bmp(fname);
							break;
						case support_type::jpeg:
							img.load_jpeg(fname);
							break;
						case support_type::png:
							img.load_png(fname);
					}
					switch(s.second)
					{
						case support_type::bmp:
							img.save_bmp(output);
							break;
						case support_type::jpeg:
							img.save_jpeg(output);
							break;
						case support_type::png:
							img.save_png(output);
					}
					if(do_move&&!std::experimental::filesystem::equivalent(in,out))
					{
						remove(in);
					}
				}
			}
		}
		else
		{
			auto s=support();
			bool edited=false;
			{
				cil::CImg<T> img;
				switch(s.first)
				{
					case support_type::bmp:
						img.load_bmp(fname);
						break;
					case support_type::jpeg:
						img.load_jpeg(fname);
						break;
					case support_type::png:
						img.load_png(fname);
				}
				for(auto it=this->begin();it<this->end();++it)
				{
					edited|=(*it)->process(img);
				}
				if(s.first!=s.second)
				{
					edited=true;
				}
				if(edited)
				{
					switch(s.second)
					{
						case support_type::bmp:
							img.save_bmp(output);
							break;
						case support_type::jpeg:
							img.save_jpeg(output);
							break;
						case support_type::png:
							img.save_png(output);
					}
					if(do_move&&!std::experimental::filesystem::equivalent(in,out))
					{
						remove(in);
					}
				}
			}
			if(!edited)
			{
				copy_or_move();
			}
		}
	}

	template<typename T>
	void ProcessList<T>::process(char const* fname,char const* output,bool move) const
	{
		try
		{
			process_unsafe(fname,output);
		}
		catch(std::exception const& ex)
		{
			if(plog)
			{
				if(vb)
				{
					plog->log_error(ex.what(),0);
				}
			}
			else
			{
				throw ex;
			}
		}
	}

	template<typename T>
	void ProcessList<T>::process(char const* filename,SaveRules const* psr,unsigned int index,bool move) const
	{
		size_t len=strlen(filename);
		bool out_loud=plog&&vb>=decltype(vb)::loud;
		if(out_loud)
		{
			std::string log("Starting ");
			log.append(filename,len);
			log.push_back('\n');
			plog->log(log,index);
		}
		{
			decltype(psr->make_filename(std::string_view(),index)) output;
			output=psr?psr->make_filename(std::string_view(filename,len),index):filename;
			try
			{
				process_unsafe(filename,output.c_str(),move);
			}
			catch(std::exception const& ex)
			{
				if(plog)
				{
					if(vb)
					{
						std::string log("Error processing ");
						log.append(filename,len);
						log.append(": ",2);
						log.append(ex.what());
						log.push_back('\n');
						plog->log_error(log,index);
					}
					return;
				}
				else
				{
					throw ex;
				}
			}
		}
		if(out_loud)
		{
			std::string log("Finished ");
			log.append(filename,len);
			log.push_back('\n');
			plog->log(log,index);
		}
	}

	template<typename T>
	void ProcessList<T>::process(
		std::vector<char*> const& imgs,
		SaveRules const* psr,
		unsigned int const num_threads,
		unsigned int const starting_index,
		bool move) const
	{
		exlib::ThreadPool tp(num_threads);
		for(size_t i=0;i<imgs.size();++i)
		{
			tp.add_task<typename ProcessList<T>::ProcessTaskFName>(imgs[i],this,psr,i+starting_index);
		}
		tp.start();
	}

	template<typename T>
	template<typename String>
	void ProcessList<T>::process(
		std::vector<String> const& imgs,
		SaveRules const* psr,
		unsigned int const num_threads,
		unsigned int const starting_index,
		bool move) const
	{
		exlib::ThreadPool tp(num_threads);
		for(size_t i=0;i<imgs.size();++i)
		{
			tp.add_task<typename ProcessList<T>::ProcessTaskFName>(imgs[i].c_str(),this,psr,i+starting_index,move);
		}
		tp.start();
	}

	template<typename T>
	void ProcessList<T>::process(
		std::vector<cimg_library::CImg<T>>& imgs,
		SaveRules const* psr,
		unsigned int const num_threads,
		unsigned int const starting_index) const
	{
		exlib::ThreadPool tp(num_threads);
		for(size_t i=0;i<imgs.size();++i)
		{
			tp.add_task<typename ProcessList<T>::ProcessTaskImg>(imgs[i],this,psr,i+starting_index);
		}
		tp.start();
	}

	template<typename String>
	SaveRules::SaveRules(String const& tmplt)
	{
		assign(tmplt.c_str());
	}

	inline SaveRules::SaveRules(char const* tmplt)
	{
		assign(tmplt);
	}

	inline SaveRules::SaveRules()
	{}

	template<typename String>
	void SaveRules::assign(String const& tmplt)
	{
		assign(tmplt.c_str());
	}

	inline void SaveRules::assign(char const* tmplt)
	{
		std::vector<part> repl;
		bool found=false;
		exlib::string str(20);
		auto put_string=[&]()
		{
			repl.emplace_back(str);
			str.reserve(20);
		};
		for(;*tmplt!=0;++tmplt)
		{
			if(found)
			{
				found=false;
				char letter=*tmplt;
				if(letter>='0'&&letter<='9')
				{
					put_string();
					repl.emplace_back(letter-'0');
				}
				else
				{
					switch(letter)
					{
						case 'x':
							put_string();
							repl.emplace_back(template_symbol::x);
							break;
						case 'f':
							put_string();
							repl.emplace_back(template_symbol::f);
							break;
						case 'p':
							put_string();
							repl.emplace_back(template_symbol::p);
							break;
						case 'c':
							put_string();
							repl.emplace_back(template_symbol::c);
							break;
						case 'w':
							put_string();
							repl.emplace_back(template_symbol::w);
							break;
						case '%':
							str.push_back('%');
							break;
						default:
							throw std::invalid_argument("Invalid escape character");
					}
				}
			}
			else
			{
				if(*tmplt=='%')
				{
					found=true;
				}
				else
				{
					str.push_back(*tmplt);
				}
			}
		}
		if(found)
		{
			throw std::invalid_argument("Trailing escape symbol");
		}
		if(!str.empty())
		{
			repl.emplace_back(str);
		}
		parts=std::move(repl);
	}

	template<typename String>
	std::string SaveRules::make_filename(String const& input,unsigned int index) const
	{
		std::string out;
		struct string_view {
			char const* data;
			size_t size;
		};
		string_view ext{nullptr,0};
		string_view filename{nullptr,0};
		string_view path{nullptr,0};
		string_view whole{nullptr,0};
		auto check_ext=[&]()
		{
			if(ext.data==nullptr)
			{
				ext.data=&*exlib::find_extension(input.cbegin(),input.cend());
				ext.size=&*input.cend()-ext.data;
			}
		};
		auto check_filename=[&]()
		{
			if(filename.data==nullptr)
			{
				check_ext();
				filename.data=exlib::find_filename(&*input.cbegin(),ext.data);
				size_t fsize=ext.data-filename.data;
				if(*(ext.data-1)=='.')
				{
					--fsize;
				}
				filename.size=fsize;
				whole.data=filename.data;
				whole.size=(&*input.cend())-whole.data;
			}
		};
		auto check_path=[&]()
		{
			if(path.data==nullptr)
			{
				check_filename();
				path.data=input.data();
				path.size=exlib::find_path_end(path.data,filename.data)-path.data;
			}
		};
		for(auto const& p:parts)
		{
			if(p.data)
			{
				out.append(p.data,p.info.size);
			}
			else
			{
				if(p.info.tmplt<10)
				{
					char buffer[10];
					char* const last=buffer+10;
					char* end=last-1;
					auto n=index;
					while(true)
					{
						*end=n%10+'0';
						n/=10;
						if(n==0) break;
						--end;
					}
					char* start=last-p.info.tmplt;
					if(start<end)
					{
						*start='0';
						for(char* it=start+1;it<end;++it)
						{
							*it='0';
						}
					}
					else
					{
						start=end;
					}
					out.append(start,size_t(last-start));
				}
				else
				{
					switch(p.info.tmplt)
					{
						case SaveRules::template_symbol::x:
							check_ext();
							out.append(ext.data,ext.size);
							break;
						case SaveRules::template_symbol::f:
							check_filename();
							out.append(filename.data,filename.size);
							break;
						case SaveRules::template_symbol::p:
							check_path();
							if(path.size==0)
							{
								out.append(".");
							}
							else
							{
								out.append(path.data,path.size);
							}
							break;
						case SaveRules::template_symbol::w:
							check_filename();
							out.append(whole.data,whole.size);
							break;
						case SaveRules::template_symbol::c:
							out.append(input.data(),input.size());
							break;
					}
				}
			}
		}
		return out;
	}

	inline std::string SaveRules::make_filename(char const* input,unsigned int index) const
	{
		return make_filename(exlib::weak_string(const_cast<char*>(input)),index);
	}

	inline bool SaveRules::empty() const
	{
		return parts.empty();
	}

	/*
	Whether template contains given templating.
	*/
	inline bool SaveRules::contains(SaveRules::template_symbol ts) const
	{
		if(ts==template_symbol::string)
		{
			for(auto const& prt:this->parts)
			{
				if(prt.data!=0)
				{
					return true;
				}
			}
		}
		else
		{
			for(auto const& prt:this->parts)
			{
				if(prt.data==0&&prt.info.tmplt==ts)
				{
					return true;
				}
			}
		}
		return false;
	}

	inline bool SaveRules::contains_indexing() const
	{
		for(auto const& prt:this->parts)
		{
			if(prt.data==0&&prt.info.tmplt>=padding_min&&prt.info.tmplt<=padding_max)
			{
				return true;
			}
		}
	}

	template<typename String>
	bool SaveRules::contains(String const& str) const
	{
		for(auto const& prt:this->parts)
		{
			if(prt.data!=0&&str==prt.data)
			{
				return true;
			}
		}
	}
}
#endif
