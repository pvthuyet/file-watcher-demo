#pragma once

#include "file_notify_info.h"
#include "circle_map.h"

namespace died
{
	struct rename_notify_info
	{
		died::file_notify_info mOldName;
		died::file_notify_info mNewName;

		explicit operator bool() const noexcept;
		friend bool operator==(rename_notify_info const&, rename_notify_info const&);

		std::wstring get_key() const;
	};
	bool operator==(rename_notify_info const&, rename_notify_info const&);
	
	/************************************************************************************************/

	class model_rename
	{
		using rename_map = died::circle_map<std::wstring, rename_notify_info, 32u>;
	public:
		void push(file_notify_info&& info);
		const rename_notify_info& front() const;

		const rename_notify_info& find(std::wstring const& key) const;
		const rename_notify_info& find_by_old_name(std::wstring const& key) const;

		void erase(std::wstring const& key);
		unsigned int next_available_item();

	private:
		rename_notify_info mInfo;
		rename_map mData;
	};
}
