#pragma once

#include "file_name_watcher.h"
#include "attribute_watcher.h"
#include "security_watcher.h"
#include "folder_name_watcher.h"
#include "task_timer.h"

namespace died
{
	class directory_watcher_mgr : public died::TaskTimer
	{
		struct watching_group
		{
			file_name_watcher mFileName;
			attribute_watcher mAttr;
			security_watcher mSecu;
			folder_name_watcher mFolderName;
		};

	public:
		explicit directory_watcher_mgr(unsigned long interval = 300ul);
		bool start(unsigned long notifyChange, bool subtree = true);
		void stop();

	private:
		TimerStatus onTimer() final;
		void notify_attribute(watching_group& group) const;
		void notify_security(watching_group& group) const;
		void notify_folder_name(watching_group& group) const;
		void notify_rename(watching_group& group) const;
		void notify_create(watching_group& group);
		void notify_remove(watching_group& group);
		void notify_modify(watching_group& group) const;
		void erase_all(watching_group& group, std::wstring const& key) const;

	private:
		std::vector<watching_group> mWatchers;
		std::shared_ptr<fat::UnnecessaryDirectory> mRule; //++ TODO
	};
}