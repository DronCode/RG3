#include <RG3/Cpp/TypeAlias.h>

#include <utility>


namespace rg3::cpp
{
	TypeAlias::TypeAlias() : TypeBase() {}

	TypeAlias::TypeAlias(const std::string& name, const std::string& prettyName, const rg3::cpp::CppNamespace& aNamespace, const rg3::cpp::DefinitionLocation& aLocation, const rg3::cpp::Tags& tags, rg3::cpp::TypeStatement sTargetType)
		: TypeBase(rg3::cpp::TypeKind::TK_ALIAS, name, prettyName, aNamespace, aLocation, tags)
	    , m_sTargetType(std::move(sTargetType))
	{
	}

	const TypeReference& TypeAlias::getTargetType() const
	{
		return m_sTargetType.sTypeRef;
	}

	TypeReference& TypeAlias::getTargetType()
	{
		return m_sTargetType.sTypeRef;
	}

	DefinitionLocation TypeAlias::getTargetTypeDefinedAt() const
	{
		static const DefinitionLocation s_Undefined {};
		return m_sTargetType.sDefinitionLocation.value_or(s_Undefined);
	}

	const TypeStatement& TypeAlias::getTargetTypeDescription() const
	{
		return m_sTargetType;
	}

	bool TypeAlias::doAreSame(const rg3::cpp::TypeBase* pOther) const
	{
		if (pOther->getKind() != TypeKind::TK_ALIAS)
			return false;

		const auto* pOtherAlias = static_cast<const TypeAlias*>(pOther); // NOLINT(*-pro-type-static-cast-downcast)
		if (pOtherAlias->m_sTargetType != m_sTargetType)
			return false;

		return TypeBase::doAreSame(pOther);
	}
}