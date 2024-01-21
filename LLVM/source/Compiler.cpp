#include <RG3/LLVM/Compiler.h>

#include <llvm/Config/llvm-config.h>


namespace rg3::llvm
{
	std::string ClangRuntimeInfo::getRuntimeInfo()
	{
		return std::format("Clang {} built for {}", LLVM_VERSION_STRING, LLVM_HOST_TRIPLE);
	}
}