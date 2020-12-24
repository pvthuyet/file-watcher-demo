#include "folder_name_watcher.h"
#include "spdlog_header.h"

namespace died
{
	void folder_name_watcher::do_notify(file_notify_info info)
	{
		SPDLOG_INFO(L"{} - {}", info.get_action(), info.get_path_wstring());
		mModel.push(std::move(info));
	}

	model_file_info& folder_name_watcher::get_model()
	{
		return mModel;
	}
}