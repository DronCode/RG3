#include <RG3/LLVM/Consumers/CompilerDiagnosticsConsumer.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/SmallVector.h>


namespace rg3::llvm::consumers
{
	CompilerDiagnosticsConsumer::CompilerDiagnosticsConsumer(rg3::llvm::AnalyzerResult& analyzerResult)
		: clang::DiagnosticConsumer()
		, m_analyzerResult(analyzerResult)
	{
	}

	void CompilerDiagnosticsConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel, const clang::Diagnostic& Info)
	{
		// Here we need to collect info, warning, error and fatal error diagnostics and store it into m_analyzerResult
		using L = clang::DiagnosticsEngine::Level;
		if (DiagLevel == L::Note || DiagLevel == L::Warning || DiagLevel == L::Error || DiagLevel == L::Fatal)
		{
			rg3::llvm::AnalyzerResult::CompilerIssue& issue = m_analyzerResult.vIssues.emplace_back();

			// Push kind
			if (DiagLevel == L::Note) issue.kind = rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_INFO;
			if (DiagLevel == L::Warning) issue.kind = rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_WARNING;
			if (DiagLevel == L::Error || DiagLevel == L::Fatal) issue.kind = rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR;

			// Push info
			::llvm::SmallVector<char, 256> message;
			Info.FormatDiagnostic(message);
			issue.sMessage = std::string(message.begin(), message.end());

			// Push location
			const clang::SourceLocation& loc = Info.getLocation();
			clang::SourceManager& sourceManager = Info.getSourceManager();
			clang::PresumedLoc presumedLoc = sourceManager.getPresumedLoc(loc);

			if (presumedLoc.isValid())
			{
				issue.iLine = presumedLoc.getLine();
				issue.iColumn = presumedLoc.getColumn();
				issue.sSourceFile = std::string(presumedLoc.getFilename());
			}
			else
			{
				issue.iLine = issue.iColumn = 0u;
				issue.sSourceFile = "(unknown)";
			}
		}
	}
}