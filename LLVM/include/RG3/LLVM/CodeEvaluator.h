#pragma once

#include <RG3/LLVM/CompilerConfigDetector.h>
#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/LLVM/CodeAnalyzer.h>

#include <boost/noncopyable.hpp>

#include <unordered_map>
#include <optional>
#include <variant>
#include <cstdint>
#include <string>
#include <vector>


namespace rg3::llvm
{
	using VariableValue = std::variant<bool, std::int64_t, std::uint64_t, float, double, std::string>;

	struct CodeEvaluateResult
	{
		AnalyzerResult::CompilerIssuesVector vIssues;
		std::unordered_map<std::string, VariableValue> mOutputs;

		explicit operator bool() const noexcept;
	};

	class CodeEvaluator : public boost::noncopyable
	{
	 public:
		CodeEvaluator();
		CodeEvaluator(CompilerConfig compilerConfig);

		void setCompilerEnvironment(const CompilerEnvironment& env);
		CompilerConfig& getCompilerConfig();
		const CompilerConfig& getCompilerConfig() const;

		CodeEvaluateResult evaluateCode(const std::string& sCode, const std::vector<std::string>& aCaptureOutputVariables);

	 private:
		std::optional<CompilerEnvironment> m_env;
		CompilerConfig m_compilerConfig;
		std::string m_sSourceCode {};
	};
}