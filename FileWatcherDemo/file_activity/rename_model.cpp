#include "rename_model.h"
#include <Windows.h>

namespace died
{
	bool operator==(rename_notify_info const& lhs, rename_notify_info const& rhs)
	{
		return lhs.mOldName == rhs.mOldName && lhs.mNewName == rhs.mNewName;
	}

	rename_notify_info::operator bool() const noexcept
	{
		return static_cast<bool>(mOldName) 
			&& static_cast<bool>(mNewName)
			&& (mOldName.get_parent_path_wstring() == mNewName.get_parent_path_wstring());
	}

	std::wstring rename_notify_info::get_key() const
	{
		return mNewName.get_path_wstring();
	}

	/************************************************************************************************/

	void rename_model::push(file_notify_info&& info)
	{
		switch (info.get_action())
		{
		case FILE_ACTION_RENAMED_OLD_NAME:
			mInfo.mOldName = std::move(info);
			break;

		case FILE_ACTION_RENAMED_NEW_NAME:
			mInfo.mNewName = std::move(info);
			break;

		default:
			break;
		}

		// valid data
		if (mInfo) {
			auto key = mInfo.get_key();
			mData[std::move(key)] = std::move(mInfo);
		}
	}

	const rename_notify_info& rename_model::front() const
	{
		return mData.front();
	}

	const file_notify_info& rename_model::find(std::wstring const& key) const
	{
		auto const& found = mData.find(key);
		return found.mNewName;
	}

	void rename_model::erase(std::wstring const& key)
	{
		mData.erase(key);
	}

	unsigned int rename_model::next_available_item()
	{
		return mData.next_available_item();
	}
}