#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/LLVM/CodeAnalyzer.h>
#include "CommonHelpers.h"


class Tests_BuiltinDefs : public ::testing::Test
{
 protected:
	void SetUp() override
	{
		g_Analyzer = std::make_unique<rg3::llvm::CodeAnalyzer>();
	}

	void TearDown() override
	{
		g_Analyzer = nullptr;
	}

 protected:
	std::unique_ptr<rg3::llvm::CodeAnalyzer> g_Analyzer { nullptr };
};

TEST_F(Tests_BuiltinDefs, CheckBuiltins)
{
	g_Analyzer->setSourceFile("Unit/test_headers/TestDefs.h");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_14;
	compilerConfig.vCompilerArgs = {"-x", "c++-header"};

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
}

#ifdef _MSC_VER
TEST_F(Tests_BuiltinDefs, CheckMSVC_yvals_core)
{
	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;
	g_Analyzer->setSourceCode(R"(
#include <yvals_core.h>

struct TestObject
{};
)");

	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("-x");
	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("c++-header");

	auto analyzeResult = g_Analyzer->analyze();
	CommonHelpers::printCompilerIssues(analyzeResult.vIssues);

	ASSERT_EQ(analyzeResult.vIssues.size(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 0) << "No types expected here";
}
#endif