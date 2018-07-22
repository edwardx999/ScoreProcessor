#include "stdafx.h"
#include <iostream>
#include "Interface.h"
#include <vector>
#include <string>
#include "lib/exstring/exfiles.h"
#include "Logs.h"

using namespace ScoreProcessor;

using Input=char*;
using InputIter=Input*;

bool could_be_command(char const* str)
{
	return str[0]=='-'&&str[1]>='a'&&str[1]<='z';
}

void info_output()
{
	char date[]=__DATE__;
	if(date[4]==' ') date[4]='0';
	char const* time=__TIME__;
	std::cout<<"Version: ";
	std::cout<<date<<' '<<time<<"Copyright 2017-";
	std::cout.write(date+7,4);
	std::cout<<" Edward Xie\n";
	std::cout<<
		"Syntax: filename_or_folder... command params... ...\n"
		"If you want to recursively search a folder,type -r before it\n"
		"If a file starts with a dash,double the starting dash:\"-my-file.jpg\" -> \"--my-file.jpg\"\n"
		"parameters that require multiple values are notated with a comma\n"
		"ex: img0.png --image1.jpg my_folder/-r rec_folder/-fg 180-ccga 20,50,30\n"
		"Type command alone to get readme\n"
		"Available commands:\n"
		"  Single Page Operations:\n";
	char const* space_buffer=":                                                                              ";
	constexpr auto const& scl=ScoreProcessor::single_command_list();
	constexpr size_t const padding=22;
	auto write=[=](auto it)
	{
		std::cout<<"    ";
		std::cout<<it.maker()->name();
		std::cout.write(space_buffer,padding-it.maker()->name().length());
		std::cout<<it.key()<<' ';
		std::cout<<it.maker()->argument_list()<<'\n';
	};
	for(auto it:ScoreProcessor::single_command_list())
	{
		write(it);
	}
	std::cout<<"  Multi Page Operations:\n";
	for(auto it:ScoreProcessor::multi_command_list())
	{
		write(it);
	}
	std::cout<<"  Options:\n";
	for(auto it:ScoreProcessor::option_list())
	{
		write(it);
	}
	std::cout<<"Multiple Single Page Operations can be done at once. They are performed in the order they are given.\n"
		"A Multi Page Operation can not be done with other operations.\n";
}

void help_output(CommandMaker const& cm)
{
	std::cout<<cm.name()<<'\n';
	std::cout<<"Args: "<<cm.argument_list()<<'\n';
	std::cout<<cm.help_message()<<'\n';
}

void list_files(std::vector<std::string> const& files)
{
	if(files.empty())
	{
		std::cout<<"No files found\n";
	}
	else
	{
		std::cout<<'\n';
		for(auto const& file:files)
		{
			std::cout<<file<<'\n';
		}
		std::cout<<'\n'<<files.size()<<" files were found.\n";
	}
	std::cout<<'\n';
}

//returns files and sets iter to the end of file list
std::vector<std::string> find_files(InputIter& iter)
{}

void parse_commands(CommandMaker::delivery& del,InputIter arg_start,InputIter end)
{
	if(arg_start!=end)
	{
		auto it=arg_start+1;
		for(;;++it)
		{
			if(it==end||could_be_command(*it))
			{
				auto cmd=find_command((*arg_start)+1);
				if(cmd==nullptr)
				{
					std::string err_msg("Unknown command: ");
					err_msg.append(*arg_start);
					throw std::invalid_argument(err_msg);
				}
				try
				{
					cmd->make_command(arg_start+1,it,del);
				}
				catch(std::exception const& err)
				{
					std::string err_msg(cmd->name());
					err_msg.append(" error: ");
					err_msg.append(err.what());
					throw std::invalid_argument(err_msg);
				}
				arg_start=it;
			}
			if(it==end)
			{
				break;
			}
		}
	}
	if(del.sr.empty())
	{
		del.sr.assign("%w");
	}
}

void filter_out_files(std::vector<std::string>& files,CommandMaker::delivery const& del)
{
	if(!del.selections.empty())
	{
		std::vector<char> what_to_keep(files.size()); //not gonna take much space anyway, so I'm not using vector<bool>
		std::memset(what_to_keep.data(),0,what_to_keep.size());
		for(auto sr:del.selections)
		{
			auto const beg=files.begin(),ed=files.end();
			auto first=std::find_if(beg,ed,[b=sr.begin](auto const& str)
			{
				return b==std::string_view(str);
			});
			if(first==ed)
			{
				throw std::logic_error(std::string(sr.begin).append(" start boundary not found"));
			}
			auto last=std::find_if(first,ed,[e=sr.end](auto const& str)
			{
				return e==std::string_view(str);
			});
			if(last==ed)
			{
				throw std::logic_error(std::string(sr.end).append(" end boundary not found beyond ").append(sr.begin));
			}
			size_t start=first-files.begin();
			size_t const end=last-files.begin();
			for(;start<=end;++start)
			{
				what_to_keep[start]=1;
			}
		}
		std::vector<std::string> filtered;
		for(size_t i=0;i<files.size();++i)
		{
			if(what_to_keep[i])
			{
				filtered.emplace_back(std::move(files[i]));
			}
		}
		files=std::move(filtered);
	}
	if(del.rgxst)
	{
		files.erase(std::remove_if(files.begin(),files.end(),
			[&del,keep=del.rgxst==CommandMaker::delivery::normal](auto const& a)
		{
			return std::regex_match(a,del.rgx)!=keep;
		}),files.end());
	}
}

