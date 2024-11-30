#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/LLVM/CodeEvaluator.h>


class Tests_CodeEvaluator : public ::testing::Test
{
 protected:
	void SetUp() override
	{
		g_Eval = std::make_unique<rg3::llvm::CodeEvaluator>();
		g_Eval->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;  //23 std raise exceptions on macOS!
		g_Eval->getCompilerConfig().bSkipFunctionBodies = true;
	}

	void TearDown() override
	{
		g_Eval = nullptr;
	}

 protected:
	std::unique_ptr<rg3::llvm::CodeEvaluator> g_Eval { nullptr };
};


TEST_F(Tests_CodeEvaluator, SimpleUsage)
{
	auto res = g_Eval->evaluateCode("static constexpr int aResult = 32 * 2;", { "aResult" });
	ASSERT_TRUE(res.vIssues.empty()) << "No issues expected to be here";
	ASSERT_EQ(res.mOutputs.size(), 1) << "Expected to have 1 output";
	ASSERT_TRUE(res.mOutputs.contains("aResult")) << "aResult should be here";
	ASSERT_TRUE(std::holds_alternative<std::int64_t>(res.mOutputs["aResult"]));
	ASSERT_EQ(std::get<std::int64_t>(res.mOutputs["aResult"]), 64) << "Must be 64!";
}

TEST_F(Tests_CodeEvaluator, ClassInheritanceConstexprTest)
{
	auto res = g_Eval->evaluateCode(R"(
#include <type_traits>

struct MyCoolBaseClass {};

struct MyDniweClass {};
struct MyBuddyClass : MyCoolBaseClass {};

constexpr bool g_bDniweInherited = std::is_base_of_v<MyCoolBaseClass, MyDniweClass>;
constexpr bool g_bBuddyInherited = std::is_base_of_v<MyCoolBaseClass, MyBuddyClass>;
)", { "g_bDniweInherited", "g_bBuddyInherited" });

	ASSERT_TRUE(res.vIssues.empty()) << "No issues expected to be here";
	ASSERT_EQ(res.mOutputs.size(), 2) << "Expected to have 2 outputs";
	ASSERT_TRUE(res.mOutputs.contains("g_bDniweInherited")) << "g_bDniweInherited should be here";
	ASSERT_TRUE(res.mOutputs.contains("g_bBuddyInherited")) << "g_bBuddyInherited should be here";
	ASSERT_TRUE(std::get<bool>(res.mOutputs["g_bBuddyInherited"])) << "Buddy should be ok!";
	ASSERT_FALSE(std::get<bool>(res.mOutputs["g_bDniweInherited"])) << "Dniwe should be bad!";
}

constexpr int fibonacci(int n)
{
	return n < 1 ? -1 : (n == 1 || n == 2 ? 1 : fibonacci(n - 1) + fibonacci(n - 2));
}

TEST_F(Tests_CodeEvaluator, FibonacciConstexprEvalTest)
{
	auto res = g_Eval->evaluateCode(R"(
constexpr int fibonacci(int n)
{
    return n < 1 ? -1 : (n == 1 || n == 2 ? 1 : fibonacci(n - 1) + fibonacci(n - 2));
}

constexpr auto g_iFib9 = fibonacci(9);
constexpr auto g_iFib10 = fibonacci(10);
constexpr auto g_iFib16 = fibonacci(16);
)", { "g_iFib9", "g_iFib10", "g_iFib16" });

	ASSERT_TRUE(res.vIssues.empty()) << "No issues expected to be here";
	ASSERT_EQ(res.mOutputs.size(), 3) << "Expected to have 3 outputs";
	ASSERT_TRUE(res.mOutputs.contains("g_iFib9")) << "g_iFib9 should be here";
	ASSERT_TRUE(res.mOutputs.contains("g_iFib10")) << "g_iFib10 should be here";
	ASSERT_TRUE(res.mOutputs.contains("g_iFib16")) << "g_iFib16 should be here";
	ASSERT_EQ(std::get<std::int64_t>(res.mOutputs["g_iFib9"]), fibonacci(9)) << "FIB(9) FAILED";
	ASSERT_EQ(std::get<std::int64_t>(res.mOutputs["g_iFib10"]), fibonacci(10)) << "FIB(10) FAILED";
	ASSERT_EQ(std::get<std::int64_t>(res.mOutputs["g_iFib16"]), fibonacci(16)) << "FIB(16) FAILED";
}

TEST_F(Tests_CodeEvaluator, SampleString)
{
	auto res = g_Eval->evaluateCode(R"(
#ifdef __RG3_CODE_EVAL__
constexpr const char* psFileID = __FILE__;
#else
#	error "What the hell is going on here???"
#endif
)", { "psFileID" });

	ASSERT_TRUE(res.vIssues.empty()) << "No issues expected to be here";
	ASSERT_TRUE(res.mOutputs.contains("psFileID"));
	ASSERT_EQ(std::get<std::string>(res.mOutputs["psFileID"]), "id0.hpp");
}

constexpr std::uint32_t FNV1aPrime = 16777619u;
constexpr std::uint32_t FNV1aOffsetBasis = 2166136261u;

constexpr std::uint32_t fnv1aHash(const char* str, std::size_t length, std::uint32_t hash = FNV1aOffsetBasis) {
	return (length == 0)
			   ? hash
			   : fnv1aHash(str + 1, length - 1, (hash ^ static_cast<std::uint8_t>(*str)) * FNV1aPrime);
}

constexpr std::uint32_t fnv1aHash(const char* str) {
	std::size_t length = 0;
	while (str[length] != '\0') ++length;
	return fnv1aHash(str, length);
}

TEST_F(Tests_CodeEvaluator, HashComputeExample)
{
	auto res = g_Eval->evaluateCode(R"(
#include <cstddef>
#include <cstdint>

constexpr std::uint32_t FNV1aPrime = 16777619u;
constexpr std::uint32_t FNV1aOffsetBasis = 2166136261u;

constexpr std::uint32_t fnv1aHash(const char* str, std::size_t length, std::uint32_t hash = FNV1aOffsetBasis) {
    return (length == 0)
        ? hash
        : fnv1aHash(str + 1, length - 1, (hash ^ static_cast<std::uint8_t>(*str)) * FNV1aPrime);
}

constexpr std::uint32_t fnv1aHash(const char* str) {
    std::size_t length = 0;
    while (str[length] != '\0') ++length;
    return fnv1aHash(str, length);
}

constexpr const char* testString = "HelloWorldThisIsSamplePr0gr7mmForTe$tHashing";
constexpr std::uint32_t testHash = fnv1aHash(testString);
)", { "testHash" });

	ASSERT_TRUE(res.vIssues.empty()) << "No issues expected to be here";
	ASSERT_EQ(std::get<std::uint64_t>(res.mOutputs["testHash"]), fnv1aHash("HelloWorldThisIsSamplePr0gr7mmForTe$tHashing"));
}