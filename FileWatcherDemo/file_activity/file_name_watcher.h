#pragma once

#include "directory_watcher_base.h"
#include "unnecessary_directory.h"

namespace died
{
	class file_name_watcher : public directory_watcher_base
	{
	public:
		file_name_watcher();

	private:
		void do_notify(file_notify_info info) final;

	private:
		died::fat::UnnecessaryDirectory mRules;
	};
}