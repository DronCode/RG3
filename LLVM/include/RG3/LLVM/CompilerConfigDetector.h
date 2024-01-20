#pragma once

#include <string>
#include <variant>

#include <RG3/LLVM/CompilerConfig.h>


namespace rg3::llvm
{
	struct CompilerEnvironment
	{
		CompilerConfig config {};
		std::string triple {};
		std::string options {};
		std::string versionString {};
	};

	struct CompilerEnvError
	{
		std::string message {}; // Error message
		enum class ErrorKind {
			EK_UNKNOWN,           ///< Really unknown error
			EK_NO_PATHS,          ///< Failed to get PATH
			EK_NO_CLANG_INSTANCE, ///< Failed to invoke clang (Clang instance not found or running with errors)
			EK_BAD_CLANG_OUTPUT,  ///< Malformed output of clang instance
			EK_NO_SYSTEM_INCLUDE_DIRS_FOUND, ///< Not found any system include dirs. Platform specific failure!
			EK_UNSUPPORTED_OS     ///< Requested feature or whole env recognition not supported yet
		} kind { ErrorKind::EK_UNKNOWN };
	};

	using CompilerEnvResult = std::variant<CompilerEnvError, CompilerEnvironment>;

	struct CompilerConfigDetector
	{
		/**
		 * @brief Trying to recognize where located system compiler and trying to get all information about it's environment
		 * @return CompilerEnvError on error, CompilerEnvironment when everything is ok
		 */
		static CompilerEnvResult detectSystemCompilerEnvironment();
	};
}