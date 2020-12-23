#include "directory_watcher_base.h"
#include "observer_impl.h"
#include "request_impl.h"

#include "gsl\gsl_assert"
#include "spdlog_header.h"
#include <utility>

namespace died
{
	directory_watcher_base::~directory_watcher_base() noexcept
	{
		stop();
	}

	directory_watcher_base::directory_watcher_base(directory_watcher_base&& other) noexcept :
		mSettings{ std::exchange(other.mSettings, std::vector<watching_setting>{}) },
		mObserverThread{ std::exchange(other.mObserverThread, nullptr) },
		mThreadId{ std::exchange(other.mThreadId, 0) },
		mObserver{ std::exchange(other.mObserver, nullptr) }
	{}

	directory_watcher_base& directory_watcher_base::operator=(directory_watcher_base&& other) noexcept
	{
		if (this != &other) {
			mSettings = std::exchange(other.mSettings, std::vector<watching_setting>{});
			mObserverThread = std::exchange(other.mObserverThread, nullptr);
			mThreadId = std::exchange(other.mThreadId, 0);
			mObserver = std::exchange(other.mObserver, nullptr);
		}
		return *this;
	}

	bool directory_watcher_base::add_setting(watching_setting&& val)
	{
		// check valid
		if (!val.valid()) {
			SPDLOG_INFO("The setting is invalid");
			return false;
		}

		// check exist
		auto found = std::find(std::cbegin(mSettings), std::cend(mSettings), val);
		if (std::cend(mSettings) != found) {
			SPDLOG_INFO("The setting is already present");
			return false;
		}

		// insert
		mSettings.push_back(std::move(val));

		return true;
	}

	bool directory_watcher_base::start()
	{
		LOGENTER;
		if (mSettings.size() == 0) {
			SPDLOG_INFO("There is no setting data");
			return false;
		}

		using namespace observer::callback;
		// 1. Create observer object
		mObserver = std::make_unique<observer_impl>(this);

		// 2. Create observer thread
		auto ret = _beginthreadex(
			NULL,
			0,
			&start_thread_proc,
			mObserver.get(),
			0,
			&mThreadId);

		if (!ret) {
			SPDLOG_ERROR("Create thread. errno: {}, doserrno: {}", errno, _doserrno);
			return false;
		}
		mObserverThread = reinterpret_cast<HANDLE>(ret);

		// 2. build request
		for (auto const& el : mSettings) {
			// The setting must valid
			if (!el.valid()) {
				SPDLOG_WARN(L"Invalid watching setting. Action: {}, directory: {}", el.mAction, el.mDirectory);
				continue;
			}
			request_impl::request_param param{};
			param.mObs = mObserver.get();
			param.mInfo = el;
			request_impl* req = new request_impl(std::move(param));
			auto succ = ::QueueUserAPC(add_directory_proc,
				mObserverThread,
				reinterpret_cast<ULONG_PTR>(req));

			if (!succ) {
				SPDLOG_ERROR("QueueUserAPC. Last error code: {}", ::GetLastError());
				return false;
			}
		}
		LOGEXIT;
		return true;
	}

	void directory_watcher_base::stop() noexcept
	{
		LOGENTER;
		// 1. cleanup settings
		mSettings.clear();

		using namespace observer::callback;
		// 2. terminate queue then close thread
		if (mObserverThread) {
			auto succ = ::QueueUserAPC(terminate_proc, mObserverThread, reinterpret_cast<ULONG_PTR>(mObserver.get()));
			if (!succ) {
				SPDLOG_ERROR("Last error code: {}", ::GetLastError());
			}

			DWORD stat = 0;
			do {
				stat = ::WaitForSingleObjectEx(mObserverThread, 3000, true);
				SPDLOG_INFO("WaitForSingleObjectEx state: {}", stat);
			} while (WAIT_TIMEOUT == stat);

			::CloseHandle(mObserverThread);
			mObserverThread = nullptr;
			mThreadId = 0;
		}

		// 3. destroy observer
		mObserver = nullptr;
		LOGEXIT;
	}
}