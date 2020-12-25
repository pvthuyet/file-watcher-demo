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
			notify_attribute(el);
			notify_security(el);
			notify_folder_name(el);
			notify_rename(el);
			notify_create(el);
			notify_remove(el);
			notify_modify(el);
			notify_modify_without_modify_event(el);
			notify_copy(el);
			notify_move(el);
		}
		return TimerStatus::TIMER_CONTINUE;
	}

	void directory_watcher_mgr::erase_all(watching_group& group, std::wstring const& key) const
	{
		group.mFileName.get_add().erase(key);
		group.mFileName.get_remove().erase(key);
		group.mFileName.get_modify().erase(key);
		group.mAttr.get_model().erase(key);
		group.mSecu.get_model().erase(key);
	}

	void directory_watcher_mgr::notify_attribute(watching_group& group) const
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

	void directory_watcher_mgr::notify_security(watching_group& group) const
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

	void directory_watcher_mgr::notify_folder_name(watching_group& group) const
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

	void directory_watcher_mgr::notify_rename(watching_group& group) const
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

		//++ TODO filter
		//...

		// 4. notify this item
		SPDLOG_INFO(L"{} - {}", info.mOldName.get_path_wstring(), info.mNewName.get_path_wstring());

		// 5. erase processed item
		model.erase(info.get_key());

		// 6. jump to next item for next step
		model.next_available_item();
	}

	void directory_watcher_mgr::notify_create(watching_group& group)
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

	void directory_watcher_mgr::notify_remove(watching_group& group)
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

	void directory_watcher_mgr::notify_modify(watching_group& group) const
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

	void directory_watcher_mgr::notify_modify_without_modify_event(watching_group& group) const
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

	void directory_watcher_mgr::notify_copy(watching_group& group)
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

	void directory_watcher_mgr::notify_move(watching_group& group)
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
}
