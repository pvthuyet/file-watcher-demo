#include "folder_name_watcher.h"
#include "spdlog_header.h"

namespace died
{
	void folder_name_watcher::do_notify(file_notify_info info)
	{
		switch (info.get_action())
		{
		case FILE_ACTION_REMOVED:
			SPDLOG_DEBUG(L"{} - {}", info.get_action(), info.get_path_wstring());
			mModel.push(std::move(info));
			break;

		default:
			SPDLOG_DEBUG(L"Ignore: {} - {}", info.get_action(), info.get_path_wstring());
			break;
		}
	}

	model_file_info& folder_name_watcher::get_model()
	{
		return mModel;
	}
}