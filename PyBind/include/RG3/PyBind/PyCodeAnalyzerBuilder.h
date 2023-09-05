#pragma once

#include <RG3/LLVM/CodeAnalyzer.h>

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
		void setCompilerArgs(const std::vector<std::string>& args);
		void setCompilerIncludeDirs(const std::vector<rg3::llvm::IncludeInfo>& includes);
		void addIncludeDir(const rg3::llvm::IncludeInfo& includeInfo);
		void addProjectIncludeDir(const std::string& includeDir);
		void analyze();

		const boost::python::list& getFoundTypes() const;
		const boost::python::list& getFoundIssues() const;

	 private:
		std::unique_ptr<llvm::CodeAnalyzer> m_pAnalyzerInstance { nullptr };
		boost::python::list m_foundTypes {};
		boost::python::list m_foundIssues {};
	};
}