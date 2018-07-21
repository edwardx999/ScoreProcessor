#include "stdafx.h"
#include <iostream>
#include "Interface.h"
#include <vector>
#include <string>
#include "lib/exstring/exfiles.h"
#include <filesystem>
using namespace ScoreProcessor;

using Input=char*;
using InputIter=Input*;

bool could_be_command(char const* str)
{
	return str[0]=='-'&&str[1]>='a'&&str[1]<='z';
}

constexpr size_t longest_name()
{
	using namespace ScoreProcessor;
	constexpr auto const& scl=single_command_list();
	constexpr auto const& mcl=multi_command_list();
	constexpr auto const& ol=option_list();
	size_t longest=0;
	for(auto it:scl)
	{
		auto l=it.info().name().length();
		if(l>longest) longest=l;
	}
	for(auto it:mcl)
	{
		auto l=it.info().name().length();
		if(l>longest) longest=l;
	}
	for(auto it:ol)
	{
		auto l=it.info().name().length();
		if(l>longest) longest=l;
	}
	return longest;
}

void info_output()
{
	char date[]=__DATE__;
	if(date[4]==' ') date[4]=='0';
	std::cout<<"Version: ";
	std::cout<<date<<" " __TIME__ " Copyright 2017-";
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
	constexpr auto const longest=longest_name()+2;
	auto write=[=](auto it)
	{
		std::cout<<"    ";
		std::cout<<it.info().name();
		std::cout.write(space_buffer,longest-it.info().name().length());
		std::cout<<it.key()<<' ';
		std::cout<<it.info().argument_list()<<'\n';
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

void help_output(CommandInfo const& cm)
{
	std::cout<<cm.name()<<'\n';
	std::cout<<"Args: "<<cm.argument_list()<<'\n';
	std::cout<<cm.help_message()<<'\n';
}

void list_files(std::vector<std::string> const& files)
{}

//returns files and sets begin to the end of file list
std::vector<std::string> find_files(InputIter& begin,InputIter end)
{
	auto is_rec=[](char const* in)
	{
		return strcmp("-r",in)==0;
	};
	auto not_empty=[](char const* in)
	{
		if(in[0]=='\0') throw std::invalid_argument("Empty folder argument");
	};
	auto is_folder=[](char const* in)
	{
		auto const len=strlen(in);
		char const c=in[len-1];
		if(c=='\\'||c=='/') return len;
		return size_t(0);
	};
	bool rec=false;
	std::vector<std::string> res;
	for(;begin<end;++begin)
	{
		not_empty(*begin);
		if(rec)
		{
			if(is_rec(*begin))
			{
				throw std::invalid_argument("Double recursive flag given");
			}
			else if(auto len=is_folder(*begin))
			{
				auto find=exlib::files_in_dir_rec(std::string(*begin,len));
				res.insert(res.end(),find.begin(),find.end());
			}
			else
			{
				throw std::invalid_argument("Cannot recursive search non-folder");
			}
		}
	}
	return res;
}

CommandMaker::delivery parse_commands(InputIter iter)
{}

int main(int argc,InputIter argv)
{
	try
	{
		info_output();
		ScoreProcessor::CommandMaker::delivery del;
		auto const& command=ScoreProcessor::find_command("-o");
		char const* args[]={"o:hello","fd:f"};
		command.maker()->make_command(args,args+2,del);
		std::cout<<del.do_move<<'\n';
	}
	catch(std::exception const& err)
	{
		std::cout<<err.what()<<'\n';
	}
	return 0;
}