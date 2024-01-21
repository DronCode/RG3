#include <RG3/PyBind/PyClangRuntime.h>

#include <RG3/LLVM/Compiler.h>


namespace rg3::pybind
{
	boost::python::str PyClangRuntime::getRuntimeInfo()
	{
		const auto str = rg3::llvm::ClangRuntimeInfo::getRuntimeInfo();
		return str.c_str();
	}
}