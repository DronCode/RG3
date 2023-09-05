#pragma once


#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>

#include <RG3/Cpp/TypeBase.h>
#include <vector>


namespace rg3::llvm::visitors
{
	class CxxTypeVisitor : public clang::RecursiveASTVisitor<CxxTypeVisitor>
	{
	 public:
		explicit CxxTypeVisitor(std::vector<rg3::cpp::TypeBasePtr>& collectedTypes);

		bool VisitEnumDecl(clang::EnumDecl* enumDecl);

		bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl);


	 private:
		std::vector<rg3::cpp::TypeBasePtr>& m_collectedTypes;
	};
}