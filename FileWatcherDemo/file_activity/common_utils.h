#pragma once

#include <string>
#include <vector>

namespace died
{
	std::vector<std::wstring> enumerate_drives();
	bool fileIsProcessing(const std::wstring& filePath, int& error);
}