#include <RG3/Cpp/Tag.h>
#include <utility>
#include <sstream>
#include <regex>


namespace rg3::cpp
{
	Tag::Tag() = default;

	Tag::Tag(const std::string& name) : m_name(name)
	{
	}

	Tag::Tag(const std::string& name, const std::vector<TagArgument>& arguments)
		: m_name(name), m_arguments(arguments)
	{
	}

	const std::string& Tag::getName() const
	{
		return m_name;
	}

	const std::vector<TagArgument>& Tag::getArguments() const
	{
		return m_arguments;
	}

	std::vector<TagArgument>& Tag::getArguments()
	{
		return m_arguments;
	}

	bool Tag::hasArguments() const
	{
		return !m_arguments.empty();
	}

	int Tag::getArgumentsCount() const
	{
		return static_cast<int>(m_arguments.size());
	}

	bool Tag::operator==(const Tag& other) const
	{
		return m_name == other.m_name && m_arguments == other.m_arguments;
	}

	bool Tag::operator!=(const Tag& other) const
	{
		return !operator==(other);
	}

	Tags Tag::parseFromCommentString(std::string_view commentText)
	{
		std::vector<Tag> tags;
		std::regex tagRegex("@([a-zA-Z_.]+)(\\([^)]*\\))?");
		std::smatch tagMatch;

		auto commentBegin = std::cregex_iterator(commentText.data(), commentText.data() + commentText.size(), tagRegex);
		auto commentEnd = std::cregex_iterator();
		for (std::cregex_iterator regexIterator = commentBegin; regexIterator != commentEnd; ++regexIterator)
		{
			auto tagName = std::string((*regexIterator)[1]);
			auto unprocessedArguments = std::string((*regexIterator)[2]);

			std::vector<TagArgument> tagArguments {};

			if (tagName.empty())
				continue;

			if (!unprocessedArguments.empty())
			{
				// Parse args & fill tags
				unprocessedArguments.erase(std::remove(unprocessedArguments.begin(), unprocessedArguments.end(), '('), unprocessedArguments.end());
				unprocessedArguments.erase(std::remove(unprocessedArguments.begin(), unprocessedArguments.end(), ')'), unprocessedArguments.end());

				// Split arguments string by commas
				std::vector<std::string> argumentStrings;
				std::istringstream argumentsStream(unprocessedArguments);

				{
					std::string argumentString;

					while (std::getline(argumentsStream, argumentString, ','))
					{
						argumentStrings.push_back(argumentString);
					}
				}

				for (auto& argumentString : argumentStrings)
				{
					//NOTE: Make this code more optimized :thinking:
					// Remove trailing spaces
					while (!argumentString.empty() && argumentString[0] == ' ')
						argumentString.erase(0, 1);

					if (argumentString.empty())
						continue;

					// Try to interpret as int64
					try
					{
						size_t pos;
						int64_t intValue = std::stoll(argumentString, &pos);
						if (pos == argumentString.size())
						{
							tagArguments.emplace_back(intValue);
							continue;
						}
					} catch (const std::invalid_argument&) {
						// Argument is not an int64_t
					}

					// Try to parse argument as float
					try {
						size_t pos;
						float floatValue =std::stof(argumentString, &pos);
						if (pos == argumentString.size()) {
							tagArguments.emplace_back(floatValue);
							continue;
						}
					} catch (const std::invalid_argument&) {
						// Argument is not a float
					}

					// Try to parse argument as bool
					if (argumentString == "true") {
						tagArguments.emplace_back(true);
						continue;
					} else if (argumentString == "false") {
						tagArguments.emplace_back(false);
						continue;
					}

					if (argumentString[0] == '@')
					{
						// Type reference
						tagArguments.emplace_back(TypeReference(argumentString.substr(1)));
						continue;
					}

					// Unknown case, save as string
					tagArguments.emplace_back(argumentString);
				}
			}

			tags.emplace_back(std::move(tagName), std::move(tagArguments));
		}

		return Tags(std::move(tags));
	}

	Tags::Tags() = default;

	Tags::Tags(const std::vector<Tag>& tags)
	{
		for (const auto& tag : tags)
		{
			m_tags[tag.getName()] = tag;
		}
	}

	bool Tags::hasTag(const std::string& tag) const
	{
		return m_tags.contains(tag);
	}

	Tag Tags::getTag(const std::string& tag) const
	{
		static Tag g_BadTag;

		if (auto it = m_tags.find(tag); it != m_tags.end())
		{
			return it->second;
		}

		return g_BadTag;
	}

	const Tags::Storage_t& Tags::getTags() const
	{
		return m_tags;
	}

	Tags::Storage_t& Tags::getTags()
	{
		return m_tags;
	}
}