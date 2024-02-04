#include <gtest/gtest.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_MemberFunctions : public ::testing::Test
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

TEST_F(Tests_MemberFunctions, CheckMemberFunctions)
{
	g_Analyzer->setSourceCode(R"(
namespace my_cool_space
{
	/// @runtime
	struct MyFooClass
	{
		static void DoSomething();
		int GetHealth() const;
		void AddHealth(int hp);
	protected:
		void SubHealth(unsigned int hp);
	};
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "my_cool_space::MyFooClass") << "Wrong pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Wrong kind";

	auto asClass = static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)

	ASSERT_EQ(asClass->getProperties().size(), 0);
	ASSERT_EQ(asClass->getFunctions().size(), 4);

	ASSERT_EQ(asClass->getFunctions()[0].sName, "DoSomething");
	ASSERT_EQ(asClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(asClass->getFunctions()[0].bIsStatic, true);
	ASSERT_EQ(asClass->getFunctions()[0].bIsConst, false);

	ASSERT_EQ(asClass->getFunctions()[1].sName, "GetHealth");
	ASSERT_EQ(asClass->getFunctions()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(asClass->getFunctions()[1].bIsStatic, false);
	ASSERT_EQ(asClass->getFunctions()[1].bIsConst, true);

	ASSERT_EQ(asClass->getFunctions()[2].sName, "AddHealth");
	ASSERT_EQ(asClass->getFunctions()[2].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(asClass->getFunctions()[2].bIsStatic, false);
	ASSERT_EQ(asClass->getFunctions()[2].bIsConst, false);

	ASSERT_EQ(asClass->getFunctions()[3].sName, "SubHealth");
	ASSERT_EQ(asClass->getFunctions()[3].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PROTECTED);
	ASSERT_EQ(asClass->getFunctions()[3].bIsStatic, false);
	ASSERT_EQ(asClass->getFunctions()[3].bIsConst, false);
}