std::vector<std::string> get_files(InputIter begin,InputIter end)
{
	bool do_recursive=false;
	std::vector<std::string> files;
	for(auto pos=begin;pos!=end;++pos)
	{
		if(strcmp(*pos,"-r")==0)
		{
			if(do_recursive)
			{
				throw std::logic_error("Double recursive flag given");
			}
			do_recursive=true;
		}
		else
		{
			size_t const s=strlen(*pos);
			if(s==0) throw std::logic_error("Empty argument found");
			char const last=(*pos)[s-1];
			if(last=='\\'||last=='/')
			{
				std::string path(*pos,s);
				auto fid=do_recursive?exlib::files_in_dir_rec(path):exlib::files_in_dir(path);
				files.reserve(files.size()+fid.size());
				if(s==2&&pos[0][0]=='.')
				{
					for(auto& str:fid)
					{
						files.emplace_back(std::move(str));
					}
				}
				else
				{
					for(auto const& str:fid)
					{
						std::string name(*pos,s);
						name.append(str);
						files.emplace_back(std::move(str));
					}
				}
				do_recursive=false;
			}
			else
			{
				if(do_recursive)
				{
					throw std::logic_error("Cannot recursively search a non-folder");
				}
				files.emplace_back(std::move(*pos));
			}
		}
	}
	if(do_recursive)
	{
		throw std::logic_error("Recursive flag given at end of input list");
	}
	return files;
}

InputIter find_file_list(InputIter begin,InputIter end)
{
	for(auto pos=begin;pos!=end;++pos)
	{
		if(pos[0][0]=='-')
		{
			char const c=pos[0][1];
			if(c=='r'&&pos[0][2]=='\0') continue;
			if(c>='a'&&c<='z') return pos;
			if(c=='-') *pos+=1;
		}
	}
	return end;
}

void do_single(CommandMaker::delivery const& del,std::vector<std::string> const& files)
{
	del.pl.process(files,&del.sr,del.num_threads,del.starting_index,del.do_move);
}

void do_cut(CommandMaker::delivery const& del,std::vector<std::string> const& files)
{
	struct cut_args {
		SaveRules const* output;
		int verbosity;
		ImageUtils::perc_or_val min_width;
		ImageUtils::perc_or_val min_height;
		ImageUtils::perc_or_val min_vert_space;
		float horiz_weight;
		Log* log;
	};
	class CutProcess:public exlib::ThreadTaskA<cut_args> {
	private:
		std::string const* input;
		unsigned int index;
	public:
		CutProcess(std::string const* input,unsigned int index,CommandMaker::delivery const& del):
			input(input),
			index(index)
		{}
		void execute(cut_args ca) override
		{
			try
			{
				if(ca.verbosity>ProcessList<>::verbosity::errors_only)
				{
					std::string coutput("Starting ");
					coutput.append(*input);
					coutput.append(1,'\n');
					ca.log->log(coutput.c_str(),index);
				}
				auto out=ca.output->make_filename(*input,index);
				auto ext=exlib::find_extension(out.begin(),out.end());
				auto s=supported(&*ext);
				if(s==support_type::no)
				{
					if(ca.verbosity>ProcessList<>::verbosity::silent)
					{
						std::string err("Error processing ");
						err.append(*input).append(": Unsupported file type ");
						err.append(&*ext,out.end()-ext).append(1,'\n');
						ca.log->log_error(err.c_str(),index);
					}
					return;
				}
				cil::CImg<unsigned char> in(input->c_str());
				cut_heuristics cut_args;
				cut_args.horizontal_energy_weight=ca.horiz_weight;
				cut_args.min_height=ca.min_height(in._height);
				cut_args.min_width=ca.min_width(in._width);
				cut_args.minimum_vertical_space=ca.min_vert_space(in._height);
				auto num_pages=ScoreProcessor::cut_page(in,out.c_str(),cut_args);
				if(ca.verbosity>ProcessList<>::verbosity::errors_only)
				{
					std::string coutput("Finished ");
					coutput.append(*input);
					coutput.append(" and created ");
					coutput.append(std::to_string(num_pages));
					coutput.append(num_pages==1?" page\n":" pages\n");
					ca.log->log(coutput.c_str(),index);
				}
			}
			catch(std::exception const& ex)
			{
				if(ca.verbosity>ProcessList<>::verbosity::silent)
				{
					std::string err(ex.what());
					err+='\n';
					ca.log->log_error(err.c_str(),index);
				}
			}
		}
	};
	exlib::ThreadPoolA<cut_args> tp(del.num_threads);
	for(size_t i=0;i<files.size();++i)
	{
		tp.add_task<CutProcess>(&files[i],i+del.starting_index,del);
	}
	tp.start({&del.sr,del.pl.get_verbosity(),del.cut_args.min_width,del.cut_args.min_height,del.cut_args.min_vert_space,del.cut_args.horiz_weight,del.pl.get_log()});
}

