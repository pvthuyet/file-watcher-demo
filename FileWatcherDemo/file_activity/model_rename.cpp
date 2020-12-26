#include "model_rename.h"
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

	void model_rename::push(file_notify_info&& info)
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

	const rename_notify_info& model_rename::front() const
	{
		return mData.front();
	}

	const file_notify_info& model_rename::find(std::wstring const& key) const
	{
		auto const& found = mData.find(key);
		return found.mNewName;
	}

	const rename_notify_info& model_rename::find_by_old_name(std::wstring const& key) const
	{
		return mData.find_if([&key](auto const& item) {
			return key == item.mOldName.get_path_wstring();
		});
	}

	void model_rename::erase(std::wstring const& key)
	{
		mData.erase(key);
	}

	unsigned int model_rename::next_available_item()
	{
		return mData.next_available_item();
	}
}