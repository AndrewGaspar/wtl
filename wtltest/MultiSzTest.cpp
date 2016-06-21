#include "stdafx.h"
#include "CppUnitTest.h"

#include <wtl\multi_sz.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wtltest
{		
	TEST_CLASS(MultiSzTest)
	{
	public:
		
		TEST_METHOD(ABC)
		{
            const auto testStr = L"A\0B\0C\0";
            auto view = wtl::multi_sz_view(testStr);

            auto it = view.begin();
            auto end = view.end();

            Assert::AreEqual(L"A", *it++);
            Assert::AreEqual(L"B", *it++);
            Assert::AreEqual(L"C", *it++);
            Assert::IsTrue(it == end);
		}

        TEST_METHOD(Empty)
        {
            const auto testStr = L"";
            auto view = wtl::multi_sz_view(testStr);

            Assert::IsTrue(view.begin() == view.end());
        }

        TEST_METHOD(One)
        {
            const auto testStr = L"One\0";
            auto view = wtl::multi_sz_view(testStr);

            auto it = view.begin();
            auto end = view.end();

            Assert::AreEqual(L"One", *it++);
            Assert::IsTrue(it == end);
        }

        TEST_METHOD(OneTwo)
        {
            const auto testStr = L"One\0Two\0";
            auto view = wtl::multi_sz_view(testStr);

            auto it = view.begin();
            auto end = view.end();

            Assert::AreEqual(L"One", *it++);
            Assert::AreEqual(L"Two", *it++);
            Assert::IsTrue(it == end);
        }

        TEST_METHOD(Reverse)
        {
            const auto testStr = L"One\0Two\0";
            auto view = wtl::multi_sz_view(testStr);

            using reverse_multi_sz = std::reverse_iterator<wtl::multi_sz_view::iterator>;

            auto it = reverse_multi_sz(view.end());
            auto end = reverse_multi_sz(view.begin());

            Assert::AreEqual(L"Two", *it++);
            Assert::AreEqual(L"One", *it++);
            Assert::IsTrue(it == end);
        }

        TEST_METHOD(ReverseViaR)
        {
            const auto testStr = L"One\0Two\0";
            auto view = wtl::multi_sz_view(testStr);

            auto it = view.rbegin();
            auto end = view.rend();

            Assert::AreEqual(L"Two", *it++);
            Assert::AreEqual(L"One", *it++);
            Assert::IsTrue(it == end);
        }

        template<size_t N>
        bool IsMultiSz(const wchar_t (&testStr)[N])
        {
            return wtl::is_valid_multi_string_buffer(std::begin(testStr), std::end(testStr));
        }

        template<size_t N>
        void PassMultiSz(const wchar_t(&testStr)[N])
        {
            Assert::IsTrue(IsMultiSz(testStr));
        }

        template<size_t N>
        void FailMultiSz(const wchar_t(&testStr)[N])
        {
            Assert::IsFalse(IsMultiSz(testStr));
        }

        TEST_METHOD(ValidMultiStringBuffers)
        {
            PassMultiSz(L"");
            FailMultiSz(L"\0");
            FailMultiSz(L"ABC");
            PassMultiSz(L"ABC\0");
            PassMultiSz(L"ABC\0DEF\0");
            FailMultiSz(L"ABC\0DEF\0\0");
            FailMultiSz(L"ABC\0\0DEF\0");
        }

        TEST_METHOD(EmptyDynamicMultiSz)
        {
            auto nilMultiSz = wtl::multi_sz();

            Assert::IsTrue(nilMultiSz.begin() == nilMultiSz.end());
            Assert::IsTrue(nilMultiSz.rbegin() == nilMultiSz.rend());

            auto emptyMultiSz = wtl::multi_sz(L"");

            Assert::IsTrue(emptyMultiSz.begin() == emptyMultiSz.end());
            Assert::IsTrue(emptyMultiSz.rbegin() == emptyMultiSz.rend());
        }

        TEST_METHOD(DynamicMultiSz)
        {
            auto multiSz = wtl::multi_sz(L"ABC\0DEF\0");

            auto it = multiSz.begin();
            auto end = multiSz.end();

            Assert::AreEqual(L"ABC", *it++);
            Assert::AreEqual(L"DEF", *it++);
            Assert::IsTrue(it == end);
        }

        template<size_t N>
        void VerifyBuffer(wchar_t const (&expected)[N], wtl::multi_sz const & multiSz)
        {
            auto & buffer = multiSz.view_buffer();

            auto e = std::begin(expected);
            auto e_end = std::end(expected);
            auto i = buffer.begin();
            auto i_end = buffer.end();

            while (e != e_end && i != i_end)
            {
                Assert::AreEqual(*e++, *i++);
            }

            Assert::IsTrue(e == e_end);
            Assert::IsTrue(i == i_end);
        }

        TEST_METHOD(PushBackMultiSz)
        {
            auto multiSz = wtl::multi_sz();

            multiSz.push_back(L"ABC");
            multiSz.push_back(L"DEF");

            auto it = multiSz.begin();
            auto end = multiSz.end();

            Assert::AreEqual(L"ABC", *it++);
            Assert::AreEqual(L"DEF", *it++);
            Assert::IsTrue(it == end);

            VerifyBuffer(L"ABC\0DEF\0", multiSz);
        }

        TEST_METHOD(InsertMultiSz)
        {
            auto multiSz = wtl::multi_sz(L"ABC\0");

            auto it = multiSz.insert(multiSz.begin(), L"DEF");

            VerifyBuffer(L"DEF\0ABC\0", multiSz);

            Assert::AreEqual(L"DEF", *it);

            ++it;

            it = multiSz.insert(it, L"GHI");

            VerifyBuffer(L"DEF\0GHI\0ABC\0", multiSz);

            Assert::AreEqual(L"GHI", *it);
        }

        TEST_METHOD(MultiInsertMultiSz)
        {
            using PCWSTR = wchar_t const *;

            PCWSTR strs[] = { L"ABC", L"JKL" };

            auto multiSz = wtl::multi_sz();

            auto it = multiSz.insert(multiSz.begin(), std::begin(strs), std::end(strs));

            VerifyBuffer(L"ABC\0JKL\0", multiSz);
            Assert::AreEqual(L"ABC", *it);

            ++it;

            PCWSTR strs2[] = { L"DEF", L"GHI" };

            it = multiSz.insert(it, { L"DEF", L"GHI" });

            VerifyBuffer(L"ABC\0DEF\0GHI\0JKL\0", multiSz);
            Assert::AreEqual(L"DEF", *it);
        }

        TEST_METHOD(ReadTwice)
        {
            auto multiSz = wtl::multi_sz(L"ABC\0");

            for (auto const & str : multiSz)
            {
                Assert::AreEqual(L"ABC", str);
                Assert::AreEqual(L"ABC", str);
            }
        }
	};
}