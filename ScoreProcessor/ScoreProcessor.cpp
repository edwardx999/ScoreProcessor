#include "stdafx.h"
#include <iostream>
#include "Interface.h"

int main(int argc,char** argv)
{
	ScoreProcessor::CommandMaker::delivery del;
	auto command=ScoreProcessor::find_command("-o");
	if(command==nullptr)
	{
		std::cout<<"Unknown command";
		return 0;
	}
	char const* args[]={"hello","t"};
	command->make_command(args,args+2,del);
	std::cout<<command->argument_list();
	return 0;
}