#include <RG3/PyBind/PyTypeClass.h>


namespace rg3::pybind
{
	PyTypeClass::PyTypeClass() = default;

	PyTypeClass::PyTypeClass(std::unique_ptr<cpp::TypeBase>&& base)
		: PyTypeBase(std::move(base))
	{
		// Precache
		if (auto self = getBase())
		{
			// Functions
			for (const auto& function : self->getProperties())
			{
				m_functions.append(function);
			}

			// Properties
			for (const auto& property : self->getProperties())
			{
				m_properties.append(property);
			}

			// Parents
			for (const auto& parent : self->getParentTypes())
			{
				m_parents.append(parent.getRefName());
			}
		}
	}

	cpp::TypeClass* PyTypeClass::getBase()
	{
		return m_base && m_base->getKind() == cpp::TypeKind::TK_STRUCT_OR_CLASS ? static_cast<cpp::TypeClass*>(m_base.get()) : nullptr; // NOLINT(*-pro-type-static-cast-downcast)
	}

	const cpp::TypeClass* PyTypeClass::getBase() const
	{
		return m_base && m_base->getKind() == cpp::TypeKind::TK_STRUCT_OR_CLASS ? static_cast<const cpp::TypeClass*>(m_base.get()) : nullptr; // NOLINT(*-pro-type-static-cast-downcast)
	}

	const boost::python::list& PyTypeClass::pyGetClassProperties() const
	{
		return m_properties;
	}

	const boost::python::list& PyTypeClass::pyGetClassFunctions() const
	{
		return m_functions;
	}

	const boost::python::list& PyTypeClass::pyGetClassParentsTypeNamesList() const
	{
		return m_parents;
	}

	bool PyTypeClass::pyIsStruct() const
	{
		if (auto self = getBase())
			return self->isStruct();

		return false;
	}

	bool PyTypeClass::pyIsTriviallyConstructible() const
	{
		if (auto self = getBase())
			return self->isTrivialConstructible();

		return false;
	}
}