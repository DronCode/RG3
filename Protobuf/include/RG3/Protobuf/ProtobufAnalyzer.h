#pragma once

#include <RG3/Protobuf/Compiler.h>

#include <boost/noncopyable.hpp>

#include <filesystem>
#include <variant>
#include <string>


namespace rg3::pb
{
	/**
	 * @brief This class implements a single threaded analyzer of protobuf code.
	 */
	class ProtobufAnalyzer final : public boost::noncopyable
	{
	 public:
		using CodeSource = std::variant<std::string, std::filesystem::path>; // string is a code repr in memory (id0.proto), filesystem::path for FS path

		ProtobufAnalyzer();

		void setCode(const std::string& sCode);
		void setFile(const std::filesystem::path& sPath);
		void setSource(const CodeSource& src);

		void setCompilerConfig(const CompilerConfig& sConfig);
		const CompilerConfig& getCompilerConfig() const;
		CompilerConfig& getCompilerConfig();

		[[nodiscard]] const CompilerIssuesVector& getIssues() const;

		bool analyze();

	 private:
		CodeSource m_sSource {};
		CompilerConfig m_sConfig {};
		CompilerIssuesVector m_aIssues {};
	};
}