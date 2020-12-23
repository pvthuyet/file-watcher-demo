#pragma once

#include "iobserver.h"
#include "irequest.h"
#include <gsl/pointers>
#include <atomic>
#include <vector>
#include <Windows.h>

namespace died
{
	namespace observer 
	{
		namespace callback
		{
			unsigned WINAPI start_thread_proc(LPVOID);
			VOID CALLBACK terminate_proc(__in ULONG_PTR);
			VOID CALLBACK add_directory_proc(__in ULONG_PTR);
		}
	}

	class observer_impl final : public iobserver
	{
	public:
		observer_impl(idirectory_watcher*);

		// diable copy
		observer_impl(observer_impl const&) = delete;
		observer_impl& operator=(observer_impl const&) = delete;

		// interface
	private:
		unsigned int do_inc_request() final;
		unsigned int do_dec_request() final;
		idirectory_watcher* do_get_watcher() const final;

	private:
		void run();
		bool terminated() const;
		bool empty_request() const;
		bool add_directory(irequest* pBlock);
		void request_termination();

		friend unsigned WINAPI observer::callback::start_thread_proc(LPVOID);
		friend VOID CALLBACK observer::callback::terminate_proc(__in ULONG_PTR);
		friend VOID CALLBACK observer::callback::add_directory_proc(__in ULONG_PTR);

	private:
		gsl::not_null<idirectory_watcher*> mDirWatcher;
		std::atomic_bool mTerminated{};
		std::atomic_uint mOutstandingRequests{};
		std::vector<gsl::not_null<irequest*>> mBlocks;
	};
}