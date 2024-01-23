#include <RG3/PyBind/PyClangRuntime.h>

#include <RG3/LLVM/CompilerConfigDetector.h>
#include <RG3/LLVM/Compiler.h>


namespace rg3::pybind
{
	boost::python::str PyClangRuntime::getRuntimeInfo()
	{
		const auto str = rg3::llvm::ClangRuntimeInfo::getRuntimeInfo();
		return str.c_str();
	}

	boost::python::object PyClangRuntime::detectSystemIncludeSources()
	{
		auto envResult = rg3::llvm::CompilerConfigDetector::detectSystemCompilerEnvironment();
		if (auto pEnv = std::get_if<rg3::llvm::CompilerEnvironment>(&envResult))
		{
			boost::python::list result;

			for (const auto& inc : pEnv->config.vSystemIncludes)
			{
				result.append(inc.sFsLocation.string());
			}

			return result;
		}

		if (auto pError = std::get_if<rg3::llvm::CompilerEnvError>(&envResult))
		{
			return boost::python::str(pError->message);
		}

		return {};
	}
}