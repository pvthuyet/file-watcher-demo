#pragma once

#include "file_notify_info.h"

namespace died
{
	class idirectory_watcher
	{
	public:
		virtual ~idirectory_watcher() noexcept = default;
		void notify(file_notify_info&& info)
		{
			do_notify(std::move(info));
		}

	private:
		virtual void do_notify(file_notify_info info) = 0;
	};
}