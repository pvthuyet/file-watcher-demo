#include "common_utils.h"
#include <array>
#include <Windows.h>

namespace died
{
	std::vector<std::wstring> enumerate_drives()
	{
		std::vector<std::wstring> drives;
		std::array<wchar_t, 4> letter = { L'A', L':', L'\\', L'\0' };

		// Get all drives in the system.
		DWORD dwDriveMask = ::GetLogicalDrives();

		if (0 == dwDriveMask) {
			return drives;
		}

		// Loop for all drives (MAX_DRIVES = 26)
		constexpr short MAX_DRIVES = 26;

		for (short i = 0; i < MAX_DRIVES; ++i) {
			// if a drive is present,
			if (dwDriveMask & 1) {
				letter[0] = 'A' + i;
				auto type = ::GetDriveTypeW(letter.data());
				switch (type) {
				case DRIVE_REMOVABLE:
				case DRIVE_FIXED:
					drives.emplace_back(letter.data());
					break;

				default:
					break;
				}
			}
			dwDriveMask >>= 1;
		}

		return drives;
	}

	bool fileIsProcessing(const std::wstring& filePath, int& error)
	{
		//++ TODO use scoped handle
		HANDLE hFile = ::CreateFileW(filePath.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			(HANDLE)NULL);

		error = ::GetLastError();
		if (INVALID_HANDLE_VALUE == hFile) {
			return ERROR_SHARING_VIOLATION == error;
		}

		::CloseHandle(hFile);
		return false;
	}
}