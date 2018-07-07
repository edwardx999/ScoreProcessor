#include "stdafx.h"
#include "CppUnitTest.h"
#include "../ScoreProcessor/ImageProcess.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace ScoreProcessor;
#define TM TEST_METHOD
namespace SProcUnitTests {
	TEST_CLASS(SaveRuleTest)
	{
	public:
		TM(empty)
		{
			SaveRules sr("");
			std::string exp="";
			auto res=sr.make_filename("blahblah");
			Assert::AreEqual(exp,res);
		}
		TM(filename_index_extension1)
		{
			SaveRules sr("%f%1.%x");
			std::string exp="hello_1.png";
			auto res=sr.make_filename("hello_.png");
			Assert::AreEqual(exp,res);
		}
		TM(filename_index_extension2)
		{
			SaveRules sr("%f%3.%x");
			std::string exp="hello_002.png";
			auto res=sr.make_filename("hello_.png",2);
			Assert::AreEqual(exp,res);
		}
		TM(copy)
		{
			SaveRules sr("%c");
			std::string exp="hello_1.png";
			auto res=sr.make_filename(exp);
			Assert::AreEqual(exp,res);
		}
		TM(path_whole)
		{
			SaveRules sr("%p/%w");
			std::string exp="my_folder/hello_1.png";
			auto res=sr.make_filename(exp);
			Assert::AreEqual(exp,res);
			std::string exp2="./hello_1.png";
			auto res2=sr.make_filename("hello_1.png");
			Assert::AreEqual(exp2,res2);
		}
		TM(indexing_index_new_extension_percent)
		{
			SaveRules sr("p%%_%3.bmp");
			std::string exp="p%_010.bmp";
			auto res=sr.make_filename("random_name.png",10);
			Assert::AreEqual(exp,res);
		}
	};
}