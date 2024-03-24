#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>

#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/Cpp/TypeClass.h>
#include <functional>
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

		// Extra types
		std::vector<rg3::cpp::TypeBasePtr> vFoundExtraTypes {};

	 public: // Types

		struct PropertyDescription
		{
			std::string propertyRefName {}; // A reference name to property (original)
			std::string propertyAliasName {}; // A new name of property (alias)
		};

	 private:
		void handleTypeAnnotation(clang::CXXRecordDecl* cxxRecordDecl, clang::AnnotateAttr* pAnnotateAttr);
		void handleAndMergeTypedefTypeWithInner(const clang::TypedefType* pType, const std::vector<PropertyDescription>& vKnownProperties, const std::vector<std::string>& vKnownFunctions);
		void handleCxxDeclAndOverridePropertiesOwner(const clang::CXXRecordDecl* pCxxDecl,
													 const rg3::cpp::TypeBase* pNewOwnerType,
													 CxxClassTypeVisitor& sTypeVisitor,
													 const std::function<bool(const std::string&)>& propFilter,
													 const std::function<bool(const std::string&)>& funcFilter);

	 private:
		const CompilerConfig& compilerConfig;
	};
}