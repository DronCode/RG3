#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_ErrorReporting : public ::testing::Test
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

TEST_F(Tests_ErrorReporting, ReportPreprocessorError)
{
	g_Analyzer->setSourceCode("#error \"LemonadeIssue\"");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vFoundTypes.empty()) << "Weird";
	ASSERT_EQ(analyzeResult.vIssues.size(), 1) << "Expected to have 1 fatal issue";
	ASSERT_EQ(analyzeResult.vIssues[0].kind, rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR);
	ASSERT_EQ(analyzeResult.vIssues[0].sSourceFile, "id0.hpp");
	ASSERT_EQ(analyzeResult.vIssues[0].iLine, 1);
	// Don't check column, idk how to predict it
	ASSERT_EQ(analyzeResult.vIssues[0].sMessage, "\"LemonadeIssue\"");
}

TEST_F(Tests_ErrorReporting, CppError_UndefinedType)
{
	g_Analyzer->setSourceCode(R"(
/**
 * @runtime
 **/
struct MyCoolType
{
	void DoFoo(std::string MyFoo);
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	// About vFoundTypes: it's UB
	ASSERT_EQ(analyzeResult.vIssues.size(), 1) << "Expected to have 1 fatal issue";
	ASSERT_EQ(analyzeResult.vIssues[0].kind, rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR);
	ASSERT_EQ(analyzeResult.vIssues[0].sSourceFile, "id0.hpp");
	ASSERT_EQ(analyzeResult.vIssues[0].sMessage, "use of undeclared identifier 'std'");
}