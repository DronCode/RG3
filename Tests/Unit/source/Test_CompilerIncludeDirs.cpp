#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/Compiler.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_CompilerIncludeDirs : public ::testing::Test
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

TEST_F(Tests_CompilerIncludeDirs, CheckIncludeDirs)
{
	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;
	g_Analyzer->setSourceFile("Unit/test_headers/TestInc.h");
	g_Analyzer->getCompilerConfig().vIncludes.emplace_back("Unit/test_headers/include", rg3::llvm::IncludeKind::IK_PROJECT);
	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("-x");
	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("c++-header");
	auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";
	// That's enough for this case
}

TEST_F(Tests_CompilerIncludeDirs, CheckStdIncludes)
{
	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;
	g_Analyzer->setSourceFile("Unit/test_headers/TestInc.h");
	g_Analyzer->getCompilerConfig().vIncludes.emplace_back("Unit/test_headers/include", rg3::llvm::IncludeKind::IK_PROJECT);
	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("-x");
	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("c++-header");

	// ------------------------------------------------------------------------------------------------------
	g_Analyzer->getCompilerConfig().vCompilerDefs.emplace_back("ENABLE_STD_TEST_ARGS=1");
	auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";
	// That's enough for this case
}