void do_splice(CommandMaker::delivery const& del,std::vector<std::string> const& files)
{
	try
	{
		auto save=del.sr.make_filename(files[0],del.starting_index);
		auto ext=exlib::find_extension(save.begin(),save.end());
		if(supported(&*ext)==support_type::no)
		{
			std::cout<<std::string("Unsupported file type ")<<&*ext<<'\n';
			return;
		}
		auto num=del.num_threads<2?
			splice_pages_nongreedy(
				files,
				del.splice_args.horiz_padding,
				del.splice_args.optimal_height,
				del.splice_args.optimal_padding,
				del.splice_args.min_padding,
				save.c_str(),
				del.splice_args.excess_weight,
				del.splice_args.padding_weight,
				del.starting_index):
			splice_pages_nongreedy_parallel(
				files,
				del.splice_args.horiz_padding,
				del.splice_args.optimal_height,
				del.splice_args.optimal_padding,
				del.splice_args.min_padding,
				save.c_str(),
				del.splice_args.excess_weight,
				del.splice_args.padding_weight,
				del.starting_index,
				del.num_threads);
		std::cout<<"Created "<<num<<(num==1?" page\n":" pages\n");
	}
	catch(std::exception const& ex)
	{
		std::cout<<"Error(s):\n"<<ex.what()<<'\n';
	}
}

int main(int argc,InputIter argv)
{
	if(argc==1)
	{
		info_output();
		return 0;
	}
	cil::cimg::exception_mode(0);
	if(could_be_command(argv[1]))
	{
		auto cmd=find_command(argv[1]);
		if(cmd!=nullptr)
		{
			help_output(*cmd);
		}
		else
		{
			std::cout<<"Unknown command: "<<argv[1]<<'\n';
		}
		return 0;
	}
	CommandMaker::delivery del;
	auto start=argv+1;
	auto end=argv+argc;
	auto const file_end=find_file_list(start,end);
	std::vector<std::string> files;
	try
	{
		parse_commands(del,file_end,end);
		if(del.flag==del.do_absolutely_nothing&&!del.list_files)
		{
			std::cout<<"No commands given\n";
			return 0;
		}
		files=get_files(start,file_end);
		filter_out_files(files,del);
	}
	catch(std::exception const& ex)
	{
		std::cout<<ex.what()<<'\n';
		return 0;
	}
	if(del.list_files)
	{
		list_files(files);
	}
	if(del.num_threads==0)
	{
		del.num_threads=std::thread::hardware_concurrency();
		if(del.num_threads==0)
		{
			del.num_threads=2;
		}
	}
	using ui=decltype(del.num_threads);
	del.num_threads=std::min(
		del.num_threads,
		ui((std::min(size_t(std::numeric_limits<ui>::max()),files.size()))));
	std::optional<Loggers::AmountLog> al;
	Loggers::CoutLog cl;
	switch(del.lt)
	{
		case del.unassigned_log:
		case del.count:
			al.emplace(files.size());
			del.pl.set_log(&*al);
			del.pl.set_verbosity(del.pl.loud);
			break;
		case del.errors_only:
			del.pl.set_log(&cl);
			del.pl.set_verbosity(del.pl.errors_only);
			break;
		case del.quiet:
			del.pl.set_log(&cl);
			del.pl.set_verbosity(del.pl.silent);
			break;
		case del.full_message:
			del.pl.set_log(&cl);
			del.pl.set_verbosity(del.pl.loud);
	}
	switch(del.flag)
	{
		case del.do_absolutely_nothing:
			std::cout<<"No commands given\n";
			break;
		case del.do_nothing:
			[[fallthrough]];
		case del.do_single:
			do_single(del,files);
			break;
		case del.do_cut:
			do_cut(del,files);
			break;
		case del.do_splice:
			do_splice(del,files);
			break;
	}
	return 0;
}