#include <RG3/Cpp/TypeBase.h>


namespace rg3::cpp
{
	enum TypeFlags : uint32_t
	{
		TF_PRODUCED_FROM_TEMPLATE = (1 << 0),
		TF_PRODUCED_FROM_ALIAS = (1 << 1),
		TF_DECLARED_IN_ANOTHER_TYPE = (1 << 2)
	};

	namespace utils
	{
		inline void hashCombine(TypeID& seed) { }

		template <typename T, typename... Rest>
		inline void hashCombine(TypeID& seed, const T& v, Rest... rest) {
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
			hashCombine(seed, rest...);
		}
	}

	TypeBase::TypeBase() = default;

	TypeBase::TypeBase(TypeKind kind, const std::string& name, const std::string& prettyName, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags)
		: m_kind(kind)
		, m_name(name)
		, m_prettyName(prettyName)
		, m_nameSpace(aNamespace)
		, m_location(aLocation)
		, m_tags(tags)
	{
	}

	TypeID TypeBase::getID() const
	{
		TypeID seed = 0x0;
		utils::hashCombine(seed, m_kind, m_name, static_cast<std::string>(m_nameSpace), m_location.getFsLocation().string(), m_location.getLine(), m_location.getInLineOffset());
		return seed;
	}

	TypeKind TypeBase::getKind() const { return m_kind; }
	const std::string& TypeBase::getName() const { return m_name; }
	const CppNamespace& TypeBase::getNamespace() const { return m_nameSpace; }
	const std::string& TypeBase::getPrettyName() const { return m_prettyName; }

	const DefinitionLocation& TypeBase::getDefinition() const
	{
		return m_location;
	}

	void TypeBase::setDefinition(rg3::cpp::DefinitionLocation&& newLoc)
	{
		m_location = std::move(newLoc);
	}

	bool TypeBase::areSame(const TypeBase* pOther) const
	{
		if (!pOther)
			return false;

		return doAreSame(pOther);
	}

	const Tags& TypeBase::getTags() const
	{
		return m_tags;
	}

	Tags& TypeBase::getTags()
	{
		return m_tags;
	}

	bool TypeBase::isForwardDeclarable() const
	{
		if (isProducedFromAlias())
			return false;

		if (isProducedFromTemplate())
			return false;

		if (isDeclaredInAnotherType())
			return false;

		return getKind() == TypeKind::TK_STRUCT_OR_CLASS || getKind() == TypeKind::TK_ENUM;
	}

	void TypeBase::overrideTypeData(const std::string& name, const std::string& prettyName)
	{
		m_name = name;
		m_prettyName = prettyName;
	}

	void TypeBase::overrideTypeData(const std::string& name, const std::string& prettyName, const rg3::cpp::CppNamespace& aNamespace)
	{
		m_name = name;
		m_prettyName = prettyName;
		m_nameSpace = aNamespace;
	}

	void TypeBase::overrideTypeData(const std::string& name, const std::string& prettyName, const rg3::cpp::CppNamespace& aNamespace, const rg3::cpp::DefinitionLocation& aLocation)
	{
		m_name = name;
		m_prettyName = prettyName;
		m_nameSpace = aNamespace;
		m_location = aLocation;
	}

	void TypeBase::overrideTypeData(const std::string& name, const std::string& prettyName, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags)
	{
		m_name = name;
		m_prettyName = prettyName;
		m_nameSpace = aNamespace;
		m_location = aLocation;
		m_tags = tags;
	}

	void TypeBase::setProducedFromTemplate()
	{
		m_flags |= TypeFlags::TF_PRODUCED_FROM_TEMPLATE;
	}

	void TypeBase::setProducedFromAlias()
	{
		m_flags |= TypeFlags::TF_PRODUCED_FROM_ALIAS;
	}

	void TypeBase::setDeclaredInAnotherType()
	{
		m_flags |= TypeFlags::TF_DECLARED_IN_ANOTHER_TYPE;
	}

	bool TypeBase::isProducedFromTemplate() const
	{
		return m_flags & TypeFlags::TF_PRODUCED_FROM_TEMPLATE;
	}

	bool TypeBase::isProducedFromAlias() const
	{
		return m_flags & TypeFlags::TF_PRODUCED_FROM_ALIAS;
	}

	bool TypeBase::isDeclaredInAnotherType() const
	{
		return m_flags & TypeFlags::TF_DECLARED_IN_ANOTHER_TYPE;
	}

	void TypeBase::addTags(const Tags& vTags)
	{
		m_tags += vTags;
	}

	bool TypeBase::doAreSame(const TypeBase* pOther) const
	{
		return
				getKind()       == pOther->getKind()      &&
				getName()       == pOther->getName()      &&
				getNamespace()  == pOther->getNamespace() &&
				getDefinition() == pOther->getDefinition();
	}
}