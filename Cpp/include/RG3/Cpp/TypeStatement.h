#pragma once

#include <RG3/Cpp/TypeReference.h>
#include <RG3/Cpp/DefinitionLocation.h>
#include <RG3/Cpp/TypeBaseInfo.h>

#include <optional>
#include <vector>


namespace rg3::cpp
{
	struct TypeStatement
	{
		// Reference
		TypeReference sTypeRef {};

		// Statement location
		std::optional<DefinitionLocation> sDefinitionLocation {};
		bool bIsConst { false };
		bool bIsPointer { false };
		bool bIsPtrConst { false };
		bool bIsReference { false };
		bool bIsTemplateSpecialization { false };

		// Type info
		TypeBaseInfo sBaseInfo {};

		// Globals
		static const TypeStatement g_sVoid; // globally known void type

		bool isVoid() const;

		bool operator==(const TypeStatement& other) const;
		bool operator!=(const TypeStatement& other) const;
	};

	using TypeStatementVector = std::vector<TypeStatement>;
}