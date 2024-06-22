#pragma once

#include <RG3/LLVM/CodeAnalyzer.h>
#include <iostream>

// Just a workaround to avoid of bad behaviour with microsoft shit
#ifdef _MSC_VER
#	define MS_WORKAROUND_FOR_LEGACY_CLANG R"(
#if __clang_major__ < 17
#	undef __clang_major__
#	define __clang_major__ 17
#endif
)"
#else
#	define MS_WORKAROUND_FOR_LEGACY_CLANG ""
#endif


struct CommonHelpers
{
	static void printCompilerIssues(const rg3::llvm::AnalyzerResult::CompilerIssuesVector& issues)
	{
		if (!issues.empty())
		{
			std::cout << "Got " << issues.size() << " issue(s):\n";
			for (const auto& issue : issues)
			{
				switch (issue.kind)
				{
				case rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR:
					std::cout << "[ERROR] " << issue.sMessage << " at " << issue.sSourceFile << ":" << issue.iLine << '\n';
					break;

				case rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_WARNING:
					std::cout << "[WARN] " << issue.sMessage << " at " << issue.sSourceFile << ":" << issue.iLine << '\n';
					break;

				case rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_INFO:
				case rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_NONE:
					std::cout << "[INFO] " << issue.sMessage << " at " << issue.sSourceFile << ":" << issue.iLine << '\n';
					break;
				}
			}
		}
	}
};