#include "file_notify_info.h"

namespace died
{
	bool operator==(file_notify_info const& lhs, file_notify_info const& rhs)
	{
		return lhs.mAction == rhs.mAction && lhs.mPath == rhs.mPath;
	}

	file_notify_info::operator bool() const noexcept
	{
		return mAction > 0 && !mPath.empty();
	}

	unsigned long file_notify_info::get_action() const noexcept
	{
		return mAction;
	}

	unsigned long file_notify_info::get_size() const noexcept
	{
		return mSize;
	}

	std::wstring file_notify_info::get_path_wstring() const
	{
		return mPath.wstring();
	}

	std::wstring file_notify_info::get_file_name_wstring() const
	{
		return mPath.filename().wstring();
	}

	std::wstring file_notify_info::get_parent_path_wstring() const
	{
		return mPath.parent_path().wstring();
	}

	bool file_notify_info::is_directory() const
	{
		return std::filesystem::is_directory(mPath);
	}

	size_t file_notify_info::alive() const
	{
		std::chrono::duration<double, std::milli> diff = std::chrono::steady_clock::now() - mCreatedTime;
		return static_cast<size_t>(diff.count());
	}

	std::chrono::time_point<std::chrono::steady_clock> file_notify_info::get_created_time() const
	{
		return mCreatedTime;
	}
}
