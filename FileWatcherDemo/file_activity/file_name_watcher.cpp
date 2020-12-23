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

		//SPDLOG_INFO(L"{} - {}", info.get_action(), info.get_path_wstring());
		switch (info.get_action())
		{
		case FILE_ACTION_ADDED:
			break;

		case FILE_ACTION_REMOVED:
			break;

		case FILE_ACTION_MODIFIED:
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
			SPDLOG_INFO(L"{} - {}", L"FILE_ACTION_RENAMED_OLD_NAME", info.get_path_wstring());
			break;

		case FILE_ACTION_RENAMED_NEW_NAME:
			SPDLOG_INFO(L"{} - {}", L"FILE_ACTION_RENAMED_NEW_NAME", info.get_path_wstring());
			break;

		default:
			break;
		}
	}
}