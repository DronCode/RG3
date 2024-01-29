#include <RG3/LLVM/Compiler.h>
#include <llvm/Config/llvm-config.h>
#include <format>


namespace rg3::llvm
{
	std::string ClangRuntimeInfo::getRuntimeInfo()
	{
		return std::format("Clang {} built for {} (build date {})", LLVM_VERSION_STRING, LLVM_HOST_TRIPLE, __DATE__);
	}
}