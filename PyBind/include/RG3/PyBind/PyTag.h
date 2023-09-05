#pragma once

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>

#include <RG3/Cpp/Tag.h>


namespace rg3::pybind
{
	class PyTag
	{
	 public:
		PyTag();
		explicit PyTag(const cpp::Tag& tag);

		/// Python magic
		const boost::python::str& __str__() const;
		const boost::python::str& __repr__() const;
		bool __eq__(const PyTag& another) const;
		bool __ne__(const PyTag& another) const;

		/// Self methods
		const boost::python::str& pyGetName() const;
		const boost::python::list& pyGetArguments() const;

	 private:
		cpp::Tag m_tag {};
		boost::python::str m_name;
		boost::python::str m_repr;
		boost::python::list m_args;
	};
}