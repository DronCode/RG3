#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_Enum : public ::testing::Test
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


TEST_F(Tests_Enum, ProcessSimpleEnum)
{
	g_Analyzer->setSourceCode(R"(
namespace my::cool::ns {
	/**
	 * @runtime
	 **/
	enum class MyCoolEnum : unsigned short
	{
		MCE_FIRST_ENTRY = 0,
		MCE_ANOTHER_ENTRY = 0xFFF0u,
		MCE_REVERTED = 0x200u,
	};
}
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(static_cast<bool>(analyzeResult)) << "Looks like we have an issues with analyzer";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Expected to have 1 type";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "MyCoolEnum") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "my::cool::ns") << "Invalid namespace detected";

	auto asEnum = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)

	ASSERT_TRUE(asEnum->isScoped()) << "Must be scoped";
	ASSERT_EQ(asEnum->getUnderlyingType().getRefName(), "unsigned short") << "Expected 'unsigned short'";
	ASSERT_EQ(asEnum->getEntries().size(), 3) << "Expected to have 3 elements";
	ASSERT_TRUE(asEnum->containsValue(0)) << "";
	ASSERT_TRUE(asEnum->containsValue(0xFFF0u)) << "";
	ASSERT_TRUE(asEnum->containsValue(0x200u)) << "";
	ASSERT_FALSE(asEnum->containsValue(333)) << "";
	ASSERT_EQ(asEnum->getEntries()[0].sName, "MCE_FIRST_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[0].iValue, 0);
	ASSERT_EQ(asEnum->getEntries()[1].sName, "MCE_ANOTHER_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[1].iValue, 0xFFF0u);
	ASSERT_EQ(asEnum->getEntries()[2].sName, "MCE_REVERTED");
	ASSERT_EQ(asEnum->getEntries()[2].iValue, 0x200u);
}

TEST_F(Tests_Enum, CheckEnumThroughTypedef)
{
	g_Analyzer->setSourceCode(R"(
namespace my::cool::ns {
	enum class MyCoolEnum : unsigned short
	{
		MCE_FIRST_ENTRY = 0,
		MCE_ANOTHER_ENTRY = 0xFFF0u,
		MCE_REVERTED = 0x200u,
	};
}

namespace world
{
	/// @runtime
	using PerfectEnum = my::cool::ns::MyCoolEnum;
}
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(static_cast<bool>(analyzeResult)) << "Looks like we have an issues with analyzer";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Expected to have 1 type";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "PerfectEnum") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "world::PerfectEnum") << "Bad pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "world") << "Invalid namespace detected";

	auto asEnum = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)

	ASSERT_TRUE(asEnum->isScoped()) << "Must be scoped";
	ASSERT_EQ(asEnum->getUnderlyingType().getRefName(), "unsigned short") << "Expected 'unsigned short'";
	ASSERT_EQ(asEnum->getEntries().size(), 3) << "Expected to have 3 elements";
	ASSERT_TRUE(asEnum->containsValue(0)) << "";
	ASSERT_TRUE(asEnum->containsValue(0xFFF0u)) << "";
	ASSERT_TRUE(asEnum->containsValue(0x200u)) << "";
	ASSERT_FALSE(asEnum->containsValue(333)) << "";
	ASSERT_EQ(asEnum->getEntries()[0].sName, "MCE_FIRST_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[0].iValue, 0);
	ASSERT_EQ(asEnum->getEntries()[1].sName, "MCE_ANOTHER_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[1].iValue, 0xFFF0u);
	ASSERT_EQ(asEnum->getEntries()[2].sName, "MCE_REVERTED");
	ASSERT_EQ(asEnum->getEntries()[2].iValue, 0x200u);
}