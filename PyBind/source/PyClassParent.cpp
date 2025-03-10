#include <RG3/PyBind/PyClassParent.h>
#include <fmt/format.h>


namespace rg3::pybind
{
	static std::string DefLocation2Str(const cpp::DefinitionLocation& sDefLoc)
	{
		return fmt::format("(Path={}, Line={}, Column={}, Angled={})", sDefLoc.getPath(), sDefLoc.getLine(), sDefLoc.getInLineOffset(), sDefLoc.isAngledPath() ? "True": "False");
	}

	PyClassParent::PyClassParent(const cpp::ClassParent& sParent) : m_sClassParent(sParent)
	{
		auto repr = fmt::format("ClassParent(Name={}, Namespace={}, Location={})",
								m_sClassParent.sTypeBaseInfo.sPrettyName,
								m_sClassParent.sTypeBaseInfo.sNameSpace.asString(),
								DefLocation2Str(m_sClassParent.sTypeBaseInfo.sDefLocation));

		m_repr = boost::python::str(repr.c_str());
		m_str = boost::python::str(m_sClassParent.sTypeBaseInfo.sPrettyName.c_str());

		if (m_sClassParent.pDeepType)
		{
			auto pSource = std::dynamic_pointer_cast<cpp::TypeClass>(m_sClassParent.pDeepType);
			if (pSource)
			{
				std::unique_ptr<cpp::TypeClass> pClassData = std::make_unique<cpp::TypeClass>(
					pSource->getName(), pSource->getPrettyName(), pSource->getNamespace(), pSource->getDefinition(), pSource->getTags(),
					pSource->getProperties(), pSource->getFunctions(), pSource->getClassFriends(),
					pSource->isStruct(), pSource->isTrivialConstructible(), pSource->hasCopyConstructor(), pSource->hasCopyAssignOperator(), pSource->hasMoveConstructor(), pSource->hasMoveAssignOperator(),
					pSource->getParentTypes()
				);

				m_pClassType = boost::shared_ptr<PyTypeClass>(new PyTypeClass(std::move(pClassData)));
			}
		}
	}

	const boost::python::str& PyClassParent::__str__() const
	{
		return m_str;
	}

	const boost::python::str& PyClassParent::__repr__() const
	{
		return m_repr;
	}

	cpp::InheritanceVisibility PyClassParent::getInheritanceKind() const
	{
		return m_sClassParent.eModifier;
	}

	const cpp::Tags& PyClassParent::getTags() const
	{
		return m_sClassParent.vTags;
	}

	const cpp::TypeBaseInfo& PyClassParent::getBaseInfo() const
	{
		return m_sClassParent.sTypeBaseInfo;
	}

	const boost::shared_ptr<PyTypeClass>& PyClassParent::getClassType() const
	{
		return m_pClassType;
	}
}