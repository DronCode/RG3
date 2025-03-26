#pragma once

#include <string>


namespace rg3::pb
{
	struct Location
	{
		std::string sFile {};
		int iLine { 0 };
		int iColumn { 0 };
	};
}