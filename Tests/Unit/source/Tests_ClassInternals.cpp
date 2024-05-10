#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>



class Tests_ClassInternals : public ::testing::Test
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


TEST_F(Tests_ClassInternals, SimpleUsage)
{
	g_Analyzer->setSourceCode(R"(
struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

/**
 * @runtime
 **/
struct Entity final : public NonCopyable
{
	Entity(Entity&& move) noexcept;
	Entity& operator=(Entity&& moveInst) noexcept;
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Expected to have 1 type here, got " << analyzeResult.vFoundTypes.size();

	// Check type
	auto* pType = analyzeResult.vFoundTypes[0].get();
	ASSERT_EQ(pType->getName(), "Entity");
	ASSERT_EQ(pType->getPrettyName(), "Entity");
	ASSERT_EQ(pType->getNamespace().asString(), "");
	ASSERT_EQ(pType->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Expected to have class or struct";
	ASSERT_EQ(pType->getTags().getTags().size(), 1) << "Expected to have at least 1 tag";
	ASSERT_EQ(pType->getTags().hasTag("runtime"), true);
	ASSERT_TRUE(pType->isForwardDeclarable()) << "Entity must be fwd declarable";

	// Check class
	auto* pClass = reinterpret_cast<rg3::cpp::TypeClass*>(pType);
	ASSERT_EQ(pClass->isStruct(), true);
	//ASSERT_TRUE(pClass->isTrivialConstructible()); ///< BUG PALACE: This stub works not so well in this case
	ASSERT_FALSE(pClass->hasCopyConstructor());
	ASSERT_FALSE(pClass->hasCopyAssignOperator());
	ASSERT_TRUE(pClass->hasMoveConstructor());
	ASSERT_TRUE(pClass->hasMoveAssignOperator());
	ASSERT_EQ(pClass->getProperties().size(), 0) << "No properties expected";
	ASSERT_EQ(pClass->getFunctions().size(), 2) << "We must have 2 functions: move ctor & move assign operators";

	// Move ctor
	ASSERT_EQ(pClass->getFunctions()[0].sName, "Entity") << "Ctor";
	ASSERT_EQ(pClass->getFunctions()[0].sOwnerClassName, "Entity");
	ASSERT_EQ(pClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(pClass->getFunctions()[0].sReturnType.isVoid(), true);
	ASSERT_EQ(pClass->getFunctions()[0].vArguments.size(), 1);
	ASSERT_EQ(pClass->getFunctions()[0].vArguments[0].sArgumentName, "move");
	ASSERT_EQ(pClass->getFunctions()[0].vArguments[0].sType.sBaseInfo.sPrettyName, "Entity");
	ASSERT_EQ(pClass->getFunctions()[0].vArguments[0].sType.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(pClass->getFunctions()[0].bIsNoExcept, true);

	// Assign move operator
	ASSERT_EQ(pClass->getFunctions()[1].sName, "operator=") << "Ctor";
	ASSERT_EQ(pClass->getFunctions()[1].sOwnerClassName, "Entity");
	ASSERT_EQ(pClass->getFunctions()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(pClass->getFunctions()[1].sReturnType.sBaseInfo.sPrettyName, "Entity");
	ASSERT_EQ(pClass->getFunctions()[1].sReturnType.bIsReference, true);
	ASSERT_EQ(pClass->getFunctions()[1].vArguments.size(), 1);
	ASSERT_EQ(pClass->getFunctions()[1].vArguments[0].sArgumentName, "moveInst");
	ASSERT_EQ(pClass->getFunctions()[1].vArguments[0].sType.sBaseInfo.sPrettyName, "Entity");
	ASSERT_EQ(pClass->getFunctions()[1].vArguments[0].sType.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(pClass->getFunctions()[1].bIsNoExcept, true);
}