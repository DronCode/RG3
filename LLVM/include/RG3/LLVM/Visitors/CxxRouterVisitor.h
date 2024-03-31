#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>

#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/LLVM/Annotations.h>
#include <RG3/Cpp/TypeBase.h>

#include <vector>


namespace rg3::llvm::visitors
{
	/**
	 * @brief This class handle all types and understand which visitor should handle this type.
	 *
	 */
	class CxxRouterVisitor : public clang::RecursiveASTVisitor<CxxRouterVisitor>
	{
	 public:
		CxxRouterVisitor(std::vector<rg3::cpp::TypeBasePtr>& vFoundTypes, const CompilerConfig& compilerConfig);

	 public: // visitors
		bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl); // For C++ types (struct, class)
		bool VisitEnumDecl(clang::EnumDecl* enumDecl); // For enumerations
		bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl); // For aliases (typedef, using)

	 private:
		bool handleAnnotationBasedType(const clang::Type* pType,
									   const rg3::llvm::Annotations& annotation,
									   const clang::ASTContext& ctx,
									   bool bDirectInvoke);

	 private:
		const CompilerConfig& m_compilerConfig;
		std::vector<rg3::cpp::TypeBasePtr>& m_vFoundTypes;
	};
}