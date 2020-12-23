#pragma once

#include "std_filesystem.h"
namespace died
{
	class file_notify_info
	{
	public:
		file_notify_info() = default;
		
		template<typename StringAble> 
		file_notify_info(StringAble&& s, unsigned long action = 0ul, unsigned long size = 0ul);

		unsigned long get_action() const noexcept;
		unsigned long get_size() const noexcept;

		std::wstring get_path_wstring() const;
		std::wstring get_file_name() const;

	private:
		std::filesystem::path mPath;
		unsigned long mAction{};
		unsigned long mSize{};
	};

	template<typename StringAble> 
	file_notify_info::file_notify_info(StringAble&& s, unsigned long action, unsigned long size) :
		mPath{ std::forward<StringAble>(s) },
		mAction{ action },
		mSize{ size }
	{}
}

