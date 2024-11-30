#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/LLVM/CodeEvaluator.h>


class Tests_CodeEvaluator : public ::testing::Test
{
 protected:
	void SetUp() override
	{
		g_Eval = std::make_unique<rg3::llvm::CodeEvaluator>();
		g_Eval->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;
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