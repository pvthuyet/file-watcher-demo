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

		//1. Invlid item => should jump to next one for next step
		if (!info) {
			model.next_available_item();
			return;
		}

		// 2. Valid item but need delay
		if (DELAY_PROCESS > info.mNewName.alive()) {
			return;
		}

		auto key = info.mNewName.get_path_wstring();
		//++ TODO filter
		//...
		auto const& add = group.mFileName.get_add().find(key);
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (add && rmv) {
			std::chrono::duration<double> diff = add.get_created_time() - rmv.get_created_time();
			if (diff.count() > 0) {
				SPDLOG_INFO(L"Rename: create file: {}, old name: {}", key, info.mOldName.get_path_wstring());
			}
			else {
				SPDLOG_INFO(L"Rename: temporary file: {}, old name: {}", key, info.mOldName.get_path_wstring());
			}
			model.next_available_item();
			return;
		}

		// **Case: old name exists in 'create' model
		// This happend when create a office files
		// => This is not rename action
		auto const& add2 = group.mFileName.get_add().find(info.mOldName.get_path_wstring());
		if (add2) {
			SPDLOG_INFO(L"Rename: This is not rename action. Maybe office files. {} - {}", 
				info.mOldName.get_path_wstring(), 
				info.mNewName.get_path_wstring());
			model.next_available_item();
			return;
		}


		// 4. notify this item
		SPDLOG_INFO(L"Rename: {} - {}", info.mOldName.get_path_wstring(), info.mNewName.get_path_wstring());

		// 5. erase processed item
		model.erase(info.get_key());

		// 6. jump to next item for next step
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

		//** case 1: check the real file
		std::wstring tempNewName, tempOldName;
		if (is_real_file(info, group, tempNewName, tempOldName)) {
			SPDLOG_INFO(L"Create: {}", key);
			SPDLOG_INFO(L"Create: temporary 1 {}", tempNewName);
			SPDLOG_INFO(L"Create: temporary 2 {}", tempOldName);
			erase_all(group, key);
			erase_all(group, tempNewName);
			erase_all(group, tempOldName);
			erase_rename(group, key);
			erase_rename(group, tempNewName);
			return;
		}

		// **Case 1: create a temporary file
		std::wstring realFile;
		if (is_temporary_file(info, group, realFile)) {
			SPDLOG_INFO(L"Create: {}", realFile);
			SPDLOG_INFO(L"Create: temporary {}", key);
			erase_all(group, key);
			erase_all(group, realFile);
			erase_rename(group, realFile);
			model.next_available_item();
			return;
		}

		// this happend when create office file
		



		//auto& renameModel = group.mFileName.get_rename();
		//auto const& remTmpNew = renameModel.find(key);
		//auto const& remTmpOld = renameModel.find_by_old_name(key);
		//if (remTmpNew && remTmpOld) {
		//	std::chrono::duration<double> diff = remTmpNew.get_created_time() - remTmpOld.mNewName.get_created_time();
		//	if (diff.count() > 0) {
		//		auto tmpKey = remTmpOld.get_key();
		//		SPDLOG_DEBUG(L"Create: {}", key);
		//		SPDLOG_DEBUG(L"Create: This is temporary file {}", tmpKey);
		//		erase_all(group, key);
		//		erase_all(group, tmpKey);
		//		renameModel.erase(key);
		//		renameModel.erase(tmpKey);
		//		return;
		//	}
		//}

		auto const& rmv = group.mFileName.get_remove().find(key);
		auto const& modi = group.mFileName.get_modify().find(key);
		bool rmValid = static_cast<bool>(rmv);
		bool mdValid = static_cast<bool>(modi);

		// **case 1: both 'rmv' and 'modi' are valid
		// => This is creation by 'save-as'
		if (rmValid && mdValid) {
			SPDLOG_INFO(L"Create: {}", key);
			erase_all(group, key);
			// jump to next item for next step
			model.next_available_item();
			return;
		}

		// Both 'rmv' and 'modi' are invalid
		if (!rmValid && !mdValid) {

			// **case 3: 'filename' should not exist in any 'remove model' of other groups
			// this happen when move file
			// receive: add, delete (in the same disk)
			// reveive: add, delete, modify (different disk)
			std::wstring filename = info.get_file_name_wstring();
			for (auto& w : mWatchers) {
				auto const& found = w.mFileName.get_remove().find_if([&filename](auto const& item) {
					return filename == item.get_file_name_wstring();
				});

				// this is not creation, lets other function checking this item
				if (found) {
					return;
				}
			}

			// **case 4: This is 100% creation
			SPDLOG_INFO(L"Create: {}", key);
			erase_all(group, key);
			// jump to next item for next step
			model.next_available_item();
			return;
		}

		// **otherwise: ignore this item
		// Lets other functions checking this item
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

		// Case 1: check 'key' in current group
		// this is happen when edit image file
		// receive: delete, add, modify
		auto const& addInfo = group.mFileName.get_add().find(key);
		if (addInfo) {
			return;
		}

		// case 2: 'filename' should not exist in any 'add model' of other groups
		// this happen when move file
		// receive: add, delete (in the same disk)
		// reveive: add, delete, modify (different disk)
		std::wstring filename = info.get_file_name_wstring();
		for (auto& w : mWatchers) {
			auto const& found = w.mFileName.get_add().find_if([&filename](auto const& item) {
				return filename == item.get_file_name_wstring();
			});

			// this is not remove action
			if (found) {
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

		auto const& add = group.mFileName.get_add().find(key);
		auto const& rmv = group.mFileName.get_remove().find(key);
		bool addValid = static_cast<bool>(add);
		bool rmValid = static_cast<bool>(rmv);

		// **case 1: 100% modify
		if (!addValid && !rmValid) {
			SPDLOG_INFO(L"Modify: {}", key);
			erase_all(group, key);
			// jump to next item for next step
			model.next_available_item();
			return;
		}
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

		// **Case 1: temporary file
		// When create word, excel, etc
		// Receive tmp file then rename
		// => lets this file process in creation
		//std::wstring temp;
		//if (is_temporary_file(info, group, temp)) {
		//	SPDLOG_INFO(L"Modify: ignore temporary file - {}", key);
		//	return;
		//}

		// **Case 2: this file will be rename
		// => lets this file process in creation
		if (will_be_rename(info, group)) {
			return;
		}

		std::wstring realFile;
		if (is_modify_by_temporary(info, group, realFile)) {
			SPDLOG_INFO(L"Modify: {}", realFile);
			SPDLOG_INFO(L"Modify: temporary file created {}", key);
			erase_all(group, key);
			erase_rename(group, realFile);
			model.next_available_item();
			return;
		}

		// **case 1: Modify case
		// receive: remove, add (may or may not 'modify') ???
		// 'remove' must before 'add'
		// Happen when edit image files
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			std::chrono::duration<double> diff = info.get_created_time() - rmv.get_created_time();
			if (diff.count() > 0) {
				SPDLOG_INFO(L"Modify: {}", key);
				erase_all(group, key);
				// jump to next item for next step
				model.next_available_item();
				return;
			}
		}
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

		// ** Goal copy: 
		// exist in 'modify' 
		// but NOT in 'remove'
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

		// **Case 2: this file will be rename
		// => lets this file process in creation
		if (will_be_rename(info, group)) {
			return;
		}

		// **Case temporary file
		// When create word, excel, etc
		// Receive tmp file then rename
		// => lets this file process in creation
		//std::wstring temp;
		//if (is_temporary_file(info, group, temp)) {
		//	SPDLOG_INFO(L"Copy: ignore temporary file - {}", key);
		//	return;
		//}

		// Not action copy => maybe move
		std::wstring filename = info.get_file_name_wstring();
		for (auto& w : mWatchers) {
			auto const& found = w.mFileName.get_remove().find_if([&filename](auto const& item) {
				return filename == item.get_file_name_wstring();
			});

			// this is not copy
			// lets other function checking this item
			if (found) {
				return;
			}
		}

		// This is copy
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

		// **Goal move
		// exist in modify
		// exist in remove but other path

		auto const& rmv = group.mFileName.get_remove().find(key);
		if (rmv) {
			// this is not MOVE
			// lets other function checking this item
			return;
		}

		// **Case 2: this file will be rename
		// => lets this file process in creation
		if (will_be_rename(info, group)) {
			return;
		}

		// **Case temporary file
		// When create word, excel, etc
		// Receive tmp file then rename
		// => lets this file process in creation
		//std::wstring temp;
		//if (is_temporary_file(info, group, temp)) {
		//	SPDLOG_INFO(L"Move: ignore temporary file - {}", key);
		//	return;
		//}

		// Should exist filename in other path
		std::wstring filename = info.get_file_name_wstring();
		for (auto& w : mWatchers) {
			auto const& found = w.mFileName.get_remove().find_if([&filename](auto const& item) {
				return filename == item.get_file_name_wstring();
			});

			// this is MOVE
			if (found) {
				SPDLOG_INFO(L"Move: {} - {}", found.get_path_wstring(), key);
				erase_all(group, key);
				w.mFileName.get_remove().erase(found.get_path_wstring());
				return;
			}
		}
	}

	bool directory_watcher_mgr::will_be_rename(file_notify_info const& info, watching_group& group)
	{
		// **behaviour
		// 1. create file: ~1.tmp
		// 2. rename file ~1.tmp => 1.docx
		// 3. create time less than rename time
		auto key = info.get_path_wstring(); // this is '~1.tmp'
		auto const& ren = group.mFileName.get_rename().find_by_old_name(key);
		if (!ren) {
			return false;
		}

		std::chrono::duration<double> diff = info.get_created_time() - ren.mNewName.get_created_time();
		if (diff.count() > 0) {
			return false;
		}

		return true;
	}

	bool directory_watcher_mgr::is_temporary_file(file_notify_info const& info, watching_group& group, std::wstring& realFile)
	{
		// **behaviour
		// 1. create file: ~1.tmp
		// 2. rename file ~1.tmp => 1.docx
		// 3. create time less than rename time
		// Hence, '~1.tmp' is temporary file, '1.docx' is final file

		// Strong condition:
		// 1.docx is not deleted
		// in-case it is deleted => rename time newer than deleted time

		auto key = info.get_path_wstring(); // this is '~1.tmp'
		auto const& renameModel = group.mFileName.get_rename();
		auto const& ren = renameModel.find_by_old_name(key);
		if (ren) {
			// checking create time and rename time
			std::chrono::duration<double> diff = ren.mNewName.get_created_time() - info.get_created_time();
			if (diff.count() < 0) {
				return false;
			}

			// check in remove model
			auto newName = ren.get_key(); // this is '1.docx'
			auto const& rmv = group.mFileName.get_remove().find(newName);
			if (!rmv) {
				realFile = newName;
				return true;
			}

			// otherwise, checking rename time and delete time
			std::chrono::duration<double> diff2 = ren.mNewName.get_created_time() - rmv.get_created_time();
			if (diff2.count() > 0) {
				realFile = newName;
				return true;
			}
		}

		return false;
	}

	bool directory_watcher_mgr::is_real_file(file_notify_info const& info, watching_group& group, std::wstring& tempNewName, std::wstring& tempOldName)
	{
		// **behaviour
		// 1. delete 1.docx
		// 2. create 1.docx
		// 3. rename 1.docx -> 1.docx~RFacd3b6.TMP
		// 4. rename ~1.tmp -> 1.docx
		// Hence, '1.docx~RFacd3b6.TMP' and '~1.tmp' is temporary file
		// '1.docx' is final file

		// Strong condition:
		// 1.docx is not deleted
		// in-case it is deleted => rename time newer than deleted time

		auto key = info.get_path_wstring(); // this is '~1.tmp'
		auto const& renameModel = group.mFileName.get_rename();
		auto const& ren1 = renameModel.find_by_old_name(key);
		auto const& ren2 = renameModel.find(key);
		if (ren1 && ren2) {
			std::chrono::duration<double> diff = ren2.mNewName.get_created_time() - ren1.mNewName.get_created_time();
			if (diff.count() < 0) {
				return false;
			}

			auto const& rmv = group.mFileName.get_remove().find(key);
			if (rmv) {
				std::chrono::duration<double> diff2 = ren2.mNewName.get_created_time() - rmv.get_created_time();
				if (diff2.count() < 0) {
					return false;
				}
			}

			tempNewName = ren1.mNewName.get_path_wstring();
			tempOldName = ren2.mOldName.get_path_wstring();
			return true;
		}
		return false;
	}

	bool directory_watcher_mgr::is_modify_by_temporary(file_notify_info const& info, watching_group& group, std::wstring& realFile)
	{
		// **behaviour
		// 1. create 1.docx~RFacd3b6.TMP
		// 2. rename 1.docx~RFacd3b6.TMP -> 1.docx
		// 3. delete 1.docx~RFacd3b6.TMP
		// Hence, '1.docx~RFacd3b6.TMP' is temporary file
		// '1.docx' is the read modify file
		auto key = info.get_path_wstring();
		auto const& rmv = group.mFileName.get_remove().find(key);
		if (!rmv) {
			return false;
		}

		std::chrono::duration<double> diff = info.get_created_time() - rmv.get_created_time();
		if (diff.count() > 0) {
			return false;
		}

		auto const& ren = group.mFileName.get_rename().find_by_old_name(key);
		if (!ren) {
			return false;
		}

		//++ TODO check retname time and create time
		realFile = ren.get_key();
		return true;
	}
}
