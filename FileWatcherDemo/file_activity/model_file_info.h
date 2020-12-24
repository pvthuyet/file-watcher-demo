#pragma once

#include "file_notify_info.h"
#include "circle_map.h"

namespace died
{
	class model_file_info
	{
		using file_info_map = died::circle_map<std::wstring, file_notify_info, 128u>;
	public:
		void push(file_notify_info&& info);
		const file_notify_info& front() const;

		const file_notify_info& find(std::wstring const& key) const;
		void erase(std::wstring const& key);
		unsigned int next_available_item();

	private:
		file_notify_info mInfo;
		file_info_map mData;
	};
}