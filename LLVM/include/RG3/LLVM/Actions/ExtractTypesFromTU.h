#pragma once

#include <RG3/Cpp/TypeBase.h>
#include <clang/Frontend/FrontendActions.h>
#include <memory>


namespace rg3::llvm::actions
{
	struct ExtractTypesFromTUAction : public clang::ASTFrontendAction
	{
		explicit ExtractTypesFromTUAction(std::vector<rg3::cpp::TypeBasePtr>& vFoundTypes) : foundTypes(vFoundTypes) {}

		std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& /*compilerInstance*/, clang::StringRef /*file*/) override;

		std::vector<rg3::cpp::TypeBasePtr>& foundTypes;
	};
}