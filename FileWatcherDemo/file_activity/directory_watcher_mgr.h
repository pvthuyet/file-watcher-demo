#pragma once

#include "file_name_watcher.h"
#include "attribute_watcher.h"
#include "security_watcher.h"
#include "folder_name_watcher.h"

namespace died
{
	class directory_watcher_mgr
	{
		struct watching_group
		{
			file_name_watcher mFileName;
			attribute_watcher mAttr;
			security_watcher mSecu;
			folder_name_watcher mFolderName;
		};

	public:
		bool start(unsigned long notifyChange, bool subtree = true);
		void stop();

	private:
		std::vector<watching_group> mWatchers;
	};
}