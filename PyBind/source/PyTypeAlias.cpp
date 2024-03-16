#include <RG3/PyBind/PyTypeAlias.h>


namespace rg3::pybind
{
	PyTypeAlias::PyTypeAlias() : PyTypeBase() {}

	PyTypeAlias::PyTypeAlias(std::unique_ptr<cpp::TypeBase>&& base)
		: PyTypeBase(std::move(base))
	{
	}

	const cpp::TypeReference& PyTypeAlias::pyGetTargetTypeRef() const
	{
		static const cpp::TypeReference s_Invalid{};
		return (getBase() && getBase()->getKind() == cpp::TypeKind::TK_ALIAS) ? static_cast<const cpp::TypeAlias*>(getBase())->getTargetType() : s_Invalid;
	}

	boost::python::object PyTypeAlias::pyGetTargetTypeLocation() const
	{
		auto pAlias = static_cast<const cpp::TypeAlias*>(getBase()); // NOLINT(*-pro-type-static-cast-downcast)
		const auto& desc = pAlias->getTargetTypeDescription();

		if (desc.sDefinitionLocation.has_value())
		{
			return boost::python::object(desc.sDefinitionLocation.value());
		}

		return {};
	}

	const cpp::TypeStatement& PyTypeAlias::getTargetTypeDescription() const
	{
		auto pAlias = static_cast<const cpp::TypeAlias*>(getBase()); // NOLINT(*-pro-type-static-cast-downcast)
		const auto& desc = pAlias->getTargetTypeDescription();

		return desc;
	}

	const cpp::TypeBase* PyTypeAlias::getBase() const
	{
		return m_base.get();
	}

	cpp::TypeBase* PyTypeAlias::getBase()
	{
		return m_base.get();
	}
}