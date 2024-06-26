#include <RG3/Cpp/DefinitionLocation.h>


namespace rg3::cpp
{
	DefinitionLocation::DefinitionLocation() = default;
	DefinitionLocation::DefinitionLocation(const std::filesystem::path& location, int line, int offset, bool bAngled)
		: m_fsLocation(location)
		, m_line(line)
		, m_offset(offset)
		, m_bAngled(bAngled)
	{
	}

	const std::filesystem::path& DefinitionLocation::getFsLocation() const
	{
		return m_fsLocation;
	}

	std::string DefinitionLocation::getPath() const
	{
		return m_fsLocation.string();
	}

	int DefinitionLocation::getLine() const
	{
		return m_line;
	}

	int DefinitionLocation::getInLineOffset() const
	{
		return m_offset;
	}

	bool DefinitionLocation::isAngledPath() const
	{
		return m_bAngled;
	}

	bool DefinitionLocation::operator==(const DefinitionLocation& other) const
	{
		return m_fsLocation == other.m_fsLocation && m_line == other.m_line && m_offset == other.m_offset && m_bAngled == other.m_bAngled;
	}

	bool DefinitionLocation::operator!=(const DefinitionLocation& other) const
	{
		return !operator==(other);
	}
}