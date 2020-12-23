#include "file_notify_info.h"

namespace died
{
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

	std::wstring file_notify_info::get_file_name() const
	{
		return mPath.filename().wstring();
	}
}
