#pragma once

#include <clang/AST/ASTConsumer.h>
#include <RG3/LLVM/CodeEvaluator.h>
#include <unordered_map>
#include <unordered_set>
#include <string>


namespace rg3::llvm::consumers
{
	struct CollectConstexprVariableEvalResult : public clang::ASTConsumer
	{
		std::unordered_set<std::string> aExpectedVariables {};
		std::unordered_map<std::string, VariableValue>* pEvaluatedVariables { nullptr };

	 public:
		CollectConstexprVariableEvalResult();

		void HandleTranslationUnit(clang::ASTContext& ctx) override;
	};
}