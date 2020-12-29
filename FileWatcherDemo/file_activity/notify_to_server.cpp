#include "notify_to_server.h"
#include "spdlog_header.h"

namespace died
{
	void notify_to_server::send(const std::wstring& action, const std::wstring& path) const
	{
		SPDLOG_INFO(L"{} - {}", action, path);
	}
}