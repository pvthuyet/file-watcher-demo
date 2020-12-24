#include "file_name_watcher.h"
#include "spdlog_header.h"

namespace died
{
	void file_name_watcher::do_notify(file_notify_info info)
	{
		SPDLOG_DEBUG(L"{} - {}", info.get_action(), info.get_path_wstring());
		if (info.is_directory()) {
			SPDLOG_DEBUG(L"Ignore => directory");
			return;
		}

		switch (info.get_action())
		{
		case FILE_ACTION_ADDED:
			mAdd.push(std::move(info));
			break;

		case FILE_ACTION_REMOVED:
			mRemove.push(std::move(info));
			break;

		case FILE_ACTION_MODIFIED:
			mModify.push(std::move(info));
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
		case FILE_ACTION_RENAMED_NEW_NAME:
			mRename.push(std::move(info));
			break;

		default:
			break;
		}
	}

	model_rename& file_name_watcher::get_rename()
	{
		return mRename;
	}

	model_file_info& file_name_watcher::get_add()
	{
		return mAdd;
	}

	model_file_info& file_name_watcher::get_remove()
	{
		return mRemove;
	}

	model_file_info& file_name_watcher::get_modify()
	{
		return mModify;
	}

	void file_name_watcher::erase_all(std::wstring const& key)
	{
		mAdd.erase(key);
		mRemove.erase(key);
		mModify.erase(key);
	}
}