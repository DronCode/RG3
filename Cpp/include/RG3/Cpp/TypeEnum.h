#pragma once

#include <RG3/Cpp/TypeReference.h>
#include <RG3/Cpp/TypeBase.h>
#include <vector>


namespace rg3::cpp
{
	struct EnumEntry
	{
		using ValueType = std::int64_t;

		std::string sName {};
		ValueType   iValue { 0 };

		EnumEntry();
		EnumEntry(const std::string& name, ValueType value);

		[[nodiscard]] bool operator==(const EnumEntry& other) const;
		[[nodiscard]] bool operator!=(const EnumEntry& other) const;
	};

	using EnumEntryVector = std::vector<EnumEntry>;

	class TypeEnum : public TypeBase
	{
	 public:
		TypeEnum();
		TypeEnum(const std::string& name, const std::string& prettyName, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags, const EnumEntryVector& aValues, bool bIsScoped, TypeReference underlyingType);

		[[nodiscard]] const EnumEntryVector& getEntries() const;
		[[nodiscard]] bool containsValue(EnumEntry::ValueType value) const;
		[[nodiscard]] std::string_view operator[](EnumEntry::ValueType value) const;
		[[nodiscard]] bool isScoped() const;
		[[nodiscard]] TypeReference getUnderlyingType() const;

	 protected:
		bool doAreSame(const TypeBase* pOther) const override;

	 private:
		EnumEntryVector m_entries {};
		bool m_bScoped { false };
		TypeReference m_rUnderlyingType {};
	};
}