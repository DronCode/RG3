#pragma once

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeReference.h>
#include <RG3/Cpp/TypeStatement.h>
#include <vector>
#include <string>
#include <optional>


namespace rg3::cpp
{
	enum class ClassEntryVisibility : uint8_t
	{
		CEV_PUBLIC = 0,	   ///< Public entry
		CEV_PRIVATE = 1,   ///< Private entry
		CEV_PROTECTED = 2  ///< Protected entry
	};

	enum class InheritanceVisibility : uint8_t
	{
		IV_PUBLIC = 0,  ///< public inheritance (default for struct)
		IV_PRIVATE = 1, ///< private inheritance (default for class)
		IV_PROTECTED = 2, ///< protected inheritance
		IV_VIRTUAL = 3  ///< virtual inheritance
	};

	struct ClassProperty
	{
		std::string sName {};
		std::string sAlias {};
		TypeStatement sTypeInfo {};
		ClassEntryVisibility eVisibility { ClassEntryVisibility::CEV_PRIVATE };
		Tags vTags {};

		bool operator==(const ClassProperty& other) const;
		bool operator!=(const ClassProperty& other) const;
	};

	struct FunctionArgument
	{
		TypeStatement sType {};
		std::string sArgumentName {};
		bool bHasDefaultValue { false };

		bool operator==(const FunctionArgument& other) const;
		bool operator!=(const FunctionArgument& other) const;
	};

	using FunctionArgumentsVector = std::vector<FunctionArgument>;

	struct ClassFunction
	{
		std::string sName {}; /// Name of function
		std::string sOwnerClassName {}; /// Owner class name
		ClassEntryVisibility eVisibility { ClassEntryVisibility::CEV_PRIVATE }; /// Visibility level of entry
		Tags vTags {}; /// List of tags of function decl
		TypeStatement sReturnType {}; // Return type of function (or void)
		FunctionArgumentsVector vArguments {}; // Function arguments

		bool bIsStatic { false }; /// Is static entry
		bool bIsConst { false }; /// Is const method (for static method doesn't matter)
		bool bIsNoExcept { false }; // If method marked as noexcept

		bool operator==(const ClassFunction& other) const;
		bool operator!=(const ClassFunction& other) const;
	};

	struct ClassParent
	{
		TypeBaseInfo sTypeBaseInfo {}; /// Base information about type
		InheritanceVisibility eModifier { InheritanceVisibility::IV_PRIVATE }; /// modifier mode
		Tags vTags {}; /// List of tags of that parent type (or empty when unable to reach them)
	};

	using ClassPropertyVector = std::vector<ClassProperty>;
	using ClassFunctionVector = std::vector<ClassFunction>;

	struct ClassFriend
	{
		// In C++ allowed to be friend of function, class, T (c++0x) and... Idk, it's really weird shit
		// I will support only class & struct friends (at least now)
		TypeBaseInfo sFriendTypeInfo {};

		ClassFriend();
		ClassFriend(TypeBaseInfo&& sInfo);
	};

	using ClassFriendVector = std::vector<ClassFriend>;

	class TypeClass : public TypeBase
	{
	 public:
		TypeClass();
		TypeClass(const std::string& name, const std::string& prettyName, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags,
				  const ClassPropertyVector& aProperties, const ClassFunctionVector& aFunctions, const ClassFriendVector& aFriends,
				  bool bIsStruct, bool bTrivialConstructible, bool bHasCopyConstructor, bool bHasCopyAssignOperator, bool bHasMoveConstructor, bool bHasMoveAssignOperator,
				  const std::vector<ClassParent>& parentTypes);

		[[nodiscard]] const ClassPropertyVector& getProperties() const;
		[[nodiscard]] ClassPropertyVector& getProperties();
		[[nodiscard]] const ClassFunctionVector& getFunctions() const;
		[[nodiscard]] ClassFunctionVector& getFunctions();
		[[nodiscard]] bool isStruct() const;
		[[nodiscard]] bool isTrivialConstructible() const;
		[[nodiscard]] const std::vector<ClassParent>& getParentTypes() const;
		[[nodiscard]] std::vector<ClassParent>& getParentTypes();
		[[nodiscard]] bool hasCopyConstructor() const;
		[[nodiscard]] bool hasMoveConstructor() const;
		[[nodiscard]] bool hasCopyAssignOperator() const;
		[[nodiscard]] bool hasMoveAssignOperator() const;
		[[nodiscard]] const ClassFriendVector& getClassFriends() const;
		[[nodiscard]] ClassFriendVector& getClassFriends();

	 protected:
		bool doAreSame(const TypeBase* pOther) const override;

	 private:
		ClassPropertyVector m_properties {};
		ClassFunctionVector m_functions {};
		ClassFriendVector m_friends {};
		bool m_bIsStruct { false };
		bool m_bIsTrivialConstructible { false };
		bool m_bHasCopyConstructor { false };
		bool m_bHasCopyAssignOperator { false };
		bool m_bHasMoveConstructor { false };
		bool m_bHasMoveAssignOperator { false };
		std::vector<ClassParent> m_parentTypes {};
	};
}