#include "unnecessary_directory.h"
#include "string_helper.h"

namespace died
{
	namespace fat
	{
		UnnecessaryDirectory::UnnecessaryDirectory() : 
			mAppDataDir { false }
		{
			mDefaultPaths[0] = L":\\$Recycle.Bin\\";
			mDefaultPaths[1] = L":\\System Volume Information\\";
			mDefaultPaths[2] = L":\\hiberfil.sys";
			mDefaultPaths[3] = L":\\pagefile.sys";
			mDefaultPaths[4] = L":\\swapfile.sys";
			mDefaultPaths[5] = L":\\Config.Msi";
		}

		void UnnecessaryDirectory::setAppDataDir(bool enable)
		{
			mAppDataDir = enable;
		}

		void UnnecessaryDirectory::addUserDefinePath(std::wstring path)
		{
			mUserDefinePaths.push_back(std::move(path));
		}

		bool UnnecessaryDirectory::contains(file_notify_info const& info) const
		{
			if (isAppDataPath(info)) {
				return true;
			}

			if (isDefaultPath(info)) {
				return true;
			}

			if (isUserDefinePath(info)) {
				return true;
			}

			return false;
		}

		bool UnnecessaryDirectory::isAppDataPath(file_notify_info const& info) const
		{
			if (mAppDataDir)
			{
				// search AppData folder
				static const std::wstring APP_DATA_DIR_PATTERN = L"C:\\\\Users\\\\.+\\\\AppData\\\\";
				if (died::StringUtils::searchRegex(info.get_path_wstring(), APP_DATA_DIR_PATTERN, true)) {
					return true;
				}
			}
			// Not found
			return false;
		}

		bool UnnecessaryDirectory::isDefaultPath(file_notify_info const& info) const
		{
			// 1. Ignore
			auto wsPath = info.get_path_wstring();
			for (const auto& p : mDefaultPaths)
			{
				auto found = died::StringUtils::search(wsPath, p, true);
				if (std::wstring::npos != found) {
					return true;
				}
			}

			// 2. search regular expression
			//static const std::wstring TMP_FILE_PATTERN = L"(~.+\\.TMP$)|(\\\\\\..+)|(~\\$.+)";
			//if (died::StringUtils::searchRegex(wsPath, TMP_FILE_PATTERN, true)) {
			//	return true;
			//}

			// MicrosoftEdgeBackups folder
			static const std::wstring EDGE_BACKUP_PATTERN = L"C:\\\\Users\\\\.+\\\\MicrosoftEdgeBackups\\\\";
			if (died::StringUtils::searchRegex(wsPath, EDGE_BACKUP_PATTERN, true)) {
				return true;
			}

			//++ [bug #17212]: NTUSER file report many times on Win7 by thuyetvp 2020.12.18
			static const std::wstring NTUSER_FILE = L"C:\\\\Users\\\\.+\\\\NTUSER\\.(DAT|INI|POL)";
			if (died::StringUtils::searchRegex(wsPath, NTUSER_FILE, true)) {
				return true;
			}
			//--

			//++ TODO test
			// 2. search regular expression
			//static const std::wstring TMP_ = L"(.+\\.TMP$)|(.+\\.crdownload$)";
			//if (died::StringUtils::searchRegex(wsPath, TMP_, true)) {
			//	return true;
			//}


			return false;
		}

		bool UnnecessaryDirectory::isUserDefinePath(file_notify_info const& info) const
		{
			auto wsPath = info.get_path_wstring();
			for (const auto& p : mUserDefinePaths)
			{
				auto found = died::StringUtils::search(wsPath, p, true);
				if (std::wstring::npos != found) {
					return true;
				}
			}

			return false;
		}
	}
}