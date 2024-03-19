#include <RG3/LLVM/Visitors/CxxClassTypeVisitor.h>
#include <RG3/LLVM/Visitors/CxxTypeVisitor.h>
#include <RG3/Cpp/BuiltinTags.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/LLVM/Utils.h>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/Decl.h>

#include <boost/algorithm/string.hpp>


namespace rg3::llvm::visitors
{
	struct ExtraPropertiesFilter
	{
		const std::vector<CxxClassTypeVisitor::PropertyDescription>& vKnownProperties;

		bool operator()(const std::string& sPropertyName) const
		{
			auto it = std::find_if(vKnownProperties.begin(), vKnownProperties.end(), [&sPropertyName](const CxxClassTypeVisitor::PropertyDescription& pd) -> bool {
				return pd.propertyRefName == sPropertyName;
			});

			return it != vKnownProperties.end();
		}
	};

	struct ExtraFunctionsFilter
	{
		const std::vector<std::string>& vKnownFunctions;

		bool operator()(const std::string& sFuncName) const
		{
			return std::find(vKnownFunctions.begin(), vKnownFunctions.end(), sFuncName) != vKnownFunctions.end();
		}
	};

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
		if (!cxxRecordDecl->isCompleteDefinition())
			return true; // skip uncompleted types

		// Extract comment
		clang::ASTContext& ctx = cxxRecordDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(cxxRecordDecl);

