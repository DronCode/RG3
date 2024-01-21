#pragma once

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>


namespace rg3::pybind
{
	struct PyClangRuntime
	{
		static boost::python::str getRuntimeInfo();
	};
}