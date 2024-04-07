#include <RG3/LLVM/Visitors/CxxRouterVisitor.h>
#include <RG3/LLVM/Visitors/CxxTemplateSpecializationVisitor.h>  // template specialization (class, struct)
#include <RG3/LLVM/Visitors/CxxClassTypeVisitor.h> // class or struct without templates
#include <RG3/LLVM/Visitors/CxxTypeVisitor.h> // simple types (enum, builtin)
#include <RG3/LLVM/Annotations.h>
#include <RG3/LLVM/Utils.h>

#include <RG3/Cpp/DefinitionLocation.h>
#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/BuiltinTags.h>
#include <RG3/Cpp/Tag.h>

#include <clang/AST/ASTContext.h>

#include <fmt/format.h>


namespace rg3::llvm::visitors
{
	CxxRouterVisitor::CxxRouterVisitor(std::vector<rg3::cpp::TypeBasePtr>& vFoundTypes, const CompilerConfig& compilerConfig)
		: m_compilerConfig(compilerConfig), m_vFoundTypes(vFoundTypes)
	{
	}

	bool CxxRouterVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl)
	{
		// Here we need to know is type templated or not
		if (auto kind = cxxRecordDecl->getKind(); kind == clang::Decl::Kind::CXXRecord || kind == clang::Decl::Kind::Record)
		{
			// Class or struct (generic)
			// Let's run simple visitor for this
			if (!cxxRecordDecl->isTemplated())
			{
				CxxClassTypeVisitor visitor { m_compilerConfig };
				visitor.TraverseDecl(cxxRecordDecl);

				if (!visitor.sClassName.empty())
				{
					m_vFoundTypes.emplace_back(
						std::make_unique<cpp::TypeClass>(
							visitor.sClassName,
							visitor.sClassPrettyName,
							visitor.sNameSpace,
							visitor.sDefinitionLocation,
							visitor.vTags,
							visitor.foundProperties,
							visitor.foundFunctions,
							visitor.bIsStruct,
							visitor.bTriviallyConstructible,
							visitor.parentClasses
						)
					);
				}
			} // otherwise it's declaration of template, not specialization
		}
		else if (kind == clang::Decl::Kind::ClassTemplateSpecialization)
		{
			Annotations sAnnotation;
			sAnnotation.collectFromDecl(cxxRecordDecl);

			if (sAnnotation.isRuntime())
			{
				// It's root template! Here we need to find "using Type = ..." and extract type of "Type" and run again
				// Nice, we ready to register this type
				if (auto* pSpecDecl = ::llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cxxRecordDecl))
				{
					clang::QualType typeToRegister{};// T2
					bool bTypeToRegisterFound{false};// "Type" chunk found flag

					const auto& templateArgument0 = pSpecDecl->getTemplateArgs().get(0); // T1
					if (auto templateArgKind = templateArgument0.getKind(); templateArgKind == clang::TemplateArgument::ArgKind::Type)
					{
						// Only single types allowed
						for (const auto* pDecl : pSpecDecl->decls())
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
					}

					if (!bTypeToRegisterFound)
					{
						// No type found, decl is invalid
						return true;
					}

					// Type found, need to detect what is it. There are 4 options: BuiltinType, EnumType, RecordType and TypedefType
					const clang::Type* pTypeToRegister = typeToRegister.getTypePtrOrNull();

					if (!pTypeToRegister)
					{
						// No way
						return true;
					}

					handleAnnotationBasedType(pTypeToRegister, sAnnotation, cxxRecordDecl->getASTContext(), true);
				}
			}
		}
		// other options not interested now

		return true;
	}

	bool CxxRouterVisitor::VisitEnumDecl(clang::EnumDecl* enumDecl)
	{
		// Handle simple enum
		CxxTypeVisitor visitor { m_vFoundTypes, m_compilerConfig };
		visitor.VisitEnumDecl(enumDecl);
		return true;
	}

	bool CxxRouterVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl)
	{
		// Breaking changes from 0.0.2 to 0.0.3: Now all using/typedef instructions produce target type with replaced name. And when type has no any registration points.
		// So, our typedef must contain runtime tag in this case
		clang::ASTContext& ctx = typedefNameDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		cpp::Tags typeTags;
		bool bHasComment = false;

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(typedefNameDecl);
		if (rawComment)
		{
			std::string rawCommentStr = rawComment->getRawText(sm).data();
			bHasComment = !rawCommentStr.empty();
			typeTags = cpp::Tag::parseFromCommentString(rawCommentStr);
		}

		if ((!bHasComment || !typeTags.hasTag(std::string(cpp::BuiltinTags::kRuntime))) && !m_compilerConfig.bAllowCollectNonRuntimeTypes)
		{
			// Ignore this type
			return true;
		}

		// Extract name, namespace and location of this typedef/using
		std::string typedefPrettyName = Utils::getPrettyNameOfDecl(typedefNameDecl);
		cpp::CppNamespace typedefNamespace {};
		cpp::DefinitionLocation typedefLocation = Utils::getDeclDefinitionInfo(typedefNameDecl);

		Utils::getDeclInfo(typedefNameDecl, typedefNamespace);

		// So, our type is registrable (by runtime tag or by flag in compiler config)
		clang::QualType    pUnderlyingType = typedefNameDecl->getUnderlyingType();

		if (pUnderlyingType->isRecordType())
		{
			// Need to know is it template or not
			clang::RecordDecl* pUnderlyingDecl = pUnderlyingType->getAsRecordDecl();

			if (auto kind = pUnderlyingDecl->getKind(); kind == clang::Decl::Kind::ClassTemplateSpecialization)
			{
				// It's template specialization. Need handle this properly & replace root part
				// And weird part is: we don't have any access to inner properties & functions because it could not be defined like other thing.
				// And we can't handle this by 'runtime tags' because it's template specialization and no tags here. Actually, we can jump to source template and map... idk.
				// Now we need to find ClassTemplateSpecializationDecl here
				if (auto* pSpecDecl = ::llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(pUnderlyingDecl))
				{
					if (auto* pClassTemplateDecl = pSpecDecl->getSpecializedTemplate())
					{
						clang::Decl* pTargetDecl = nullptr;

						for (auto* pDecl : pClassTemplateDecl->redecls())
						{
							if (auto* classTemplateDecl = ::llvm::dyn_cast<clang::ClassTemplateDecl>(pDecl))
							{
								if (classTemplateDecl->isThisDeclarationADefinition())
								{
									pTargetDecl = classTemplateDecl;
									break;
								}
							}
						}

						if (pTargetDecl)
						{
							auto newConfig = m_compilerConfig;
							newConfig.bAllowCollectNonRuntimeTypes = true; // allow to read type without runtime tag

							CxxTemplateSpecializationVisitor visitor { newConfig, pSpecDecl, false, false, nullptr, nullptr };
							visitor.TraverseDecl(pTargetDecl);

							if (visitor.getClassDefInfo().has_value())
							{
								const auto& sClassDefInfo = visitor.getClassDefInfo().value();

								m_vFoundTypes.emplace_back(
									std::make_unique<cpp::TypeClass>(
										typedefNameDecl->getNameAsString(),  // I'm not sure that this is correct.
										typedefPrettyName,
										typedefNamespace,
										typedefLocation,
										typeTags,
										sClassDefInfo.vProperties,
										sClassDefInfo.vFunctions,
										sClassDefInfo.bIsStruct,
										sClassDefInfo.bTriviallyConstructible,
										sClassDefInfo.vParents
										)
								);
							}
						}
					}
				}
			}
			else if (kind == clang::Decl::Kind::CXXRecord || kind == clang::Decl::Kind::Record)
			{
				// It's struct or class, but inside using/typedef. Need to rethink about how to handle that things properly
				auto newConfig = m_compilerConfig;
				newConfig.bAllowCollectNonRuntimeTypes = true; // allow to read type without runtime tag

				CxxClassTypeVisitor visitor { newConfig };
				visitor.TraverseDecl(pUnderlyingDecl);

				if (!visitor.sClassName.empty())
				{
					m_vFoundTypes.emplace_back(
						std::make_unique<cpp::TypeClass>(
							typedefNameDecl->getNameAsString(),  // I'm not sure that this is correct.
							typedefPrettyName,
							typedefNamespace,
							typedefLocation,
							typeTags,
							visitor.foundProperties,
							visitor.foundFunctions,
							visitor.bIsStruct,
							visitor.bTriviallyConstructible,
							visitor.parentClasses
						)
					);
				}
			}
		}
		else if (pUnderlyingType->isEnumeralType() || pUnderlyingType->isScopedEnumeralType())
		{
			// It's enum
			if (auto* pAsEnum = pUnderlyingType->getAs<clang::EnumType>())
			{
				clang::EnumDecl* pEnumDecl = pAsEnum->getDecl();
				if (pEnumDecl)
				{
					// Allow to read enum without runtime tag
					auto newConfig = m_compilerConfig;
					newConfig.bAllowCollectNonRuntimeTypes = true;

					std::vector<cpp::TypeBasePtr> vTypes;
					CxxTypeVisitor visitor { vTypes, newConfig };
					visitor.VisitEnumDecl(pEnumDecl);

					if (!vTypes.empty())
					{
						vTypes[0]->overrideTypeData(
							typedefNameDecl->getNameAsString(),
							typedefPrettyName,
							typedefNamespace,
							typedefLocation,
							typeTags
						);

						m_vFoundTypes.emplace_back(std::move(vTypes[0]));
					}
				}
			}
		}
		// Other cases?

		return true;
	}

	bool CxxRouterVisitor::handleAnnotationBasedType(const clang::Type* pType, const rg3::llvm::Annotations& annotation, const clang::ASTContext& ctx, bool bDirectInvoke) // NOLINT(*-no-recursion)
	{
		/**
		 * Annotations is an explicit mechanism to declare types from third party code without changes inside that code.
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
		 *    and this template specialization contains another annotations (see BuiltinAnnotations for details)
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
		 * Algorithm:
		 * 	If typedef -> use type merger (see handleAndMergeTypedefTypeWithInner)
		 * 	If enum -> handle type
		 * 	If builtin -> handle builtin
		 * 	If class/struct -> check is is template specialization or not. Handle as is
		 */
		bool bHandled = false; // is type variation handler or not

		// Must be first: it's because typedef can contain other options! So, we need to run recursively this stub
		if (pType->isTypedefNameType())
		{
			// It's typedef based declaration. Need handle this properly
			// Need check what's inside that typedef
			// if builtin - extract base data
			// if enum - extract enum data
			// if record - extract base info & check template specs
			// 1) need to extract type info. We've able to use our visitor in separated instance
			if (const clang::TypedefType* pAsTypedef = pType->getAs<clang::TypedefType>())
			{
				if (const clang::TypedefNameDecl* pAsTypedefDecl = pAsTypedef->getDecl())
				{
					// Here we need to extract pointee type and extract type information about that type
					// Then we need to override name, namespace, location, tags & pretty name of type and save it
					// Here we need to understand what kind of type stored inside:
					clang::QualType underlyingType = pAsTypedefDecl->getUnderlyingType();
					const clang::Type* pInnerType = underlyingType.getTypePtrOrNull();

					// Type is handled here
					const size_t iKnownTypes = m_vFoundTypes.size();

					// In my case pAsTypedefDecl contains correct pointer in glm case, but... it's weird. Anyway I'm unable to deconstruct this inside that func
					bHandled = handleAnnotationBasedType(pInnerType, annotation, ctx, false);

					// Ok, last inserted type is our new type (?)
					if (bHandled && iKnownTypes < m_vFoundTypes.size())
					{
						// Ok, type found & registered. For all types we need to override type name & locations
						cpp::TypeBase* pAddedType = m_vFoundTypes[iKnownTypes].get();

						const std::string sOldClassName = pAddedType->getPrettyName();
						const std::string & newName = pAsTypedef->getDecl()->getNameAsString();
						const std::string & prettyName = Utils::getPrettyNameOfDecl(pAsTypedef->getDecl());

						cpp::DefinitionLocation defLoc = Utils::getDeclDefinitionInfo(pAsTypedef->getDecl());
						cpp::CppNamespace aNamespace;
						Utils::getDeclInfo(pAsTypedef->getDecl(), aNamespace);

						// Here we need to override source type
						pAddedType->overrideTypeData(newName, prettyName, aNamespace, defLoc);

						// Now we need to apply post-processing to override some stubs.
						// NOTE: It supported only for class/struct types because they had ownership links
						if (pAddedType->getKind() == cpp::TypeKind::TK_STRUCT_OR_CLASS)
						{
							// Just visit functions and override ownership
							auto* pAsClass = reinterpret_cast<rg3::cpp::TypeClass*>(pAddedType);

							for (auto& func : pAsClass->getFunctions())
							{
								if (func.sOwnerClassName == sOldClassName)
								{
									func.sOwnerClassName = pAddedType->getPrettyName();
								}
							}
							// NOTE: Maybe we need to filter functions & properties here?
						}
					}
				}
			}
		}

		if (!bHandled && (pType->isEnumeralType() || pType->isScopedEnumeralType()))
		{
			// It's enum
			if (const clang::EnumType* pAsEnum = pType->getAs<clang::EnumType>())
			{
				auto newConfig = m_compilerConfig;
				newConfig.bAllowCollectNonRuntimeTypes = true;
				std::vector<cpp::TypeBasePtr> vFoundTypes {};
				CxxTypeVisitor visitor { vFoundTypes, newConfig };
				visitor.TraverseDecl(pAsEnum->getDecl());

				if (!vFoundTypes.empty())
				{
					m_vFoundTypes.emplace_back(std::move(vFoundTypes[0]));
					bHandled = true;

					// No need to handle "real" type naming here (like in struct/class/typedef)
				}
			}
		}

		if (!bHandled && pType->isBuiltinType())
		{
			// It's builtin type
			if (auto* pAsBuiltinType = pType->getAs<clang::BuiltinType>())
			{
				// It's builtin, just register a type
				// Our builtins are store as generic rg3::cpp::TypeBase instances
				clang::PrintingPolicy typeNamePrintingPolicy { ctx.getLangOpts() };
				typeNamePrintingPolicy.SuppressTagKeyword = true;
				typeNamePrintingPolicy.SuppressScope = false;

				const std::string kName { rg3::cpp::BuiltinTags::kRuntime };
				std::string typeName { pAsBuiltinType->getNameAsCString(typeNamePrintingPolicy) };
				rg3::cpp::CppNamespace sNamespace {}; // always empty in this case
				rg3::cpp::DefinitionLocation definitionLocation {}; // always empty in this case
				rg3::cpp::Tags tags {{ rg3::cpp::Tag { kName } }};

				m_vFoundTypes.emplace_back(
					std::make_unique<rg3::cpp::TypeBase>(
						rg3::cpp::TypeKind::TK_TRIVIAL,
						typeName, typeName, // typeName and pretty name are same in this case
						sNamespace, definitionLocation, tags
					)
				);

				bHandled = true;

				// No need to handle "real" type naming here (like in struct/class/typedef)
			}
		}

		if (!bHandled && pType->isRecordType())
		{
			// It's C++ class (maybe templated)
			if (const clang::TemplateSpecializationType* pAsTemplateSpec = pType->getAs<clang::TemplateSpecializationType>())
			{
				// Ok, it's template spec
				if (auto* pAsCxxRecordDecl = pAsTemplateSpec->getAsCXXRecordDecl())
				{
					if (auto* pTemplateSpecDecl = ::llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(pAsCxxRecordDecl))
					{
						rg3::llvm::CompilerConfig newConfig = m_compilerConfig;
						newConfig.bAllowCollectNonRuntimeTypes = true;

						ExtraPropertiesFilter propertiesFilter { annotation.knownProperties };
						ExtraFunctionsFilter functionsFilter { annotation.knownFunctions };
						CxxTemplateSpecializationVisitor visitor { newConfig, pTemplateSpecDecl, !annotation.knownProperties.empty(), !annotation.knownFunctions.empty(), propertiesFilter, functionsFilter };

						// Here we need to find a correct specialization, but for glm there are no specialization at all...
						if (auto* pSpecializedTemplate = pTemplateSpecDecl->getSpecializedTemplate())
						{
							clang::Decl* pTargetDecl = nullptr;

							for (auto* pDecl : pSpecializedTemplate->redecls())
							{
								if (auto* classTemplateDecl = ::llvm::dyn_cast<clang::ClassTemplateDecl>(pDecl))
								{
									if (classTemplateDecl->isThisDeclarationADefinition())
									{
										pTargetDecl = classTemplateDecl;
										break;
									}
									// wrong!
								}
							}

							if (pTargetDecl)
							{
								// run visitor
								visitor.TraverseDecl(pTargetDecl);

								if (visitor.getClassDefInfo().has_value())
								{
									const auto& sClassDef = visitor.getClassDefInfo().value();

									// Nice, smth found. Need to register type 'as-is' because parent router will override our results if required
									auto pNewType = std::make_unique<cpp::TypeClass>(
										sClassDef.sClassName,
										sClassDef.sPrettyClassName,
										sClassDef.sNameSpace,
										sClassDef.sDefLocation,
										sClassDef.sTags,
										sClassDef.vProperties,
										sClassDef.vFunctions,
										sClassDef.bIsStruct,
										sClassDef.bTriviallyConstructible,
										sClassDef.vParents);

									auto opaquePtr = clang::QualType::getFromOpaquePtr(pAsTemplateSpec);
									if (opaquePtr->isLinkageValid() && bDirectInvoke)
									{
										// avoid of _Bool on MSVC
										clang::PrintingPolicy printingPolicy { ctx.getLangOpts() };
										printingPolicy.Bool = true;

										const std::string sOldPrettyTypeName = pNewType->getPrettyName();
										std::string sName = opaquePtr.getAsString(printingPolicy);
										std::string sPrettyName = sClassDef.sNameSpace.isEmpty() ? sName : fmt::format("{}::{}", sClassDef.sNameSpace.asString(), sName);

										for (auto& func : pNewType->getFunctions())
										{
											if (func.sOwnerClassName == sOldPrettyTypeName)
											{
												func.sOwnerClassName = sPrettyName;
											}
										}

										pNewType->overrideTypeData(sName, sPrettyName);
									}

									m_vFoundTypes.emplace_back(std::move(pNewType));
									bHandled = true;
								}
							}
						}
					}
				}
			}
			else if (const clang::RecordType* pAsRecordType = pType->getAs<clang::RecordType>())
			{
				// Hm, it's generic C++ class/struct
				// It's record type (actually CXX record)
				rg3::llvm::CompilerConfig newConfig = m_compilerConfig;
				newConfig.bAllowCollectNonRuntimeTypes = true; // Yep, it's hack
				CxxClassTypeVisitor sTypeVisitor { newConfig };

				// Ok, let's run
				if (auto pCxxDecl = ::llvm::dyn_cast<clang::CXXRecordDecl>(pAsRecordType->getDecl()))
				{
					sTypeVisitor.TraverseDecl(pCxxDecl);

					auto pClassType = std::make_unique<cpp::TypeClass>(
										  sTypeVisitor.sClassName,
										  sTypeVisitor.sClassPrettyName,
										  sTypeVisitor.sNameSpace,
										  sTypeVisitor.sDefinitionLocation,
										  sTypeVisitor.vTags,
										  sTypeVisitor.foundProperties,
										  sTypeVisitor.foundFunctions,
										  sTypeVisitor.bIsStruct,
										  sTypeVisitor.bTriviallyConstructible,
										  sTypeVisitor.parentClasses);

					// Save
					m_vFoundTypes.emplace_back(std::move(pClassType));

					bHandled = true;
				}
			}

			if (bHandled && !m_vFoundTypes.empty() && m_vFoundTypes.back()->getKind() == cpp::TypeKind::TK_STRUCT_OR_CLASS)
			{
				auto* pClassType = reinterpret_cast<cpp::TypeClass*>(m_vFoundTypes.back().get());

				{
					ExtraPropertiesFilter propChanger { annotation.knownProperties };

					// And we need to override properties here.
					auto& props = pClassType->getProperties();
					for (auto propIt = props.begin(); propIt != props.end(); )
					{
						if (!propChanger(*propIt))
						{
							propIt = props.erase(propIt);
						}
						else
						{
							++propIt;
						}
					}
				}

				{
					ExtraFunctionsFilter funcFilter { annotation.knownFunctions };
					auto& funcs = pClassType->getFunctions();
					for (auto funcIt = funcs.begin(); funcIt != funcs.end(); )
					{
						if (!funcFilter(funcIt->sName))
						{
							funcIt = funcs.erase(funcIt);
						}
						else
						{
							++funcIt;
						}
					}
				}
			}
			// else idk
		}

		if (bHandled && bDirectInvoke)
		{
			if (annotation.overrideLocation.has_value())
			{
				m_vFoundTypes.back()->setDefinition(cpp::DefinitionLocation { annotation.overrideLocation.value(), 0, 0, true });
			}

			cpp::Tags tags = annotation.additionalTags;
			tags += cpp::Tag { std::string(cpp::BuiltinTags::kRuntime) };

			m_vFoundTypes.back()->addTags(tags);
		}

		return bHandled;
	}
}