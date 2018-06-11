#include "stdafx.h"
#include "CppUnitTest.h"
#include "../ScoreProcessor/ScoreProcesses.h"
#include <thread>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace ScoreProcessor;
using namespace cil;
namespace SProcUnitTests {
	using Rint=ImageUtils::Rectangle<signed int>;
	TEST_CLASS(UnitTest2)
	{
	private:
		template<typename T>
		void AssertEquals(CImg<T> const& a,CImg<T> const& b)
		{
			if(a!=b)
			{
				std::thread c([&]()
				{
					a.display();
				});
				std::thread d([&]()
				{
					b.display();
				});
				c.join();
				d.join();
				Assert::IsFalse(true);
			}
		}
	public:
		TEST_METHOD(CropFill1)
		{
			CImg<unsigned char> img(10,10);
			img.fill(0);
			auto res=get_crop_fill(img,Rint({-2,12,-4,14}),unsigned char(255));
			Assert::AreEqual(15U,res._width);
			Assert::AreEqual(19U,res._height);
			CImg<unsigned char> exp(15,19);
			exp.fill(0);
			for(unsigned int x=0;x<15;++x)
			{
				for(unsigned int y=0;y<4;++y)
				{
					exp(x,y)=255;
				}
			}
			for(unsigned int x=0;x<15;++x)
			{
				for(unsigned int y=14;y<19;++y)
				{
					exp(x,y)=255;
				}
			}
			for(unsigned int x=0;x<2;++x)
			{
				for(unsigned int y=0;y<19;++y)
				{
					exp(x,y)=255;
				}
			}
			for(unsigned int x=12;x<15;++x)
			{
				for(unsigned int y=0;y<19;++y)
				{
					exp(x,y)=255;
				}
			}
			AssertEquals(exp,res);
		}
		TEST_METHOD(HorizPadding1)
		{
			CImg<unsigned char> res(10,20);
			res.fill(0);
			horiz_padding(res,5,10);
			CImg<unsigned char> exp(25,20);
			exp.fill(0);
			for(unsigned int x=0;x<5;++x)
			{
				for(unsigned int y=0;y<20;++y)
				{
					exp(x,y)=255;
				}
			}
			for(unsigned int x=15;x<25;++x)
			{
				for(unsigned int y=0;y<20;++y)
				{
					exp(x,y)=255;
				}
			}
			AssertEquals(exp,res);
		}
	};
}