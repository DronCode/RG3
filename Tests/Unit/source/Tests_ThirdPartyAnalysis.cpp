#include <gtest/gtest.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_ThirdPartyAnalysis : public ::testing::Test
{
 protected:
	void SetUp() override
	{
		g_Analyzer = std::make_unique<rg3::llvm::CodeAnalyzer>();
		g_Analyzer->getCompilerConfig().vIncludes.emplace_back("Unit/test_headers/ThirdParty/glm"); // Add glm to lookup
	}

	void TearDown() override
	{
		g_Analyzer = nullptr;
	}

 protected:
	std::unique_ptr<rg3::llvm::CodeAnalyzer> g_Analyzer { nullptr };
};

#if 0
TEST_F(Tests_ThirdPartyAnalysis, CheckGlmVec2AnonymousReg)
{
	g_Analyzer->setSourceCode(R"(
#ifdef __RG3__
#include <glm/vec2.hpp>

// Types
template <typename T> struct RegisterType {};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<glm::vec2> {
	using Type = glm::vec2;
};

#endif
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";
	// **** BUG HERE: partial specializations are not supported! This test will work! ***
}
#endif