		if (rawComment)
		{
			const auto rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
			vTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		bool bHasAnnotateAttribute = cxxRecordDecl->hasAttr<clang::AnnotateAttr>();
		clang::AnnotateAttr* pAnnotateAttr = bHasAnnotateAttribute ? cxxRecordDecl->getAttr<clang::AnnotateAttr>() : nullptr;

		if (!vTags.hasTag(std::string(rg3::cpp::BuiltinTags::kRuntime)) && !compilerConfig.bAllowCollectNonRuntimeTypes)
		{
			// Allowed to have explicit specialization of any template subject. Anyway, our target is first template argument
			if (bHasAnnotateAttribute && pAnnotateAttr)
			{
				handleTypeAnnotation(cxxRecordDecl, pAnnotateAttr);
			}

			// Finish
			return true;
		}

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

	void CxxClassTypeVisitor::handleTypeAnnotation(clang::CXXRecordDecl* cxxRecordDecl, clang::AnnotateAttr* pAnnotateAttr) // NOLINT(*-convert-member-functions-to-static)
	{
		/**
		 * Concept:
		 *
		 * Annotations is a explicit mechanism to declare types from third party code without changes inside that code.
		 * You able to create a special header for that types and use standard (for clang) attributes to declare third party types as 'runtime'
		 *
		 * Example:
		 *
		 * struct SampleStruct {
		 *   int x;
		 *   int y;
		 * };
		 *
		 * template <typename T> struct RG3RegisterType {};
		 * template <> struct
		 *     __attribute__((annotate("RG3_RegisterRuntime")))
		 *     __attribute__((annotate("RG3_RegisterField[x]")))
		 *     __attribute__((annotate("RG3_RegisterField[y:Y]")))
		 * RG3RegisterType<SampleStruct> { using Type = SampleStruct; };
		 *
		 * So, here we have a few points:
		 * 1. Struct defined somewhere else (for example we can see it)
		 * 2. We have template struct RG3RegisterType<T>. It can have body or not, it's doesn't matter.
		 * 3. We have template specialization of RG3RegisterType for type SampleStruct (RG3RegisterType<SampleStruct>)
		 *    and this template specialization contains few annotations:
		 *    	1. RG3_RegisterRuntime - this attribute is a trigger for RG3 to incldue this type into 'post' stage
		 *    	2. RG3_RegisterField - allowed to have this only with structures. It store information about field (original name and it's alias (optional))
		 *    that annotations helps RG3 to generate basic information about type.
		 * 4. Inside RG3RegisterType<SampleStruct> we have "using Type = SampleStruct", it was made to avoid of "naked types" in case when <T> is alias to something (otherwise it will be replaced to final type)
		 *
		 * We always must remember about those requirements:
		 * 	1. It will not work without template
		 * 	2. It will not work without "using Type = T"
		 * 	3. It will not work without "__attribute__((annotate("RG3_RegisterRuntime")))"
		 * 	4. It will not work if "runtime" tag declared for that type
		 *
		 * So, minimum example is:
		 * template <typename T> struct RG3RegisterType;
		 * template <> struct __attribute__((annotate("RG3_RegisterRuntime"))) RG3RegisterType<SampleStruct> {
		 *   using Type = SampleStruct;
		 * };
		 *
		 * A better example:
		 *
		 * template <typename T> struct RG3Register;
		 * template <> struct __attribute__((annotate("RG3_RegisterRuntime"))) RG3Register<SampleStruct> final : public RG3Base<SampleStruct> {
		 *   using Type = SampleStruct;
		 * };
		 *
		 * REMEMBER: It won't work if you will apply inheritance or other 'implicit declarations' here [Mark for v0.0.4: Maybe we will support all forms of decls?]
		 *
		 * NOTE: Annotations won't work if you will make template arguments pack like
		 *
		 * template <typename... TArgs> struct RG3Register;
		 * template <> struct RG3Register<int> {};
		 *
		 * ^^^ it will not work because this semantics works with "template pack argument" instead of "template type argument".
		 *     we won't fix this behaviour in v0.0.x (maybe in v1.x.x, idk).
		 *
		 * When RG3 meets annotation RG3_RegisterRuntime it trying to find template arguments.
		 * RG3 expect to have 1 template argument and it works with it.
		 * Then RG3 trying to locate "Type" alias declaration. "typedef" semantics supported too.
		 *
		 * Algorithm is simple:
		 *
		 * 	If typedef -> use type merger (see handleAndMergeTypedefTypeWithInner)
		 * 	If enum -> handle type
		 * 	If builtin -> handle builtin
		 * 	If class/struct -> handle directly
		 */
		if (static_cast<std::string_view>(pAnnotateAttr->getAnnotation()) == cpp::BuiltinAnnotations::kRegisterRuntime &&
			cxxRecordDecl->getTemplateSpecializationKind() == clang::TemplateSpecializationKind::TSK_ExplicitSpecialization &&
			cxxRecordDecl->getNumTemplateParameterLists() > 0)
		{
			auto* ir = ::llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cxxRecordDecl);
			if (!ir || !ir->getTemplateArgs().size())
			{
				return; // Impossible
			}

			const auto& templateArgument0 = ir->getTemplateArgs().get(0); // T1
			if (auto templateArgKind = templateArgument0.getKind(); templateArgKind == clang::TemplateArgument::ArgKind::Type)
			{
				// Only single types allowed
				clang::QualType typeToRegister {};  // T2
				bool bTypeToRegisterFound { false }; // "Type" chunk found flag

				for (const auto* pDecl : ir->decls())
				{
					if (const auto* pTypeAliasDecl = ::llvm::dyn_cast<clang::TypeAliasDecl>(pDecl))
					{
						if (pTypeAliasDecl->getNameAsString() == "Type")
						{
							// Found original type ref
							typeToRegister = pTypeAliasDecl->getUnderlyingType();
							bTypeToRegisterFound = true;
							break;
						}
					}

					if (const auto* pTypeDefDecl = ::llvm::dyn_cast<clang::TypedefDecl>(pDecl))
					{
						if (pTypeDefDecl->getNameAsString() == "Type")
						{
							// Found original type ref
							typeToRegister = pTypeDefDecl->getUnderlyingType();
							bTypeToRegisterFound = true;
							break;
						}
					}
				}

				if (!bTypeToRegisterFound)
				{
					// No type found, decl is invalid
					return;
				}

				// Try to locate properties
				std::vector<std::string> vFunctionsToCollect {};
				std::vector<PropertyDescription> vExtraProperties {};
				rg3::cpp::Tags sExtraTags;

				for (const auto* pAttr : cxxRecordDecl->attrs())
				{
					if (const auto* pAnnotationAttr = ::llvm::dyn_cast<clang::AnnotateAttr>(pAttr))
					{
						const auto annotation = static_cast<std::string_view>(pAnnotationAttr->getAnnotation());

						if (annotation.starts_with(rg3::cpp::BuiltinAnnotations::kRegisterField))
						{
							// Here we have annotation of RG3_RegisterField[X:Y] where X - original name, Y - new name.
							// We just need to split string by ':' and remove first char in first string and last char in last string
							std::vector<std::string> splitResult;
							boost::algorithm::split(splitResult, annotation, boost::is_any_of("[]:"));

							if (splitResult.size() == 3 || splitResult.size() == 4)
							{
								// Vlaid in this case
								splitResult.erase(splitResult.begin()); // remove first element

								if (splitResult.size() == 3)
								{
									splitResult.erase(std::prev(splitResult.end()));
								}
							}

							if (!splitResult.empty())
							{
								// Nice, ready to save
								PropertyDescription& newProperty = vExtraProperties.emplace_back();
								newProperty.propertyRefName = splitResult[0];
								newProperty.propertyAliasName = (splitResult.size() > 1) ? splitResult[1] : splitResult[0];
							}
						}
						else if (annotation.starts_with(rg3::cpp::BuiltinAnnotations::kRegisterFunction))
						{
							// Here we have annotation RG3_RegisterFunction[X] where X - function name to register.
							// NOTE: This part of code can't handle function overloads & other things. It means that code will have UB
							std::vector<std::string> splitResult;
							boost::algorithm::split(splitResult, annotation, boost::is_any_of("[]"));

							if (splitResult.size() >= 2)
							{
								vFunctionsToCollect.emplace_back(splitResult[1]); // #0 - tag, #1 - func name
							}
						}
						else if (annotation.starts_with(rg3::cpp::BuiltinAnnotations::kRegisterTag))
						{
							// Here we have annotation of RG3_RegisterTag[X] where X - tag declaration.
							std::vector<std::string> splitResult;
							boost::algorithm::split(splitResult, annotation, boost::is_any_of("[]"));

							if (splitResult.size() >= 2)
							{
								// Tag contents stored at arg #1
								// splitResult[1]
								const auto tags = rg3::cpp::Tag::parseFromCommentString(splitResult[1]);
								for (const auto& [name, tag] : tags.getTags())
								{
									sExtraTags.getTags()[name] = tag;
								}
							}
						}
					}
				}

				// Type found, need to detect what is it. There are 4 options: BuiltinType, EnumType, RecordType and TypedefType
				const clang::Type* pType = typeToRegister.getTypePtrOrNull();

				if (!pType) return;

				// First of all we've trying to locate TypeAlias node. If it's located - use a main type and lookup from this place.
				// If first type is Enum, CXXRecord or BuiltinType - start collect from this
				const clang::TypedefType* pAsTypedef = pType->getAs<clang::TypedefType>();
				const size_t iTypesBeforeHandled = vFoundExtraTypes.size(); // how much types was before we've handled situation
				bool bHandled = false;

				if (pAsTypedef)
				{
					// Use merger for typedef
					handleAndMergeTypedefTypeWithInner(pAsTypedef, vExtraProperties, vFunctionsToCollect);
					bHandled = true;
				}

				if (!bHandled)
				{
					if (auto pAsEnum = pType->getAs<clang::EnumType>())
					{
						// It's enum.
						// We've ready to handle it because it's pretty easy thing
						// Ok, we know that visitor will try to collect type only if it's marked as 'runtime' or if any type allowed to be collected
						// So, I'll use second option
						rg3::llvm::CompilerConfig newConfig = compilerConfig;
						newConfig.bAllowCollectNonRuntimeTypes = true; // Yep, it's hack
						rg3::llvm::visitors::CxxTypeVisitor sTypeVisitor { vFoundExtraTypes, newConfig };
						sTypeVisitor.VisitEnumDecl(pAsEnum->getDecl());

						// Pretty awesome, enum ready
						bHandled = true;
					}
				}

				if (!bHandled)
				{
					if (auto pAsCxxRecordType = pType->getAs<clang::RecordType>())
					{
						// It's record type (actually CXX record)
						// See comments for enum handling code for more details about this creepy shit
						rg3::llvm::CompilerConfig newConfig = compilerConfig;
						newConfig.bAllowCollectNonRuntimeTypes = true; // Yep, it's hack
						CxxClassTypeVisitor sTypeVisitor { newConfig };

						// Ok, let's run
						if (auto pCxxDecl = ::llvm::dyn_cast<clang::CXXRecordDecl>(pAsCxxRecordType->getDecl()))
						{
							sTypeVisitor.VisitCXXRecordDecl(pCxxDecl);

							// Handle fields & methods
							handleCxxDeclAndOverridePropertiesOwner(pCxxDecl,
																	nullptr,  // Don't override anything
																	sTypeVisitor,
																	ExtraPropertiesFilter { vExtraProperties },
																	ExtraFunctionsFilter { vFunctionsToCollect });

							// And store finalized type here
							vFoundExtraTypes.emplace_back(
								std::make_unique<rg3::cpp::TypeClass>(
									sTypeVisitor.sClassName,
									sTypeVisitor.sClassPrettyName,
									sTypeVisitor.sNameSpace,
									sTypeVisitor.sDefinitionLocation,
									sTypeVisitor.vTags,
									sTypeVisitor.foundProperties,
									sTypeVisitor.foundFunctions,
									sTypeVisitor.bIsStruct,
									sTypeVisitor.bTriviallyConstructible,
									sTypeVisitor.parentClasses
								)
							);

							bHandled = true;
						}
					}
				}

				if (!bHandled)
				{
					if (auto* pAsBuiltinType = pType->getAs<clang::BuiltinType>())
					{
						// It's builtin, just register a type
						// Our builtins are store as generic rg3::cpp::TypeBase instances
						// It's a literally single way to register trivial type into runtime
						// NOTE: Move this impl into another place later
						clang::PrintingPolicy typeNamePrintingPolicy { cxxRecordDecl->getASTContext().getLangOpts() };
						typeNamePrintingPolicy.SuppressTagKeyword = true;
						typeNamePrintingPolicy.SuppressScope = false;

						const std::string kName { rg3::cpp::BuiltinTags::kRuntime };
						std::string typeName { pAsBuiltinType->getNameAsCString(typeNamePrintingPolicy) };
						rg3::cpp::CppNamespace sNamespace {}; // always empty in this case
						rg3::cpp::DefinitionLocation definitionLocation {}; // always empty in this case
						rg3::cpp::Tags tags {{ rg3::cpp::Tag { kName } }};

						vFoundExtraTypes.emplace_back(
							std::make_unique<rg3::cpp::TypeBase>(
								rg3::cpp::TypeKind::TK_TRIVIAL,
								typeName, typeName, // typeName and pretty name are same in this case
								sNamespace, definitionLocation, tags
							)
						);
					}
				}

				if (bHandled)
				{
					// POST PROCESSING:
					// Ok, now we've need to know which types added
					const bool bSmthAdded = iTypesBeforeHandled < vFoundExtraTypes.size();
					if (bSmthAdded)
					{
						const std::string kName { rg3::cpp::BuiltinTags::kRuntime };

						for (size_t i = iTypesBeforeHandled; i < vFoundExtraTypes.size(); ++i)
						{
							// Need to add tag if it's not added before
							vFoundExtraTypes[i]->getTags().getTags()[kName] = rg3::cpp::Tag { kName };

							// If type is struct - need to check that we should override properties
							// Oh, and it's struct - override fields by their names
							if (vFoundExtraTypes[i]->getKind() == rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS && !vExtraProperties.empty())
							{
								// Override fields
								auto& properties = reinterpret_cast<cpp::TypeClass*>(vFoundExtraTypes[i].get())->getProperties();

								// Yep, really stupid shit, will fix later (or won't, idk)
								for (const auto& [propertyOriginalName, propertyAliasName] : vExtraProperties)
								{
									if (auto propToOverrideIt = std::find_if(
											properties.begin(),
											properties.end(),
											[&propertyOriginalName](const rg3::cpp::ClassProperty& property) -> bool {
												return property.sName == propertyOriginalName;
											});
										propToOverrideIt != properties.end()
									)
									{
										propToOverrideIt->sAlias = propertyAliasName;
									}
								}
							}

							// Add extra tags
							// NOTE: It's weird but we've thinking that could be added only 1 entry. It's a little buggy behaviour, but I can't find case when it fails.
							//       Anyway, I'll think about this later
							for (const auto& [name, tag] : sExtraTags.getTags())
							{
								// Override in any case
								vFoundExtraTypes[i]->getTags().getTags()[name] = tag;
							}
						}
					}
				}
			}
		}
	}

