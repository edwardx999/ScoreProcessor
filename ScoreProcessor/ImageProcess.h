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
			i,x=10,f,p,c
		};
		union part {
			exlib::weak_string fixed;
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
				fixed(str.data(),str.size())
			{
				str.release();
			}
			~part()
			{
				delete[] fixed.data();
			}
		};
		std::vector<part> parts;
	public:
		/*
			Makes a string out of the input, based on the current template.
		*/
		template<typename String>
		exlib::string make_filename(String const& input,unsigned int index=0) const;

		exlib::string make_filename(char const* input,unsigned int index=0) const;
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
		A series of processes to done onto an image.
	*/
	template<typename T=unsigned char>
	class ProcessList:public std::vector<std::unique_ptr<ImageProcess<T>>> {
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
		std::string log_buffer;
	public:
		ProcessList(Log* log):plog(log)
		{
			log_buffer.reserve(200);
		}
		ProcessList():plog(nullptr)
		{}

		void set_log(Log* log)
		{
			plog=log;
		}

		Log* get_log() const
		{
			return plog;
		}
/*
	Adds a process to the list.
*/
		template<typename U,typename... Args>
		void add_process(Args&&... args);

		void process_unsafe(cimg_library::CImg<T>& img,char const* output) const;
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
		void process(cimg_library::CImg<T>& img,SaveRules const* psr,unsigned int index=0) const;

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
		void process(char const* filename,SaveRules const* psr,unsigned int index=0) const;

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
		catch(cimg_library::CImgException const& ex)
		{
			if(plog)
			{
				std::string& log_buffer=const_cast<std::string&>(this->log_buffer);
				log_buffer=ex.what();
				log_buffer.push_back('\n');
				plog->log_error(log_buffer.c_str(),0);
			}
			else
			{
				throw ex;
			}
		}
	}

	template<typename T>
	void ProcessList<T>::process(cimg_library::CImg<T>& img,SaveRules const* psr,unsigned int index=0) const
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
	void ProcessList<T>::process(char const* fname,char const* output) const
	{
		process(cimg_library::CImg<T>(fname),output);
	}

	template<typename T>
	void ProcessList<T>::process(char const* filename,SaveRules const* psr,unsigned int index) const
	{
		std::string& log_buffer=const_cast<std::string&>(this->log_buffer);
		size_t len=strlen(filename);
		if(plog)
		{
			log_buffer="Starting ";
			log_buffer.append(filename,len);
			log_buffer.push_back('\n');
			plog->log(log_buffer.c_str(),index);
		}
		if(psr==nullptr)
		{
			process(filename);
		}
		auto output=psr->make_filename(exlib::weak_string(const_cast<char*>(filename),len),index);
		try
		{
			process_unsafe(cimg_library::CImg<T>(filename),output.c_str());
		}
		catch(cimg_library::CImgIOException const& ex)
		{
			if(plog)
			{
				log_buffer="Error processing ";
				log_buffer.append(filename,len);
				log_buffer.append(": ",2);
				log_buffer.append(ex.what());
				log_buffer.push_back('\n');
				plog->log_error(log_buffer.c_str(),index);
				return;
			}
			else
			{
				throw ex;
			}
		}
		if(plog)
		{
			log_buffer="Finished ";
			log_buffer.append(filename,len);
			log_buffer.push_back('\n');
			plog->log(log_buffer.c_str(),index);
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

	SaveRules::SaveRules(char const* tmplt)
	{
		assign(tmplt);
	}

	SaveRules::SaveRules()
	{}

	template<typename String>
	void SaveRules::assign(String const& tmplt)
	{
		assign(tmplt.c_str());
	}

	inline void SaveRules::assign(char const* tmplt)
	{
		parts.clear();
		size_t i=0;
		bool found=false;
		exlib::string str(20);
		auto put_string=[&]()
		{
			parts.emplace_back(str);
			str.reserve(20);
		};
		while(tmplt[i]!=0)
		{
			if(found)
			{
				found=false;
				char letter=tmplt[i];
				if(letter>='0'&&letter<='9')
				{
					put_string();
					parts.emplace_back(letter-'0');
				}
				else
				{
					switch(letter)
					{
						case 'x':
							put_string();
							parts.emplace_back(template_symbol::x);
							break;
						case 'f':
							put_string();
							parts.emplace_back(template_symbol::f);
							break;
						case 'p':
							put_string();
							parts.emplace_back(template_symbol::p);
							break;
						case 'c':
							put_string();
							parts.emplace_back(template_symbol::c);
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
				if(tmplt[i]=='%')
				{
					found=true;
				}
				else
				{
					str.push_back(tmplt[i]);
				}
			}
			++i;
		}
		put_string();
	}

	template<typename String>
	exlib::string SaveRules::make_filename(String const& input,unsigned int index) const
	{
		exlib::string out(30L);
		exlib::weak_string ext(nullptr,0);
		exlib::weak_string filename(nullptr,0);
		exlib::weak_string path(nullptr,0);
		auto find_filename=[](String const& in)
		{
			char* it=const_cast<char*>(&(*(in.cend()-1)));
			while(1)
			{
				if(*it=='\\'||*it=='/')
				{
					return it+1;
				}
				if(it==(&(*in.cbegin())))
				{
					return it;
				}
				--it;
			}
		};
		auto check_ext=[&]()
		{
			if(ext.data()==nullptr)
			{
				ext=exlib::weak_string(const_cast<char*>(::cimg_library::cimg::split_filename(input.c_str())));
			}
		};
		auto check_filename=[&]()
		{
			if(filename.data()==nullptr)
			{
				check_ext();
				char* fstart=find_filename(input);
				size_t fsize=ext.data()-fstart;
				if(ext.data()!=(&(*input.cend())))
				{
					--fsize;
				}
				filename=exlib::weak_string(fstart,fsize);
			}
		};
		auto check_path=[&]()
		{
			if(path.data()==nullptr)
			{
				check_filename();
				size_t psize=filename.data()-input.data();
				path=exlib::weak_string(const_cast<char*>(input.data()),psize);
			}
		};
		for(auto const& p:parts)
		{
			if(p.is_fixed)
			{
				out+=p.fixed;
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
							out+=ext;
							break;
						case SaveRules::template_symbol::f:
							check_filename();
							out+=filename;
							break;
						case SaveRules::template_symbol::p:
							check_path();
							out+=path;
							break;
						case SaveRules::template_symbol::c:
							out+=input;
							break;
					}
				}
			}
		}
		return out;
	}

	inline exlib::string SaveRules::make_filename(char const* input,unsigned int index) const
	{
		return make_filename(exlib::weak_string(const_cast<char*>(input)),index);
	}

	inline bool SaveRules::empty() const
	{
		return parts.empty();
	}
}
#endif
