#pragma once

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeStatement.h>


namespace rg3::cpp
{
	/**
	 * @brief This class represents any typedef or using which marked as "runtime". It could be a part of another "record" type (like struct, class)
	 */
	class TypeAlias : public TypeBase
	{
	 public:
		TypeAlias();
		TypeAlias(const std::string& name, const std::string& prettyName, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags, TypeStatement sTargetType);

		[[nodiscard]] const TypeReference& getTargetType() const;
		[[nodiscard]] TypeReference& getTargetType();

		/**
		 * @note It's better to use getTargetTypeDescription
		 */
		[[nodiscard]] DefinitionLocation getTargetTypeDefinedAt() const;

		bool hasTargetTypeDefinitionLocation() const;

		[[nodiscard]] const TypeStatement& getTargetTypeDescription() const;

	 protected:
		bool doAreSame(const TypeBase* pOther) const override;

	 protected:
		TypeStatement m_sTargetType {};
	};
}