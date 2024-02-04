#pragma once

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeReference.h>
#include <vector>
#include <string>
#include <optional>


namespace rg3::cpp
{
	enum class ClassEntryVisibility : uint8_t
	{
		CEV_PRIVATE,   ///< Private entry
		CEV_PUBLIC,	   ///< Public entry
		CEV_PROTECTED  ///< Protected entry
	};

	struct ClassProperty
	{
		std::string sName {};
		std::string sAlias {};
		TypeReference sTypeName {};
		ClassEntryVisibility eVisibility { ClassEntryVisibility::CEV_PRIVATE };
		Tags vTags {};

		bool operator==(const ClassProperty& other) const;
		bool operator!=(const ClassProperty& other) const;
	};

	struct ClassFunction
	{
		std::string sName {}; /// Name of function
		std::string sOwnerClassName {}; /// Owner class name
		ClassEntryVisibility eVisibility { ClassEntryVisibility::CEV_PRIVATE }; /// Visibility level of entry
		Tags vTags {}; /// List of tags of function decl
		bool bIsStatic { false }; /// Is static entry
		bool bIsConst { false }; /// Is const method (for static method doesn't matter)

		bool operator==(const ClassFunction& other) const;
		bool operator!=(const ClassFunction& other) const;
	};

	using ClassPropertyVector = std::vector<ClassProperty>;
	using ClassFunctionVector = std::vector<ClassFunction>;

	class TypeClass : public TypeBase
	{
	 public:
		TypeClass();
		TypeClass(const std::string& name, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags, const ClassPropertyVector& aProperties, const ClassFunctionVector& aFunctions, bool bIsStruct, bool bTrivialConstructible, const std::vector<TypeReference>& parentTypes);

		[[nodiscard]] const ClassPropertyVector& getProperties() const;
		[[nodiscard]] ClassPropertyVector& getProperties();
		[[nodiscard]] const ClassFunctionVector& getFunctions() const;
		[[nodiscard]] ClassFunctionVector& getFunctions();
		[[nodiscard]] bool isStruct() const;
		[[nodiscard]] bool isTrivialConstructible() const;
		[[nodiscard]] const std::vector<TypeReference>& getParentTypes() const;
		[[nodiscard]] std::vector<TypeReference>& getParentTypes();

	 protected:
		bool doAreSame(const TypeBase* pOther) const override;

	 private:
		ClassPropertyVector m_properties {};
		ClassFunctionVector m_functions {};
		bool m_bIsStruct { false };
		bool m_bIsTrivialConstructible { false };
		std::vector<TypeReference> m_parentTypes {};
	};
}