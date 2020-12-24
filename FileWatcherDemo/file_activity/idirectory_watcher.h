#pragma once

#include "file_notify_info.h"
#include <memory>

namespace died
{
	class idirectory_watcher
	{
	public:
		virtual ~idirectory_watcher() noexcept = default;

		void notify(file_notify_info&& info)
		{
			filter_notify(std::move(info));
		}

	private:
		virtual void filter_notify(file_notify_info info) = 0;
	};
}