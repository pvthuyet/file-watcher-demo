#pragma once

#include "irequest.h"
#include "watching_setting.h"
#include <vector>
#include <Windows.h>

namespace died
{
	class request_impl : public irequest
	{
		struct request_param
		{
			DWORD mBufferLength{ 16384 };
			iobserver* mObs{ nullptr };
			watching_setting mInfo;
		};
		friend class directory_watcher_base;

	public:
		request_impl(request_param);

		request_impl(request_impl const&) = delete;
		request_impl& operator=(request_impl const&) = delete;

		void process_notification();
		void backup_buffer(DWORD dwSize);

	private:
		bool do_open_directory() final;
		bool do_begin_read() final;
		void do_request_termination() final;
		iobserver* do_get_observer() const final;
		std::wstring do_get_request_id() const final;

	private:
		static VOID CALLBACK notification_completion(
			DWORD dwErrorCode,							// completion code
			DWORD dwNumberOfBytesTransfered,			// number of bytes transferred
			LPOVERLAPPED lpOverlapped);					// I/O information buffer

	private:
		request_param mParam;

		// Result of calling CreateFile().
		HANDLE		mHdlDirectory{ INVALID_HANDLE_VALUE };

		// Required parameter for ReadDirectoryChangesW().
		OVERLAPPED	mOverlapped;

		// Data buffer for the request_impl.
		// Since the memory is allocated by malloc, it will always
		// be aligned as required by ReadDirectoryChangesW().
		std::vector<BYTE> mBuffer;

		// Double buffer strategy so that we can issue a new read
		// request_impl before we process the current buffer.
		std::vector<BYTE> mBackupBuffer;
	};
}