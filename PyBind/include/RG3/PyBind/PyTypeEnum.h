#pragma once

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>

#include <RG3/Cpp/TypeEnum.h>
#include <RG3/PyBind/PyTypeBase.h>


namespace rg3::pybind
{
	/**
	 * @brief Python representation of cpp::TypeEnum. See bindings in PyBind.cpp
	 */
	class PyTypeEnum : public PyTypeBase
	{
	 public:
		PyTypeEnum();
		explicit PyTypeEnum(std::unique_ptr<cpp::TypeBase>&& base);

		/// Self methods
		[[nodiscard]] const boost::python::list& pyGetEnumEntries() const;
		[[nodiscard]] bool pyIsScoped() const;
		[[nodiscard]] const boost::python::str& pyGetUnderlyingTypeStr() const;

	 private:
		cpp::TypeEnum* getBase();
		const cpp::TypeEnum* getBase() const;

	 private:
		boost::python::list m_entries;
		boost::python::str m_underlyingType;
	};
}