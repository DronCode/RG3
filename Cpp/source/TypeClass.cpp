#include <RG3/Cpp/TypeClass.h>


namespace rg3::cpp
{
	bool ClassProperty::operator==(const ClassProperty& other) const
	{
		return  sName == other.sName &&
				sAlias == other.sAlias &&
				sTypeName == other.sTypeName &&
				eVisibility == other.eVisibility;
	}

	bool ClassProperty::operator!=(const ClassProperty& other) const
	{
		return !operator==(other);
	}

	bool ClassFunction::operator==(const ClassFunction& other) const
	{
		return  sName == other.sName &&
				sOwnerClassName == other.sOwnerClassName &&
				eVisibility == other.eVisibility &&
				bIsStatic == other.bIsStatic;
	}

	bool ClassFunction::operator!=(const ClassFunction& other) const
	{
		return !operator==(other);
	}

	TypeClass::TypeClass() = default;

	TypeClass::TypeClass(const std::string& name, const rg3::cpp::CppNamespace& aNamespace, const rg3::cpp::DefinitionLocation& aLocation, const Tags& tags, const rg3::cpp::ClassPropertyVector& aProperties, const rg3::cpp::ClassFunctionVector& aFunctions, bool bIsStruct, bool bTrivialConstructible, const std::vector<TypeReference>& parentTypes)
		: TypeBase(TypeKind::TK_STRUCT_OR_CLASS, name, aNamespace, aLocation, tags)
		, m_properties(aProperties)
		, m_functions(aFunctions)
		, m_bIsStruct(bIsStruct)
		, m_bIsTrivialConstructible(bTrivialConstructible)
		, m_parentTypes(parentTypes)
	{
	}

	const ClassPropertyVector& TypeClass::getProperties() const
	{
		return m_properties;
	}

	const ClassFunctionVector& TypeClass::getFunctions() const
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

	const std::vector<TypeReference>& TypeClass::getParentTypes() const
	{
		return m_parentTypes;
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