#include <RG3/PyBind/PyTypeClass.h>
#include <RG3/PyBind/PyClassParent.h>


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
			for (const auto& function : self->getFunctions())
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
				m_parents.append(boost::shared_ptr<PyClassParent>(new PyClassParent(parent)));
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

	const boost::python::list& PyTypeClass::pyGetClassParentTypeRefs() const
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

	bool PyTypeClass::pyHasCopyConstructor() const
	{
		if (auto self = getBase())
			return self->hasCopyConstructor();

		return false;
	}

	bool PyTypeClass::pyHasCopyAssignOperator() const
	{
		if (auto self = getBase())
			return self->hasCopyAssignOperator();

		return false;
	}

	bool PyTypeClass::pyHasMoveConstructor() const
	{
		if (auto self = getBase())
			return self->hasMoveConstructor();

		return false;
	}

	bool PyTypeClass::pyHasMoveAssignOperator() const
	{
		if (auto self = getBase())
			return self->hasMoveAssignOperator();

		return false;
	}
}