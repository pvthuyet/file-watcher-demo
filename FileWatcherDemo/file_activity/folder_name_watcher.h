#pragma once

#include "directory_watcher_base.h"
#include "model_file_info.h"

namespace died
{
	class folder_name_watcher : public directory_watcher_base
	{
	public:
		model_file_info& get_add();
		model_file_info& get_remove();

	private:
		void do_notify(file_notify_info info) final;

	private:
		model_file_info mAdd;
		model_file_info mRemove;
	};
}
