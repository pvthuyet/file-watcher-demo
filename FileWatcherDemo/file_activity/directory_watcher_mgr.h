#pragma once

#include "file_name_watcher.h"
#include "attribute_watcher.h"
#include "security_watcher.h"
#include "folder_name_watcher.h"
#include "task_timer.h"
#include "notify_to_server.h"

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
		void erase_all(watching_group& group, std::wstring const& key);
		void erase_rename(watching_group& group, std::wstring const& key);

		void checking_attribute(watching_group& group);
		void checking_security(watching_group& group);
		void checking_folder_remove(watching_group& group);
		void checking_folder_move(watching_group& group);
		void checking_rename(watching_group& group);
		void checking_create(watching_group& group);
		void checking_remove(watching_group& group);
		void checking_modify(watching_group& group) ;
		void checking_modify_without_modify_event(watching_group& group);
		void checking_copy(watching_group& group);
		void checking_move(watching_group& group);

	private:
		bool is_rename_only(rename_notify_info const& info, watching_group& group);
		bool is_rename_one_time(rename_notify_info const& info, watching_group& group);
		bool is_rename_download_auto_save(rename_notify_info const& info, watching_group& group, std::wstring& realFile, std::wstring& temp1, std::wstring& temp2);
		bool is_rename_word_save_as(rename_notify_info const& info, watching_group& group, std::wstring& realFile, std::wstring& temp1, std::wstring& temp2);
		bool is_rename_word_save(rename_notify_info const& info, watching_group& group, std::wstring& realFile, std::wstring& temp1, std::wstring& temp2);
		bool is_temporary_file(file_notify_info const& info, watching_group& group);
		bool is_save_as_txt(file_notify_info const& info, watching_group& group);
		bool is_create_only(file_notify_info const& info, watching_group& group);

	private:
		std::vector<watching_group> mWatchers;
		std::shared_ptr<fat::UnnecessaryDirectory> mRule; //++ TODO
		notify_to_server mSender;
	};
}