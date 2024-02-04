#include <gtest/gtest.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_TypeReference : public ::testing::Test
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

TEST_F(Tests_TypeReference, CheckTypeReferenceRecognition)
{
	g_Analyzer->setSourceCode(R"(
namespace my_cool_space
{
	/// @runtime
	struct Serializer
	{
		// DO FOO
	};

	/**
	 * @runtime
	 * @serializer(@my_cool_space::Serializer)
	 */
	struct Subject
	{
		// DO FOO
	};
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "my_cool_space::Serializer") << "Wrong type at #0";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "my_cool_space::Subject") << "Wrong type at #1";
	ASSERT_TRUE(analyzeResult.vFoundTypes[1]->getTags().hasTag("serializer")) << "No @serializer tag found!";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().getTag("serializer").getArgumentsCount(), 1) << "Wrong args count";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().getTag("serializer").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_TYPEREF);

	static const rg3::cpp::TypeReference s_InvalidRef { "#INVALID_DUCK" }; // U -> I
	const rg3::cpp::TypeReference& rType = analyzeResult.vFoundTypes[1]->getTags().getTag("serializer").getArguments()[0].asTypeRef(s_InvalidRef);

	ASSERT_NE(rType.getRefName(), s_InvalidRef.getRefName()) << "Wrong ref!";
	ASSERT_EQ(rType.getRefName(), "my_cool_space::Serializer") << "Wrong ref name!";
	ASSERT_EQ(rType.get(), nullptr) << "Resolved? WTF?!";
}