#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/LLVM/CompilerConfig.h>


class Tests_Compiler : public ::testing::Test
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

TEST_F(Tests_Compiler, CheckIgnoreRuntimeFlag)
{
	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17; // Use C++17 (but allowed to use any version here)
	g_Analyzer->getCompilerConfig().bAllowCollectNonRuntimeTypes = false; // Prohibit to process types without @runtime

	g_Analyzer->setSourceCode(R"(
struct MyFirstStruct
{};

/**
 * @runtime
 **/
struct MySecondStruct
{};
)");

	{
		auto result = g_Analyzer->analyze();

		ASSERT_TRUE(result.vIssues.empty()) << "No issues in this test";
		ASSERT_EQ(result.vFoundTypes.size(), 1) << "Allowed to have only 1 type!";
		ASSERT_EQ(result.vFoundTypes[0]->getPrettyName(), "MySecondStruct") << "Wront type";
	}

	g_Analyzer->getCompilerConfig().bAllowCollectNonRuntimeTypes = true; // Allow to collect all types (bad performance in theory)
	{
		auto result = g_Analyzer->analyze();

		ASSERT_TRUE(result.vIssues.empty()) << "No issues in this test";
		ASSERT_EQ(result.vFoundTypes.size(), 2) << "Must be 2 types!";
		ASSERT_EQ(result.vFoundTypes[0]->getPrettyName(), "MyFirstStruct") << "Wront type";
		ASSERT_EQ(result.vFoundTypes[1]->getPrettyName(), "MySecondStruct") << "Wront type";
	}
}

TEST_F(Tests_Compiler, CheckNotCompletedTemplateSpecialization) // Bugfix
{
	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	g_Analyzer->setSourceCode(R"(
/// @runtime
template <typename T> struct TSample;
)");

	auto result = g_Analyzer->analyze();

	ASSERT_TRUE(result.vIssues.empty()) << "No issues in this test";
	ASSERT_TRUE(result.vFoundTypes.empty()) << "No types expected to have here";
}

TEST_F(Tests_Compiler, CheckEnumForwardDeclaration)
{
	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	g_Analyzer->setSourceCode(R"(
/// @runtime
enum class EBoomer;
)");

	auto result = g_Analyzer->analyze();

	ASSERT_TRUE(result.vIssues.empty()) << "No issues in this test";
	ASSERT_TRUE(result.vFoundTypes.empty()) << "No types expected to have here";
}