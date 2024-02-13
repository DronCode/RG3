#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_Inheritance : public ::testing::Test
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

TEST_F(Tests_Inheritance, CheckDefaultInheritanceMode)
{
	g_Analyzer->setSourceCode(R"(
/// @runtime
class A {};

/// @runtime
struct B : A {};

/// @runtime
class C : A {};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_14;
	compilerConfig.vCompilerArgs = {"-x", "c++-header"};

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 3);

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "A");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get())->getParentTypes().size(), 0);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "B");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get())->getParentTypes().size(), 1);
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get())->getParentTypes()[0].rParentType.getRefName(), "A");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get())->getParentTypes()[0].eModifier, rg3::cpp::InheritanceVisibility::IV_PUBLIC);

	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getPrettyName(), "C");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[2].get())->getParentTypes().size(), 1);
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[2].get())->getParentTypes()[0].rParentType.getRefName(), "A");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[2].get())->getParentTypes()[0].eModifier, rg3::cpp::InheritanceVisibility::IV_PRIVATE);
}

TEST_F(Tests_Inheritance, CheckVirtualInheritance)
{
	g_Analyzer->setSourceCode(R"(
/// @runtime
class A {};

/// @runtime
struct B : virtual A {};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_14;
	compilerConfig.vCompilerArgs = {"-x", "c++-header"};

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2);

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "A");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get())->getParentTypes().size(), 0);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "B");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get())->getParentTypes().size(), 1);
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get())->getParentTypes()[0].rParentType.getRefName(), "A");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get())->getParentTypes()[0].eModifier, rg3::cpp::InheritanceVisibility::IV_VIRTUAL);
}