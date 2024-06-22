#pragma once


#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>

#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/Cpp/TypeBase.h>

#include <cstdint>
#include <vector>
#include <set>


namespace rg3::llvm::visitors
{
	class CxxTypeVisitor : public clang::RecursiveASTVisitor<CxxTypeVisitor>
	{
	 public:
		CxxTypeVisitor(std::vector<rg3::cpp::TypeBasePtr>& collectedTypes, const CompilerConfig& cc);

		bool VisitEnumDecl(clang::EnumDecl* enumDecl);

		bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl);

		bool VisitEnumConstantDecl(clang::EnumConstantDecl* D);

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
		std::vector<std::int64_t> m_collectedTypeIDs;
		const CompilerConfig& compilerConfig;
	};
}