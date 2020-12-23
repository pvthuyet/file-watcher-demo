#pragma once

namespace died
{
	class idirectory_watcher;
	class iobserver
	{
	public:
		virtual ~iobserver() noexcept = default;

		unsigned int inc_request()
		{
			return do_inc_request();
		}

		unsigned int dec_request()
		{
			return do_dec_request();
		}

		idirectory_watcher* get_watcher() const
		{
			return do_get_watcher();
		}

	private:
		virtual unsigned int do_inc_request() = 0;
		virtual unsigned int do_dec_request() = 0;
		virtual idirectory_watcher* do_get_watcher() const = 0;
	};
}