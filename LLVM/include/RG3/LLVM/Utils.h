#pragma once

#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/DefinitionLocation.h>
#include <RG3/Cpp/TypeClass.h>
#include <clang/AST/Decl.h>

#include <filesystem>
#include <string>


namespace rg3::llvm
{
	struct Utils
	{
		static void getDeclInfo(const clang::Decl* decl, rg3::cpp::CppNamespace& nameSpace);

		static cpp::DefinitionLocation getDeclDefinitionInfo(const clang::Decl* decl);

		static cpp::ClassEntryVisibility getDeclVisibilityLevel(const clang::Decl* decl);

		static std::string getNormalizedTypeRef(const std::string& typeName);

		static void fillTypeStatementFromQualType(rg3::cpp::TypeStatement& typeStatement, clang::QualType qt, const clang::ASTContext& astContext);

		static std::string getPrettyNameOfDecl(clang::NamedDecl* pDecl);
	};
}