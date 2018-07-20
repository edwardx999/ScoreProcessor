#include "stdafx.h"
#include "CppUnitTest.h"
#include "../ScoreProcessor/ImageUtils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace ImageUtils;
#define TM TEST_METHOD
namespace SProcUnitTests {
	TEST_CLASS(MaybeFixedTests)
	{
		TM(fixed1)
		{
			maybe_fixed<> t(10);
			Assert::AreEqual(t(10),10);
		}
	};
}