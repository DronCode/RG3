#pragma once

#include <clang/Frontend/CompilerInstance.h>

#include <RG3/LLVM/CompilerConfigDetector.h>
#include <RG3/LLVM/CompilerConfig.h>

#include <filesystem>
#include <variant>
#include <string>


namespace rg3::llvm
{
	struct CompilerInstanceFactory
	{
		static void makeInstance(
			clang::CompilerInstance* pOutInstance,
			const std::variant<std::filesystem::path, std::string>& sInput,
			const CompilerConfig& sCompilerConfig,
			const CompilerEnvironment* pCompilerEnv = nullptr);
	};
}