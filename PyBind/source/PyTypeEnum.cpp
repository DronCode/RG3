#include <RG3/PyBind/PyTypeEnum.h>


namespace rg3::pybind
{
	PyTypeEnum::PyTypeEnum() = default;

	PyTypeEnum::PyTypeEnum(std::unique_ptr<cpp::TypeBase>&& base)
		: PyTypeBase(std::move(base))
	{
		// Precache
		if (auto self = getBase())
		{
			for (const auto& entry : self->getEntries())
			{
				m_entries.append(entry);
			}

			// Underlying type
			const auto& ut = self->getUnderlyingType();
			const auto& utr = ut.getRefName();
			m_underlyingType = { utr.c_str(), utr.length() };
		}
	}

	const boost::python::list& PyTypeEnum::pyGetEnumEntries() const
	{
		return m_entries;
	}

	bool PyTypeEnum::pyIsScoped() const
	{
		if (auto self = getBase())
			return self->isScoped();

		return false;
	}

	const boost::python::str& PyTypeEnum::pyGetUnderlyingTypeStr() const
	{
		return m_underlyingType;
	}

	cpp::TypeEnum* PyTypeEnum::getBase()
	{
		return m_base && m_base->getKind() == cpp::TypeKind::TK_ENUM ? static_cast<cpp::TypeEnum*>(m_base.get()) : nullptr; // NOLINT(*-pro-type-static-cast-downcast)
	}

	const cpp::TypeEnum* PyTypeEnum::getBase() const
	{
		return m_base && m_base->getKind() == cpp::TypeKind::TK_ENUM ? static_cast<const cpp::TypeEnum*>(m_base.get()) : nullptr; // NOLINT(*-pro-type-static-cast-downcast)
	}
}