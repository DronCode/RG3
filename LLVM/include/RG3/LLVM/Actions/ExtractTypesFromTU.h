#pragma once


#include <RG3/Cpp/TypeBase.h>
#include <RG3/LLVM/CompilerConfig.h>
#include <clang/Frontend/FrontendActions.h>
#include <memory>


namespace rg3::llvm::actions
{
	struct ExtractTypesFromTUAction : public clang::ASTFrontendAction
	{
		ExtractTypesFromTUAction(std::vector<rg3::cpp::TypeBasePtr>& vFoundTypes, const CompilerConfig& cc) : foundTypes(vFoundTypes), compilerConfig(cc) {}

		std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& /*compilerInstance*/, clang::StringRef /*file*/) override;

		std::vector<rg3::cpp::TypeBasePtr>& foundTypes;
		const CompilerConfig& compilerConfig;
	};
}