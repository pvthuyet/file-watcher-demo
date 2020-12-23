#pragma once

#include "directory_watcher_base.h"

namespace died
{
	class security_watcher : public directory_watcher_base
	{
	private:
		void do_notify(file_notify_info info) final;
	};
}
