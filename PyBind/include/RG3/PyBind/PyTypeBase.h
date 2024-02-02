#pragma once

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>

#include <RG3/Cpp/TypeBase.h>


namespace rg3::pybind
{
	/**
	 * @brief Python representation of cpp::TypeBase. See bindings in PyBind.cpp
	 */
	class PyTypeBase : public boost::noncopyable
	{
	 public:
		PyTypeBase();
		explicit PyTypeBase(cpp::TypeBasePtr&& base);

		/// Python Magic
		[[nodiscard]] const boost::python::str& __str__() const;
		[[nodiscard]] const boost::python::str& __repr__() const;
		[[nodiscard]] bool __eq__(const PyTypeBase& another) const;
		[[nodiscard]] bool __ne__(const PyTypeBase& another) const;
		[[nodiscard]] std::uint64_t __hash__() const;

		/// Methods
		[[nodiscard]] const rg3::cpp::Tags& pyGetTags() const;
		[[nodiscard]] cpp::TypeKind pyGetTypeKind() const;
		[[nodiscard]] boost::python::str pyGetName() const;
		[[nodiscard]] boost::python::str pyGetPrettyName() const;
		[[nodiscard]] const cpp::CppNamespace& pyGetNamespace() const;
		[[nodiscard]] const cpp::DefinitionLocation& pyGetLocation() const;

	 protected:
		boost::shared_ptr<cpp::TypeBase> m_base { nullptr };
		boost::python::str m_str { "null" };
		boost::python::str m_repr { "null" };
	};
}