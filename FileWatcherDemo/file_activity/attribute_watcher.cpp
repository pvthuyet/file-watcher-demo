#include "attribute_watcher.h"
#include "spdlog_header.h"

namespace died
{
	void attribute_watcher::do_notify(file_notify_info info)
	{
		SPDLOG_INFO(L"{} - {}", info.get_action(), info.get_path_wstring());
		mModel.push(std::move(info));
	}

	model_file_info& attribute_watcher::get_model()
	{
		return mModel;
	}
}