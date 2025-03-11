#pragma once

#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/PyBind/PyTypeBase.h>
#include <unordered_map>

#define BOOST_PYTHON_STATIC_LIB  // required because we using boost.python as static library
#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>


namespace rg3::pybind
{
	class PyCodeAnalyzerBuilder : public boost::noncopyable
	{
	 public:
		PyCodeAnalyzerBuilder();

		static boost::shared_ptr<PyCodeAnalyzerBuilder> makeInstance()
		{
			return boost::shared_ptr<PyCodeAnalyzerBuilder>(new PyCodeAnalyzerBuilder());
		}

		void setSourceCode(const std::string& sourceCode);
		void setSourceFile(const std::string& pathToFile);
		void setCppStandard(rg3::llvm::CxxStandard standard);
		void setCompilerArgs(const boost::python::list& args);
		void setCompilerIncludeDirs(const boost::python::list& includes);
		void addIncludeDir(const rg3::llvm::IncludeInfo& includeInfo);
		void addProjectIncludeDir(const std::string& includeDir);
		void setAllowToCollectNonRuntimeTypes(bool value);
		[[nodiscard]] bool isNonRuntimeTypesAllowedToBeCollected() const;

		void setCompilerDefinitions(const boost::python::list& compilerDefs);
		[[nodiscard]] boost::python::list getCompilerDefinitions() const;

		void setUseDeepAnalysis(bool bUseDeepAnalysis);
		bool isDeepAnalysisEnabled() const;

		void analyze();

		const boost::python::list& getFoundTypes() const;
		const boost::python::list& getFoundIssues() const;

		[[nodiscard]] const rg3::llvm::CompilerConfig& getCompilerConfig() const;

	 private:
		std::unique_ptr<llvm::CodeAnalyzer> m_pAnalyzerInstance { nullptr };
		std::unordered_map<std::string, boost::shared_ptr<PyTypeBase>> m_mFoundTypesMap {};
		boost::python::list m_foundTypes {};
		boost::python::list m_foundIssues {};
	};
}