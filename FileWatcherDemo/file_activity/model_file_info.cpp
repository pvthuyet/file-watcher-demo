#include "model_file_info.h"

namespace died
{
	void model_file_info::push(file_notify_info&& info)
	{
		auto key = info.get_path_wstring();
		mData[std::move(key)] = std::move(info);
	}

	const file_notify_info& model_file_info::front() const
	{
		return mData.front();
	}

	const file_notify_info& model_file_info::find(std::wstring const& key) const
	{
		return mData.find(key);
	}

	void model_file_info::erase(std::wstring const& key)
	{
		mData.erase(key);
	}

	unsigned int model_file_info::next_available_item()
	{
		return mData.next_available_item();
	}
}