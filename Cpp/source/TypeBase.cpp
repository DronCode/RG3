#include <RG3/Cpp/TypeBase.h>


namespace rg3::cpp
{
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

	bool TypeBase::doAreSame(const TypeBase* pOther) const
	{
		return
				getKind()       == pOther->getKind()      &&
				getName()       == pOther->getName()      &&
				getNamespace()  == pOther->getNamespace() &&
				getDefinition() == pOther->getDefinition();
	}
}