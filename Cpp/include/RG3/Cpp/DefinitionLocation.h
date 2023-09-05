#pragma once

#include <filesystem>
#include <string>


namespace rg3::cpp
{
	class DefinitionLocation
	{
	 public:
		DefinitionLocation();
		DefinitionLocation(const std::filesystem::path& location, int line, int offset);

		[[nodiscard]] const std::filesystem::path& getFsLocation() const;
		[[nodiscard]] std::string getPath() const;
		[[nodiscard]] int getLine() const;
		[[nodiscard]] int getInLineOffset() const;

		bool operator==(const DefinitionLocation& other) const;
		bool operator!=(const DefinitionLocation& other) const;

	 private:
		std::filesystem::path m_fsLocation {};
		int m_line { 0 };
		int m_offset { 0 };
	};
}