#include <RG3/Cpp/TypeEnum.h>
#include <algorithm>


namespace rg3::cpp
{
	EnumEntry::EnumEntry() = default;

	EnumEntry::EnumEntry(const std::string& name, EnumEntry::ValueType value)
		: sName(name), iValue(value)
	{
	}

	bool EnumEntry::operator==(const EnumEntry& other) const
	{
		return sName == other.sName && iValue == other.iValue;
	}

	bool EnumEntry::operator!=(const EnumEntry& other) const
	{
		return !operator==(other);
	}

	TypeEnum::TypeEnum() = default;

	TypeEnum::TypeEnum(const std::string& name, const std::string& prettyName, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags, const EnumEntryVector& aValues, bool bIsScoped, TypeReference underlyingType)
		: TypeBase(TypeKind::TK_ENUM, name, prettyName, aNamespace, aLocation, tags)
		, m_entries(aValues)
		, m_bScoped(bIsScoped)
		, m_rUnderlyingType(underlyingType)
	{
	}

	const EnumEntryVector& TypeEnum::getEntries() const
	{
		return m_entries;
	}

	EnumEntryVector& TypeEnum::getEntries()
	{
		return m_entries;
	}

	bool TypeEnum::containsValue(EnumEntry::ValueType value) const
	{
		auto it = std::find_if(m_entries.begin(), m_entries.end(), [&expected = value](const EnumEntry& entry) -> bool {
			return entry.iValue == expected;
		});

		return it != m_entries.end();
	}

	std::string_view TypeEnum::operator[](EnumEntry::ValueType value) const
	{
		std::string_view kInvalid;

		auto it = std::find_if(m_entries.begin(), m_entries.end(), [&expected = value](const EnumEntry& entry) -> bool {
		  return entry.iValue == expected;
		});

		return it == m_entries.end() ? kInvalid : it->sName;
	}

	bool TypeEnum::isScoped() const
	{
		return m_bScoped;
	}

	TypeReference TypeEnum::getUnderlyingType() const
	{
		return m_rUnderlyingType;
	}

	bool TypeEnum::doAreSame(const TypeBase* pOther) const
	{
		if (pOther->getKind() != TypeKind::TK_ENUM)
			return false;

		const auto* pOtherEnum = static_cast<const TypeEnum*>(pOther); // NOLINT(*-pro-type-static-cast-downcast)

		if (pOtherEnum->isScoped() != isScoped())
			return false;

		const EnumEntryVector& v1 = pOtherEnum->getEntries();
		const EnumEntryVector& v2 = getEntries();

		return v1 == v2 && TypeBase::areSame(pOther);
	}
}