	void CxxClassTypeVisitor::handleAndMergeTypedefTypeWithInner(const clang::TypedefType* pType, const std::vector<PropertyDescription>& vKnownProperties, const std::vector<std::string>& vKnownFunctions)
	{
		// Nice day, huh? Let's talk about type merge process.
		// Common rules:
		//    1. Name, namespace, pretty name, declaration location, tags - use from typedef decl
		//    2. Kind (class, struct, enum, builtin) - use from target type
		//    3. Combine that data into a single type of specific kind
		// We've able to use pretty simple approach: construct alias, then construct inner type, then combine 'em all
		const std::string kRuntimeTagName { rg3::cpp::BuiltinTags::kRuntime };

		// Stage 1: extract alias type info
		rg3::cpp::TypeBasePtr pTypeDefInfo { nullptr };
		rg3::cpp::TypeBasePtr pFinalType { nullptr };

		{
			std::vector<rg3::cpp::TypeBasePtr> vFoundTypeDefs {};
			rg3::llvm::CompilerConfig newConfig = compilerConfig;
			newConfig.bAllowCollectNonRuntimeTypes = true;
			vFoundTypeDefs.reserve(1); //expected to have 1 type
			rg3::llvm::visitors::CxxTypeVisitor sTypeDefVisitor { vFoundTypeDefs, newConfig };
			sTypeDefVisitor.VisitTypedefNameDecl(pType->getDecl());

			if (!vFoundTypeDefs.empty())
			{
				pTypeDefInfo = std::move(vFoundTypeDefs[0]);
			}
		}

		if (!pTypeDefInfo) return; // type not found

		// Stage 2: collect info about inner type
		auto* pAsBuiltin = pType->getAs<clang::BuiltinType>();
		if (pAsBuiltin)
		{
			// We have a trivial type
			pFinalType = std::make_unique<rg3::cpp::TypeBase>(rg3::cpp::TypeKind::TK_TRIVIAL,
															  pTypeDefInfo->getName(),
															  pTypeDefInfo->getPrettyName(),
															  pTypeDefInfo->getNamespace(),
															  pTypeDefInfo->getDefinition(),
															  pTypeDefInfo->getTags());

			vFoundExtraTypes.emplace_back(std::move(pFinalType));
			return;
		}

		auto* pAsEnum = pType->getAs<clang::EnumType>();
		if (pAsEnum)
		{
			// We have Enum
			std::vector<rg3::cpp::TypeBasePtr> vFoundTypes {};
			rg3::llvm::CompilerConfig newConfig = compilerConfig;
			newConfig.bAllowCollectNonRuntimeTypes = true;

			rg3::llvm::visitors::CxxTypeVisitor sEnumVisitor { vFoundTypes, newConfig };
			sEnumVisitor.VisitEnumDecl(pAsEnum->getDecl());

			if (!vFoundTypes.empty())
			{
				auto* asEnum = reinterpret_cast<const rg3::cpp::TypeEnum*>(vFoundTypes[0].get());

				vFoundExtraTypes.emplace_back(
					std::make_unique<rg3::cpp::TypeEnum>(
						pTypeDefInfo->getName(),
						pTypeDefInfo->getPrettyName(),
						pTypeDefInfo->getNamespace(),
						pTypeDefInfo->getDefinition(),
						vFoundTypes[0]->getTags(),
						asEnum->getEntries(),
						asEnum->isScoped(),
						asEnum->getUnderlyingType()
					)
				);
			}

			return;
		}

		auto* pAsRecord = pType->getAs<clang::RecordType>();
		if (pAsRecord)
		{
			// We have class or struct
			rg3::llvm::CompilerConfig newConfig = compilerConfig;
			newConfig.bAllowCollectNonRuntimeTypes = true;

			CxxClassTypeVisitor sClassVisitor { newConfig };

			if (auto* asCxxRecord = ::llvm::dyn_cast<clang::CXXRecordDecl>(pAsRecord->getDecl()))
			{
				sClassVisitor.VisitCXXRecordDecl(asCxxRecord);

				if (!sClassVisitor.sClassName.empty())
				{
					// Really found. Now we need to iterate over decls and locate methods and fields
					handleCxxDeclAndOverridePropertiesOwner(asCxxRecord,
															pTypeDefInfo.get(),
															sClassVisitor,
															ExtraPropertiesFilter { vKnownProperties },
															ExtraFunctionsFilter { vKnownFunctions });

					// And store a new type
					vFoundExtraTypes.emplace_back(
						std::make_unique<rg3::cpp::TypeClass>(
							pTypeDefInfo->getName(),
							pTypeDefInfo->getPrettyName(),
							pTypeDefInfo->getNamespace(),
							pTypeDefInfo->getDefinition(),
							pTypeDefInfo->getTags(),
							sClassVisitor.foundProperties,
							sClassVisitor.foundFunctions,
							sClassVisitor.bIsStruct,
							sClassVisitor.bTriviallyConstructible,
							sClassVisitor.parentClasses
						)
					);
				}
			}

			return;
		}
	}

