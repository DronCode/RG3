#pragma once

#include <clang/AST/Decl.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeClass.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>


namespace rg3::llvm
{
	struct PropertyDescription
	{
		std::string propertyRefName {}; // A reference name to property (original)
		std::string propertyAliasName {}; // A new name of property (alias)
	};

	struct ExtraPropertiesFilter
	{
		const std::vector<PropertyDescription>& vKnownProperties;

		bool operator()(const std::string& sPropertyName) const
		{
			auto it = std::find_if(vKnownProperties.begin(), vKnownProperties.end(), [&sPropertyName](const PropertyDescription& pd) -> bool {
				return pd.propertyRefName == sPropertyName;
			});

			return it != vKnownProperties.end();
		}

		/**
		 * @brief Mutator overflow: this method return true if property acceptable and modified for future use
		 */
		bool operator()(cpp::ClassProperty& sProperty)
		{
			auto it = std::find_if(vKnownProperties.begin(), vKnownProperties.end(), [&sProperty](const PropertyDescription& pd) -> bool {
				return pd.propertyRefName == sProperty.sName;
			});

			if (it != vKnownProperties.end())
			{
				sProperty.sAlias = it->propertyAliasName;
				return true;
			}

			return false;
		}
	};

	struct ExtraFunctionsFilter
	{
		const std::vector<std::string>& vKnownFunctions;

		bool operator()(const std::string& sFuncName) const
		{
			return std::find(vKnownFunctions.begin(), vKnownFunctions.end(), sFuncName) != vKnownFunctions.end();
		}
	};

	struct Annotations
	{
		bool bIsRuntime { false };
		bool bInterpretAsTrivial { false };
		std::optional<std::string> overrideLocation {};
		std::vector<PropertyDescription> knownProperties {};
		std::vector<std::string> knownFunctions {};
		cpp::Tags additionalTags {};

		[[nodiscard]] bool isRuntime() const { return bIsRuntime; }

		void collectFromDecl(clang::Decl* pDecl);
	};
}