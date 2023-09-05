#include <RG3/Cpp/Tag.h>


namespace rg3::cpp
{
	TagArgument::TagArgument() = default;

	TagArgument::TagArgument(bool bVal) : m_argType(TagArgumentType::AT_BOOL), m_argValue(bVal)
	{
	}

	TagArgument::TagArgument(float fVal) : m_argType(TagArgumentType::AT_FLOAT), m_argValue(fVal)
	{
	}

	TagArgument::TagArgument(std::int64_t i64v) : m_argType(TagArgumentType::AT_I64), m_argValue(i64v)
	{
	}

	TagArgument::TagArgument(std::string str) : m_argType(TagArgumentType::AT_STRING), m_argValue(std::move(str))
	{
	}

	TagArgument::TagArgument(rg3::cpp::TypeReference tref) : m_argType(TagArgumentType::AT_TYPEREF), m_argValue(tref)
	{
	}

#define RG3_GENERATE_METHOD_IMPL(method, type, ret)         \
	ret TagArgument::method(type defValue) const {         \
    	if (auto casted = std::get_if<typename std::decay_t<type>>(&m_argValue)) { \
            return *casted;                                 \
		}                                                   \
		return defValue;                                    \
	}

	RG3_GENERATE_METHOD_IMPL(asBool, bool, bool)
	RG3_GENERATE_METHOD_IMPL(asFloat, float, float)
	RG3_GENERATE_METHOD_IMPL(asI64, std::int64_t, std::int64_t)
	RG3_GENERATE_METHOD_IMPL(asString, const std::string&, std::string)
	RG3_GENERATE_METHOD_IMPL(asTypeRef, const TypeReference&, TypeReference)

#undef RG3_GENERATE_METHOD_IMPL

	TagArgumentType TagArgument::getHoldedType() const
	{
		return m_argType;
	}

	namespace helper {
		template <typename T> struct TResolver;
		template <> struct TResolver<bool> { static constexpr TagArgumentType g_type = TagArgumentType::AT_BOOL; };
		template <> struct TResolver<float> { static constexpr TagArgumentType g_type = TagArgumentType::AT_FLOAT; };
		template <> struct TResolver<std::string> { static constexpr TagArgumentType g_type = TagArgumentType::AT_STRING; };
		template <> struct TResolver<std::int64_t> { static constexpr TagArgumentType g_type = TagArgumentType::AT_I64; };
		template <> struct TResolver<TypeReference> { static constexpr TagArgumentType g_type = TagArgumentType::AT_TYPEREF; };
	}

#define RG3_GENERATE_ASSIGN_OPERATOR_IMPL(type)      \
	TagArgument& TagArgument::operator=(type v) {    \
		m_argType = helper::TResolver<type>::g_type; \
		m_argValue = v;                              \
		return *this;                                \
	}

	RG3_GENERATE_ASSIGN_OPERATOR_IMPL(bool)
	RG3_GENERATE_ASSIGN_OPERATOR_IMPL(float)
	RG3_GENERATE_ASSIGN_OPERATOR_IMPL(std::int64_t)
	RG3_GENERATE_ASSIGN_OPERATOR_IMPL(std::string)
	RG3_GENERATE_ASSIGN_OPERATOR_IMPL(TypeReference)

#undef RG3_GENERATE_ASSIGN_OPERATOR_IMPL

	bool TagArgument::operator==(const TagArgument& other) const
	{
		return m_argType == other.m_argType && m_argValue == other.m_argValue;
	}

	bool TagArgument::operator!=(const TagArgument& other) const
	{
		return !operator==(other);
	}

	TagArgument::operator bool() const noexcept
	{
		return m_argType != TagArgumentType::AT_UNDEFINED;
	}
}