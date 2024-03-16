#include <RG3/LLVM/Visitors/CxxClassTypeVisitor.h>
#include <RG3/Cpp/BuiltinTags.h>
#include <RG3/LLVM/Utils.h>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/Decl.h>


namespace rg3::llvm::visitors
{
	static void fillTypeStatementFromLLVMEntry(rg3::cpp::TypeStatement& typeStatement, clang::CXXMethodDecl* cxxMethodDecl)
	{
		clang::QualType qt = cxxMethodDecl->getReturnType();
		rg3::llvm::Utils::fillTypeStatementFromQualType(typeStatement, qt, cxxMethodDecl->getASTContext());
	}

	static void fillTypeStatementFromLLVMEntry(rg3::cpp::TypeStatement& typeStatement, clang::FieldDecl* fieldDecl)
	{
		clang::QualType qt = fieldDecl->getType();
		rg3::llvm::Utils::fillTypeStatementFromQualType(typeStatement, qt, fieldDecl->getASTContext());
	}

	CxxClassTypeVisitor::CxxClassTypeVisitor(const rg3::llvm::CompilerConfig& cc)
		: compilerConfig(cc)
	{
	}

	bool CxxClassTypeVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl)
	{
		// Extract comment
		clang::ASTContext& ctx = cxxRecordDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxRecordDecl);

		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			vTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		if (!vTags.hasTag(std::string(rg3::cpp::BuiltinTags::kRuntime)) && !compilerConfig.bAllowCollectNonRuntimeTypes)
			return true;

		// Create entry
		sClassName = cxxRecordDecl->getName().str();
		sClassPrettyName = Utils::getPrettyNameOfDecl(cxxRecordDecl);
		Utils::getDeclInfo(cxxRecordDecl, sNameSpace);

		// Location
		sDefinitionLocation = Utils::getDeclDefinitionInfo(cxxRecordDecl);

		// Is struct or class?
		bIsStruct = cxxRecordDecl->isStruct();
		bTriviallyConstructible = cxxRecordDecl->hasDefaultConstructor();

		// Collect parent class list
		for (const clang::CXXBaseSpecifier& baseSpecifier : cxxRecordDecl->bases()) {
			cpp::ClassParent& parent = parentClasses.emplace_back();
			parent.rParentType = cpp::TypeReference(baseSpecifier.getType().getAsString());

			if (baseSpecifier.isVirtual())
			{
				parent.eModifier = cpp::InheritanceVisibility::IV_VIRTUAL;
			}
			else
			{
				switch (baseSpecifier.getAccessSpecifier())
				{
					case clang::AS_public:
						parent.eModifier = cpp::InheritanceVisibility::IV_PUBLIC;
						break;
					case clang::AS_protected:
						parent.eModifier = cpp::InheritanceVisibility::IV_PROTECTED;
						break;
					case clang::AS_private:
						parent.eModifier = cpp::InheritanceVisibility::IV_PRIVATE;
						break;
					case clang::AS_none:
						parent.eModifier = bIsStruct ? cpp::InheritanceVisibility::IV_PUBLIC : cpp::InheritanceVisibility::IV_PRIVATE;
						break;
				}
			}
		}

		return true;
	}

	bool CxxClassTypeVisitor::VisitFieldDecl(clang::FieldDecl* cxxFieldDecl)
	{
		// Save field info
		cpp::ClassProperty& newProperty = foundProperties.emplace_back();
		newProperty.sAlias = newProperty.sName = cxxFieldDecl->getNameAsString();

		// Fill type info (and decl info)
		fillTypeStatementFromLLVMEntry(newProperty.sTypeInfo, cxxFieldDecl);

		// Save other info
		clang::ASTContext& ctx = cxxFieldDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxFieldDecl);
		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			newProperty.vTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		// Override alias if @property provided
		if (newProperty.vTags.hasTag(std::string(rg3::cpp::BuiltinTags::kProperty)))
		{
			const auto& propDef = newProperty.vTags.getTag(std::string(rg3::cpp::BuiltinTags::kProperty));

			if (propDef.hasArguments() && propDef.getArguments()[0].getHoldedType() == rg3::cpp::TagArgumentType::AT_STRING)
			{
				newProperty.sAlias = propDef.getArguments()[0].asString(newProperty.sName);
			}
		}

		newProperty.eVisibility = Utils::getDeclVisibilityLevel(cxxFieldDecl);

		return true;
	}

	bool CxxClassTypeVisitor::VisitCXXMethodDecl(clang::CXXMethodDecl* cxxMethodDecl)
	{
		// Save method info
		cpp::ClassFunction& newFunction = foundFunctions.emplace_back();
		newFunction.sName = cxxMethodDecl->getNameAsString();
		newFunction.bIsStatic = cxxMethodDecl->isStatic();
		newFunction.sOwnerClassName = sClassName;
		newFunction.bIsConst = cxxMethodDecl->isConst();

		clang::ASTContext& ctx = cxxMethodDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxMethodDecl);
		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			newFunction.vTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		newFunction.eVisibility = Utils::getDeclVisibilityLevel(cxxMethodDecl);

		// Extract return type
		fillTypeStatementFromLLVMEntry(newFunction.sReturnType, cxxMethodDecl);

		// Extract function arguments
		for (auto it = cxxMethodDecl->param_begin(); it != cxxMethodDecl->param_end(); ++it)
		{
			const clang::ParmVarDecl* pParam = (*it);
			cpp::FunctionArgument& newArgument = newFunction.vArguments.emplace_back();

			// Extract type info
			rg3::llvm::Utils::fillTypeStatementFromQualType(newArgument.sType, pParam->getType(), ctx);

			// Save arg name
			newArgument.sArgumentName = pParam->getNameAsString();

			// Save info about default value
			newArgument.bHasDefaultValue = pParam->hasDefaultArg();
		}

		return true;
	}
}