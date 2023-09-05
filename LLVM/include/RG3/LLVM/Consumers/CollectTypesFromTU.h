#pragma once

#include <vector>
#include <RG3/Cpp/TypeBase.h>
#include <clang/AST/ASTConsumer.h>


namespace rg3::llvm::consumers
{
	struct CollectTypesFromTUConsumer : public clang::ASTConsumer
	{
		explicit CollectTypesFromTUConsumer(std::vector<rg3::cpp::TypeBasePtr>& vCollectedTypes);

		void HandleTranslationUnit(clang::ASTContext& ctx) override;

		std::vector<rg3::cpp::TypeBasePtr>& collectedTypes;
	};
}