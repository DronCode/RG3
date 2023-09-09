#include <RG3/LLVM/Visitors/CxxClassTypeVisitor.h>
#include <RG3/Cpp/BuiltinTags.h>
#include <RG3/LLVM/Utils.h>


namespace rg3::llvm::visitors
{
	bool CxxClassTypeVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl)
	{
		// Extract comment
		clang::ASTContext& ctx = cxxRecordDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxRecordDecl);
		if (!rawComment)
			return true;

		const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
		vTags = cpp::Tag::parseFromCommentString(rawCommentStr);

		// Check this somewhere else
		if (!vTags.hasTag(std::string(rg3::cpp::BuiltinTags::kRuntime)))
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
			parentClasses.emplace_back(baseSpecifier.getType().getAsString());
		}

		return true;
	}

	bool CxxClassTypeVisitor::VisitFieldDecl(clang::FieldDecl* cxxFieldDecl)
	{
		// Save field info
		cpp::ClassProperty& newProperty = foundProperties.emplace_back();

		newProperty.sAlias = newProperty.sName = cxxFieldDecl->getNameAsString();
		newProperty.sTypeName = cpp::TypeReference(cxxFieldDecl->getType().getAsString());

		clang::ASTContext& ctx = cxxFieldDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxFieldDecl);
		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			newProperty.vTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		switch (cxxFieldDecl->getVisibility())
		{
			case clang::HiddenVisibility:
				newProperty.eVisibility = cpp::ClassEntryVisibility::CEV_PRIVATE;
				break;
			case clang::ProtectedVisibility:
				newProperty.eVisibility = cpp::ClassEntryVisibility::CEV_PROTECTED;
				break;
			case clang::DefaultVisibility:
				newProperty.eVisibility = bIsStruct ? cpp::ClassEntryVisibility::CEV_PUBLIC : cpp::ClassEntryVisibility::CEV_PRIVATE;
				break;
		}

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

		switch (cxxMethodDecl->getVisibility())
		{
		case clang::HiddenVisibility:
			newFunction.eVisibility = cpp::ClassEntryVisibility::CEV_PRIVATE;
			break;
		case clang::ProtectedVisibility:
			newFunction.eVisibility = cpp::ClassEntryVisibility::CEV_PROTECTED;
			break;
		case clang::DefaultVisibility:
			newFunction.eVisibility = bIsStruct ? cpp::ClassEntryVisibility::CEV_PUBLIC : cpp::ClassEntryVisibility::CEV_PRIVATE;
			break;
		}

		return true;
	}
}