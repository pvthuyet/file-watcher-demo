#pragma once

#include "directory_watcher_base.h"
#include "model_rename.h"

namespace died
{
	class file_name_watcher : public directory_watcher_base
	{
	public:
		model_rename& get_rename();

	private:
		void do_notify(file_notify_info info) final;

	private:
		model_rename mRename;
	};
}