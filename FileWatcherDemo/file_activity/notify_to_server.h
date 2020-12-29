#pragma once
#include <string>

namespace died
{
	class notify_to_server
	{
	public:
		void send(const std::wstring& action, const std::wstring& path) const;
	};
}
