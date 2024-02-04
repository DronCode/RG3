#pragma once

#include <string>


namespace rg3::cpp
{
	class TypeBase;

	class TypeReference
	{
	 public:
		TypeReference();
		explicit TypeReference(const std::string& typeName);
		TypeReference(const std::string& typeName, TypeBase* resolvedReference);

		TypeBase* get();
		const TypeBase* get() const;

		[[nodiscard]] const std::string& getRefName() const;

		explicit operator bool() const noexcept;
		bool operator==(const TypeReference& other) const;
		bool operator!=(const TypeReference& other) const;

		void setResolvedType(TypeBase* pType);

	 private:
		std::string m_typeName {};
		mutable TypeBase* m_typeRef { nullptr };
	};
}