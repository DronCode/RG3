#include <RG3/LLVM/CompilerInstanceFactory.h>
#include <RG3/LLVM/CodeEvaluator.h>

#include <RG3/LLVM/Actions/CollectConstexprVariableEvalResultAction.h>
#include <RG3/LLVM/Consumers/CompilerDiagnosticsConsumer.h>

#include <clang/Lex/PreprocessorOptions.h>

#include <RG3/LLVM/CompilerInstanceFactory.h>
#include <RG3/LLVM/CompilerConfigDetector.h>

#include <algorithm>
#include <utility>


namespace rg3::llvm
{
	CodeEvaluateResult::operator bool() const noexcept
	{
		return std::count_if(
				   vIssues.begin(),
				   vIssues.end(),
				   [](const AnalyzerResult::CompilerIssue& issue) -> bool {
					   return issue.kind != AnalyzerResult::CompilerIssue::IssueKind::IK_INFO &&
							  issue.kind != AnalyzerResult::CompilerIssue::IssueKind::IK_NONE;
				   }) == 0;
	}

	CodeEvaluator::CodeEvaluator() = default;

	CodeEvaluator::CodeEvaluator(rg3::llvm::CompilerConfig compilerConfig)
		: m_compilerConfig(std::move(compilerConfig))
	{
	}

	void CodeEvaluator::setCompilerEnvironment(const rg3::llvm::CompilerEnvironment& env)
	{
		m_env = env;
	}

	CompilerConfig& CodeEvaluator::getCompilerConfig()
	{
		return m_compilerConfig;
	}

	CodeEvaluateResult CodeEvaluator::evaluateCode(const std::string& sCode, const std::vector<std::string>& aCaptureOutputVariables)
	{
		CodeEvaluateResult sResult {};
		AnalyzerResult sTempResult {};
		CompilerEnvironment* pCompilerEnv = nullptr;

		m_sSourceCode = sCode;

		// Run platform env detector
		if (!m_env.has_value())
		{
			const auto compilerEnvironment = CompilerConfigDetector::detectSystemCompilerEnvironment();
			if (auto pEnvFailure = std::get_if<CompilerEnvError>(&compilerEnvironment))
			{
				// Fatal error
				sResult.vIssues.emplace_back(AnalyzerResult::CompilerIssue { AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR, m_sSourceCode, 0, 0, pEnvFailure->message });
				return sResult;
			}

			// Override env
			m_env = *std::get_if<CompilerEnvironment>(&compilerEnvironment);
		}

		pCompilerEnv = &m_env.value();
		clang::CompilerInstance compilerInstance {};
		CompilerInstanceFactory::makeInstance(&compilerInstance, m_sSourceCode, m_compilerConfig, pCompilerEnv);

		// Add extra definition in our case
		compilerInstance.getPreprocessorOpts().addMacroDef("__RG3_CODE_EVAL__=1");

		// Add diagnostics consumer
		{
			auto errorCollector = std::make_unique<consumers::CompilerDiagnosticsConsumer>(sTempResult);
			compilerInstance.getDiagnostics().setClient(errorCollector.release(), false);
		}

		// Run actions
		{
			rg3::llvm::actions::CollectConstexprVariableEvalResultAction collectConstexprVariableEvalResultAction {};
			collectConstexprVariableEvalResultAction.aExpectedVariables = std::unordered_set<std::string> { aCaptureOutputVariables.begin(), aCaptureOutputVariables.end() };
			collectConstexprVariableEvalResultAction.pEvaluatedVariables = &sResult.mOutputs;

			compilerInstance.ExecuteAction(collectConstexprVariableEvalResultAction);
		}

		// Copy result
		std::copy(sTempResult.vIssues.begin(), sTempResult.vIssues.end(), std::back_inserter(sResult.vIssues));

		return sResult;
	}
}