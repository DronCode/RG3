#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/LLVM/Actions/ExtractTypesFromTU.h>

#include <RG3/LLVM/Consumers/CompilerDiagnosticsConsumer.h>

#include <RG3/LLVM/CompilerInstanceFactory.h>
#include <RG3/LLVM/CompilerConfigDetector.h>

#include <RG3/Cpp/TypeClass.h>

#include <algorithm>
#include <utility>


namespace rg3::llvm
{
	AnalyzerResult::operator bool() const
	{
		return std::count_if(
			vIssues.begin(),
			vIssues.end(),
			[](const CompilerIssue& issue) -> bool {
				return 	issue.kind != CompilerIssue::IssueKind::IK_INFO &&
						issue.kind != CompilerIssue::IssueKind::IK_NONE;
			}) == 0;
	}

	CodeAnalyzer::CodeAnalyzer() = default;

	CodeAnalyzer::CodeAnalyzer(const std::filesystem::path& sourceFile, CompilerConfig compilerConfig)
		: m_source(sourceFile)
		, m_compilerConfig(std::move(compilerConfig))
	{
	}

	CodeAnalyzer::CodeAnalyzer(std::string sourceCodeBuffer, CompilerConfig compilerConfig)
		: m_source(sourceCodeBuffer)
		, m_compilerConfig(std::move(compilerConfig))
	{
	}

	void CodeAnalyzer::setSourceCode(const std::string& sourceCode)
	{
		m_source.emplace<std::string>(sourceCode);
	}

	void CodeAnalyzer::setSourceFile(const std::filesystem::path& sourceFile)
	{
		m_source.emplace<std::filesystem::path>(sourceFile);
	}

	void CodeAnalyzer::setCompilerEnvironment(const CompilerEnvironment& env)
	{
		m_env = env;
	}

	CompilerConfig& CodeAnalyzer::getCompilerConfig()
	{
		return m_compilerConfig;
	}

	std::string sourceToString(const std::variant<std::filesystem::path, std::string>& src)
	{
		struct
		{
			std::string repr;

			void operator()(const std::filesystem::path& p)
			{
				repr = p.string();
			}

			void operator()(const std::string& s)
			{
				repr = s;
			}
		} v;

		std::visit(v, src);
		return v.repr;
	};

	AnalyzerResult CodeAnalyzer::analyze()
	{
		AnalyzerResult result;

		CompilerEnvironment* pCompilerEnv = nullptr;

		// Run platform env detector
		if (!m_env.has_value())
		{
			const auto compilerEnvironment = CompilerConfigDetector::detectSystemCompilerEnvironment();
			if (auto pEnvFailure = std::get_if<CompilerEnvError>(&compilerEnvironment))
			{
				// Fatal error
				result.vIssues.emplace_back(AnalyzerResult::CompilerIssue { AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR, sourceToString(m_source), 0, 0, pEnvFailure->message });
				return result;
			}

			// Override env
			m_env = *std::get_if<CompilerEnvironment>(&compilerEnvironment);
		}

		pCompilerEnv = &m_env.value();
		clang::CompilerInstance compilerInstance {};
		CompilerInstanceFactory::makeInstance(&compilerInstance, m_source, m_compilerConfig, pCompilerEnv);

		// Add diagnostics consumer
		{
			auto errorCollector = std::make_unique<consumers::CompilerDiagnosticsConsumer>(result);
			compilerInstance.getDiagnostics().setClient(errorCollector.release(), false);
		}

		// Run actions
		{
			rg3::llvm::actions::ExtractTypesFromTUAction findTypesAction { result.vFoundTypes, m_compilerConfig };
			compilerInstance.ExecuteAction(findTypesAction);
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if (m_compilerConfig.bUseDeepAnalysis && result)
		{
			resolveDeepReferences(result);
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		return result;
	}

	void CodeAnalyzer::resolveDeepReferences(AnalyzerResult& result)
	{
		for (auto& pType : result.vFoundTypes)
		{
			if (pType->getKind() != cpp::TypeKind::TK_STRUCT_OR_CLASS)
			{
				continue;
			}

			auto* pAsClass = reinterpret_cast<cpp::TypeClass*>(pType.get());
			for (auto& sParent : pAsClass->getParentTypes())
			{
				if (sParent.pDeepType)
					continue;

				auto it = std::find_if(result.vFoundTypes.begin(), result.vFoundTypes.end(), [&sParent](const cpp::TypeBasePtr& pType) -> bool {
					return pType->getPrettyName() == sParent.sTypeBaseInfo.sPrettyName &&
						   pType->getDefinition() == sParent.sTypeBaseInfo.sDefLocation;
				});

				if (it != result.vFoundTypes.end())
				{
					auto* pParentAsClass = reinterpret_cast<cpp::TypeClass*>((*it).get());

					sParent.pDeepType = std::make_shared<cpp::TypeClass>(
						pParentAsClass->getName(), pParentAsClass->getPrettyName(), pParentAsClass->getNamespace(), pParentAsClass->getDefinition(), pParentAsClass->getTags(),
						pParentAsClass->getProperties(), pParentAsClass->getFunctions(), pParentAsClass->getClassFriends(),
						pParentAsClass->isStruct(), pParentAsClass->isTrivialConstructible(), pParentAsClass->hasCopyConstructor(), pParentAsClass->hasCopyAssignOperator(), pParentAsClass->hasMoveConstructor(), pParentAsClass->hasMoveAssignOperator(),
						pParentAsClass->getParentTypes()
					);
				}
			}
		}
	}
}