#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>

#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/Cpp/TypeClass.h>
#include <vector>


namespace rg3::llvm::visitors
{
	class CxxClassTypeVisitor : public clang::RecursiveASTVisitor<CxxClassTypeVisitor>
	{
	 public:
		explicit CxxClassTypeVisitor(const CompilerConfig& cc);

		bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl);
		bool VisitFieldDecl(clang::FieldDecl* cxxFieldDecl);
		bool VisitCXXMethodDecl(clang::CXXMethodDecl* cxxMethodDecl);

		std::string sClassName;
		std::string sClassPrettyName;
		cpp::CppNamespace sNameSpace;
		cpp::Tags vTags {};
		cpp::DefinitionLocation sDefinitionLocation;
		cpp::ClassPropertyVector foundProperties {};
		cpp::ClassFunctionVector foundFunctions {};
		bool bIsStruct { false };
		bool bTriviallyConstructible { false };
		std::vector<cpp::ClassParent> parentClasses {};

	 private:
		const CompilerConfig& compilerConfig;
	};
}