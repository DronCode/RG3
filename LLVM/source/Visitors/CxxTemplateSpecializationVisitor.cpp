#include <RG3/LLVM/Visitors/CxxTemplateSpecializationVisitor.h>
#include <RG3/Cpp/BuiltinTags.h>
#include <RG3/LLVM/Utils.h>


namespace rg3::llvm::visitors
{
	CxxTemplateSpecializationVisitor::CxxTemplateSpecializationVisitor(
		const rg3::llvm::CompilerConfig& cc,
		clang::ClassTemplateSpecializationDecl* pTemplateSpecialization,
		bool bHasProperties, bool bHasFunctions,
		PropertyFilterFunc propertyFilterFunc,
		FunctionFilterFunc functionFilterFunc
	)
		: m_compilerConfig(cc)
		, m_pSpecialization(pTemplateSpecialization)
		, m_classDefInfo(std::nullopt)
		, m_bHasProperties(bHasProperties)
		, m_bHasFunctions(bHasFunctions)
		, m_propertyFilterFunc(propertyFilterFunc)
		, m_functionFilterFunc(functionFilterFunc)
	{
		// Collect instance cache
		const clang::TemplateArgumentList& templateArgs = m_pSpecialization->getTemplateArgs();
		clang::ClassTemplateDecl* pTemplateDecl = m_pSpecialization->getSpecializedTemplate();
		clang::TemplateParameterList* pParameterList = pTemplateDecl->getTemplateParameters();

		for (unsigned int i = 0; i < templateArgs.size(); ++i)
		{
			const clang::TemplateArgument& arg = templateArgs[i];

			if (arg.getKind() == clang::TemplateArgument::Type) {
				clang::QualType sArgType = arg.getAsType();
				clang::NamedDecl* pNamedParamDecl = pParameterList->getParam(i);
				std::string sParamName = pNamedParamDecl->getNameAsString();

				m_mTemplateParamNameToInstantiatedType[sParamName] = sArgType;
			}
		}
	}

	bool CxxTemplateSpecializationVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl)
	{
		if (m_classDefInfo.has_value())
		{
			// WTF? It's wrong case. Ignore this type at all!
			return false;
		}

		// Collect type base info (name, namespace, location, tags, inheritance)
		// NOTE: code copied from CxxClassTypeVisitor
		if (!cxxRecordDecl->isCompleteDefinition())
			return true; // skip uncompleted types

		// Extract comment
		clang::ASTContext& ctx = cxxRecordDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxRecordDecl);

		SClassDefInfo& sDef = m_classDefInfo.emplace();

		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			sDef.sTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		if (!sDef.sTags.hasTag(std::string(rg3::cpp::BuiltinTags::kRuntime)) && !m_compilerConfig.bAllowCollectNonRuntimeTypes)
		{
			// NOTE: Annotations aren't allowed here
			return true;
		}

		// Namespace
		Utils::getDeclInfo(cxxRecordDecl, sDef.sNameSpace);

		// Get name & pretty name
		sDef.sClassName       = cxxRecordDecl->getName().str();
		sDef.sPrettyClassName = Utils::getPrettyNameOfDecl(cxxRecordDecl);  // Are you sure that this shit is correct?

		// Location
		sDef.sDefLocation = Utils::getDeclDefinitionInfo(cxxRecordDecl);

		// Is struct or class?
		sDef.bIsStruct = cxxRecordDecl->isStruct();
		sDef.bTriviallyConstructible = cxxRecordDecl->hasDefaultConstructor();

		// Collect parent class list
		for (const clang::CXXBaseSpecifier& baseSpecifier : cxxRecordDecl->bases()) {
			cpp::ClassParent& parent = sDef.vParents.emplace_back();
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
					parent.eModifier = sDef.bIsStruct ? cpp::InheritanceVisibility::IV_PUBLIC : cpp::InheritanceVisibility::IV_PRIVATE;
					break;
				}
			}
		}

		return true;
	}

	bool CxxTemplateSpecializationVisitor::VisitFieldDecl(clang::FieldDecl* cxxFieldDecl)
	{
		if (!m_bHasProperties)
			return true; // Nothing to do here

		if (!m_classDefInfo.has_value())
			return true; // WTF?

		SClassDefInfo& sDef = m_classDefInfo.value();

		// Collect current type
		bool bPropertyOk = false;
		cpp::ClassProperty newProperty;
		newProperty.sAlias = newProperty.sName = cxxFieldDecl->getNameAsString();

		if (!m_propertyFilterFunc(newProperty.sName))
			return true; // Ignored

		// Fill type info (and decl info)
		if (cxxFieldDecl->getType()->isInstantiationDependentType())
		{
			// Type could be `const T*` and this will be templated, but not isTemplateTypeParmType. We need to resolve this situation correctly
			// First of all we need to locate TemplateTypeParmType - this node will be our way to link types data.
			// But 'TypeStatement' could be compiled from cxxFieldDecl->getType() because everything is ready for this.
			rg3::cpp::TypeStatement sCoreStmt; // Contains main type properties (const, ptr, const-ptr, ...)
			clang::QualType sFieldType = cxxFieldDecl->getType();

			Utils::fillTypeStatementFromQualType(sCoreStmt, sFieldType, cxxFieldDecl->getASTContext());

			// And now we need to find TemplateTypeParmType and resolve final type

			const clang::Type* pType = cxxFieldDecl->getType().getTypePtr();
			if (tryResolveTemplateType(sCoreStmt, pType, cxxFieldDecl->getASTContext()))
			{
				newProperty.sTypeInfo = std::move(sCoreStmt);
				bPropertyOk =true;
			}
		}
		else
		{
			// Easy type
			clang::QualType declType = cxxFieldDecl->getType();
			Utils::fillTypeStatementFromQualType(newProperty.sTypeInfo, declType, cxxFieldDecl->getASTContext());
			bPropertyOk = true;
		}

		// Save other info
		clang::ASTContext& ctx = cxxFieldDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxFieldDecl);
		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			newProperty.vTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		// 'property' alias override not allowed here
		newProperty.eVisibility = Utils::getDeclVisibilityLevel(cxxFieldDecl);

		// If property good enough
		if (bPropertyOk)
		{
			sDef.vProperties.emplace_back(std::move(newProperty));
		}
		else
		{
			sDef.bHasResolverErrors = true;
		}

		return true;
	}

	bool CxxTemplateSpecializationVisitor::VisitCXXMethodDecl(clang::CXXMethodDecl* cxxMethodDecl)
	{
		if (!m_bHasFunctions)
			return true; // Nothing to do here

		if (!m_classDefInfo.has_value())
			return true; // WTF?

		SClassDefInfo& sDef = m_classDefInfo.value();
		bool bIsValid = false;

		// Find ParmVarDecl and check params
		// For ret type: cxxMethodDecl->getReturnType()->isTemplateTypeParmType()
		// For arg type: cxxMethodDecl->parameters()[i]->getType()->isTemplateTypeParmType()

		// Save method info
		cpp::ClassFunction newFunction;
		newFunction.sName = cxxMethodDecl->getNameAsString();
		newFunction.bIsStatic = cxxMethodDecl->isStatic();
		newFunction.sOwnerClassName = sDef.sPrettyClassName;
		newFunction.bIsConst = cxxMethodDecl->isConst();

		if (!m_functionFilterFunc(cxxMethodDecl->getNameAsString()))
			return true; // Ignored

		clang::ASTContext& ctx = cxxMethodDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxMethodDecl);
		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			newFunction.vTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		newFunction.eVisibility = Utils::getDeclVisibilityLevel(cxxMethodDecl);

		if (cxxMethodDecl->getType()->isInstantiationDependentType())
		{
			bool bReturnTypeIsOk = false;
			bool bArgTypesAreOk = true;

			// Need process method as templated subject
			{
				rg3::cpp::TypeStatement sReturnTypeStmt; // Contains main type properties (const, ptr, const-ptr, ...)
				clang::QualType sReturnType = cxxMethodDecl->getReturnType();

				Utils::fillTypeStatementFromQualType(sReturnTypeStmt, sReturnType, cxxMethodDecl->getASTContext());

				const clang::Type* pType = cxxMethodDecl->getReturnType().getTypePtr();
				if (tryResolveTemplateType(sReturnTypeStmt, pType, cxxMethodDecl->getASTContext()))
				{
					// return type is ok
					newFunction.sReturnType = std::move(sReturnTypeStmt);
					bReturnTypeIsOk = true;
				}
			}

			// Extract function arguments
			for (auto it = cxxMethodDecl->param_begin(); bReturnTypeIsOk && it != cxxMethodDecl->param_end(); ++it)
			{
				bool bArgIsOk = false;
				const clang::ParmVarDecl* pParam = (*it);
				cpp::FunctionArgument newArgument;

				if (pParam->getType()->isInstantiationDependentType())
				{
					// Nice, it's templated, need resolve this stuff
					rg3::cpp::TypeStatement sArgTypeStmt; // Contains main type properties (const, ptr, const-ptr, ...)
					clang::QualType sArgType = pParam->getType();

					Utils::fillTypeStatementFromQualType(sArgTypeStmt, sArgType, cxxMethodDecl->getASTContext());

					const clang::Type* pArgType = cxxMethodDecl->getReturnType().getTypePtr();
					if (tryResolveTemplateType(sArgTypeStmt, pArgType, cxxMethodDecl->getASTContext()))
					{
						// return type is ok
						newArgument.sType = std::move(sArgTypeStmt);
						bArgIsOk = true;
					}
				}
				else
				{
					// Extract type info
					rg3::llvm::Utils::fillTypeStatementFromQualType(newArgument.sType, pParam->getType(), ctx);

					// Save arg name
					newArgument.sArgumentName = pParam->getNameAsString();

					// Save info about default value
					newArgument.bHasDefaultValue = pParam->hasDefaultArg();

					bArgIsOk = true;
				}

				if (!bArgIsOk)
				{
					bArgTypesAreOk = false;
					break; // No need to process whole type here
				}
				else
				{
					newFunction.vArguments.emplace_back(std::move(newArgument));
				}
			}

			bIsValid = bReturnTypeIsOk && bArgTypesAreOk;
		}
		else
		{
			// Trivial class method
			// Return type
			rg3::llvm::Utils::fillTypeStatementFromQualType(newFunction.sReturnType,
															cxxMethodDecl->getReturnType(),
															cxxMethodDecl->getASTContext());

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

			bIsValid = true;
		}

		if (bIsValid)
		{
			sDef.vFunctions.emplace_back(std::move(newFunction));
		}
		else
		{
			sDef.bHasResolverErrors = true;
		}

		return true;
	}

	const std::optional<SClassDefInfo>& CxxTemplateSpecializationVisitor::getClassDefInfo() const
	{
		return m_classDefInfo;
	}

	bool CxxTemplateSpecializationVisitor::tryResolveTemplateType(rg3::cpp::TypeStatement& stmt, const clang::Type* pOriginalType, clang::ASTContext& ctx) const
	{
		if (!pOriginalType)
			return false;

		const clang::TemplateTypeParmType* pTemplateTypeParam = nullptr; // Will be found here

		if (const clang::PointerType* pAsPtr = ::llvm::dyn_cast<clang::PointerType>(pOriginalType))
		{
			pOriginalType = pAsPtr->getPointeeType().getTypePtr();
		}

		// And find template stub
		pTemplateTypeParam = ::llvm::dyn_cast<clang::TemplateTypeParmType>(pOriginalType);
		if (pTemplateTypeParam)
		{
			// Extract template arg name
			const std::string& sTemplateArgName = pTemplateTypeParam->getDecl()->getNameAsString();

			if (m_mTemplateParamNameToInstantiatedType.contains(sTemplateArgName))
			{
				const clang::QualType& qualFinalType = m_mTemplateParamNameToInstantiatedType.at(sTemplateArgName);

				rg3::cpp::TypeStatement sTargetStmt;
				Utils::fillTypeStatementFromQualType(sTargetStmt, qualFinalType, ctx);

				// Override type name & location in sTargetStmt from sCoreStmt
				stmt.sTypeRef = sTargetStmt.sTypeRef;
				stmt.sDefinitionLocation = sTargetStmt.sDefinitionLocation;
				stmt.bIsConst = sTargetStmt.bIsConst || stmt.bIsConst;
				stmt.bIsPointer = sTargetStmt.bIsPointer || stmt.bIsPointer;
				stmt.bIsPtrConst = sTargetStmt.bIsPtrConst || stmt.bIsPtrConst;
				stmt.bIsReference = sTargetStmt.bIsReference || stmt.bIsReference;
				stmt.bIsTemplateSpecialization = sTargetStmt.bIsTemplateSpecialization || stmt.bIsTemplateSpecialization;

				return true;
			}
		}

		return false;
	}
}