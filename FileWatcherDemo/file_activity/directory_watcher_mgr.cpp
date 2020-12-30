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
		mRule->addUserDefinePath(L"D:\\share\\");
		mRule->addUserDefinePath(L"C:\\Program Files (x86)\\");
		mRule->addUserDefinePath(L"C:\\Program Files\\");
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
		// 1. attribute and security first
		group.mAttr.get_model().erase(key);
		group.mSecu.get_model().erase(key);

		// 2. add, remove, modiy
		group.mFileName.get_add().erase(key);
		group.mFileName.get_remove().erase(key);
		group.mFileName.get_modify().erase(key);
	}

	void directory_watcher_mgr::erase_rename(watching_group& group, rename_notify_info const& info)
	{
		SPDLOG_INFO(info.get_key__());
		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		// 1. attribute and security first
		group.mAttr.get_model().erase(oldName);
		group.mSecu.get_model().erase(oldName);
		group.mAttr.get_model().erase(newName);
		group.mSecu.get_model().erase(newName);

		// 2. add, remove, modiy
		group.mFileName.get_add().erase(oldName);
		group.mFileName.get_remove().erase(oldName);
		group.mFileName.get_modify().erase(oldName);
		group.mFileName.get_add().erase(newName);
		group.mFileName.get_remove().erase(newName);
		group.mFileName.get_modify().erase(newName);

		// 3.rename
		group.mFileName.get_rename().erase(info.get_key__());
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

		if (group.mFileName.get_rename().get_number_family(key) > 0) {
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

		if (group.mFileName.get_rename().get_number_family(key) > 0) {
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
		static std::chrono::time_point<std::chrono::steady_clock> lastProcessingTime{};
		int error{};
		bool isProcessing = died::fileIsProcessing(newName, error);
		if (isProcessing) {
			lastProcessingTime = std::chrono::steady_clock::now();
		}
		// Just wait for stable file
		constexpr unsigned long DELAY_FOR_STABLE = 1000; // milli-second
		std::chrono::duration<double, std::milli> diff = std::chrono::steady_clock::now() - lastProcessingTime;
		bool needDelay = diff.count() < DELAY_FOR_STABLE;

		// **case 1: only rename action
		// happen when rename a file
		if (!needDelay && is_rename_only(info, group)) {
			mSender.send(L"Rename only", oldName + L", " + newName);
			erase_rename(group, info);
			model.next_available_item();
			return;
		}

		// **case 2: family rename, happen when 
		// use Word: save-as, save
		// use Excel: save
		// Brower download file: auto-save
		auto family = group.mFileName.get_rename().get_family(info);
		auto numRename = family.size();

		if (numRename >= 2) {
			rename_notify_info const& before = family[0].get();
			rename_notify_info const& after = family[1].get();

			// 2.1 brower download file auto-save
			if (is_rename_download_auto_save(group, before, after)) {
				mSender.send(L"Create download auto-save", 
					after.mNewName.get_path_wstring() + L", " +
					after.mOldName.get_path_wstring() + L", "
					+ before.mOldName.get_path_wstring());
				erase_rename(group, after);
				erase_rename(group, before);
				model.next_available_item();
				return;
			}

			// 2.3 Using Word
			if (is_rename_word(group, before, after)) {
				// 2.1.1 Save-as
				// realFile exist in add model
				std::wstring msgAction;
				auto realNewName = after.mNewName.get_path_wstring();
				if (group.mFileName.get_add().find(realNewName)) {
					msgAction = L"Create Word save-as";
				}
				// 2.1.2 Save
				// realFile not exist in add model
				else {
					msgAction = L"Modify Word save";
				}
				mSender.send(msgAction,
					after.mNewName.get_path_wstring() + L", " +
					after.mOldName.get_path_wstring() + L", "
					+ before.mOldName.get_path_wstring());
				erase_rename(group, after);
				erase_rename(group, before);
				model.next_available_item();
				return;
			}

			// 2.3 Excel save-as => save
			// Actually, there is more 2 rename events
			// Assume current event is create
			mSender.send(L"Create excel save-as", newName + L", " + oldName);
			erase_rename(group, info);
			model.next_available_item();
			return;
		}

		// **case 3: 1 event rename
		// happen when: save-as brower, create and rename a file
		if (!needDelay && is_rename_one_time(info, group)) {
			mSender.send(L"Create rename", newName + L", " + oldName);
			erase_rename(group, info);
			model.next_available_item();
			return;
		}

		// Here we don't have enough information
		// Hence, continue waiting on this file
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
			// will be processed in remove
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

		// case 2: clear the temporary file
		if (is_temporary_file(info, group)) {
			erase_all(group, key);
			model.next_available_item();
			return;
		}

		// Case 3: should not exist in add
		// happen when edit image file
		auto const& add = group.mFileName.get_add().find(key);
		if (add) {
			model.next_available_item();
			return;
		}

		// case 4: 'filename' should not exist in any 'add model' of other groups
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
			// will be processed in remove
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
			// will be processed in remove
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
			// will be process in remove
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
		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		auto const& addModel = group.mFileName.get_add();
		if (addModel.find(oldName)) {
			return false;
		}

		if (addModel.find(newName)) {
			return false;
		}

		// Make sure oldName, newName appears only 1 time
		if (!group.mFileName.get_rename().is_only_one_family_info(info)) {
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

		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		auto const& addModel = group.mFileName.get_add();

		// step 1
		if (!addModel.find(oldName)) {
			return false;
		}

		// step 2
		// Make sure oldName, newName appears only 1 time
		if (!group.mFileName.get_rename().is_only_one_family_info(info)) {
			return false;
		}

		return true;
	}

	bool directory_watcher_mgr::is_rename_download_auto_save(
		watching_group& group,
		rename_notify_info const& before, 
		rename_notify_info const& after)
	{
		// **behaviour
		//step 1: create - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp
		//step 2: modify - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp
		//step 3: rename - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp => D:\test\1.jpg.crdownload
		//step 4: modify - D:\test\1.jpg.crdownload
		//step 5: rename - D:\test\1.jpg.crdownload => D:\test\1.jpg
		//step 6: modify - D:\test\1.jpg
		//step 7: modify - D:\test\1.jpg
		auto istrue = sequence_rename(before, after);
		if (!istrue) {
			return false;
		}
		// newName must not exist in delete
		auto const& rmv = group.mFileName.get_remove().find(after.mNewName.get_path_wstring());
		if (rmv) {
			std::chrono::duration<double> diff = after.mNewName.get_created_time() - rmv.get_created_time();
			if (diff.count() < 0) {
				return false;
			}
		}
		return true;
	}

	bool directory_watcher_mgr::is_rename_word(
		watching_group& group,
		rename_notify_info const& before, 
		rename_notify_info const& after)
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
		auto istrue = circle_rename(before, after);
		if (!istrue) {
			return false;
		}

		// newName must not exist in delete
		auto const& rmv = group.mFileName.get_remove().find(after.mNewName.get_path_wstring());
		if (rmv) {
			std::chrono::duration<double> diff = after.mNewName.get_created_time() - rmv.get_created_time();
			if (diff.count() < 0) {
				return false;
			}
		}
		return true;
	}

	bool directory_watcher_mgr::is_temporary_file(file_notify_info const& info, watching_group& group)
	{
		// happen when download big file by save-as
		// will create -> remove -> waiting to rename
		auto key = info.get_path_wstring();
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (!rmv) {
			return false;
		}

		// Consider as temporary file when exist remove and add or modify
		auto const& add = group.mFileName.get_add().find(key);
		auto const& modi = group.mFileName.get_add().find(key);
		if (!add && !modi) {
			return false;
		}

		if (add) {
			std::chrono::duration<double> diff = rmv.get_created_time() - add.get_created_time();
			if (diff.count() < 0) {
				return false;
			}
		}

		if (modi) {
			std::chrono::duration<double> diff = rmv.get_created_time() - modi.get_created_time();
			if (diff.count() < 0) {
				return false;
			}
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
