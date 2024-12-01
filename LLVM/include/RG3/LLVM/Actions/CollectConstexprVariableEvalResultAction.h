#pragma once

#include <clang/Frontend/FrontendActions.h>
#include <RG3/LLVM/CodeEvaluator.h>
#include <unordered_map>
#include <unordered_set>
#include <string>



namespace rg3::llvm::actions
{
	struct CollectConstexprVariableEvalResultAction : public clang::ASTFrontendAction
	{
		std::unordered_set<std::string> aExpectedVariables {};
		std::unordered_map<std::string, VariableValue>* pEvaluatedVariables { nullptr };

		std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& /*compilerInstance*/, clang::StringRef /*file*/) override;
	};
}