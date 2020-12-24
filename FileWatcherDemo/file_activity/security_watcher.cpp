#include "security_watcher.h"
#include "spdlog_header.h"

namespace died
{
	void security_watcher::do_notify(file_notify_info info)
	{
		SPDLOG_INFO(L"{} - {}", info.get_action(), info.get_path_wstring());
		mModel.push(std::move(info));
	}

	model_file_info& security_watcher::get_model()
	{
		return mModel;
	}
}