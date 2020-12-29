#include "directory_watcher_mgr.h"
#include "common_utils.h"
#include "watching_setting.h"
#include "spdlog_header.h"

namespace died
{
	constexpr size_t DELAY_PROCESS = 3000; // milli-second
	directory_watcher_mgr::directory_watcher_mgr(unsigned long interval) :
		TaskTimer(interval),
		mRule{ std::make_shared<fat::UnnecessaryDirectory>() }
	{
		mRule->setAppDataDir(true);
		mRule->addUserDefinePath(L"C:\\Windows\\");
		mRule->addUserDefinePath(L"C:\\ProgramData\\");
		mRule->addUserDefinePath(L"C:\\project\\");
		mRule->addUserDefinePath(L"D:\\work\\");
	}

	bool directory_watcher_mgr::start(unsigned long notifyChange, bool subtree)
	{
		unsigned long actionFileName	= notifyChange & (notifyChange ^ (FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SECURITY));
		unsigned long actionAttr		= FILE_NOTIFY_CHANGE_ATTRIBUTES & notifyChange;
		unsigned long actionSecu		= FILE_NOTIFY_CHANGE_SECURITY & notifyChange;
		unsigned long actionFolderName	= FILE_NOTIFY_CHANGE_DIR_NAME & notifyChange;

		auto drives = enumerate_drives();
		auto size = drives.size();

		// cleanup data
		mWatchers.clear();
		mWatchers.reserve(size);
		
		for (auto const& el : drives) {
			watching_group group;
			
			// 1. watching file name
			watching_setting setFileName(actionFileName, el, subtree);
			group.mFileName.add_setting(std::move(setFileName));
			group.mFileName.set_rule(mRule);

			// 2. watching attribute
			watching_setting setAttr(actionAttr, el, subtree);
			group.mAttr.add_setting(std::move(setAttr));
			group.mAttr.set_rule(mRule);

			// 3. watching security
			watching_setting setSecu(actionSecu, el, subtree);
			group.mSecu.add_setting(std::move(setSecu));
			group.mSecu.set_rule(mRule);

			// 4. watching folder name
			watching_setting setFolderName(actionFolderName, el, subtree);
			group.mFolderName.add_setting(std::move(setFolderName));
			group.mFolderName.set_rule(mRule);

			mWatchers.push_back(std::move(group));
		}

		// start watching
		for (auto& el : mWatchers) {
			el.mFileName.start();
			el.mAttr.start();
			el.mSecu.start();
			el.mFolderName.start();
		}

		// start timer thread
		startTimer();
		return true;
	}

	void directory_watcher_mgr::stop()
	{
		stopTimer();
		for (auto& el : mWatchers) {
			el.mFileName.stop();
			el.mAttr.stop();
			el.mSecu.stop();
			el.mFolderName.stop();
		}
	}

	TimerStatus directory_watcher_mgr::onTimer()
	{
		for (auto& el : mWatchers) {
			checking_attribute(el);
			checking_security(el);
			checking_folder_remove(el);
			checking_folder_move(el);
			checking_rename(el);
			checking_create(el);
			checking_remove(el);
			checking_modify(el);
			checking_modify_without_modify_event(el);
			checking_copy(el);
			checking_move(el);
		}
		return TimerStatus::TIMER_CONTINUE;
	}

	void directory_watcher_mgr::erase_all(watching_group& group, std::wstring const& key)
	{
		SPDLOG_INFO(key);
		group.mFileName.get_add().erase(key);
		group.mFileName.get_remove().erase(key);
		group.mFileName.get_modify().erase(key);
		group.mAttr.get_model().erase(key);
		group.mSecu.get_model().erase(key);
	}

	void directory_watcher_mgr::erase_rename(watching_group& group, std::wstring const& key)
	{
		SPDLOG_INFO(key);
		group.mFileName.get_rename().erase(key);
		group.mAttr.get_model().erase(key);
		group.mSecu.get_model().erase(key);
	}

