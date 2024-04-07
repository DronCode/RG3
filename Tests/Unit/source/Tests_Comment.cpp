#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_Comments : public ::testing::Test
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

TEST_F(Tests_Comments, SimpleCommentBeforeTypeDecl)
{
	g_Analyzer->setSourceCode(R"(
/**
 * @runtime
 * @serialize(Test)
 * @make_fun(1,2,17.2,TestValue)
 **/
struct MyCoolStructure
{};
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_14;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Type must be struct or class";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTags().size(), 3) << "Expected to have 3 tags";
	ASSERT_TRUE(analyzeResult.vFoundTypes[0]->getTags().hasTag("runtime")) << "Expected to have 'runtime' tag";
	ASSERT_FALSE(analyzeResult.vFoundTypes[0]->getTags().getTag("runtime").hasArguments()) << "runtime must have 0 args";
	ASSERT_TRUE(analyzeResult.vFoundTypes[0]->getTags().hasTag("serialize")) << "Expected to have 'serialize' tag";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("serialize").getArgumentsCount(), 1) << "runtime must have 1 args";
	ASSERT_TRUE(analyzeResult.vFoundTypes[0]->getTags().hasTag("make_fun")) << "make_fun tag should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArgumentsCount(), 4) << "make_fun should have 4 args";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_I64) << "[0] arg of 'make_fun' must be i64";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[0].asI64(0), 1) << "[0] arg of 'make_fun' must be 1";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[1].getHoldedType(), rg3::cpp::TagArgumentType::AT_I64) << "[1] arg of 'make_fun' must be i64";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[1].asI64(0), 2) << "[1] arg of 'make_fun' must be 2";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[2].getHoldedType(), rg3::cpp::TagArgumentType::AT_FLOAT) << "[2] arg of 'make_fun' must be f32";
	ASSERT_FLOAT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[2].asFloat(0.0f), 17.2f) << "[2] arg of 'make_fun' must be 17.2";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[3].getHoldedType(), rg3::cpp::TagArgumentType::AT_STRING) << "[3] arg of 'make_fun' must be str";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("make_fun").getArguments()[3].asString(""), "TestValue") << "[3] arg of 'make_fun' must be 'TestValue'";
}

TEST_F(Tests_Comments, SimpleCommentBeforePropertyDecl)
{
	g_Analyzer->setSourceCode(R"(
/**
 * @runtime
 **/
struct MyCoolStructure
{
	/**
  	 * @property(GoodBuddy)
	 * @toolset(true)
     **/
	bool m_bIsGoodBuddy { false };

	/**
     * @my_func
     * @use_anyway(1)
     **/
	void DoFoo();
};
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_14;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";

	const auto& tags = analyzeResult.vFoundTypes[0]->getTags();
	ASSERT_TRUE(tags.hasTag("runtime")) << "Type must have 'runtime' tag";
	ASSERT_FALSE(tags.hasTag("property")) << "Type must not have 'property' tag";
	ASSERT_FALSE(tags.hasTag("toolset")) << "Type must not have 'toolset' tag";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Type 0 must be struct or class";

	auto asClass = static_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_TRUE(asClass->isStruct()) << "T0 must be struct!";

	ASSERT_EQ(asClass->getProperties().size(), 1) << "Expected to have 1 property";
	ASSERT_TRUE(asClass->getProperties()[0].vTags.hasTag("property")) << "Property 0 must have tag 'property'";
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("property").getArgumentsCount(), 1) << "Expected to have 1 arg in property";
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("property").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_STRING) << "Expected to have string";
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("property").getArguments()[0].asString(""), "GoodBuddy") << "Expected to have valid string";

	ASSERT_TRUE(asClass->getProperties()[0].vTags.hasTag("toolset")) << "Property 0 must have tag 'toolset'";
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("toolset").getArgumentsCount(), 1) << "Expected to have 1 arg in property";
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("toolset").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_BOOL) << "Expected to have bool";
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("toolset").getArguments()[0].asBool(false), true) << "Expected to have valid bool";

	ASSERT_EQ(asClass->getFunctions().size(), 1) << "Expected to have 1 function";
	ASSERT_TRUE(asClass->getFunctions()[0].vTags.hasTag("my_func")) << "Property 0 must have tag 'my_func'";
	ASSERT_TRUE(asClass->getFunctions()[0].vTags.hasTag("use_anyway")) << "Property 0 must have tag 'use_anyway'";
	ASSERT_EQ(asClass->getFunctions()[0].vTags.getTag("use_anyway").getArgumentsCount(), 1) << "Expected to have 1 arg";
	ASSERT_EQ(asClass->getFunctions()[0].vTags.getTag("use_anyway").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_I64) << "Expected to have i64";
	ASSERT_EQ(asClass->getFunctions()[0].vTags.getTag("use_anyway").getArguments()[0].asI64(0), 1) << "Expected to have 1";
}

