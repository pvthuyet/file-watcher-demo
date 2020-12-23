#pragma once
#include <string>
#include <array>
#include <vector>
#include "file_notify_info.h"

namespace died
{
	namespace fat
	{
		class UnnecessaryDirectory
		{
		public:
			UnnecessaryDirectory();
			void addUserDefinePath(std::wstring path);
			void setAppDataDir(bool enable);
			bool contains(file_notify_info const& info) const;

		private:
			bool isDefaultPath(file_notify_info const& info) const;
			bool isAppDataPath(file_notify_info const& info) const;
			bool isUserDefinePath(file_notify_info const& info) const;

		private:
			std::array<std::wstring, 6> mDefaultPaths;
			std::vector<std::wstring> mUserDefinePaths;
			bool mAppDataDir;
		};
	}
}