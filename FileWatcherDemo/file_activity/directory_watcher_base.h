#pragma once

#include "idirectory_watcher.h"
#include <Windows.h>
#include "iobserver.h"
#include "watching_setting.h"
#include "unnecessary_directory.h" //++ TODO

namespace died
{
	class directory_watcher_base : public idirectory_watcher
	{
	public:
		directory_watcher_base() noexcept = default;
		~directory_watcher_base() noexcept override;

		void set_rule(std::shared_ptr<fat::UnnecessaryDirectory>);

		bool add_setting(watching_setting&& sett);

		bool start();
		void stop() noexcept;

		// movable
		directory_watcher_base(directory_watcher_base&&) noexcept;
		directory_watcher_base& operator=(directory_watcher_base&&) noexcept;

	private:
		void filter_notify(file_notify_info info) final;
		virtual void do_notify(file_notify_info info) = 0;

	private:
		std::vector<watching_setting> mSettings;
		HANDLE mObserverThread{ nullptr };
		unsigned mThreadId{};
		std::unique_ptr<iobserver> mObserver{};
		std::shared_ptr<fat::UnnecessaryDirectory> mRule; //++ TODO
	};
}