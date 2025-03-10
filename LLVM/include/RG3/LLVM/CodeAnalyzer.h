#pragma once

#include <RG3/LLVM/CompilerConfigDetector.h>
#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/Cpp/TypeBase.h>
#include <filesystem>
#include <variant>
#include <cstdint>
#include <optional>
#include <span>


namespace rg3::llvm
{
	struct AnalyzerResult
	{
		struct CompilerIssue
		{
			enum class IssueKind { IK_NONE, IK_WARNING, IK_INFO, IK_ERROR };

			IssueKind kind { IssueKind::IK_NONE };
			std::string sSourceFile;
			uint32_t iLine { 0 };
			uint32_t iColumn { 0 };
			std::string sMessage {};
		};

		using CompilerIssuesVector = std::vector<CompilerIssue>;

		CompilerIssuesVector vIssues {};
		std::vector<cpp::TypeBasePtr> vFoundTypes {};

		explicit operator bool() const;
	};

	class CodeAnalyzer
	{
	 public:
		CodeAnalyzer();
		CodeAnalyzer(const std::filesystem::path& sourceFile, CompilerConfig compilerConfig);
		CodeAnalyzer(std::string sourceCodeBuffer, CompilerConfig compilerConfig);

		void setSourceCode(const std::string& sourceCode);
		void setSourceFile(const std::filesystem::path& sourceFile);
		void setCompilerEnvironment(const CompilerEnvironment& env);
		CompilerConfig& getCompilerConfig();

		AnalyzerResult analyze();

	 private:
		void resolveDeepReferences(AnalyzerResult& result);

	 private:
		std::variant<std::filesystem::path, std::string> m_source;
		std::optional<CompilerEnvironment> m_env;
		CompilerConfig m_compilerConfig;
	};
}