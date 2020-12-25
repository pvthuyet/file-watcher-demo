#pragma once

#include "std_filesystem.h"
#include <chrono>

namespace died
{
	class file_notify_info
	{
	public:
		file_notify_info() = default;
		
		template<typename StringAble> 
		file_notify_info(StringAble&& s, unsigned long action = 0ul, unsigned long size = 0ul);

		explicit operator bool() const noexcept;

		unsigned long get_action() const noexcept;
		unsigned long get_size() const noexcept;
		std::wstring get_path_wstring() const;
		std::wstring get_file_name_wstring() const;
		std::wstring get_parent_path_wstring() const;
		bool is_directory() const;

		size_t alive() const; // in milli-seconds
		std::chrono::time_point<std::chrono::steady_clock> get_created_time() const;

		friend bool operator==(file_notify_info const&, file_notify_info const&);

	private:
		std::filesystem::path mPath;
		unsigned long mAction{};
		unsigned long mSize{};
		std::chrono::time_point<std::chrono::steady_clock> mCreatedTime;
	};

	bool operator==(file_notify_info const& lhs, file_notify_info const& rhs);

	template<typename StringAble> 
	file_notify_info::file_notify_info(StringAble&& s, unsigned long action, unsigned long size) :
		mPath{ std::forward<StringAble>(s) },
		mAction{ action },
		mSize{ size },
		mCreatedTime{ (std::chrono::steady_clock::now()) }
	{}
}

