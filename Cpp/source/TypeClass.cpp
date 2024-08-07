#include <RG3/Cpp/TypeClass.h>


namespace rg3::cpp
{
	bool ClassProperty::operator==(const ClassProperty& other) const
	{
		return  sName == other.sName &&
				sAlias == other.sAlias &&
			    sTypeInfo == other.sTypeInfo &&
				eVisibility == other.eVisibility;
	}

	bool ClassProperty::operator!=(const ClassProperty& other) const
	{
		return !operator==(other);
	}

	bool FunctionArgument::operator==(const FunctionArgument& other) const
	{
		return sType == other.sType &&
			   sArgumentName == other.sArgumentName &&
			   bHasDefaultValue == other.bHasDefaultValue;
	}

	bool FunctionArgument::operator!=(const FunctionArgument& other) const
	{
		return !operator==(other);
	}

	bool ClassFunction::operator==(const ClassFunction& other) const
	{
		return  sName == other.sName &&
				sOwnerClassName == other.sOwnerClassName &&
				eVisibility == other.eVisibility &&
				bIsStatic == other.bIsStatic &&
				sReturnType == other.sReturnType &&
			    vArguments == other.vArguments;
	}

	bool ClassFunction::operator!=(const ClassFunction& other) const
	{
		return !operator==(other);
	}

	ClassFriend::ClassFriend()
	{
	}

	ClassFriend::ClassFriend(rg3::cpp::TypeBaseInfo&& sInfo)
		: sFriendTypeInfo(std::move(sInfo))
	{
	}

	TypeClass::TypeClass() = default;

	TypeClass::TypeClass(const std::string& name, const std::string& prettyName, const rg3::cpp::CppNamespace& aNamespace, const rg3::cpp::DefinitionLocation& aLocation, const Tags& tags, const rg3::cpp::ClassPropertyVector& aProperties, const rg3::cpp::ClassFunctionVector& aFunctions, const ClassFriendVector& aFriends, bool bIsStruct, bool bTrivialConstructible, bool bHasCopyConstructor, bool bHasCopyAssignOperator, bool bHasMoveConstructor, bool bHasMoveAssignOperator, const std::vector<ClassParent>& parentTypes)
		: TypeBase(TypeKind::TK_STRUCT_OR_CLASS, name, prettyName, aNamespace, aLocation, tags)
		, m_properties(aProperties)
		, m_functions(aFunctions)
		, m_friends(aFriends)
		, m_bIsStruct(bIsStruct)
		, m_bIsTrivialConstructible(bTrivialConstructible)
		, m_bHasCopyConstructor(bHasCopyConstructor)
		, m_bHasCopyAssignOperator(bHasCopyAssignOperator)
		, m_bHasMoveConstructor(bHasMoveConstructor)
		, m_bHasMoveAssignOperator(bHasMoveAssignOperator)
		, m_parentTypes(parentTypes)
	{
	}

	const ClassPropertyVector& TypeClass::getProperties() const
	{
		return m_properties;
	}

	ClassPropertyVector& TypeClass::getProperties()
	{
		return m_properties;
	}

	const ClassFunctionVector& TypeClass::getFunctions() const
	{
		return m_functions;
	}

	ClassFunctionVector& TypeClass::getFunctions()
	{
		return m_functions;
	}

	bool TypeClass::isStruct() const
	{
		return m_bIsStruct;
	}

	bool TypeClass::isTrivialConstructible() const
	{
		return m_bIsTrivialConstructible;
	}

	const std::vector<ClassParent>& TypeClass::getParentTypes() const
	{
		return m_parentTypes;
	}

	std::vector<ClassParent>& TypeClass::getParentTypes()
	{
		return m_parentTypes;
	}

	bool TypeClass::hasCopyConstructor() const
	{
		return m_bHasCopyConstructor;
	}

	bool TypeClass::hasCopyAssignOperator() const
	{
		return m_bHasCopyAssignOperator;
	}

	bool TypeClass::hasMoveConstructor() const
	{
		return m_bHasMoveConstructor;
	}

	bool TypeClass::hasMoveAssignOperator() const
	{
		return m_bHasMoveAssignOperator;
	}

	const ClassFriendVector& TypeClass::getClassFriends() const
	{
		return m_friends;
	}

	ClassFriendVector& TypeClass::getClassFriends()
	{
		return m_friends;
	}

	bool TypeClass::doAreSame(const TypeBase* pOther) const
	{
		if (pOther->getKind() != TypeKind::TK_STRUCT_OR_CLASS)
			return false;

		const auto* pOtherClass = static_cast<const TypeClass*>(pOther);
		if (pOtherClass->isStruct() != isStruct())
			return false;

		if (pOtherClass->getProperties() != getProperties())
			return false;

		if (pOtherClass->getFunctions() != getFunctions())
			return false;

		return TypeBase::areSame(pOther);
	}
}