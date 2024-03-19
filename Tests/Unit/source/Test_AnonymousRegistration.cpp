#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_AnonymousRegistration : public ::testing::Test
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


TEST_F(Tests_AnonymousRegistration, CheckSimpleTypeRegistrationAndAnnotation)
{
	g_Analyzer->setSourceCode(R"(
// Samples
struct V2
{
	float x { .0f };
	float y { .0f };

	bool IsNormalized();
};

using Vec2 = V2;

// Registrator
template <typename T> struct RegisterType {};
template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
	__attribute__((annotate("RG3_RegisterField[x:MyPropX]")))
	__attribute__((annotate("RG3_RegisterField[y:MyPropY]")))
	__attribute__((annotate("RG3_RegisterFunction[IsNormalized]")))
	__attribute__((annotate("RG3_RegisterTag[@serialize(\"MyCoolSerializer\")]")))
RegisterType<Vec2> {
	using Type = Vec2;
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
}

// Test cases:
// 1. Simple int, float, bool, char registration
// 2. Class registration (custom class, class over alias, std::string). Check property name overload. Check function capturer (ADD CAPTURER __attribute__((annotate("RG3_RegisterFunction[myPrettyCoolFunc]"))) )
// 3. Enum registration (scoped, unscoped, without underlying type, with custom underlying type)
// 4. Class registration: check RG3_RegisterFunction, RG3_RegisterField, check valid & invalid cases
// 5. Class registration: check that unknown properties not saved
// 6. Class registration: check that unknown functions not saved
// 7. Tag annotation: check tag annotations (string, int, type ref is enough)
// 8. Function ownership: check that functions from different spaces registered correctly