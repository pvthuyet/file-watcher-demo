#pragma once

#include "directory_watcher_base.h"
#include "model_file_info.h"

namespace died
{
	class attribute_watcher : public directory_watcher_base
	{
	public:
		model_file_info& get_model();

	private:
		void do_notify(file_notify_info info) final;

	private:
		model_file_info mModel;
	};
}
