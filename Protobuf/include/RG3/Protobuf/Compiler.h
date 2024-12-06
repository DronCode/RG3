#pragma once

#include <filesystem>
#include <vector>
#include <string>


namespace rg3::pb
{
	/**
	 * @brief In most cases used syntax from .proto file, but you able to specify it directly here (in C++/Python code)
	 */
	enum class ProtobufSyntax { PB_SYNTAX_2 = 2, PB_SYNTAX_3 = 3 };

	/**
	 * @brief There are 2 issues in protoc: errors and warnings
	 */
	enum class IssueKind { IK_WARNING, IK_ERROR };

	/**
	 * @brief Most useful parameters for protoc (in-memory version). Unlike LLVM CompilerConfig, that options useful only for protoc
	 */
	struct CompilerConfig {
		ProtobufSyntax eSyntax { ProtobufSyntax::PB_SYNTAX_3 };
		std::vector<std::filesystem::path> vIncludeDirs {};
		bool bUseStrictMode { true };
		bool bEnableGRPC { false };
		bool bGenerateClientStubs { false };
		bool bUseLiteGenerator { false };
	};

	/**
	 * @brief Describe issue in protoc
	 */
	struct CompilerIssue
	{
		IssueKind eKind { IssueKind::IK_WARNING };
		int iLine { 0 };
		int iColumn { 0 };
		std::string sMessage {};
	};

	using CompilerIssuesVector = std::vector<CompilerIssue>;
}