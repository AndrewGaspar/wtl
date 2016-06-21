#include "stdafx.h"
#include "CppUnitTest.h"

#include <wtl\file.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wtltest
{
    TEST_CLASS(FileTest)
    {
    public:

        TEST_METHOD(CreateFile)
        {
            auto file = wtl::file::create(L"foo.txt", KEY_READ | KEY_WRITE, 0, nullptr, OPEN_ALWAYS);

            Assert::IsTrue(file);
            Assert::IsTrue(file.get());
        }
    };
}