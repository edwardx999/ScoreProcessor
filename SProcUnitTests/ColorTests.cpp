#include "stdafx.h"
#include "CppUnitTest.h"
#include "../ScoreProcessor/ImageUtils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SProcUnitTests {
	TEST_CLASS(UnitTest1)
	{
	public:

		TEST_METHOD(ColorRGBToColorHSV1)
		{
			using namespace ImageUtils;
			ColorRGB red_dark={255,0,0};
			ColorHSV red_dark_hsv=red_dark;
			ColorHSV expected={0,255,255};
			Assert::AreEqual(red_dark_hsv.h,expected.h);
			Assert::AreEqual(red_dark_hsv.s,expected.s);
			Assert::AreEqual(red_dark_hsv.v,expected.v);
		}
		TEST_METHOD(ColorRGBToColorHSV2)
		{
			using namespace ImageUtils;
			ColorRGB red_dark={128,0,0};
			ColorHSV red_dark_hsv=red_dark;
			ColorHSV expected={0,255,128};
			Assert::AreEqual(red_dark_hsv.h,expected.h);
			Assert::AreEqual(red_dark_hsv.s,expected.s);
			Assert::AreEqual(red_dark_hsv.v,expected.v);
		}
		TEST_METHOD(ColorRGBToColorHSV3)
		{
			using namespace ImageUtils;
			ColorRGB red_dark={0,0,0};
			ColorHSV red_dark_hsv=red_dark;
			ColorHSV expected={0,0,0};
			//Assert::AreEqual(red_dark_hsv.h,expected.h);
			Assert::AreEqual(red_dark_hsv.s,expected.s);
			Assert::AreEqual(red_dark_hsv.v,expected.v);
		}
		TEST_METHOD(ColorRGBToColorHSV4)
		{
			using namespace ImageUtils;
			ColorRGB rgb={128,128,128};
			ColorHSV hsv=rgb;
			ColorHSV expected={0,0,128};
			Assert::AreEqual(hsv.h,expected.h);
			Assert::AreEqual(hsv.s,expected.s);
			Assert::AreEqual(hsv.v,expected.v);
		}
		TEST_METHOD(ColorRGBToColorHSV5)
		{
			using namespace ImageUtils;
			ColorRGB rgb={69,47,127};
			ColorHSV hsv=rgb;
			ColorHSV expected={182,161,127};
			Assert::AreEqual(hsv.h,expected.h);
			Assert::AreEqual(hsv.s,expected.s);
			Assert::AreEqual(hsv.v,expected.v);
		}
		TEST_METHOD(AssertWithinRange)
		{
			using namespace ImageUtils;
			for(unsigned int r=0;r<256;++r)
			{
				for(unsigned int g=0;g<256;++g)
				{
					for(unsigned int b=0;b<256;++b)
					{
						float red=r;float green=g;float blue=b;
						auto min=std::min(red,std::min(green,blue));
						auto max=std::max(red,std::max(green,blue));
						auto delta=max-min;
						if(max!=0)
							Assert::IsTrue(std::round(delta/max*255)<256);
					}
				}
			}
		}
		TEST_METHOD(ColorRGBToColorHSV6)
		{
			using namespace ImageUtils;
			ColorRGB rgb={46,255,22};
			ColorHSV hsv=rgb;
			ColorHSV expected={81,233,255};
			Assert::AreEqual(hsv.h,expected.h);
			Assert::AreEqual(hsv.s,expected.s);
			Assert::AreEqual(hsv.v,expected.v);
		}
	};
}