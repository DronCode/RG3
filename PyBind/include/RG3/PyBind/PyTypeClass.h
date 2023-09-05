#pragma once

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>

#include <RG3/Cpp/TypeClass.h>
#include <RG3/PyBind/PyTypeBase.h>


namespace rg3::pybind
{
	/**
	 * @brief Python representation of cpp::TypeEnum. See bindings in PyBind.cpp
	 */
	class PyTypeClass : public PyTypeBase
	{
	 public:
		PyTypeClass();
		explicit PyTypeClass(std::unique_ptr<cpp::TypeBase>&& base);

		/// Another implementations
		const boost::python::list& pyGetClassProperties() const;
		const boost::python::list& pyGetClassFunctions() const;
		const boost::python::list& pyGetClassParentsTypeNamesList() const;
		bool pyIsStruct() const;
		bool pyIsTriviallyConstructible() const;

	 private:
		[[nodiscard]] cpp::TypeClass* getBase();
	 	[[nodiscard]] const cpp::TypeClass* getBase() const;

	 private:
		boost::python::list m_properties {};
		boost::python::list m_functions {};
		boost::python::list m_parents {};
	};
}