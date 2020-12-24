#include "file_name_watcher.h"
#include "spdlog_header.h"

namespace died
{
	void file_name_watcher::do_notify(file_notify_info info)
	{
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

	model_rename& file_name_watcher::get_rename()
	{
		return mRename;
	}
}