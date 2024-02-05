#include <RG3/LLVM/Visitors/CxxClassTypeVisitor.h>
#include <RG3/Cpp/BuiltinTags.h>
#include <RG3/LLVM/Utils.h>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/Decl.h>


namespace rg3::llvm::visitors
{
	static void fillTypeStatementFromQualType(rg3::cpp::TypeStatement& typeStatement, clang::QualType qt, clang::SourceManager& sm)
	{
		typeStatement.sTypeRef = cpp::TypeReference(rg3::llvm::Utils::getNormalizedTypeRef(qt.getAsString()));
		typeStatement.bIsConst = qt.isConstQualified();

		if (qt->isPointerType() || qt->isReferenceType())
		{
			typeStatement.bIsPointer = qt->isPointerType();
			typeStatement.bIsReference = qt->isReferenceType();
			typeStatement.sTypeRef = cpp::TypeReference(rg3::llvm::Utils::getNormalizedTypeRef(qt->getPointeeType().getUnqualifiedType().getAsString()));
			typeStatement.bIsPtrConst = qt->getPointeeType().isConstQualified();
		}

		if (qt->getAs<clang::TemplateSpecializationType>())
		{
			typeStatement.bIsTemplateSpecialization = true;
		}

		// Try extract location info
		const clang::Type* pType = qt.getTypePtr();
		if (const auto* pRecord = pType->getAs<clang::RecordType>())
		{
			const clang::RecordDecl* pRecordDecl = pRecord->getDecl();
			if (pRecordDecl)
			{
				clang::SourceLocation location = pRecordDecl->getLocation();
				clang::PresumedLoc presumedLoc = sm.getPresumedLoc(location);

				if (presumedLoc.isValid())
				{
					typeStatement.sDefinitionLocation.emplace(
						std::filesystem::path(presumedLoc.getFilename()),
						presumedLoc.getLine(),
						presumedLoc.getColumn()
					);

					/*
					 * Maybe it's better, who knows...
					typeStatement.sDefinitionLocation.emplace(
							std::filesystem::path(presumedLoc.getIncludeLoc().printToString(sm)),
							presumedLoc.getLine(),
							presumedLoc.getColumn()
					);
					 */
				}
			}
		}
	}

	static void fillTypeStatementFromLLVMEntry(rg3::cpp::TypeStatement& typeStatement, clang::CXXMethodDecl* cxxMethodDecl)
	{
		clang::QualType qt = cxxMethodDecl->getReturnType();
		fillTypeStatementFromQualType(typeStatement, qt, cxxMethodDecl->getASTContext().getSourceManager());
	}

	static void fillTypeStatementFromLLVMEntry(rg3::cpp::TypeStatement& typeStatement, clang::FieldDecl* fieldDecl)
	{
		clang::QualType qt = fieldDecl->getType();
		fillTypeStatementFromQualType(typeStatement, qt, fieldDecl->getASTContext().getSourceManager());
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
			fillTypeStatementFromQualType(newArgument.sType, pParam->getType(), sm);

			// Save arg name
			newArgument.sArgumentName = pParam->getNameAsString();

			// Save info about default value
			newArgument.bHasDefaultValue = pParam->hasDefaultArg();
		}

		return true;
	}
}