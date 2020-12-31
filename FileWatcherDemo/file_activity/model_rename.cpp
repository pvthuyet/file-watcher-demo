#include "model_rename.h"
#include <Windows.h>

namespace died
{
	rename_notify_info::operator bool() const noexcept
	{
		return static_cast<bool>(mOldName)
			&& static_cast<bool>(mNewName)
			&& (mOldName.get_parent_path_wstring() == mNewName.get_parent_path_wstring());
	}

	std::wstring rename_notify_info::get_key() const
	{
		return mOldName.get_path_wstring() + mNewName.get_path_wstring();
	}

	bool rename_notify_info::match_any(std::wstring const& key) const
	{
		return key == mOldName.get_path_wstring()
			|| key == mNewName.get_path_wstring();
	}

	bool operator==(rename_notify_info const& lhs, rename_notify_info const& rhs)
	{
		return lhs.mOldName == rhs.mOldName && lhs.mNewName == rhs.mNewName;
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

	bool model_rename::is_only_one_family_info(rename_notify_info const& info) const
	{
		unsigned int family = 0;
		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		mData.loop_all([&oldName, &newName, &family](auto const& item) {
			if (item.match_any(oldName) || item.match_any(newName)) {
				++family;
			}
		});
		return 1u == family;
	}

	unsigned int model_rename::get_number_family(std::wstring const& key) const
	{
		unsigned int family = 0;
		mData.loop_all([&key, &family](auto const& item) {
			if (item.match_any(key)) {
				++family;
			}
		});
		return family;
	}

	std::vector<std::reference_wrapper<const rename_notify_info>> model_rename::get_family(rename_notify_info const& info) const
	{
		std::vector<std::reference_wrapper<const rename_notify_info>> family;
		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();
		mData.loop_all([&oldName, &newName, &family](auto const& item) {
			if (item.match_any(oldName) || item.match_any(newName)) {
				family.push_back(item);
			}
		});
		return family;
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