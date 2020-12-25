#pragma once

#include "file_notify_info.h"
#include "circle_map.h"

namespace died
{
	class model_file_info
	{
		using file_info_map = died::circle_map<std::wstring, file_notify_info, 32u>;
	public:
		void push(file_notify_info&& info);
		const file_notify_info& front() const;

		const file_notify_info& find(std::wstring const& key) const;

		template<typename Predicate>
		const file_notify_info& find_if(Predicate pre) const
		{
			return mData.find_if(pre);
		}

		void erase(std::wstring const& key);
		unsigned int next_available_item();

	private:
		file_info_map mData;
	};
}