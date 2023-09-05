#include <RG3/Cpp/TypeReference.h>


namespace rg3::cpp
{
	TypeReference::TypeReference() = default;

	TypeReference::TypeReference(const std::string& typeName) : m_typeName(typeName), m_typeRef(nullptr)
	{
	}

	TypeReference::TypeReference(const std::string& typeName, rg3::cpp::TypeBase* resolvedReference)
		: m_typeName(typeName), m_typeRef(resolvedReference)
	{
	}

	TypeBase* TypeReference::get()
	{
		if (!m_typeRef)
		{
			resolveTypeRef();
		}

		return m_typeRef;
	}

	const TypeBase* TypeReference::get() const
	{
		return m_typeRef;
	}

	const std::string& TypeReference::getRefName() const
	{
		return m_typeName;
	}

	TypeReference::operator bool() const noexcept
	{
		return !m_typeName.empty();
	}

	bool TypeReference::operator==(const TypeReference& other) const
	{
		return m_typeName == other.m_typeName;
	}

	bool TypeReference::operator!=(const TypeReference& other) const
	{
		return m_typeName != other.m_typeName;
	}

	void TypeReference::resolveTypeRef() const
	{
		// TODO: Implement me
	}
}