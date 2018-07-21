#include "stdafx.h"
#include <iostream>
#include "Interface.h"

bool could_be_command(char const* str)
{
	return str[0]=='-'&&str[1]>='a'&&str[1]<='z';
}

size_t longest_name()
{
	using namespace ScoreProcessor;
	constexpr auto const& scl=single_command_list();
	constexpr auto const& mcl=multi_command_list();
	constexpr auto const& ol=option_list();
	size_t longest=0;
	for(auto it:scl)
	{
		auto l=it.maker()->name().length();
		if(l>longest) longest=l;
	}
	for(auto it:mcl)
	{
		auto l=it.maker()->name().length();
		if(l>longest) longest=l;
	}
	for(auto it:ol)
	{
		auto l=it.maker()->name().length();
		if(l>longest) longest=l;
	}
	return longest;
}

void info_output()
{
	char date[]=__DATE__;
	if(date[4]==' ') date[4]=='0';
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
	auto const longest=longest_name()+2;
	auto write=[=](auto it)
	{
		std::cout<<"    ";
		std::cout<<it.maker()->name();
		std::cout.write(space_buffer,longest-it.maker()->name().length());
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

int main(int argc,char** argv)
{
	info_output();
	ScoreProcessor::CommandMaker::delivery del;
	auto command=ScoreProcessor::find_command("-o");
	if(command==nullptr)
	{
		std::cout<<"Unknown command";
		return 0;
	}
	char const* args[]={"o:hello","fd:f"};
	try
	{
		command->make_command(args,args+2,del);
	}
	catch(std::exception const& err)
	{
		std::cout<<err.what()<<'\n';
	}
	std::cout<<del.do_move<<'\n';
	std::cout<<command->argument_list();
	constexpr auto ma=decltype(ScoreProcessor::Output::maker)::MaxArgs;
	return 0;
}