	void CxxClassTypeVisitor::handleCxxDeclAndOverridePropertiesOwner(const clang::CXXRecordDecl* pCxxDecl,
																	  const rg3::cpp::TypeBase* pNewOwnerType,
																	  rg3::llvm::visitors::CxxClassTypeVisitor& sTypeVisitor,
																	  const std::function<bool(const std::string&)>& propFilter,
																	  const std::function<bool(const std::string&)>& funcFilter)
	{
		for (auto* pDecl : pCxxDecl->decls())
		{
			if (auto* pFieldDecl = ::llvm::dyn_cast<clang::FieldDecl>(pDecl))
			{
				const size_t kPropertiesAmountBeforeHandle = sTypeVisitor.foundProperties.size();
				sTypeVisitor.VisitFieldDecl(pFieldDecl);

				// Check field, if it's not known - remove from list
				if (kPropertiesAmountBeforeHandle < sTypeVisitor.foundProperties.size())
				{
					const auto& addedProperty = sTypeVisitor.foundProperties.back();

					if (propFilter && !propFilter(addedProperty.sName))
					{
						// Not found or not allowed
						sTypeVisitor.foundProperties.pop_back();
					}
				}
			}

			if (auto* pCXXMethodDecl = ::llvm::dyn_cast<clang::CXXMethodDecl>(pDecl))
			{
				const size_t kFunctionsAmountBeforeHandle = sTypeVisitor.foundFunctions.size();
				sTypeVisitor.VisitCXXMethodDecl(pCXXMethodDecl);

				// Check method, if it's not known - remove from list
				if (kFunctionsAmountBeforeHandle < sTypeVisitor.foundFunctions.size())
				{
					const auto& addedFunction = sTypeVisitor.foundFunctions.back();

					if (funcFilter && !funcFilter(addedFunction.sName))
					{
						// Function not found - remove last
						sTypeVisitor.foundFunctions.pop_back();
					}
					else if (pNewOwnerType != nullptr)
					{
						if (sTypeVisitor.foundFunctions.back().sOwnerClassName == sTypeVisitor.sClassName)
						{
							// Override owner class from old type to the new type
							sTypeVisitor.foundFunctions.back().sOwnerClassName = pNewOwnerType->getName();
						}
					}
				}
			}
		}
	}
}