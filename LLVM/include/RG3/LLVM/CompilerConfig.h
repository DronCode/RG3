#pragma once

#include <RG3/LLVM/Compiler.h>

#include <filesystem>
#include <vector>


namespace rg3::llvm
{
	struct CompilerConfig
	{
		CxxStandard cppStandard { CxxStandard::CC_DEFAULT };
		IncludeVector vIncludes {};
		IncludeVector vSystemIncludes {};
		std::vector<std::string> vCompilerArgs;
		std::vector<std::string> vCompilerDefs;
		bool bAllowCollectNonRuntimeTypes { false };
	};
}