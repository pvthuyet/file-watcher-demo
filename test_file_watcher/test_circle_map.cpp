#include "stdafx.h"
#include "CppUnitTest.h"
#include <string>
#include "file_notify_info.h"
#include "circle_map.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

constexpr unsigned int MAX_SIZE_MAP = 2;
using test_key_t = std::wstring;
using test_item_t = died::file_notify_info;
using test_map_t = died::circle_map<test_key_t, test_item_t, MAX_SIZE_MAP>;


const test_item_t EMPTY_ITEM;

namespace test_file_watcher
{		
	TEST_CLASS(test_circle_map)
	{
		
	public:
		
		TEST_METHOD(ctor_circle_map)
		{
			test_map_t mp;
			Assert::AreEqual(mp.max_size(), MAX_SIZE_MAP);
			Assert::AreEqual(mp.size(), 0u);
		}

		TEST_METHOD(find_item_empty_map)
		{
			test_item_t v(L"key - 1", 1);
			test_map_t mp;
			auto& fnd = mp.find(v.get_path_wstring());
			Assert::AreEqual(fnd.get_path_wstring(), EMPTY_ITEM.get_path_wstring());
		}

		TEST_METHOD(find_if_item_empty_map)
		{
			test_item_t v(L"key - 1", 1);
			test_map_t mp;
			auto& fnd = mp.find_if([&v](auto const& el) {
				return v == el;
			});
			Assert::AreEqual(fnd.get_path_wstring(), EMPTY_ITEM.get_path_wstring());
		}

		TEST_METHOD(add_none_exist_item)
		{
			test_item_t v(L"key - 1", 1);
			test_map_t mp;
			mp[v.get_path_wstring()] = v;
			Assert::IsTrue(1 == mp.size());
		}

		TEST_METHOD(erase_item_empty_map)
		{
			test_item_t v(L"key - 1", 1);
			test_map_t mp;
			mp.erase(v.get_path_wstring());
			Assert::IsTrue(0 == mp.size());
		}

		TEST_METHOD(front_item_empty_map)
		{
			test_map_t mp;
			auto v = mp.front();
			Assert::IsTrue(v == EMPTY_ITEM);
		}

		TEST_METHOD(next_available_item_empty_map)
		{
			test_map_t mp;
			auto v = mp.next_available_item();
			Assert::IsTrue(v > mp.max_size());
		}
	};
}