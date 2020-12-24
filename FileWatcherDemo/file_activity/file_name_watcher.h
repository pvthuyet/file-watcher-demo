#pragma once

#include "directory_watcher_base.h"
#include "unnecessary_directory.h"
#include "rename_model.h"

namespace died
{
	class file_name_watcher : public directory_watcher_base
	{
	public:
		file_name_watcher();

		rename_model& get_rename();

	private:
		void do_notify(file_notify_info info) final;

	private:
		died::fat::UnnecessaryDirectory mRules; //++ TODO
		rename_model mRename;
	};
}