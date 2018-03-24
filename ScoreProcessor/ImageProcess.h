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
		virtual void process(Img&)=0;
	};
	/*
		Logs to some output.
	*/
	class Log {
	public:
		virtual ~Log()=default;
		virtual void log(char const* message,size_t id)=0;
		virtual void log_error(char const* message,size_t id)=0;
	};

	/*
		Template for creating an output name based on an input name.
	*/
	class SaveRules {
	private:
		enum template_symbol:unsigned int {
			i,x=10,f,p,c,w
		};
		union part {
			struct {
				char* data;
				size_t size;
			} fixed;
			struct {
				size_t is_fixed;//relies on weak_string having format [data_pointer,size]
				size_t tmplt;
			};
			part(part&& o):is_fixed(o.is_fixed),tmplt(o.tmplt)
			{
				o.is_fixed=0;
			}
			part(unsigned int tmplt):is_fixed(0),tmplt(tmplt)
			{}
			part(exlib::string& str):
				fixed({str.data(),str.size()})
			{
				str.release();
			}
			~part()
			{
				delete[] fixed.data;
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
		public:
			ProcessTaskFName(char const* fname,ProcessList<T> const* pparent,SaveRules const* output,unsigned int index):
				fname(fname),pparent(pparent),output(output),index(index)
			{}
			void execute() override
			{
				pparent->process(fname,output,index);
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
		void process_unsafe(char const* input,char const* output) const;
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
		void process(char const* filename,char const* output) const;
		/*
			Processes an image at the given filename, and saves it based on the SaveRules.
			Pass nullptr to SaveRules if you do not want it saved (useless, but ok).
		*/
		void process(char const* filename,SaveRules const* psr,unsigned int index=1) const;

		/*
			Processes all the images in the vector.
			Pass nullptr to psr if you do not want the imgs to be saved.
			Might add iterator version one day.
		*/
		void process(std::vector<cimg_library::CImg<T>>& files,
			SaveRules const* psr,
			unsigned int num_threads=std::thread::hardware_concurrency()) const;

		/*
			Processes all the images in the vector.
			Might add iterator version one day.
		*/
		template<typename String>
		void process(std::vector<String> const& filenames,
			SaveRules const* psr,
			unsigned int num_threads=std::thread::hardware_concurrency()) const;

		/*
			Processes all the images in the vector.
			Might add iterator version one day.
		*/
		void process(std::vector<char*> const& filenames,
			SaveRules const* psr,
			unsigned int num_threads=std::thread::hardware_concurrency()) const;
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
					plog->log_error(log.c_str(),0);
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
	void ProcessList<T>::process_unsafe(char const* fname,char const* output) const
	{
		using namespace std::experimental::filesystem;
		path in(fname),out(output);
		if(!exists(in))
		{
			throw std::invalid_argument((std::string("Failed to open ").append(fname,in.native().size()).c_str()));
		}
		auto const& instr=in.native();
		auto in_ext=exlib::find_extension(instr.cbegin(),instr.cend());
		auto const& outstr=out.native();
		auto out_ext=exlib::find_extension(outstr.cbegin(),outstr.cend());
		auto proc=[this,fname,output,in_ext,in_end=instr.cend(),out_ext,out_end=outstr.cend(),&out]()
		{
			auto s=supported(&*out_ext);
			auto sin=supported(&*in_ext);
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
			if(s==support_type::no)
			{
				auto ext=make_ext_string(out_ext,out_end);
				throw std::invalid_argument(std::string("Unsupported file type ")+ext);
			}
			if(sin==support_type::no)
			{
				auto ext=make_ext_string(in_ext,in_end);
				throw std::invalid_argument(std::string("Unsupported file type ")+ext);
			}
			if(!std::ofstream(output,std::ios::app))
			{
				throw std::runtime_error(std::string("Failed to save to ").append(output,out.native().size()));
			}
			cil::CImg<T> img(fname);
			for(auto& pprocess:*this)
			{
				pprocess->process(img);
			}
			switch(s)
			{
				case support_type::png:
					img.save_png(output);
					break;
				case support_type::jpeg:
					img.save_jpeg(output);
					break;
				case support_type::bmp:
					img.save_bmp(output);
					break;
			}
		};
		if(empty())
		{
			if(std::experimental::filesystem::equivalent(in,out))
			{
				return;
			}
			if(!exlib::strncmp_nocase(in_ext,out_ext))
			{
				std::ifstream src(fname,std::ios::binary);
				if(!src)
				{
					throw std::invalid_argument((std::string("Failed to open ").append(fname,in.native().size()).c_str()));
				}
				std::ofstream dst(output,std::ios::binary);
				if(!dst)
				{
					throw std::invalid_argument((std::string("Failed to save to ").append(output,out.native().size()).c_str()));
				}
				dst<<src.rdbuf();
			}
			else
			{
				proc();
			}
		}
		else
		{
			proc();
		}
	}

	template<typename T>
	void ProcessList<T>::process(char const* fname,char const* output) const
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
	void ProcessList<T>::process(char const* filename,SaveRules const* psr,unsigned int index) const
	{
		size_t len=strlen(filename);
		bool out_loud=plog&&vb>=decltype(vb)::loud;
		if(out_loud)
		{
			std::string log("Starting ");
			log.append(filename,len);
			log.push_back('\n');
			plog->log(log.c_str(),index);
		}
		if(psr==nullptr)
		{
			process(filename);
		}
		auto output=psr->make_filename(exlib::weak_string(const_cast<char*>(filename),len),index);
		try
		{
			process_unsafe(filename,output.c_str());
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
					plog->log_error(log.c_str(),index);
				}
				return;
			}
			else
			{
				throw ex;
			}
		}
		if(out_loud)
		{
			std::string log("Finished ");
			log.append(filename,len);
			log.push_back('\n');
			plog->log(log.c_str(),index);
		}
	}

	template<typename T>
	void ProcessList<T>::process(
		std::vector<char*> const& imgs,
		SaveRules const* psr,
		unsigned int num_threads) const
	{
		exlib::ThreadPool tp(num_threads);
		for(size_t i=0;i<imgs.size();++i)
		{
			tp.add_task<typename ProcessList<T>::ProcessTaskFName>(imgs[i],this,psr,i+1);
		}
		tp.start();
	}

	template<typename T>
	template<typename String>
	void ProcessList<T>::process(
		std::vector<String> const& imgs,
		SaveRules const* psr,
		unsigned int num_threads) const
	{
		exlib::ThreadPool tp(num_threads);
		for(size_t i=0;i<imgs.size();++i)
		{
			tp.add_task<typename ProcessList<T>::ProcessTaskFName>(imgs[i].c_str(),this,psr,i+1);
		}
		tp.start();
	}

	template<typename T>
	void ProcessList<T>::process(
		std::vector<cimg_library::CImg<T>>& imgs,
		SaveRules const* psr,
		unsigned int num_threads) const
	{
		exlib::ThreadPool tp(num_threads);
		for(size_t i=0;i<imgs.size();++i)
		{
			tp.add_task<typename ProcessList<T>::ProcessTaskImg>(imgs[i],this,psr,i+1);
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
			if(p.is_fixed)
			{
				out.append(p.fixed.data,p.fixed.size);
			}
			else
			{
				if(p.tmplt<10)
				{
					out+=exlib::front_padded_string(std::to_string(index),p.tmplt,'0');
				}
				else
				{
					switch(p.tmplt)
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
							out.append(path.data,path.size);
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
}
#endif
