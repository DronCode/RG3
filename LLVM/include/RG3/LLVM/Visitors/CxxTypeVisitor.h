#pragma once


#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>

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

		bool VisitTypedefDecl(clang::TypedefDecl* typedefDecl);

		bool VisitTypeAliasDecl(clang::TypeAliasDecl* typeAliasDecl);

	 private:
		template <typename T>
		static bool isDeclInsideClassOrStruct(T* pDecl) requires (std::is_base_of_v<clang::Decl, T>)
		{
			if (!pDecl)
			{
				return false;
			}

			clang::DeclContext* pCtx = pDecl->getDeclContext();
			if (!pCtx)
			{
				return false;
			}

			return pCtx->isRecord();
		}

	 private:
		std::vector<rg3::cpp::TypeBasePtr>& m_collectedTypes;
		const CompilerConfig& compilerConfig;
	};
}