#pragma once

#include <vector>
#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/Cpp/TypeBase.h>
#include <clang/AST/ASTConsumer.h>


namespace rg3::llvm::consumers
{
	struct CollectTypesFromTUConsumer : public clang::ASTConsumer
	{
		CollectTypesFromTUConsumer(std::vector<rg3::cpp::TypeBasePtr>& vCollectedTypes, const CompilerConfig& cc);

		void HandleTranslationUnit(clang::ASTContext& ctx) override;

		std::vector<rg3::cpp::TypeBasePtr>& collectedTypes;
		const CompilerConfig& compilerConfig;
	};
}