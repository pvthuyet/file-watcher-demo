#pragma once

#include "directory_watcher_base.h"
#include "model_rename.h"
#include "model_file_info.h"

namespace died
{
	class file_name_watcher : public directory_watcher_base
	{
	public:
		model_file_info& get_add();
		model_file_info& get_remove();
		model_file_info& get_modify();
		model_rename& get_rename();
		void erase_all(std::wstring const& key);

	private:
		void do_notify(file_notify_info info) final;

	private:
		model_file_info mAdd;
		model_file_info mRemove;
		model_file_info mModify;
		model_rename mRename;
	};
}