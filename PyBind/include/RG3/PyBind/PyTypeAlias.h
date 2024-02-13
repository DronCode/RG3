#pragma once

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>

#include <RG3/Cpp/TypeAlias.h>
#include <RG3/Cpp/DefinitionLocation.h>
#include <RG3/PyBind/PyTypeBase.h>


namespace rg3::pybind
{
	/**
	 * @brief Python representation for cpp::TypeAlias
	 */
	class PyTypeAlias : public PyTypeBase
	{
	 public:
		PyTypeAlias();
		explicit PyTypeAlias(std::unique_ptr<cpp::TypeBase>&& base);

		/// Another stubs
		[[nodiscard]] const cpp::TypeReference& pyGetTargetTypeRef() const;

		[[nodiscard]] boost::python::object pyGetTargetTypeLocation() const;

		[[nodiscard]] const cpp::TypeStatement& getTargetTypeDescription() const;

	 private:
		[[nodiscard]] cpp::TypeBase* getBase();
		[[nodiscard]] const cpp::TypeBase* getBase() const;
	};
}