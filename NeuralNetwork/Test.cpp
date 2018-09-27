#include "neural_net.h"

int main()
{
	neural_net::net<> test(0x0A,0x0B,0x0C);
	test.randomize();
	std::cout<<test;
	test.save("test.ssn");
	std::cout<<"--------------------------------------\n";
	test.load("test.ssn");
	std::cout<<test;
	std::cout<<'\n';
}