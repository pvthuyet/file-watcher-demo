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
			checking_folder_name(el);
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
		//++ TODO attribute, security
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

		//++ TODO filter
		//...

		// 4. notify this item
		SPDLOG_INFO(L"{}", info.get_path_wstring());

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

		//++ TODO filter
		//...

		// 4. notify this item
		SPDLOG_INFO(L"{}", info.get_path_wstring());

		// 5. erase processed item
		model.erase(info.get_path_wstring());

		// 6. jump to next item for next step
		model.next_available_item();
	}

	void directory_watcher_mgr::checking_folder_name(watching_group& group) 
	{
		auto& model = group.mFolderName.get_model();
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

		//++ TODO filter
		//...

		// 4. notify this item
		SPDLOG_INFO(L"{}", info.get_path_wstring());

		// 5. erase processed item
		model.erase(info.get_path_wstring());

		// 6. jump to next item for next step
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
			return;
		}

		// **Goal of rename
		// newName and oldName should not appear in 'add' or 'remove' or 'modify'

		auto oldName = info.mOldName.get_path_wstring();
		auto newName = info.mNewName.get_path_wstring();

		// 3. checking in add
		auto const& addOld = group.mFileName.get_add().find(oldName);
		if (addOld) {
			model.next_available_item();
			return;
		}

		auto const& addNew = group.mFileName.get_add().find(newName);
		if (addNew) {
			model.next_available_item();
			return;
		}

		// 4. checking in remove
		auto const& rmvOld = group.mFileName.get_remove().find(oldName);
		if (rmvOld) {
			model.next_available_item();
			return;
		}

		auto const& rmvNew = group.mFileName.get_remove().find(newName);
		if (rmvNew) {
			model.next_available_item();
			return;
		}

		// 5. checking in modify
		auto const& modiOld = group.mFileName.get_modify().find(oldName);
		if (modiOld) {
			model.next_available_item();
			return;
		}

		auto const& modiNew = group.mFileName.get_modify().find(newName);
		if (modiNew) {
			model.next_available_item();
			return;
		}

		// 6. Finally this is 100% rename
		SPDLOG_INFO(L"Rename: {} - {}", oldName, newName);
		erase_rename(group, newName);
		model.next_available_item();
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
		if (died::fileIsProcessing(key)) {
			model.next_available_item();
			return;
		}

		// **case 1: pure creation
		if (is_create_pure(info, group)) {
			SPDLOG_INFO(L"Create pure {}", key);
			erase_all(group, key);
			model.next_available_item();
			return;
		}

		//** case 2: create file by temporary
		// happen when save-as word, excel, etc
		std::wstring temp1, temp2;
		if (is_create_word_save_as(info, group, temp1, temp2)) {
			SPDLOG_INFO(L"Create word save-as {} - {} - {}", key, temp1, temp2);
			erase_all(group, key);
			erase_all(group, temp1);
			erase_all(group, temp2);
			erase_rename(group, key);
			erase_rename(group, temp1);
			erase_rename(group, temp2);
			model.next_available_item();
			return;
		}

		//** case 3: Create 2 temporary file
		// happen when download chrome auto-save
		std::wstring finalName, temp3;
		if (is_create_brower_auto_save(info, group, temp3, finalName)) {
			SPDLOG_INFO(L"Create brower auto-save {} - {} - {}", finalName, key, temp3);
			erase_all(group, key);
			erase_all(group, temp3);
			erase_all(group, finalName);
			erase_rename(group, key);
			erase_rename(group, temp3);
			erase_rename(group, finalName);
			model.next_available_item();
			return;
		}

		//** case 4: modify by temporary file
		// happen when save word
		std::wstring realFile;
		if (is_create_temporary_for_modify(info, group, realFile)) {
			// will process in checking_modify
			return;
		}

		//** case 5: middle temporary file
		// happen when download chrome save-as
		if (is_create_middle_temporary(info, group)) {
			// No report to server
			SPDLOG_INFO(L"Create middle temporary file {}", key);
			erase_all(group, key);
			erase_rename(group, key);
			model.next_available_item();
			return;
		}

		//** case 6 create txt file then rename
		// happen when create txt file by explore then rename
		// this case doesn't exist modify
		std::wstring realFileCreateChange;
		if (is_create_txt_then_rename_name(info, group, realFileCreateChange)) {
			SPDLOG_INFO(L"Create txt file then rename name {}", realFileCreateChange);
			erase_all(group, key);
			erase_all(group, realFileCreateChange);
			erase_rename(group, realFileCreateChange);
			model.next_available_item();
			return;
		}

		//** case 7: save-as notepad
		// happen when save-as notepad
		if (is_create_txt_save_as(info, group)) {
			SPDLOG_INFO(L"Create txt save-as", key);
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
		auto const& addInfo = group.mFileName.get_add().find(key);
		if (addInfo) {
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

		// Otherwise this is remove action
		SPDLOG_INFO(L"Remove: {}", key);
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
		if (died::fileIsProcessing(key)) {
			model.next_available_item();
			return;
		}

		// Happen when save word
		std::wstring temp2;
		std::wstring realFile;
		if (is_modify_by_temporary(info, group, temp2, realFile)) {
			SPDLOG_INFO(L"Modify by 2 temporary files: {} - {} - {}", realFile, key, temp2);
			erase_all(group, key);
			erase_all(group, temp2);
			erase_all(group, realFile);
			erase_rename(group, temp2);
			erase_rename(group, realFile);
			model.next_available_item();
			return;
		}

		// happen when download chrome save-as
		// middle temporary file
		if (is_create_middle_temporary(info, group)) {
			// will process in creation
			model.next_available_item();
			return;
		}

		//** case: pure modify
		auto const& add = group.mFileName.get_add().find(key);
		if (add) {
			model.next_available_item();
			return;
		}

		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			model.next_available_item();
			return;
		}

		// should not exist in rename
		if (group.mFileName.exist_in_rename_any(key)) {
			model.next_available_item();
			return;
		}

		// 100% modify
		SPDLOG_INFO(L"Modify: {}", key);
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
		if (died::fileIsProcessing(key)) {
			model.next_available_item();
			return;
		}

		// **Case 1: create pure
		if (is_create_pure(info, group)) {
			// will be process in creation
			return;
		}

		// Make sure not exist in modify
		auto const& modi = group.mFileName.get_modify().find(key);
		if (modi) {
			return;
		}

		// **case 1: create temporary file
		// happen when save-as word, excel, etc
		std::wstring temp1, temp2;
		if (is_create_word_save_as(info, group, temp1, temp2)) {
			// will process in creation
			return;
		}

		// **case 2: happen when download chrome auto-save
		std::wstring temp3, finalName;
		if (is_create_brower_auto_save(info, group, temp3, finalName)) {
			// will process in creation
			return;
		}

		// **case 3: happen when save word
		std::wstring realFile;
		if (is_create_temporary_for_modify(info, group, realFile)) {
			// will be processs in modify
			return;
		}

		// **case 4: happen when download chrome save-as
		// middle temporary file
		if (is_create_middle_temporary(info, group)) {
			// will process in creation
			return;
		}

		// case 5: create notepad file then change name
		// happen when create txt file by explore then rename
		// this case doesn't exist modify
		std::wstring realFileCreateChange;
		if (is_create_txt_then_rename_name(info, group, realFileCreateChange)) {
			// will process in creation
			return;
		}

		// case 6: save-as notepad
		// happen when save-as notepad
		if (is_create_txt_save_as(info, group)) {
			// will process in creation
			return;
		}

		// **case 7: modify
		// Happen when edit image files
		// receive: remove, add
		// 'remove' must before 'add'
		// should not exist in rename
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (!rmv) {
			return;
		}

		std::chrono::duration<double> diff = info.get_created_time() - rmv.get_created_time();
		if (diff.count() < 0) {
			return;
		}

		if (group.mFileName.exist_in_rename_any(key)) {
			return;
		}

		// 100% for edit emage by mspaint
		SPDLOG_INFO(L"Modify: {}", key);
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
		if (died::fileIsProcessing(key)) {
			model.next_available_item();
			return;
		}

		// **Case 1: create pure
		if (is_create_pure(info, group)) {
			// will be process in creation
			return;
		}

		// **case 2: create temporary file
		// happen when save-as word, excel, etc
		std::wstring temp1, temp2;
		if (is_create_word_save_as(info, group, temp1, temp2)) {
			// will process in creation
			return;
		}

		// **case 3: happen when download chrome auto-save
		std::wstring temp3, finalName;
		if (is_create_brower_auto_save(info, group, temp3, finalName)) {
			// will process in creation
			return;
		}

		// **case 4: happen when save word
		std::wstring realFile;
		if (is_create_temporary_for_modify(info, group, realFile)) {
			// will be processs in modify
			return;
		}

		// **case 5: happen when download chrome save-as
		// middle temporary file
		if (is_create_middle_temporary(info, group)) {
			// will process in creation
			return;
		}

		// happen when create txt file by explore then rename
		// **case 6: create notepad file then rename name
		// this case doesn't exist modify
		std::wstring realFileCreateChange;
		if (is_create_txt_then_rename_name(info, group, realFileCreateChange)) {
			// will process in creation
			return;
		}

		// happen when save-as notepad
		// case 6: save-as notepad
		if (is_create_txt_save_as(info, group)) {
			// will process in creation
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

		if (group.mFileName.exist_in_rename_any(key)) {
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
		SPDLOG_INFO(L"Copy: {}", key);
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
		if (died::fileIsProcessing(key)) {
			model.next_available_item();
			return;
		}

		// **Case 1: create pure
		if (is_create_pure(info, group)) {
			// will be process in creation
			return;
		}

		// **case 2: create temporary file
		// happen when save-as word, excel, etc
		std::wstring temp1, temp2;
		if (is_create_word_save_as(info, group, temp1, temp2)) {
			// will process in creation
			return;
		}

		// **case 3: happen when download chrome auto-save
		std::wstring temp3, finalName;
		if (is_create_brower_auto_save(info, group, temp3, finalName)) {
			// will process in creation
			return;
		}

		// **case 4: happen when save word
		std::wstring realFile;
		if (is_create_temporary_for_modify(info, group, realFile)) {
			// will be processs in modify
			return;
		}

		// **case 5: happen when download chrome save-as
		// middle temporary file
		if (is_create_middle_temporary(info, group)) {
			// will process in creation
			return;
		}

		// case 6: create notepad file then change name
		// happen when create txt file by explore then rename
		// this case doesn't exist modify
		std::wstring realFileCreateChange;
		if (is_create_txt_then_rename_name(info, group, realFileCreateChange)) {
			// will process in creation
			return;
		}

		// **case 7: happen when save-as notepad
		// case 6: save-as notepad
		if (is_create_txt_save_as(info, group)) {
			// will process in creation
			return;
		}

		// **Goal move
		// exist in modify
		// exist in remove but other path
		// NOT exist in rename

		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			// this is not MOVE
			// lets other function checking this item
			return;
		}

		if (group.mFileName.exist_in_rename_any(key)) {
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
				SPDLOG_INFO(L"Move: {} - {}", found.get_path_wstring(), key);
				erase_all(group, key);
				w.mFileName.get_remove().erase(found.get_path_wstring());
				return;
			}
		}
	}

	bool directory_watcher_mgr::is_create_pure(file_notify_info const& info, watching_group& group)
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

		if (group.mFileName.exist_in_rename_any(key)) {
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

	bool directory_watcher_mgr::is_create_word_save_as(file_notify_info const& info, watching_group& group, std::wstring& temp1, std::wstring& temp2)
	{
		// **behaviour
		//step 1: create - D:\test\1.docx
		//step 2: remove - D:\test\1.docx
		//step 3: create - D:\test\1.docx
		//step 4: create - D:\test\~.tmp
		//step 5: modify - D:\test\~.tmp
		//step 6: create - D:\test\1.docx~RF1994986.TMP
		//step 7: remove - D:\test\1.docx~RF1994986.TMP
		//step 8: rename - D:\test\1.docx => D:\test\1.docx~RF1994986.TMP
		//step 9: rename - D:\test\~.tmp => D:\test\1.docx
		//step 10:remove - D:\test\1.docx~RF1994986.TMP

		// Strong condition:
		// 1.docx is not deleted
		// in-case it is deleted => rename time newer than deleted time

		auto key = info.get_path_wstring(); // this is '1.docx'
		auto const& renameModel = group.mFileName.get_rename();
		auto const& ren1 = renameModel.find_by_old_name(key); // step 8
		auto const& ren2 = renameModel.find(key); // step 9

		if (!ren1 || !ren2) {
			return false;
		}

		// checking the time step 8, 9
		std::chrono::duration<double> diff = ren2.mNewName.get_created_time() - ren1.mNewName.get_created_time();
		if (diff.count() < 0) {
			return false;
		}

		// verify step 10
		auto const& rmvTmp = group.mFileName.get_remove().find(ren1.mNewName.get_path_wstring());
		if (!rmvTmp) {
			return false;
		}

		// strong condition
		auto const& rmvRealFile = group.mFileName.get_remove().find(key);
		if (rmvRealFile) {
			std::chrono::duration<double> diff2 = ren2.mNewName.get_created_time() - rmvRealFile.get_created_time();
			if (diff2.count() < 0) {
				return false;
			}
		}

		temp1 = ren1.mNewName.get_path_wstring(); // 1.docx~RF1994986.TMP
		temp2 = ren2.mOldName.get_path_wstring(); // ~.tmp
		return true;
	}

	bool directory_watcher_mgr::is_create_brower_auto_save(file_notify_info const& info, watching_group& group, std::wstring& temp, std::wstring& finalName)
	{
		// **behaviour
		//step 1: create - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp
		//step 2: modify - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp
		//step 3: rename - D:\test\9be830ee-221a-4a6e-a369-c53ac5a76098.tmp => D:\test\1.jpg.crdownload
		//step 4: modify - D:\test\1.jpg.crdownload
		//step 5: rename - D:\test\1.jpg.crdownload => D:\test\1.jpg
		//step 6: modify - D:\test\1.jpg
		//step 7: modify - D:\test\1.jpg

		auto key = info.get_path_wstring(); // step 1
		auto const& modi = group.mFileName.get_modify().find(key);
		if (!modi) {
			return false;
		}

		std::chrono::duration<double> diff1 = modi.get_created_time() - info.get_created_time();
		if (diff1.count() < 0) {
			return false;
		}

		auto const& ren1 = group.mFileName.get_rename().find_by_old_name(key); // step 3
		if (!ren1) {
			return false;
		}

		std::chrono::duration<double> diff2 = ren1.mNewName.get_created_time() - modi.get_created_time();
		if (diff2.count() < 0) {
			return false;
		}

		temp = ren1.mNewName.get_path_wstring();

		// step 4: very file realFile
		auto const& ren2 = group.mFileName.get_rename().find_by_old_name(temp);
		if (!ren2) {
			return false;
		}

		if (ren2) {
			if (ren2.mOldName.get_path_wstring() != ren1.mNewName.get_path_wstring()) {
				return false;
			}

			std::chrono::duration<double> diff3 = ren2.mNewName.get_created_time() - ren1.mNewName.get_created_time();
			if (diff3.count() < 0) {
				return false;
			}
		}

		finalName = ren2.mNewName.get_path_wstring();

		return true;
	}

	bool directory_watcher_mgr::is_create_txt_then_rename_name(file_notify_info const& info, watching_group& group, std::wstring& realFile)
	{
		// **behaviour
		// step 1. create New Text Document.txt
		// step 2. rename New Text Document.txt => 1.txt

		// Strong condition
		// cond 1: New Text Document.txt should not exist in 'modify'
		// cond 2: 1.txt should not exist in 'modify', 'remove', 'rename' old name
		auto key = info.get_path_wstring();

		// step 2
		auto const& ren1 = group.mFileName.get_rename().find_by_old_name(key);
		if (!ren1) {
			return false;
		}
		std::chrono::duration<double> diff = ren1.mOldName.get_created_time() - info.get_created_time();
		if (diff.count() < 0) {
			return false;
		}

		// cond 1
		auto const& modi1 = group.mFileName.get_modify().find(key);
		if (modi1) {
			return false;
		}

		// Cond 2: verify realFile
		realFile = ren1.mNewName.get_path_wstring();
		auto const& modi2 = group.mFileName.get_modify().find(realFile);
		if (modi2) {
			return false;
		}

		auto const& rmv = group.mFileName.get_remove().find(realFile);
		if (rmv) {
			return false;
		}

		auto const& ren2 = group.mFileName.get_rename().find_by_old_name(realFile);
		if (ren2) {
			return false;
		}
		return true;
	}

	bool directory_watcher_mgr::is_create_txt_save_as(file_notify_info const& info, watching_group& group)
	{
		// **behaviour
		// step 1. create 1.txt
		// step 2. remove 1.txt
		// step 3. create 1.txt
		// step 4. modify 1.txt
		
		// Strong condition
		// 1.txt should not exist in rename, 
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

		if (group.mFileName.exist_in_rename_any(key)) {
			return false;
		}

		return true;
	}

	bool directory_watcher_mgr::is_create_temporary_for_modify(file_notify_info const& info, watching_group& group, std::wstring& realFile)
	{
		// **behaviour
		// step 1. create ~.tmp
		// step 2. modify ~.tmp
		// step 3. rename ~.tmp -> 1.docx
		// step 4. 1.docx doesn't exist in rename model
		// Hence, '~.tmp' is temporary file and '1.docx' is the real modify file

		auto key = info.get_path_wstring(); // step 1 (~.tmp)
		auto const& modi = group.mFileName.get_modify().find(key); // step 2 (~.tmp)
		if (!modi) {
			return false;
		}

		std::chrono::duration<double> diff1 = modi.get_created_time() - info.get_created_time();
		if (diff1.count() < 0) {
			return false;
		}

		auto const& ren1 = group.mFileName.get_rename().find_by_old_name(key); // step 3
		if (!ren1) {
			return false;
		}

		std::chrono::duration<double> diff2 = ren1.mNewName.get_created_time() - modi.get_created_time();
		if (diff2.count() < 0) {
			return false;
		}

		realFile = ren1.mNewName.get_path_wstring(); // 1.docx

		// step 4: very file realFile
		auto const& ren2 = group.mFileName.get_rename().find_by_old_name(realFile);
		if (ren2) {
			return false;
		}

		return true;
	}

	bool directory_watcher_mgr::is_create_middle_temporary(file_notify_info const& info, watching_group& group)
	{
		// **behaviour
		// step 1: create 1.docx~RF19b562f.TMP
		// step 2: rename 1.docx => 1.docx~RF19b562f.TMP
		// step 3: remove 1.docx~RF19b562f.TMP
		auto key = info.get_path_wstring();

		auto const& ren = group.mFileName.get_rename().find(key);
		if (!ren) {
			return false;
		}

		std::chrono::duration<double> diff1 = ren.mNewName.get_created_time() - info.get_created_time();
		if (diff1.count() < 0) {
			return false;
		}

		auto const& rmv = group.mFileName.get_remove().find(key);
		if (!rmv) {
			return false;
		}

		std::chrono::duration<double> diff2 = rmv.get_created_time() - ren.mNewName.get_created_time();
		if (diff2.count() < 0) {
			return false;
		}
		
		return true;
	}

	bool directory_watcher_mgr::is_modify_by_temporary(file_notify_info const& info, watching_group& group, std::wstring& temp2, std::wstring& realFile)
	{
		// **behaviour
		//step 1: create - C:\tmp\~.tmp
		//step 2: modify - C:\tmp\~.tmp
		//step 3: create - C:\tmp\2.docx~RF5cb3d899.TMP
		//step 4: remove - C:\tmp\2.docx~RF5cb3d899.TMP
		//step 5: rename - C:\tmp\2.docx => C:\tmp\2.docx~RF5cb3d899.TMP
		//step 6: rename - C:\tmp\~.tmp => C:\tmp\2.docx
		//step 7: remove - C:\tmp\2.docx~RF5cb3d899.TMP

		auto key = info.get_path_wstring(); // step 1 (~.tmp)
		auto const& add = group.mFileName.get_add().find(key); // step 2 (~.tmp)
		if (!add) {
			return false;
		}

		std::chrono::duration<double> diff1 = info.get_created_time() - add.get_created_time();
		if (diff1.count() < 0) {
			return false;
		}

		auto const& ren1 = group.mFileName.get_rename().find_by_old_name(key); // step 3
		if (!ren1) {
			return false;
		}

		std::chrono::duration<double> diff2 = ren1.mNewName.get_created_time() - info.get_created_time();
		if (diff2.count() < 0) {
			return false;
		}

		realFile = ren1.mNewName.get_path_wstring();

		// step 4: very file realFile
		auto const& ren2 = group.mFileName.get_rename().find_by_old_name(realFile);

		if (ren2) {
			std::chrono::duration<double> diff3 = ren1.mNewName.get_created_time() - ren2.mNewName.get_created_time();
			if (diff3.count() < 0) {
				return false;
			}
		}

		temp2 = ren2.mNewName.get_path_wstring();

		return true;
	}
}
