#include <RG3/PyBind/PyCodeAnalyzerBuilder.h>
#include <RG3/PyBind/PyTypeClass.h>
#include <RG3/PyBind/PyTypeBase.h>
#include <RG3/PyBind/PyTypeEnum.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeEnum.h>

#include <boost/shared_ptr.hpp>



namespace rg3::pybind
{
	PyCodeAnalyzerBuilder::PyCodeAnalyzerBuilder()
	{
		m_pAnalyzerInstance = std::make_unique<rg3::llvm::CodeAnalyzer>();
	}

	void PyCodeAnalyzerBuilder::setSourceCode(const std::string& sourceCode)
	{
		m_pAnalyzerInstance->setSourceCode(sourceCode);
	}

	void PyCodeAnalyzerBuilder::setSourceFile(const std::string& pathToFile)
	{
		m_pAnalyzerInstance->setSourceFile(pathToFile);
	}

	void PyCodeAnalyzerBuilder::setCppStandard(rg3::llvm::CxxStandard standard)
	{
		m_pAnalyzerInstance->getCompilerConfig().cppStandard = standard;
	}

	void PyCodeAnalyzerBuilder::setCompilerArgs(const boost::python::list& args)
	{
		std::vector<std::string> cxxArgs;

		for (int i = 0; i < boost::python::len(args); i++)
		{
			cxxArgs.emplace_back(boost::python::extract<std::string>(args[i]));
		}

		m_pAnalyzerInstance->getCompilerConfig().vCompilerArgs = cxxArgs;
	}

	void PyCodeAnalyzerBuilder::setCompilerIncludeDirs(const boost::python::list& includes)
	{
		std::vector<rg3::llvm::IncludeInfo> cxxIncludes;

		for (int i = 0; i < boost::python::len(includes); i++)
		{
			cxxIncludes.emplace_back(boost::python::extract<rg3::llvm::IncludeInfo>(includes[i]));
		}

		m_pAnalyzerInstance->getCompilerConfig().vIncludes = cxxIncludes;
	}

	void PyCodeAnalyzerBuilder::addIncludeDir(const rg3::llvm::IncludeInfo& includeInfo)
	{
		m_pAnalyzerInstance->getCompilerConfig().vIncludes.push_back(includeInfo);
	}

	void PyCodeAnalyzerBuilder::addProjectIncludeDir(const std::string& includeDir)
	{
		addIncludeDir(rg3::llvm::IncludeInfo(includeDir, rg3::llvm::IncludeKind::IK_PROJECT));
	}

	void PyCodeAnalyzerBuilder::setAllowToCollectNonRuntimeTypes(bool value)
	{
		m_pAnalyzerInstance->getCompilerConfig().bAllowCollectNonRuntimeTypes = value;
	}

	bool PyCodeAnalyzerBuilder::isNonRuntimeTypesAllowedToBeCollected() const
	{
		return m_pAnalyzerInstance->getCompilerConfig().bAllowCollectNonRuntimeTypes;
	}

	void PyCodeAnalyzerBuilder::analyze()
	{
		auto analyzeInfo = m_pAnalyzerInstance->analyze();

		m_foundIssues = {};
		m_foundTypes = {};

		for (const auto& issue : analyzeInfo.vIssues)
		{
			m_foundIssues.append(issue);
		}

		for (auto&& type : analyzeInfo.vFoundTypes)
		{
			switch (type->getKind())
			{
				case cpp::TypeKind::TK_NONE:
				case cpp::TypeKind::TK_TEMPLATE_SPECIALIZATION:
					// Unsupported yet, lost, yep
				break;
				case cpp::TypeKind::TK_TRIVIAL:
				{
					auto object = boost::shared_ptr<PyTypeBase>(new PyTypeBase(std::move(type)));
					m_foundTypes.append(object);
				}
				break;
				case cpp::TypeKind::TK_ENUM:
				{
					auto object = boost::shared_ptr<PyTypeEnum>(new PyTypeEnum(std::move(type)));
					m_foundTypes.append(object);
				}
				break;
				case cpp::TypeKind::TK_STRUCT_OR_CLASS:
				{
					auto object = boost::shared_ptr<PyTypeClass>(new PyTypeClass(std::move(type)));
					m_foundTypes.append(object);
				}
				break;
			}
		}
	}

	const boost::python::list& PyCodeAnalyzerBuilder::getFoundTypes() const
	{
		return m_foundTypes;
	}

	const boost::python::list& PyCodeAnalyzerBuilder::getFoundIssues() const
	{
		return m_foundIssues;
	}
}