TEST_F(Tests_Comments, CommentWithDots)
{
	g_Analyzer->setSourceCode(R"(
/**
 * @runtime
 * @ecs.component(1024, true)
 **/
class PlayerComponent
{
};
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_14;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";

	const auto& tags = analyzeResult.vFoundTypes[0]->getTags();
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "PlayerComponent");
	ASSERT_TRUE(tags.hasTag("runtime")) << "Type must have 'runtime' tag";
	ASSERT_TRUE(tags.hasTag("ecs.component")) << "Type must have 'ecs.component' tag";
	ASSERT_FALSE(tags.hasTag("ecs")) << "Type must have not 'ecs' tag";
	ASSERT_EQ(tags.getTag("ecs.component").getArgumentsCount(), 2) << "Invalid args count";
	ASSERT_EQ(tags.getTag("ecs.component").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_I64) << "A0 must be i64";
	ASSERT_EQ(tags.getTag("ecs.component").getArguments()[0].asI64(0), 1024) << "A0 == 1024";
	ASSERT_EQ(tags.getTag("ecs.component").getArguments()[1].getHoldedType(), rg3::cpp::TagArgumentType::AT_BOOL) << "A1 must be bool";
	ASSERT_EQ(tags.getTag("ecs.component").getArguments()[1].asBool(false), true) << "A1 == true";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Type 0 must be struct or class";

	auto asClass = static_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_FALSE(asClass->isStruct()) << "T0 must be a class!";
}

TEST_F(Tests_Comments, CheckCommentPoisonous)
{
	g_Analyzer->setSourceCode(R"(
/// @runtime
/// @test1(640,"Test")
enum class ELayer
{
    L_FIRST,
    L_LAST
};

/// @runtime
/// @test23(true)
enum class EPicType
{
    PC_GENERIC = (1 << 10),
    PC_ALPHA = (1 << 15),
    PC_DIRTY = (1 << 4)
};

/**
 * @runtime
 * @serializer(@common::Serializer<DrawableBase>)
 * @world_mgmt(false)
 */
class DrawableBase
{
public:
    /// @property(Layer)
    ELayer  m_eLayer { ELayer::L_FIRST };
};

)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_14;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 3) << "Only 3 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTags().size(), 2);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().hasTag("runtime"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("runtime").getArgumentsCount(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().hasTag("test1"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().hasTag("serializer"), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("test1").getArgumentsCount(), 2);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("test1").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_I64);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("test1").getArguments()[0].asI64(0), 640);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("test1").getArguments()[1].getHoldedType(), rg3::cpp::TagArgumentType::AT_STRING);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().getTag("test1").getArguments()[1].asString(""), "Test");

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().getTags().size(), 2);  // only runtime
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().hasTag("runtime"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().getTag("runtime").getArgumentsCount(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().hasTag("test23"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().hasTag("serializer"), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().getTag("test23").getArgumentsCount(), 1);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().getTag("test23").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_BOOL);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().getTag("test23").getArguments()[0].asBool(false), true);

	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTags().size(), 3);  // only runtime and serializer
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().hasTag("runtime"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().hasTag("serializer"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().hasTag("world_mgmt"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTag("runtime").getArgumentsCount(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTag("serializer").getArgumentsCount(), 1);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTag("serializer").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_TYPEREF);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTag("serializer").getArguments()[0].asTypeRefMutable()->getRefName(), "common::Serializer<DrawableBase>");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTag("world_mgmt").getArgumentsCount(), 1);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTag("world_mgmt").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_BOOL);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getTags().getTag("world_mgmt").getArguments()[0].asBool(true), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	auto* asClass = reinterpret_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[2].get());
	ASSERT_EQ(asClass->getProperties().size(), 1);
	ASSERT_EQ(asClass->getFunctions().size(), 0);
	ASSERT_EQ(asClass->getProperties()[0].vTags.hasTag("property"), true);
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("property").getArgumentsCount(), 1);
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("property").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_STRING);
	ASSERT_EQ(asClass->getProperties()[0].vTags.getTag("property").getArguments()[0].asString(""), "Layer");
	ASSERT_EQ(asClass->getProperties()[0].vTags.hasTag("runtime"), false);
	ASSERT_EQ(asClass->getProperties()[0].vTags.hasTag("serializer"), false);
}