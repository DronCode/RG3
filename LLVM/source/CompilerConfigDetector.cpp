#include <RG3/LLVM/CompilerConfigDetector.h>

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>



namespace rg3::llvm
{
	/**
	 * @brief Run abstract command sequence. For Windows - Powershell, for Linux/MacOS - /bin/bash
	 * @param args - arguments for system shell
	 * @return command output
	 */
	std::string runShellCommand(const boost::filesystem::path& clangPath, const std::vector<std::string>& args)
	{
		// Try to invoke clang
		namespace bp = boost::process;

		bp::ipstream is;
		std::string line, response {};

		bp::child c {
			bp::search_path("powershell.exe"), args, // TODO: use ifdef
			bp::start_dir(clangPath.parent_path()),
			bp::std_out > is,
			bp::std_err > is
		};

		while (c.running() && std::getline(is, line))
		{
			if (line.empty())
				continue;

			while (!line.empty() && line.back() == '\r')
				line.pop_back();

			response.append(line);
			response.push_back('\n');
		}

		c.wait();
		return response;
	}

	std::optional<CompilerEnvError> parseClangOutput(CompilerEnvironment& env, const std::string& response)
	{
		// Here we need to parse output
		// We have 2 blocks:
		//		#include "..." search starts here:
		//		#include <...> search starts here:
		// 		End of search list.
		constexpr std::string_view sSecondSegmentTemplate = "#include <...> search starts here:";
		constexpr std::string_view sEndSegmentTemplate = "End of search list.";

		const auto includesBlockSegment = response.find(sSecondSegmentTemplate);
		if (includesBlockSegment == std::string::npos)
			return CompilerEnvError{ "Failed to find segment '#include <...> search starts here:'" , CompilerEnvError::ErrorKind::EK_BAD_CLANG_OUTPUT }; // Bad format

		const auto endOfLookup = response.find(sEndSegmentTemplate, includesBlockSegment);
		if (endOfLookup == std::string::npos)
			return CompilerEnvError{ "Failed to find segment 'End of search list:'" , CompilerEnvError::ErrorKind::EK_BAD_CLANG_OUTPUT }; // Bad format

		// Take substring between first and second block
		const auto startPoint = includesBlockSegment + sSecondSegmentTemplate.length() + 1;
		std::string includesBlockSegmentContent = response.substr(startPoint, endOfLookup - startPoint - 1);

		// Split by '\n', remove first ' '
		std::vector<std::string> includesList {};

		boost::split(includesList, includesBlockSegmentContent, boost::is_any_of("\n"));

		bool bAddedAtLeastOneEntry = false;

		for (auto it = includesList.begin(); it != includesList.end(); )
		{
			while (!(*it).empty() && ( *it).front() == ' ')
				(*it).erase(0, 1); // remove first space

			if ((*it).empty())
			{
				it = includesList.erase(it);
				continue;
			}

			// And insert result
			rg3::llvm::IncludeInfo& ii = env.config.vSystemIncludes.emplace_back();

			ii.sFsLocation = std::filesystem::path { (*it) };
			ii.eKind = rg3::llvm::IncludeKind::IK_SYSTEM;
			bAddedAtLeastOneEntry = true;

			++it;
		}

		if (!bAddedAtLeastOneEntry)
			return CompilerEnvError{ "System includes not found", CompilerEnvError::ErrorKind::EK_NO_SYSTEM_INCLUDE_DIRS_FOUND };

		// now let's try to find triple, on Windows it looks like -triple x86_64-pc-windows-msvc19.35.32217
		constexpr std::string_view sTripleDecl = "-triple ";
		auto tripleIt = response.find(sTripleDecl);
		if (tripleIt != std::string::npos)
		{
			// Ok, it's defined
			auto tripleDeclEndIt = response.find(' ', tripleIt + sTripleDecl.length());
			if (tripleDeclEndIt != std::string::npos)
			{
				env.triple = response.substr(tripleIt + sTripleDecl.length(), tripleDeclEndIt - (tripleIt + sTripleDecl.length()));
			}
		}

		// End
		return std::nullopt;
	}

	CompilerEnvResult CompilerConfigDetector::detectSystemCompilerEnvironment()
	{
#if defined(__linux__) || defined(__APPLE__)
		// On macOS we need to find xcrun instance and then find clang
		// Smth like "xcrun", "--find", "clang++", nullptr
		// Also, we need to find all 'frameworks' (macOS specific things)

		// On linux we need to find gcc instance and use it like
		// "gcc", "-Wp,-v", "-x", "c++", "/dev/null", "-fsyntax-only"

		//Support it later!
		return CompilerEnvError { "Current OS not supported yet. See README.md for details!", CompilerEnvError::ErrorKind::EK_UNSUPPORTED_OS };
#endif

		// Detect where is located clang++.exe via PATH
		const auto selfENV = boost::this_process::environment();

		std::vector<boost::process::filesystem::path> pathLocations;
		for (const auto& i : selfENV)
		{
			if (const auto& name = i.get_name(); name == "Path" || name == "PATH")
			{
				// Found PATH or Path, need to use it to locate clang++ location
				const auto locationsAsStr = i.to_vector();
				pathLocations.reserve(locationsAsStr.size());

				for (const auto& locationAsStr : locationsAsStr)
				{
					pathLocations.emplace_back(locationAsStr);
				}
				break;
			}
		}

		// No PATH found (wtf?)
		if (pathLocations.empty())
			return CompilerEnvError { "PATH or Path system environment variable not found", CompilerEnvError::ErrorKind::EK_NO_PATHS };

		// Trying to locate clang++ via PATH
		const boost::filesystem::path clangLocation = boost::process::search_path("clang++", pathLocations);
		if (clangLocation.empty())
			return CompilerEnvError { "Failed to locate clang++ instance. Please, make sure that your os contains it. (MacOS: check that xcrun available and configured properly!)", CompilerEnvError::ErrorKind::EK_NO_CLANG_INSTANCE };

		const std::string response = runShellCommand(clangLocation, { "''", "|", "clang++.exe", "-x", "c++-header", "-v", "-E", "-" });

		if (response.empty())
			return CompilerEnvError { "Failed to invoke clang++ process", CompilerEnvError::ErrorKind::EK_BAD_CLANG_OUTPUT };

		CompilerEnvironment compilerEnvironment {};
		if (auto parseResult = parseClangOutput(compilerEnvironment, response); parseResult.has_value())
		{
			// report an error
			return parseResult.value();
		}

		// Everything is ok, return env
		return compilerEnvironment;
	}
}