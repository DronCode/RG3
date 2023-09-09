#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_EntryVisibillity : public ::testing::Test
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

TEST_F(Tests_EntryVisibillity, CheckVisibillityDetector)
{
	g_Analyzer->setSourceCode(R"(
/**
 * @runtime
 **/
struct Struct1
{
	int m_field0 { 0 };

	void DoFoo();
};

/**
 * @runtime
 **/
class Class1
{
	int m_field0 { 0 };

	void DoFoo();
};

/**
 * @runtime
 **/
struct Struct2
{
	int m_field0 { 0 };
	void DoFunc1();

private:
	int m_field1 { 0 };
	void DoFunc2();
};

/**
 * @runtime
 **/
class Class2
{
protected:
	int m_field0 { 0 };
	void DoFunc1();

public:
	int m_field1 { 0 };
	void DoFunc2();
};
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_14;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 4) << "Expected 4 types";

	// *** CASE 1 ***
	{
		auto asClass = static_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)

		ASSERT_EQ(asClass->getProperties().size(), 1) << "Bad properties count";
		ASSERT_EQ(asClass->getProperties()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC) << "[1] Bad prop vis";

		ASSERT_EQ(asClass->getFunctions().size(), 1) << "Bad functions count";
		ASSERT_EQ(asClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC) << "[1] Bad func vis";
	}

	// *** CASE 2 ***
	{
		auto asClass = static_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)

		ASSERT_EQ(asClass->getProperties().size(), 1) << "Bad properties count";
		ASSERT_EQ(asClass->getProperties()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PRIVATE) << "[2] Bad prop vis";

		ASSERT_EQ(asClass->getFunctions().size(), 1) << "Bad functions count";
		ASSERT_EQ(asClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PRIVATE) << "[2] Bad func vis";
	}

	// *** CASE 3 ***
	{
		auto asClass = static_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[2].get()); // NOLINT(*-pro-type-static-cast-downcast)

		ASSERT_EQ(asClass->getProperties().size(), 2) << "Bad properties count";
		ASSERT_EQ(asClass->getProperties()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC) << "[3] Bad prop vis 1";
		ASSERT_EQ(asClass->getProperties()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PRIVATE) << "[3] Bad prop vis 2";

		ASSERT_EQ(asClass->getFunctions().size(), 2) << "Bad functions count";
		ASSERT_EQ(asClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC) << "[3] Bad func vis 1";
		ASSERT_EQ(asClass->getFunctions()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PRIVATE) << "[3] Bad func vis 2";
	}

	// *** CASE 4 ***
	{
		auto asClass = static_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[3].get()); // NOLINT(*-pro-type-static-cast-downcast)

		ASSERT_EQ(asClass->getProperties().size(), 2) << "Bad properties count";
		ASSERT_EQ(asClass->getProperties()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PROTECTED) << "[4] Bad prop vis 1";
		ASSERT_EQ(asClass->getProperties()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC) << "[4] Bad prop vis 2";

		ASSERT_EQ(asClass->getFunctions().size(), 2) << "Bad functions count";
		ASSERT_EQ(asClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PROTECTED) << "[4] Bad func vis 1";
		ASSERT_EQ(asClass->getFunctions()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC) << "[4] Bad func vis 2";
	}
}