#include <gtest/gtest.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/Cpp/TypeAlias.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_InnerDeclaration : public ::testing::Test
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

TEST_F(Tests_InnerDeclaration, CheckInnerDecl)
{
	g_Analyzer->setSourceCode(R"(
namespace base {
	/// @runtime
	struct MyStruct
	{
		/// @runtime
		enum class MyEnum { ENT_0 = 0, ENT_1 = 1, ENT_2 = 2 };
	};
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	// expected to have 8 aliases
	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "MyStruct");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "base::MyStruct");

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "MyEnum");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ENUM);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "base::MyStruct::MyEnum");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->isScoped(), true);
}