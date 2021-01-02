#include "request_impl.h"
#include "file_notify_info.h"
#include "iobserver.h"
#include "idirectory_watcher.h"
#include "spdlog_header.h"
#include "gsl/assert"

#include <shlwapi.h>
#pragma comment(lib, "Shlwapi")

namespace died
{
	request_impl::request_impl(request_param param) :
		mParam{ param },
		mBuffer(param.mBufferLength),
		mBackupBuffer(param.mBufferLength)
	{
		::ZeroMemory(&mOverlapped, sizeof(OVERLAPPED));
		// The hEvent member is not used when there is a completion
		// function, so it's ok to use it to point to the object.
		mOverlapped.hEvent = this;
		Ensures(mParam.mObs);
	}

	iobserver* request_impl::do_get_observer() const
	{
		Ensures(mParam.mObs);
		return mParam.mObs;
	}

	std::wstring request_impl::do_get_request_id() const
	{
		return std::to_wstring(mParam.mInfo.mAction) +
			L"-" + std::to_wstring(mParam.mInfo.mSubtree) +
			L"-" + mParam.mInfo.mDirectory;
	}

	void request_impl::do_request_termination()
	{
		LOGENTER;
		if (INVALID_HANDLE_VALUE != mHdlDirectory) {
			::CancelIo(mHdlDirectory);
			::CloseHandle(mHdlDirectory);
			mHdlDirectory = INVALID_HANDLE_VALUE;
		}
		LOGEXIT;
	}

	bool request_impl::do_open_directory()
	{
		// Allow this routine to be called redundantly.
		if (INVALID_HANDLE_VALUE != mHdlDirectory) {
			return true;
		}

		mHdlDirectory = ::CreateFileW(
			mParam.mInfo.mDirectory.c_str(),					// pointer to the file name
			FILE_LIST_DIRECTORY,                // access (read/write) mode
			FILE_SHARE_READ						// share mode
			| FILE_SHARE_WRITE
			| FILE_SHARE_DELETE,
			NULL,                               // security descriptor
			OPEN_EXISTING,                      // how to create
			FILE_FLAG_BACKUP_SEMANTICS			// file attributes
			| FILE_FLAG_OVERLAPPED,
			NULL);                              // file with attributes to copy

		return (INVALID_HANDLE_VALUE != mHdlDirectory) ? true : false;
	}

	void request_impl::backup_buffer(DWORD dwSize)
	{
		// We could just swap back and forth between the two
		// buffers, but this code is easier to understand and debug.
		memcpy(&mBackupBuffer[0], &mBuffer[0], dwSize);
	}

	bool request_impl::do_begin_read()
	{
		DWORD dwBytes = 0;

		// This call needs to be reissued after every APC.
		return TRUE == ::ReadDirectoryChangesW(
			mHdlDirectory,						// handle to directory
			&mBuffer[0],						// read results buffer
			(DWORD)mBuffer.size(),				// length of buffer
			mParam.mInfo.mSubtree,					// monitoring option
			mParam.mInfo.mAction,						// filter conditions
			&dwBytes,                           // bytes returned
			&mOverlapped,						// overlapped buffer
			&notification_completion);           // completion routine
	}

	void request_impl::process_notification()
	{
		BYTE* pBase = mBackupBuffer.data();

		for (;;) {
			FILE_NOTIFY_INFORMATION& fni = (FILE_NOTIFY_INFORMATION&)*pBase;

			std::wstring wsFileName(fni.FileName, fni.FileNameLength / sizeof(wchar_t));
			wchar_t wcRight = mParam.mInfo.mDirectory.at(mParam.mInfo.mDirectory.length() - 1);
			// Handle a trailing backslash, such as for a root directory.
			if (L'\\' != wcRight) {
				wsFileName = mParam.mInfo.mDirectory + L"\\" + wsFileName;
			}
			else {
				wsFileName = mParam.mInfo.mDirectory + wsFileName;
			}

			// If it could be a short filename, expand it.
			LPCWSTR shortFileName = ::PathFindFileNameW(wsFileName.c_str());
			int len = lstrlenW(shortFileName);
			// The maximum length of an 8.3 filename is twelve, including the dot.
			if (len <= 12 && wcschr(shortFileName, L'~'))
			{
				// Convert to the long filename form. Unfortunately, this
				// does not work for deletions, so it's an imperfect fix.
				wchar_t wbuf[MAX_PATH] = { 0 };
				if (::GetLongPathNameW(wsFileName.c_str(), wbuf, _countof(wbuf)) > 0) {
					wsFileName = wbuf;
				}
			}
			get_observer()->get_watcher()->notify(file_notify_info{ wsFileName, fni.Action });

			if (!fni.NextEntryOffset) {
				break;
			}
			pBase += fni.NextEntryOffset;
		}
	}

	VOID CALLBACK request_impl::notification_completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		request_impl* pBlock = (request_impl*)lpOverlapped->hEvent;
		_ASSERTE(pBlock);

		if (dwErrorCode == ERROR_OPERATION_ABORTED)
		{
			auto num = pBlock->get_observer()->dec_request();
			SPDLOG_INFO(L"Cancel request id: {}, remain request: {}", pBlock->get_request_id(), num);
			delete pBlock;
			return;
		}

		// Can't use sizeof(FILE_NOTIFY_INFORMATION) because
		// the structure is padded to 16 bytes.
		//_ASSERTE(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));
		// This might mean overflow? Not sure.
		//if (!dwNumberOfBytesTransfered) {
		//	return;
		//}

		pBlock->backup_buffer(dwNumberOfBytesTransfered);

		// Get the new read issued as fast as possible. The documentation
		// says that the original OVERLAPPED structure will not be used
		// again once the completion routine is called.

		// Make sure begin_read success
		if (!pBlock->begin_read()) {
			auto num = pBlock->get_observer()->dec_request();
			SPDLOG_WARN(L"Can't read request id: {}, dwErrorCode: {}, remain request: {}", pBlock->get_request_id(), dwErrorCode, num);
			delete pBlock;
			return;
		}

		// start processing
		pBlock->process_notification();
	}
}