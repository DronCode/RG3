#include <RG3/PyBind/PyTypeBase.h>


namespace rg3::pybind
{
	PyTypeBase::PyTypeBase() = default;

	PyTypeBase::PyTypeBase(cpp::TypeBasePtr&& base)
		: m_base(std::move(base))
	{
		if (m_base)
		{
			// Repr & str
			{
				std::string str;

				switch (m_base->getKind())
				{
				case cpp::TypeKind::TK_NONE:
					str = "none";
					break;
				case cpp::TypeKind::TK_TRIVIAL:
					str = m_base->getPrettyName();
					break;
				case cpp::TypeKind::TK_ENUM:
					str = "enum " + m_base->getPrettyName();
					break;
				case cpp::TypeKind::TK_STRUCT_OR_CLASS:
					str = "class " + m_base->getPrettyName();
					break;
				case cpp::TypeKind::TK_TEMPLATE_SPECIALIZATION:
					str = "template " + m_base->getPrettyName();
					break;
				case cpp::TypeKind::TK_ALIAS:
					str = "alias " + m_base->getPrettyName();
					break;
				}

				m_str = { str.c_str(), str.length() };
			}

			m_repr = m_str;
		}
	}

	const boost::python::str& PyTypeBase::__str__() const
	{
		return m_str;
	}

	const boost::python::str& PyTypeBase::__repr__() const
	{
		return m_repr;
	}

	bool PyTypeBase::__eq__(const PyTypeBase& another) const
	{
		if (another.m_base != nullptr && m_base != nullptr)
		{
			return m_base->areSame(another.m_base.get());
		}

		return false;
	}

	bool PyTypeBase::__ne__(const PyTypeBase& another) const
	{
		return !__eq__(another);
	}

	std::uint64_t PyTypeBase::__hash__() const
	{
		return m_base ? m_base->getID() : 0u;
	}

	const rg3::cpp::Tags& PyTypeBase::pyGetTags() const
	{
		static rg3::cpp::Tags s_Empty {};
		return m_base ? m_base->getTags() : s_Empty;
	}


	cpp::TypeKind PyTypeBase::pyGetTypeKind() const
	{
		return m_base ? m_base->getKind() : cpp::TypeKind::TK_NONE;
	}

	boost::python::str PyTypeBase::pyGetName() const
	{
		if (m_base)
		{
			const auto& str = m_base->getName();
			return { str.c_str(), str.length() };
		}

		return {};
	}

	boost::python::str PyTypeBase::pyGetPrettyName() const
	{
		boost::python::str result;
		if (m_base)
		{
			const auto pretty = m_base->getPrettyName();
			result = { pretty.c_str(), pretty.length() };
		}

		return result;
	}

	const cpp::CppNamespace& PyTypeBase::pyGetNamespace() const
	{
		static cpp::CppNamespace g_invalidNamespace;
		return m_base ? m_base->getNamespace() : g_invalidNamespace;
	}

	const cpp::DefinitionLocation& PyTypeBase::pyGetLocation() const
	{
		static cpp::DefinitionLocation g_invalidDef;
		return m_base ? m_base->getDefinition() : g_invalidDef;
	}
}