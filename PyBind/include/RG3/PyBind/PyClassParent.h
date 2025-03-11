#pragma once

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>

#include <RG3/Cpp/TypeClass.h>
#include <RG3/PyBind/PyTypeClass.h>


namespace rg3::pybind
{
	/**
	 * @brief Python representation of cpp::ClassParent
	 */
	class PyClassParent : public boost::noncopyable
	{
	 public:
		explicit PyClassParent(const cpp::ClassParent& pParent);

		/// Python Magic
		[[nodiscard]] const boost::python::str& __str__() const;
		[[nodiscard]] const boost::python::str& __repr__() const;

		/// Methods
		[[nodiscard]] cpp::InheritanceVisibility getInheritanceKind() const;
		[[nodiscard]] const cpp::Tags& getTags() const;
		[[nodiscard]] const cpp::TypeBaseInfo& getBaseInfo() const;
		[[nodiscard]] const boost::shared_ptr<PyTypeClass>& getClassType() const;

	 public: // Resolver only
		void setParentClassDataReference(const boost::shared_ptr<PyTypeClass>& pPyClass);

	 protected:
		cpp::ClassParent m_sClassParent {};
		boost::shared_ptr<PyTypeClass> m_pClassType { nullptr };
		boost::python::str m_str { "null" };
		boost::python::str m_repr { "null" };
	};
}