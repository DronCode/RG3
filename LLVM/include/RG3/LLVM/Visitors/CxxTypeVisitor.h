#pragma once


#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>

#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/Cpp/TypeBase.h>

#include <vector>


namespace rg3::llvm::visitors
{
	class CxxTypeVisitor : public clang::RecursiveASTVisitor<CxxTypeVisitor>
	{
	 public:
		CxxTypeVisitor(std::vector<rg3::cpp::TypeBasePtr>& collectedTypes, const CompilerConfig& cc);

		bool VisitEnumDecl(clang::EnumDecl* enumDecl);

		bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl);


	 private:
		std::vector<rg3::cpp::TypeBasePtr>& m_collectedTypes;
		const CompilerConfig& compilerConfig;
	};
}