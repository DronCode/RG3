#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_FsFile : public ::testing::Test
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

TEST_F(Tests_FsFile, CheckAnalyzeFromFile)
{
	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;
	g_Analyzer->setSourceFile("Unit/test_headers/Test.h");
	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("-x");
	g_Analyzer->getCompilerConfig().vCompilerArgs.emplace_back("c++-header");
	auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Type must be struct or class";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "my_cool_namespace::MyRuntimeClass") << "Bad thing pretty name";
	ASSERT_TRUE(analyzeResult.vFoundTypes[0]->getTags().hasTag("runtime")) << "Expected to have 'runtime' tag";
	ASSERT_FALSE(analyzeResult.vFoundTypes[0]->getTags().getTag("runtime").hasArguments()) << "runtime must have 0 args";
	ASSERT_TRUE(analyzeResult.vFoundTypes[0]->getTags().hasTag("serialize")) << "Expected to have 'serialize' tag";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("serialize").getArgumentsCount(), 1) << "runtime must have 1 args";
}