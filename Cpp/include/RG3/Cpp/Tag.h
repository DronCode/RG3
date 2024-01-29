#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <variant>

#include <RG3/Cpp/TypeReference.h>


namespace rg3::cpp
{
	enum class TagArgumentType
	{
		AT_UNDEFINED,
		AT_BOOL,
		AT_FLOAT,
		AT_I64,
		AT_STRING,
		AT_TYPEREF
	};

	class TagArgument
	{
	 public:
		TagArgument();
		explicit TagArgument(bool bVal);
		explicit TagArgument(float fVal);
		explicit TagArgument(std::int64_t i64v);
		explicit TagArgument(std::string str);
		explicit TagArgument(TypeReference tref);

		bool asBool(bool defValue) const;
		float asFloat(float defValue) const;
		std::int64_t asI64(std::int64_t defValue) const;
		std::string asString(const std::string& defValue) const;
		TypeReference asTypeRef(const TypeReference& defValue) const;
		TagArgumentType getHoldedType() const;

		TagArgument& operator=(bool val);
		TagArgument& operator=(float val);
		TagArgument& operator=(std::int64_t val);
		TagArgument& operator=(std::string val);
		TagArgument& operator=(TypeReference val);

		bool operator==(const TagArgument& other) const;
		bool operator!=(const TagArgument& other) const;
		explicit operator bool() const noexcept;

	 private:
		TagArgumentType m_argType { TagArgumentType::AT_UNDEFINED };
		std::variant<bool, float, std::int64_t, std::string, TypeReference> m_argValue {};
	};

	class Tags;

	class Tag
	{
	 public:
		Tag();
		explicit Tag(const std::string& name);
		Tag(const std::string& name, const std::vector<TagArgument>& arguments);

		[[nodiscard]] const std::string& getName() const;
		[[nodiscard]] const std::vector<TagArgument>& getArguments() const;
		[[nodiscard]] bool hasArguments() const;
		[[nodiscard]] int getArgumentsCount() const;

		static Tags parseFromCommentString(std::string_view commentString);

		bool operator==(const Tag& other) const;
		bool operator!=(const Tag& other) const;

	 private:
		std::string m_name;
		std::vector<TagArgument> m_arguments;
	};

	class Tags
	{
	 private:
		std::map<std::string, Tag> m_tags;

	 public:
		Tags();
		explicit Tags(const std::vector<Tag>& tags);

		[[nodiscard]] bool hasTag(const std::string& tag) const;
		[[nodiscard]] Tag getTag(const std::string& tag) const;
		[[nodiscard]] const std::map<std::string, Tag>& getTags() const;
	};
}