#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <RG3/LLVM/CompilerConfig.h>
#include <RG3/Cpp/DefinitionLocation.h>
#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/Tag.h>
#include <unordered_map>
#include <functional>
#include <optional>


namespace rg3::llvm::visitors
{
	using PropertyFilterFunc = std::function<bool(const std::string&)>;
	using FunctionFilterFunc = std::function<bool(const std::string&)>;

	struct SClassDefInfo
	{
		bool bIsStruct{ false };
		bool bTriviallyConstructible{ false };
		bool bHasCopyConstructor { false };
		bool bHasCopyAssignOperator { false };
		bool bHasMoveConstructor { false };
		bool bHasMoveAssignOperator { false };
		bool bHasResolverErrors { false };
		std::string sClassName{};
		std::string sPrettyClassName{};
		rg3::cpp::CppNamespace sNameSpace{};
		rg3::cpp::DefinitionLocation sDefLocation{};
		rg3::cpp::Tags sTags{};
		rg3::cpp::ClassPropertyVector vProperties{};
		rg3::cpp::ClassFunctionVector vFunctions{};
		rg3::cpp::ClassFriendVector vFriends{};
		std::vector<rg3::cpp::ClassParent> vParents{}; // NOTE: Should be replaced for short descriptions of that types (later; legacy)
	};

	/**
	 * @brief This visitor traverses class template specialization, collect fields, functions and resolve final types
	 * @note This class should be a successor of CxxClassTypeVisitor (later)
	 */
	class CxxTemplateSpecializationVisitor : public clang::RecursiveASTVisitor<CxxTemplateSpecializationVisitor>
	{
	 public:
		CxxTemplateSpecializationVisitor(const CompilerConfig& cc,
										 clang::ClassTemplateSpecializationDecl* pTemplateSpecialization,
										 bool bHasProperties, bool bHasFunctions,
										 PropertyFilterFunc propertyFilterFunc,
										 FunctionFilterFunc functionFilterFunc);

		bool VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl);
		bool VisitFieldDecl(clang::FieldDecl* cxxFieldDecl);
		bool VisitCXXMethodDecl(clang::CXXMethodDecl* cxxMethodDecl);

		const std::optional<SClassDefInfo>& getClassDefInfo() const;

	 private:
		bool tryResolveTemplateType(rg3::cpp::TypeStatement& stmt, const clang::Type* pOriginalType, clang::ASTContext& ctx) const;

	 private:
		const CompilerConfig& m_compilerConfig;
		clang::ClassTemplateSpecializationDecl* m_pSpecialization { nullptr };
		std::optional<SClassDefInfo> m_classDefInfo {};
		std::unordered_map<std::string, clang::QualType> m_mTemplateParamNameToInstantiatedType;
		bool m_bHasProperties;
		bool m_bHasFunctions;
		PropertyFilterFunc m_propertyFilterFunc;
		FunctionFilterFunc m_functionFilterFunc;
	};
}