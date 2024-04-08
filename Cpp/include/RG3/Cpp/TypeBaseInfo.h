#pragma once

#include <RG3/Cpp/DefinitionLocation.h>
#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/TypeKind.h>

#include <string>


namespace rg3::cpp
{
	/**
	 * Base information about type (what kind, name, pretty name, namespace, where defined)
	 */
	struct TypeBaseInfo
	{
		cpp::TypeKind eKind {};
		std::string sName {};
		std::string sPrettyName {};
		cpp::CppNamespace sNameSpace {};
		cpp::DefinitionLocation sDefLocation {};

		TypeBaseInfo();
		TypeBaseInfo(const TypeBaseInfo& copy);
		TypeBaseInfo(TypeBaseInfo&& move) noexcept;
		TypeBaseInfo& operator=(const TypeBaseInfo& copy);
		TypeBaseInfo& operator=(TypeBaseInfo&& move) noexcept;
	};
}