#pragma once

#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/DefinitionLocation.h>
#include <clang/AST/Decl.h>

#include <filesystem>
#include <string>


namespace rg3::llvm
{
	struct Utils
	{
		static void getDeclInfo(const clang::Decl* decl, rg3::cpp::CppNamespace& nameSpace);

		static cpp::DefinitionLocation getDeclDefinitionInfo(const clang::Decl* decl);
	};
}