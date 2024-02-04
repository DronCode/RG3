#include <RG3/LLVM/CompilerConfigDetector.h>

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>



namespace rg3::llvm
{
	/**
	 * @brief Run abstract command sequence. For Windows - Powershell, for Linux - /usr/bin/bash, for macOS - idk
	 * @param args - arguments for system shell
	 * @return command output
	 */
	std::string runShellCommand(const boost::filesystem::path& compilerPath, const std::vector<std::string>& args)
	{
		namespace bp = boost::process;

		std::string response {};
#if defined(_WIN32)
		// Windows specific impl
		bp::ipstream is;
		std::string line {};

		bp::child c {
			bp::search_path("powershell.exe"),
			args,
			bp::start_dir(compilerPath.parent_path()),
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
#elif defined(__linux__)
		// Linux specific impl
		// This impl works directly with compiler instance and working with it.
		// Bad moment is all output from clang going through stderr. It looks weird, need to think how to fix that.
		bp::ipstream stdErr, stdOut;
		std::string stdErrLine, stdOutLine, stdErrResponse, stdOutResponse {};
		std::vector<std::string> compilerArgs {};

		if (args.size() > 1)
		{
			for (size_t i = 1; i < args.size(); i++)
			{
				compilerArgs.emplace_back(args[i]);
			}
		}

		bp::child c {
			compilerPath,
			compilerArgs,
			bp::std_err > stdErr,
			bp::std_out > stdOut
		};

		bool bCanRead = false;
		do
		{
			auto storeLine = [](std::string& line, std::string& out) {
				while (!line.empty() && line.back() == '\r')
					line.pop_back();

				out.append(line);
				out.push_back('\n');
			};

			const bool bHasStdErr = static_cast<bool>(std::getline(stdErr, stdErrLine));
			const bool bHasStdOut = static_cast<bool>(std::getline(stdOut, stdOutLine));

			if (!stdErrLine.empty())
			{
				storeLine(stdErrLine, stdErrResponse);
			}

			if (!stdOutLine.empty())
			{
				storeLine(stdOutLine, stdOutResponse);
			}

			bCanRead = c.running() || bHasStdErr || bHasStdOut;
		} while (bCanRead);

		c.wait();
		response.append(stdOutResponse);
		response.append(stdErrResponse);
#else
#		error Unsupported
#endif

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
#if defined(__APPLE__)
		// On macOS we need to find xcrun instance and then find clang
		// Smth like "xcrun", "--find", "clang++", nullptr
		// Also, we need to find all 'frameworks' (macOS specific things)

		//Support it later!
		return CompilerEnvError { "Current OS not supported yet. See README.md for details!", CompilerEnvError::ErrorKind::EK_UNSUPPORTED_OS };
#endif

#if defined(_WIN32)
		constexpr const char* kCompilerInstanceExecutable = "clang++.exe";
#elif defined(__linux__)
		constexpr const char* kCompilerInstanceExecutable = "g++";
#elif defined(__APPLE__)
		constexpr const char* kCompilerInstanceExecutable = "clang++";
#endif

		// Detect where is located clang++.exe via PATH
		const auto selfENV = boost::this_process::environment();

		std::vector<boost::filesystem::path> pathLocations;
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
		{
			return CompilerEnvError { "PATH or Path system environment variable not found", CompilerEnvError::ErrorKind::EK_NO_PATHS };
		}

		// Trying to locate clang++ via PATH
		const boost::filesystem::path compilerLocation = boost::process::search_path(kCompilerInstanceExecutable, pathLocations);
		if (compilerLocation.empty())
		{
			return CompilerEnvError {
				std::format("Failed to locate {} instance. Please, make sure that your os contains it. (MacOS: check that xcrun available and configured properly!)", kCompilerInstanceExecutable),
				CompilerEnvError::ErrorKind::EK_NO_CLANG_INSTANCE
			};
		}

#if defined(_WIN32)
		const std::string response = runShellCommand(compilerLocation, { "''", "|", kCompilerInstanceExecutable, "-x", "c++-header", "-v", "-E", "-" });
#elif defined(__linux__)
		const std::string response = runShellCommand(compilerLocation, { kCompilerInstanceExecutable, "-x", "c++-header", "/dev/null", "-v", "-E" });
#elif defined(__APPLE__)
		const std::string response = runShellCommand(compilerLocation, { kCompilerInstanceExecutable, "-x", "c++-header", "/dev/null", "-v", "-E" });
#else
#		error Unsupported
#endif

		if (response.empty())
		{
			return CompilerEnvError { std::format("Failed to invoke '{}' executable", kCompilerInstanceExecutable), CompilerEnvError::ErrorKind::EK_BAD_CLANG_OUTPUT };
		}

		CompilerEnvironment compilerEnvironment {};
		if (auto parseResult = parseClangOutput(compilerEnvironment, response); parseResult.has_value())
		{
			// report an error
			return parseResult.value();
		}

		if (compilerEnvironment.triple.empty())
		{
#if defined(__linux__)
			// Try to find triple by another way on linux with gcc
			constexpr std::string_view sTargetDecl = "--target=";
			if (auto it = response.find(sTargetDecl.data()); it != std::string::npos)
			{
				auto nextSpaceIt = response.find(' ', it);
				const int predictedLength = std::min(static_cast<int>(response.length() - it), static_cast<int>(nextSpaceIt  - (it + sTargetDecl.length())));
				compilerEnvironment.triple = response.substr(it + sTargetDecl.length(), predictedLength);
			}
#endif
		}

		// Everything is ok, return env
		return compilerEnvironment;
	}
}