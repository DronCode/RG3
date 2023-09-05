#include <RG3/PyBind/PyTag.h>


namespace rg3::pybind
{
	PyTag::PyTag() = default;

	PyTag::PyTag(const cpp::Tag& tag) : m_tag(tag)
	{
		const auto& name = m_tag.getName();
		m_name = { name.c_str(), name.length() };
		m_repr = m_name; //TODO: Fixme

		for (const auto& arg : m_tag.getArguments())
		{
			switch (arg.getHoldedType())
			{
			case cpp::TagArgumentType::AT_UNDEFINED:
				m_args.append(boost::python::object()); // put None
				break;
			case cpp::TagArgumentType::AT_BOOL:
				m_args.append(arg.asBool(false)); // put bool
				break;
			case cpp::TagArgumentType::AT_FLOAT:
				m_args.append(arg.asFloat(0.f)); // put float
				break;
			case cpp::TagArgumentType::AT_I64:
				m_args.append(arg.asI64(0)); // put i64
				break;
			case cpp::TagArgumentType::AT_STRING:
				m_args.append(arg.asString("")); // put str
				break;
			case cpp::TagArgumentType::AT_TYPEREF:
				m_args.append(arg.asTypeRef({}).getRefName()); // put typeref as string
				break;
			}
		}
	}

	const boost::python::str& PyTag::__str__() const
	{
		return m_name;
	}

	const boost::python::str& PyTag::__repr__() const
	{
		return m_repr;
	}

	bool PyTag::__eq__(const PyTag& another) const
	{
		return m_tag == another.m_tag;
	}

	bool PyTag::__ne__(const PyTag& another) const
	{
		return !__eq__(another);
	}

	const boost::python::str& PyTag::pyGetName() const
	{
		return m_name;
	}

	const boost::python::list& PyTag::pyGetArguments() const
	{
		return m_args;
	}
}