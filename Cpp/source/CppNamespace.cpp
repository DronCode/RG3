#include <RG3/Cpp/CppNamespace.h>
#include <algorithm>
#include <utility>


namespace rg3::cpp
{
	static const std::string kNamespaceDelimiter = "::";

    CppNamespace::CppNamespace() = default;

	CppNamespace::CppNamespace(std::string aNamespace)
		: m_sNamespace(std::move(aNamespace))
	{
		parseNamespace();
	}

	const std::string& CppNamespace::operator[](size_t i) const
	{
		return m_vNamespace.at(i);
	}

	std::string& CppNamespace::operator[](size_t i)
	{
		return m_vNamespace.at(i);
	}

	CppNamespace& CppNamespace::operator=(const rg3::cpp::CppNamespace& copy)
	{
		m_sNamespace = copy.m_sNamespace;
		m_vNamespace = copy.m_vNamespace;

		return *this;
	}

	CppNamespace& CppNamespace::operator/(const std::string& part)
	{
		if (!m_sNamespace.empty())
			m_sNamespace.append("::");

		m_sNamespace.append(part);
		m_vNamespace.push_back(part);

		return *this;
	}

	bool CppNamespace::operator==(const CppNamespace& other) const
	{
		return m_sNamespace == other.m_sNamespace;
	}

	bool CppNamespace::operator!=(const CppNamespace& other) const
	{
		return m_sNamespace != other.m_sNamespace;
	}

	void CppNamespace::prepend(const std::string& part)
	{
		m_sNamespace = (m_sNamespace.empty() ? part : part + "::" + m_sNamespace);
		m_vNamespace.insert(m_vNamespace.begin(), part);
	}

	void CppNamespace::parseNamespace()
	{
		m_vNamespace.clear();

		size_t pos = 0;
		std::string s = m_sNamespace;
		std::string tok;

		while ((pos = s.find(kNamespaceDelimiter)) != std::string::npos)
		{
			tok = s.substr(0, pos);
#ifdef __APPLE__ // A special fix for macOS (their sdk contains __1 part)
			if (tok != "__1")
#endif
			{
				m_vNamespace.push_back(tok);
			}
			s.erase(0, pos + kNamespaceDelimiter.length());
		}
	}
}