	void directory_watcher_mgr::checking_attribute(watching_group& group) 
	{
		auto& model = group.mAttr.get_model();
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		auto key = info.get_path_wstring();

		// 3. should not exist in add, modify, remove, rename
		if (group.mFileName.get_add().find(key)) {
			model.next_available_item();
			return;
		}

		if (group.mFileName.get_remove().find(key)) {
			model.next_available_item();
			return;
		}

		if (group.mFileName.get_modify().find(key)) {
			model.next_available_item();
			return;
		}

		if (group.mFileName.get_rename().find(key)) {
			model.next_available_item();
			return;
		}

		// 4. notify this item
		mSender.send(L"Attribute", key);

		// 5. erase processed item
		model.erase(info.get_path_wstring());

		// 6. jump to next item for next step
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_security(watching_group& group) 
	{
		auto& model = group.mSecu.get_model();
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		auto key = info.get_path_wstring();

		// 3. should not exist in add, modify, remove, rename
		if (group.mFileName.get_add().find(key)) {
			model.next_available_item();
			return;
		}

		if (group.mFileName.get_remove().find(key)) {
			model.next_available_item();
			return;
		}

		if (group.mFileName.get_modify().find(key)) {
			model.next_available_item();
			return;
		}

		if (group.mFileName.get_rename().find(key)) {
			model.next_available_item();
			return;
		}

		// 4. notify this item
		mSender.send(L"Security", key);

		// 5. erase processed item
		model.erase(info.get_path_wstring());

		// 6. jump to next item for next step
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_folder_remove(watching_group& group) 
	{
		auto& model = group.mFolderName.get_remove();
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// Make sure this is not move
		// receive: add, delete (in the same disk)
		// reveive: add, delete, modify (different disk)
		for (auto& w : mWatchers) {
			auto const& found = w.mFolderName.get_add().find_if([&info](auto const& item) {
				return info.get_file_name_wstring() == item.get_file_name_wstring()
					&& info.get_parent_path_wstring() != item.get_parent_path_wstring();
			});

			// this is move action will process in folder move
			if (found) {
				model.next_available_item();
				return;
			}
		}

		// 100% only remove
		mSender.send(L"Folder remove", key);
		model.erase(key);
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_folder_move(watching_group& group)
	{
		// get model
		auto& model = group.mFolderName.get_add();

		// pop item
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// **Goal move
		// exist in remove but other path
		// Should exist filename in other path
		for (auto& w : mWatchers) {
			auto const& found = w.mFolderName.get_remove().find_if([&info](auto const& item) {
				return info.get_file_name_wstring() == item.get_file_name_wstring()
					&& info.get_parent_path_wstring() != item.get_parent_path_wstring();
			});

			// The parent path must differnt
			// 100% MOVE
			if (found) {
				mSender.send(L"Folder move", found.get_path_wstring() + L", " + key);
				model.erase(key);
				w.mFolderName.get_remove().erase(found.get_path_wstring());
				model.next_available_item();
				return;
			}
		}

		// If not move should erase of out data
		model.erase(key);
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_rename(watching_group& group) 
	{
		auto& model = group.mFileName.get_rename();
		auto const& info = model.front();

		// 1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.mNewName.alive()) {
			// Waiting on this file
			return;
		}

		// **Goal of rename
		// newName and oldName should not appear in 'add' or 'remove' or 'modify'

		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		// 3. file is processing => ignore this file, jump to next one
		int error{};
		if (died::fileIsProcessing(newName, error)) {
			// Waiting on this file
			return;
		}

		// **case 1: only rename action
		// happen when rename a file
		if (is_rename_only(info, group)) {
			mSender.send(L"Rename only", oldName + L", " + newName);
			erase_rename(group, newName);
			model.next_available_item();
			return;
		}

		// **case 2: rename 1 time
		// happen when create a file by explore then rename
		// consider as create file
		if (is_rename_one_time(info, group)) {
			auto const& add = group.mFileName.get_add().find(newName);
			if (add) {
				mSender.send(L"Create by rename", oldName + L", " + newName);
			}
			else {
				mSender.send(L"Modify by rename", oldName + L", " + newName);
			}
			erase_all(group, oldName);
			erase_all(group, newName);
			erase_rename(group, newName);
			model.next_available_item();
			return;
		}

		// **case 3: download auto-save
		std::wstring realFile, temp1, temp2;
		if (is_rename_download_auto_save(info, group, realFile, temp1, temp2)) {
			mSender.send(L"Create download auto-save", realFile + L", " + temp1 + L", " + temp2);
			erase_all(group, realFile);
			erase_all(group, temp1);
			erase_all(group, temp2);
			erase_rename(group, realFile);
			erase_rename(group, temp1);
			erase_rename(group, temp2);
			model.next_available_item();
			return;
		}

		// **case 4: save-as word
		if (is_rename_word_save_as(info, group, realFile, temp1, temp2)) {
			mSender.send(L"Create word save-as", realFile + L", " + temp1 + L", " + temp2);
			erase_all(group, realFile);
			erase_all(group, temp1);
			erase_all(group, temp2);
			erase_rename(group, realFile);
			erase_rename(group, temp1);
			erase_rename(group, temp2);
			model.next_available_item();
			return;
		}

		// **case 5: save word
		if (is_rename_word_save(info, group, realFile, temp1, temp2)) {
			mSender.send(L"Modify word save", realFile + L", " + temp1 + L", " + temp2);
			erase_all(group, realFile);
			erase_all(group, temp1);
			erase_all(group, temp2);
			erase_rename(group, realFile);
			erase_rename(group, temp1);
			erase_rename(group, temp2);
			model.next_available_item();
			return;
		}

		// Here consider as not enough information
		// so just wait
	}

	void directory_watcher_mgr::checking_create(watching_group& group)
	{
		// get model
		auto& model = group.mFileName.get_add();

		// pop item
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// 3. file is processing => ignore this file, jump to next one
		int error;
		if (died::fileIsProcessing(key, error)) {
			model.next_available_item();
			return;
		}

		// **case 1: dont interest in file that exist in rename
		// happen when save, save-as word
		if (group.mFileName.exist_in_rename_any(key)) {
			// will be processed in rename
			model.next_available_item();
			return;
		}

		// **case 2: temporary file
		// happen when download big file by save-as
		// will create -> remove -> waiting to rename
		if (is_temporary_file(info, group)) {
			// will be processed later in rename
			//++ TODO delete temporary
			model.next_available_item();
			return;
		}

		// **case 3: save-as .txt by notepad
		if (is_save_as_txt(info, group)) {
			mSender.send(L"Create by save-as", key);
			erase_all(group, key);
			model.next_available_item();
			return;
		}

		// **case 4: only create
		if (is_create_only(info, group)) {
			mSender.send(L"Create only", key);
			erase_all(group, key);
			model.next_available_item();
			return;
		}
	}

	void directory_watcher_mgr::checking_remove(watching_group& group)
	{
		auto& model = group.mFileName.get_remove();
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// **Goal of remove : should not exist in any groups

		// case 1: should not exist in rename
		// happen when create, save, save-as word
		if (group.mFileName.exist_in_rename_any(key)) {
			model.next_available_item();
			return;
		}

		// Case 2: should not exist in add
		// happen when edit image file
		auto const& add = group.mFileName.get_add().find(key);
		if (add) {
			model.next_available_item();
			return;
		}

		// case 3: 'filename' should not exist in any 'add model' of other groups
		// happen when move file
		// receive: add, delete (in the same disk)
		// reveive: add, delete, modify (different disk)
		for (auto& w : mWatchers) {
			auto const& found = w.mFileName.get_add().find_if([&info](auto const& item) {
				return info.get_file_name_wstring() == item.get_file_name_wstring()
					&& info.get_parent_path_wstring() != item.get_parent_path_wstring();
			});

			// this is not remove action
			if (found) {
				model.next_available_item();
				return;
			}
		}

		// 100% only remove
		mSender.send(L"Remove", key);
		model.erase(key);
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_modify(watching_group& group) 
	{
		// get model
		auto& model = group.mFileName.get_modify();
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// 3. file is processing => ignore this file, jump to next one
		int error;
		if (died::fileIsProcessing(key, error)) {
			model.next_available_item();
			return;
		}

		// **case 1: should not exist in rename
		// happen when save, save-as word
		if (group.mFileName.exist_in_rename_any(key)) {
			// will be processed in rename
			model.next_available_item();
			return;
		}

		// **case 2: not exist in add
		auto const& add = group.mFileName.get_add().find(key);
		if (add) {
			model.next_available_item();
			return;
		}

		// **case 3: not exist in remove
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			model.next_available_item();
			return;
		}

		// 100% modify
		mSender.send(L"Modify", key);
		erase_all(group, key);
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_modify_without_modify_event(watching_group& group) 
	{
		// get model
		auto& model = group.mFileName.get_add();

		// pop item
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// 3. file is processing => ignore this file, jump to next one
		int error;
		if (died::fileIsProcessing(key, error)) {
			model.next_available_item();
			return;
		}

		// Make sure not exist in modify
		auto const& modi = group.mFileName.get_modify().find(key);
		if (modi) {
			return;
		}

		// **case 1: should not exist in rename
		// happen when save, save-as word
		if (group.mFileName.exist_in_rename_any(key)) {
			// will be processed in rename
			model.next_available_item();
			return;
		}

		// **case 2: temporary file
		// happen when download big file by save-as
		// will create -> remove -> waiting to rename
		if (is_temporary_file(info, group)) {
			// will be processed later in rename
			model.next_available_item();
			return;
		}

		// **case 3: save-as .txt by notepad
		if (is_save_as_txt(info, group)) {
			// process in create
			return;
		}

		// **case 4: only create
		if (is_create_only(info, group)) {
			// process in create
			return;
		}

		// **case 5: modify
		// Happen when edit image files
		// receive: remove -> add
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (!rmv) {
			return;
		}

		std::chrono::duration<double> diff = info.get_created_time() - rmv.get_created_time();
		if (diff.count() < 0) {
			return;
		}

		// 100% for edit emage by mspaint
		mSender.send(L"Modify without modify event", key);
		erase_all(group, key);
		// jump to next item for next step
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_copy(watching_group& group)
	{
		// get model
		auto& model = group.mFileName.get_add();

		// pop item
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// 3. file is processing => ignore this file, jump to next one
		int error;
		if (died::fileIsProcessing(key, error)) {
			model.next_available_item();
			return;
		}

		// **case 1: should not exist in rename
		// happen when save, save-as word
		if (group.mFileName.exist_in_rename_any(key)) {
			// will be processed in rename
			model.next_available_item();
			return;
		}

		// **case 2: temporary file
		// happen when download big file by save-as
		// will create -> remove -> waiting to rename
		if (is_temporary_file(info, group)) {
			// will be processed later in rename
			model.next_available_item();
			return;
		}

		// **case 3: save-as .txt by notepad
		if (is_save_as_txt(info, group)) {
			// process in create
			return;
		}

		// **case 4: only create
		if (is_create_only(info, group)) {
			// process in create
			return;
		}

		// ** Goal copy: 
		// exist in 'modify' 
		// but NOT in 'remove', 'rename'
		auto const& modi = group.mFileName.get_modify().find(key);
		if (!modi) {
			// Not action copy
			// lets other function checking this item
			return;
		}

		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			// Not action copy
			// lets other function checking this item
			return;
		}

		// Not action copy => maybe move
		for (auto& w : mWatchers) {
			auto const& found = w.mFileName.get_remove().find_if([&info](auto const& item) {
				return info.get_file_name_wstring() == item.get_file_name_wstring()
					&& info.get_parent_path_wstring() != item.get_parent_path_wstring();
			});

			// this is not copy
			// lets other function checking this item
			if (found) {
				return;
			}
		}

		// 100% copy
		mSender.send(L"Copy", key);
		erase_all(group, key);
		// jump to next item for next step
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_move(watching_group& group)
	{
		// get model
		auto& model = group.mFileName.get_add();

		// pop item
		auto const& info = model.front();

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.alive()) {
			return;
		}

		// get key
		auto key = info.get_path_wstring();

		// 3. file is processing => ignore this file, jump to next one
		int error;
		if (died::fileIsProcessing(key, error)) {
			model.next_available_item();
			return;
		}

		// **case 1: should not exist in rename
		// happen when save, save-as word
		if (group.mFileName.exist_in_rename_any(key)) {
			// will be processed in rename
			model.next_available_item();
			return;
		}

		// **case 2: temporary file
		// happen when download big file by save-as
		// will create -> remove -> waiting to rename
		if (is_temporary_file(info, group)) {
			// will be processed later in rename
			model.next_available_item();
			return;
		}

		// **case 3: save-as .txt by notepad
		if (is_save_as_txt(info, group)) {
			// process in create
			return;
		}

		// **case 4: only create
		if (is_create_only(info, group)) {
			// process in create
			return;
		}

		// **Goal move
		// exist in remove but other path
		// NOT exist in rename

		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			// this is not MOVE
			// lets other function checking this item
			return;
		}

		// Should exist filename in other path
		for (auto& w : mWatchers) {
			auto const& found = w.mFileName.get_remove().find_if([&info](auto const& item) {
				return info.get_file_name_wstring() == item.get_file_name_wstring()
					&& info.get_parent_path_wstring() != item.get_parent_path_wstring();
			});

			// The parent path must differnt
			// 100% MOVE
			if (found) {
				mSender.send(L"Move", found.get_path_wstring() + L", " + key);
				erase_all(group, key);
				w.mFileName.get_remove().erase(found.get_path_wstring());
				model.next_available_item();
				return;
			}
		}
	}

	bool directory_watcher_mgr::is_rename_only(rename_notify_info const& info, watching_group& group)
	{
		// oldName and newName should not exist in add
		auto const& addModel = group.mFileName.get_add();
		if (addModel.find(info.mOldName.get_path_wstring())) {
			return false;
		}

		if (addModel.find(info.mNewName.get_path_wstring())) {
			return false;
		}
		return true;
	}

	bool directory_watcher_mgr::is_rename_one_time(rename_notify_info const& info, watching_group& group)
	{
		// method
		// step 1: oldName must exist in add
		// step 2: newName must NOT exist in other 'oldname rename model'
		// step 3: oldName must NOT exist in other 'newname rename model'

		/// step 2: newName must NOT exist in add
		// step 3: newName must NOT exist in remove

		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		auto const& addModel = group.mFileName.get_add();

		// step 1
		if (!addModel.find(oldName)) {
			return false;
		}

		// step 2
		auto const& renameModel = group.mFileName.get_rename();
		if (renameModel.find_by_old_name(newName)) {
			return false;
		}

		// step 3:
		if (renameModel.find(oldName)) {
			return false;
		}

		return true;
	}

	bool directory_watcher_mgr::is_rename_download_auto_save(rename_notify_info const& info, watching_group& group, std::wstring& realFile, std::wstring& temp1, std::wstring& temp2)
	{
		// **behaviour
		//step 1: create - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp
		//step 2: modify - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp
		//step 3: rename - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp => D:\test\1.jpg.crdownload
		//step 4: modify - D:\test\1.jpg.crdownload
		//step 5: rename - D:\test\1.jpg.crdownload => D:\test\1.jpg
		//step 6: modify - D:\test\1.jpg
		//step 7: modify - D:\test\1.jpg
		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		// **case 1: rename at stpe 3
		auto const& renNew = group.mFileName.get_rename().find_by_old_name(newName);
		if (renNew) {
			std::chrono::duration<double> diff = renNew.mNewName.get_created_time() - info.mNewName.get_created_time();
			if (diff.count() > 0) {
				realFile = renNew.mNewName.get_path_wstring();
				temp1 = renNew.mOldName.get_path_wstring();
				temp2 = info.mOldName.get_path_wstring();
				return true;
			}
		}

		// **case 2: rename at step 5
		//auto const& renOld = group.mFileName.get_rename().find(oldName);
		//if (renOld) {
		//	std::chrono::duration<double> diff = info.mNewName.get_created_time() - renOld.mNewName.get_created_time();
		//	if (diff.count() > 0) {
		//		realFile = info.mNewName.get_path_wstring();
		//		temp1 = info.mOldName.get_path_wstring();
		//		temp2 = renOld.mOldName.get_path_wstring();
		//		return true;
		//	}
		//}

		return false;
	}

	bool directory_watcher_mgr::is_rename_word_save_as(rename_notify_info const& info, watching_group& group, std::wstring& realFile, std::wstring& temp1, std::wstring& temp2)
	{
		//step 1: create - D:\test\8.docx
		//step 2: remove - D:\test\8.docx
		//step 3: create - D:\test\8.docx
		//step 4: create - D:\test\~.tmp
		//step 5: modify - D:\test\~.tmp
		//step 6: create - D:\test\8.docx~RF1994986.TMP
		//step 7: remove - D:\test\8.docx~RF1994986.TMP
		//step 8: rename - D:\test\8.docx => D:\test\8.docx~RF1994986.TMP
		//step 9: rename - D:\test\~.tmp => D:\test\8.docx
		//step 10 remove - D:\test\8.docx~RF1994986.TMP
		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		// **case 1: at step 8
		auto const& renNew = group.mFileName.get_rename().find(oldName);
		if (renNew) {
			std::chrono::duration<double> diff = renNew.mNewName.get_created_time() - info.mNewName.get_created_time();
			if (diff.count() > 0) {
				auto const& add = group.mFileName.get_add().find(renNew.mNewName.get_path_wstring());
				if (add) {
					std::chrono::duration<double> diff2 = renNew.mNewName.get_created_time() - add.get_created_time();
					if (diff2.count() > 0) {
						realFile = renNew.mNewName.get_path_wstring();
						temp1 = renNew.mOldName.get_path_wstring();
						temp2 = newName;
						return true;
					}
				}
			}
		}
		return false;
	}

	bool directory_watcher_mgr::is_rename_word_save(rename_notify_info const& info, watching_group& group, std::wstring& realFile, std::wstring& temp1, std::wstring& temp2)
	{
		//step 1: create - D:\test\~.tmp
		//step 2: modify - D:\test\~.tmp
		//step 3: create - D:\test\9.docx~RF19b562f.TMP
		//step 4: remove - D:\test\9.docx~RF19b562f.TMP
		//step 5: rename - D:\test\9.docx => D:\test\9.docx~RF19b562f.TMP
		//step 6: rename - D:\test\~.tmp => D:\test\9.docx
		//step 7: remove - D:\test\9.docx~RF19b562f.TMP

		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		// **case 1: at step 5
		auto const& renNew = group.mFileName.get_rename().find(oldName);
		if (renNew) {
			std::chrono::duration<double> diff = renNew.mNewName.get_created_time() - info.mNewName.get_created_time();
			if (diff.count() > 0) {
				auto const& add = group.mFileName.get_add().find(renNew.mNewName.get_path_wstring());
				if (!add) {
					realFile = renNew.mNewName.get_path_wstring();
					temp1 = renNew.mOldName.get_path_wstring();
					temp2 = newName;
					return true;
				}
			}
		}

		return false;
	}

	bool directory_watcher_mgr::is_temporary_file(file_notify_info const& info, watching_group& group)
	{
		// happen when download big file by save-as
		// will create -> remove -> waiting to rename
		auto key = info.get_path_wstring();
		auto const& rmv = group.mFileName.get_remove().find(key);
		std::chrono::duration<double> diff = info.get_created_time() - rmv.get_created_time();
		if (diff.count() > 0) {
			return false;
		}
		return true;
	}

	bool directory_watcher_mgr::is_save_as_txt(file_notify_info const& info, watching_group& group)
	{
		auto key = info.get_path_wstring();
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (!rmv) {
			return false;
		}

		std::chrono::duration<double> diff1 = info.get_created_time() - rmv.get_created_time();
		if (diff1.count() < 0) {
			return false;
		}

		auto const& modi = group.mFileName.get_modify().find(key);
		if (!modi) {
			return false;
		}

		std::chrono::duration<double> diff2 = modi.get_created_time() - info.get_created_time();
		if (diff2.count() < 0) {
			return false;
		}
		return true;
	}

	bool directory_watcher_mgr::is_create_only(file_notify_info const& info, watching_group& group)
	{
		// **behaviour
		// step 1. create 1.txt
		// should not exist in others: remove, modify rename
		auto key = info.get_path_wstring();

		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			return false;
		}

		auto const& modi = group.mFileName.get_modify().find(key);
		if (modi) {
			return false;
		}

		// Make sure this is not move action
		// this happen when move file
		// receive: add, delete (in the same disk)
		// reveive: add, delete, modify (different disk)
		for (auto& w : mWatchers) {
			auto const& found = w.mFileName.get_remove().find_if([&info](auto const& item) {
				return info.get_file_name_wstring() == item.get_file_name_wstring()
					&& info.get_parent_path_wstring() != item.get_parent_path_wstring();
			});

			// this is not creation, lets other function checking this item
			if (found) {
				return false;
			}
		}

		return true;
	}
}
