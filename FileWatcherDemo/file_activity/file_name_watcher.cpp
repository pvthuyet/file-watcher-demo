#include "file_name_watcher.h"
#include "spdlog_header.h"

namespace died
{
	file_name_watcher::file_name_watcher()
	{
		mRules.setAppDataDir(true);
	}

	void file_name_watcher::do_notify(file_notify_info info)
	{
		if (mRules.contains(info)) {
			return;
		}

		switch (info.get_action())
		{
		case FILE_ACTION_ADDED:
			break;

		case FILE_ACTION_REMOVED:
			break;

		case FILE_ACTION_MODIFIED:
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
		case FILE_ACTION_RENAMED_NEW_NAME:
			SPDLOG_INFO(L"{} - {}", info.get_action(), info.get_path_wstring());
			mRename.push(std::move(info));
			break;

		default:
			break;
		}
	}

	rename_model& file_name_watcher::get_rename()
	{
		return mRename;
	}
}