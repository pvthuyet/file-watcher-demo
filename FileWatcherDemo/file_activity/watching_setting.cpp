#include "watching_setting.h"

namespace died
{
	watching_setting::watching_setting(unsigned long action, std::wstring dir, bool subtree) : 
		mSubtree{ subtree },
		mAction { action },
		mDirectory {dir}
	{}

	bool watching_setting::valid() const 
	{ 
		return mAction > 0 && !mDirectory.empty(); 
	}

	bool operator==(watching_setting const& lhs, watching_setting const& rhs)
	{
		return lhs.mSubtree		== rhs.mSubtree
			&& lhs.mAction		== rhs.mAction
			&& lhs.mDirectory	== rhs.mDirectory;
	}
}