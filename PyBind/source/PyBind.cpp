#include <string>
#include <vector>
#include <variant>
#include <fmt/format.h>

#define BOOST_PYTHON_STATIC_LIB  // required because we using boost.python as static library
#include <boost/python.hpp>
#include <boost/python/dict.hpp>

#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/Cpp/TypeBaseInfo.h>

#include <RG3/LLVM/CodeEvaluator.h>

#include <RG3/PyBind/PyCodeAnalyzerBuilder.h>
#include <RG3/PyBind/PyTypeBase.h>
#include <RG3/PyBind/PyTypeEnum.h>
#include <RG3/PyBind/PyTypeClass.h>
#include <RG3/PyBind/PyAnalyzerContext.h>
#include <RG3/PyBind/PyClangRuntime.h>


using namespace boost::python;


namespace rg3::pybind::wrappers
{
	static boost::python::str CppTypeReference_getTypeName(const rg3::cpp::TypeReference& typeRef)
	{
		return boost::python::str(typeRef.getRefName());
	}

	static boost::python::list Tag_getArguments(const rg3::cpp::Tag& tag)
	{
		boost::python::list l;

		for (const auto& arg : tag.getArguments())
		{
			l.append(arg);
		}

		return l;
	}

	static boost::python::str TagArgument_getAsStringRepr(const rg3::cpp::TagArgument& arg)
	{
		switch (arg.getHoldedType())
		{
			case rg3::cpp::TagArgumentType::AT_TYPEREF:
			{
				static const rg3::cpp::TypeReference s_None {};
				rg3::cpp::TypeReference rType = arg.asTypeRef(s_None);
				return boost::python::str(fmt::format("TypeREF: {}", rType.getRefName()));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_BOOL:
			{
				return boost::python::str(arg.asBool(false));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_STRING:
			{
				static const std::string s_None {};
				return boost::python::str(arg.asString(s_None));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_FLOAT:
			{
				return boost::python::str(arg.asFloat(.0f));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_I64:
			{
				return boost::python::str(arg.asI64(0));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_UNDEFINED:
			default:
				return boost::python::str("<UNDEFINED>");
		}

		return boost::python::str("<UNDEFINED>");
	}

	static boost::python::object TagArgument_getAsTypeReference(const rg3::cpp::TagArgument& arg)
	{
		if (arg.getHoldedType() == rg3::cpp::TagArgumentType::AT_TYPEREF)
		{
			static rg3::cpp::TypeReference s_NullRef{};
			const rg3::cpp::TypeReference& res = arg.asTypeRef(s_NullRef);

			if (res.getRefName() == s_NullRef.getRefName())
				return {};

			return boost::python::object(res);
		}

		return {};
	}

	static boost::python::list Tags_getTagItemsList(const rg3::cpp::Tags& tags)
	{
		boost::python::list l;

		for (const auto& [name, tag] : tags.getTags())
		{
			l.append(tag);
		}

		return l;
	}

	static boost::python::str CppIncludeInfo_getPath(const rg3::llvm::IncludeInfo& ii)
	{
		return boost::python::str(ii.sFsLocation.string());
	}

	static boost::python::object TypeStatement_getDefinitionLocation(const rg3::cpp::TypeStatement& stmt)
	{
		if (stmt.sDefinitionLocation.has_value())
		{
			return boost::python::object(stmt.sDefinitionLocation.value());
		}

		return {};
	}

	static boost::python::str TypeStatement_getTypeName(const rg3::cpp::TypeStatement& stmt)
	{
		return boost::python::str(stmt.sTypeRef.getRefName());
	}

	static boost::python::list ClassFunction_getArgumentsList(const rg3::cpp::ClassFunction& classFunction)
	{
		boost::python::list l;

		for (const auto& arg : classFunction.vArguments)
		{
			l.append(arg);
		}

		return l;
	}

	static boost::python::str TagArgument_getArgumentAsString(const rg3::cpp::TagArgument& arg)
	{
		static const std::string s_Null {};
		return boost::python::str(arg.asString(s_Null));
	}

	static boost::python::str TypeBaseInfo_getName(const rg3::cpp::TypeBaseInfo& sInfo)
	{
		return boost::python::str(sInfo.sName);
	}

	static boost::python::str TypeBaseInfo_getPrettyName(const rg3::cpp::TypeBaseInfo& sInfo)
	{
		return boost::python::str(sInfo.sPrettyName);
	}

	static boost::python::object CodeEvaluator_eval(rg3::llvm::CodeEvaluator& sEval, const std::string& sCode, const boost::python::list& aCapture)
	{
		std::vector<std::string> aCaptureList {};
		aCaptureList.reserve(len(aCapture));

		for (int i = 0; i < len(aCapture); ++i)
		{
			aCaptureList.push_back(boost::python::extract<std::string>(aCapture[i]));
		}

		// Invoke originals
		auto sEvalResult = sEval.evaluateCode(sCode, aCaptureList);
		if (!sEvalResult)
		{
			// Error!
			boost::python::list aIssues {};

			for (const auto& sIssue : sEvalResult.vIssues)
			{
				aIssues.append(sIssue);
			}

			return aIssues;
		}

		// Make dict
		boost::python::dict result {};

		for (const auto& [sKey, sValue] : sEvalResult.mOutputs)
		{
			std::visit([&result, &sKey](auto&& v) {
				result[sKey] = v;
			}, sValue);
		}

		return result;
	}

	static void CodeEvaluator_setCppStandard(rg3::llvm::CodeEvaluator& sEval, rg3::llvm::CxxStandard eStandard)
	{
		sEval.getCompilerConfig().cppStandard = eStandard;
	}

	static void CodeEvaluator_setCompilerConfigFromDict(rg3::llvm::CodeEvaluator& sEval, const boost::python::object& sDescription)
	{
		/**
		 * Allowed fields:
		 *
		 * cpp_standard
		 * definitions
		 * allow_collect_non_runtime
		 * skip_function_bodies
		 *
		 * If description presented as dict - use directly, otherwise try cast to dict via __dict__
		 */
		if (sDescription.is_none())
		{
			PyErr_SetString(PyExc_MemoryError, "Expected to have a valid description instead of None");
			boost::python::throw_error_already_set();
			return;
		}

		if (PyDict_Check(sDescription.ptr()))
		{
			// Use directly as dict
			boost::python::dict pyDict = boost::python::extract<boost::python::dict>(sDescription);
			boost::python::list aKeys = pyDict.keys();

			for (int i = 0; i < boost::python::len(aKeys); ++i) {
				const std::string sKey = boost::python::extract<std::string>(aKeys[i]);

				if (sKey == "cpp_standard")
				{
					sEval.getCompilerConfig().cppStandard = boost::python::extract<rg3::llvm::CxxStandard>(pyDict[sKey]);
				}

				if (sKey == "definitions")
				{
					boost::python::list pyList = boost::python::extract<boost::python::list>(pyDict[sKey]);

					auto& definitions = sEval.getCompilerConfig().vCompilerDefs;
					definitions.clear();
					definitions.reserve(boost::python::len(pyList));

					for (auto j = 0; j < boost::python::len(pyList); ++j) {
						definitions.push_back(boost::python::extract<std::string>(pyList[j]));
					}
				}

				if (sKey == "allow_collect_non_runtime")
				{
					sEval.getCompilerConfig().bAllowCollectNonRuntimeTypes = boost::python::extract<bool>(pyDict[sKey]);
				}

				if (sKey == "skip_function_bodies")
				{
					sEval.getCompilerConfig().bSkipFunctionBodies = boost::python::extract<bool>(pyDict[sKey]);
				}
			}
		}
		else
		{
			// Maybe subject able to cast with __dict__ ?
			boost::python::object dictAttr = sDescription.attr("__dict__");
			if (PyDict_Check(dictAttr.ptr()))
			{
				// Use as dict
				boost::python::dict pyDict = boost::python::extract<boost::python::dict>(dictAttr);

				// Run self. NOTE: Maybe we've should check this for recursion? Idk)
				CodeEvaluator_setCompilerConfigFromDict(sEval, pyDict);
			}
			else
			{
				// Omg, dict is not a dict. Raise our own error
				PyErr_SetString(PyExc_TypeError, "Meta method __dict__ returned not a dict!");
				boost::python::throw_error_already_set();
			}
		}
	}

	static rg3::llvm::CxxStandard CodeEvaluator_getCppStandard(const rg3::llvm::CodeEvaluator& sEval)
	{
		return sEval.getCompilerConfig().cppStandard;
	}

	static boost::shared_ptr<rg3::llvm::CodeEvaluator> CodeEvaluator_makeFromSystemEnv()
	{
		auto environmentExtractResult = rg3::llvm::CompilerConfigDetector::detectSystemCompilerEnvironment();
		if (rg3::llvm::CompilerEnvError* pError = std::get_if<rg3::llvm::CompilerEnvError>(&environmentExtractResult))
		{
			std::string sErrorMessage = fmt::format("Failed to detect compiler environment: {}", pError->message);

			PyErr_SetString(PyExc_RuntimeError, sErrorMessage.c_str());
			boost::python::throw_error_already_set();
			return nullptr;
		}

		auto pInstance = boost::shared_ptr<rg3::llvm::CodeEvaluator>(new rg3::llvm::CodeEvaluator());
		if (!pInstance)
		{
			PyErr_SetString(PyExc_MemoryError, "Out of memory (unable to allocate CodeEvaluator)");
			boost::python::throw_error_already_set();
			return nullptr;
		}

		pInstance->setCompilerEnvironment(std::get<rg3::llvm::CompilerEnvironment>(environmentExtractResult));
		return pInstance;
	}

	static boost::shared_ptr<rg3::llvm::CodeEvaluator> PyAnalyzerContext_makeEvaluator(const rg3::pybind::PyAnalyzerContext& sContext)
	{
		return boost::shared_ptr<rg3::llvm::CodeEvaluator>(new rg3::llvm::CodeEvaluator(sContext.getCompilerConfig()));
	}

	static boost::shared_ptr<rg3::llvm::CodeEvaluator> PyCodeAnalyzerBuilder_makeEvaluator(const rg3::pybind::PyCodeAnalyzerBuilder& sContext)
	{
		return boost::shared_ptr<rg3::llvm::CodeEvaluator>(new rg3::llvm::CodeEvaluator(sContext.getCompilerConfig()));
	}
}


BOOST_PYTHON_MODULE(rg3py)
{
	/**
	 * Register base and trivial types here, other in impls
	 * Please, don't forget to add your public available types into PyBind/rg3py.pyi file!!!
	 */
	/// ----------- ENUMS -----------
	enum_<rg3::cpp::TypeKind>("CppTypeKind", "A kind of C++ type (trivial, enum, class or struct)")
	    .value("TK_NONE", rg3::cpp::TypeKind::TK_NONE)
	    .value("TK_TRIVIAL", rg3::cpp::TypeKind::TK_TRIVIAL)
	    .value("TK_ENUM", rg3::cpp::TypeKind::TK_ENUM)
	    .value("TK_STRUCT_OR_CLASS", rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS)
	;

	enum_<rg3::cpp::TagArgumentType>("TagArgumentType", "Type of tag argument")
	    .value("AT_UNDEFINED", rg3::cpp::TagArgumentType::AT_UNDEFINED)
	    .value("AT_BOOL", rg3::cpp::TagArgumentType::AT_BOOL)
	    .value("AT_FLOAT", rg3::cpp::TagArgumentType::AT_FLOAT)
	    .value("AT_I64", rg3::cpp::TagArgumentType::AT_I64)
	    .value("AT_STRING", rg3::cpp::TagArgumentType::AT_STRING)
	    .value("AT_TYPEREF", rg3::cpp::TagArgumentType::AT_TYPEREF)
	;

	enum_<rg3::cpp::ClassEntryVisibility>("CppClassEntryVisibillity", "Visibility level of C++ entry (inheritance, declaration, etc...)")
	    .value("CEV_PRIVATE", rg3::cpp::ClassEntryVisibility::CEV_PRIVATE)
	    .value("CEV_PROTECTED", rg3::cpp::ClassEntryVisibility::CEV_PROTECTED)
	    .value("CEV_PUBLIC", rg3::cpp::ClassEntryVisibility::CEV_PUBLIC)
	;

	/// ----------- CLASSES -----------
	class_<rg3::cpp::CppNamespace>("CppNamespace", "Represent generic namespace in C++")
	    .def(init<std::string>(args("namespace")))
		.def("__eq__", &rg3::cpp::CppNamespace::operator==)
		.def("__ne__", &rg3::cpp::CppNamespace::operator!=)
		.def("__str__", make_function(&rg3::cpp::CppNamespace::asString, return_value_policy<copy_const_reference>()))
		.def("__repr__", make_function(&rg3::cpp::CppNamespace::asString, return_value_policy<copy_const_reference>()))
	;

	class_<rg3::cpp::DefinitionLocation>("Location", "Location of declaration (file, line, column, is angled, etc...)")
	    .def(init<std::string, int, int>(args("path", "line", "offset")))
		.add_property("path", make_function(&rg3::cpp::DefinitionLocation::getPath, return_value_policy<return_by_value>()), "A path to file with decl")
		.add_property("line", &rg3::cpp::DefinitionLocation::getLine, "Line in file where declaration starts")
		.add_property("column", &rg3::cpp::DefinitionLocation::getInLineOffset, "Column in file where declaration starts")
		.add_property("angled", &rg3::cpp::DefinitionLocation::isAngledPath, "Is path angled (should be included via <>)")
	;

	class_<rg3::cpp::TagArgument>("TagArgument", "Argument of tag")
	    .def(init<bool>(arg("barg")))
	    .def(init<float>(arg("farg")))
	    .def(init<std::int64_t>(arg("i64")))
	    .def(init<std::string>(arg("string")))
		.def("__str__", &rg3::pybind::wrappers::TagArgument_getAsStringRepr)
		.def("get_type", &rg3::cpp::TagArgument::getHoldedType, "Returns holded type")
		.def("as_bool", make_function(&rg3::cpp::TagArgument::asBool, return_value_policy<return_by_value>()))
		.def("as_float", make_function(&rg3::cpp::TagArgument::asFloat, return_value_policy<return_by_value>()))
		.def("as_i64", make_function(&rg3::cpp::TagArgument::asI64, return_value_policy<return_by_value>()))
		.def("as_string", make_function(&rg3::pybind::wrappers::TagArgument_getArgumentAsString, return_value_policy<return_by_value>()))
		.def("as_type_ref", &rg3::pybind::wrappers::TagArgument_getAsTypeReference)
	;

	class_<rg3::cpp::Tag>("Tag", "Represent codegen tag data")
	    .add_property("name", make_function(&rg3::cpp::Tag::getName, return_value_policy<copy_const_reference>()), "Name of tag")
		.add_property("arguments", make_function(&rg3::pybind::wrappers::Tag_getArguments, return_value_policy<return_by_value>()), "Arguments list")
		.def("__eq__", make_function(&rg3::cpp::Tag::operator==, return_value_policy<return_by_value>()))
		.def("__ne__", make_function(&rg3::cpp::Tag::operator!=, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::Tags>("Tags", "Container of the tags")
	    .add_property("items", &rg3::pybind::wrappers::Tags_getTagItemsList, "List of tags inside this registry")
		.def("__contains__", &rg3::cpp::Tags::hasTag)
		.def("has_tag", &rg3::cpp::Tags::hasTag)
		.def("get_tag", make_function(&rg3::cpp::Tags::getTag, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::TypeReference>("CppTypeReference", "A reference to type (DEPRECATED)")
		.add_property("name", &rg3::pybind::wrappers::CppTypeReference_getTypeName)
	;

	enum_<rg3::cpp::InheritanceVisibility>("InheritanceVisibility", "Inheritance visibility level")
		.value("IV_PRIVATE", rg3::cpp::InheritanceVisibility::IV_PRIVATE)
		.value("IV_PUBLIC", rg3::cpp::InheritanceVisibility::IV_PUBLIC)
		.value("IV_PROTECTED", rg3::cpp::InheritanceVisibility::IV_PROTECTED)
		.value("IV_VIRTUAL", rg3::cpp::InheritanceVisibility::IV_VIRTUAL)
	;

	class_<rg3::cpp::ClassParent>("ClassParent", "Basic information about parent type")
	    .add_property("info", make_getter(&rg3::cpp::ClassParent::sTypeBaseInfo), "Information about parent type")
		.add_property("inheritance", make_getter(&rg3::cpp::ClassParent::eModifier), "Inheritance type")
	;

	class_<rg3::cpp::EnumEntry>("CppEnumEntry", "A single element of enum")
		.add_property("name", make_getter(&rg3::cpp::EnumEntry::sName), "Name of entry")
		.add_property("value", make_getter(&rg3::cpp::EnumEntry::iValue), "Value of entry")
	;

	class_<rg3::cpp::TypeBaseInfo>("TypeBaseInfo", "Base information about type instantiation")
	    .add_property("kind", make_getter(&rg3::cpp::TypeBaseInfo::eKind), "Kind of type")
		.add_property("namespace", make_getter(&rg3::cpp::TypeBaseInfo::sNameSpace), "Namespace where type declared")
		.add_property("location", make_getter(&rg3::cpp::TypeBaseInfo::sDefLocation), "Place where type declared")
		.add_property("name", &rg3::pybind::wrappers::TypeBaseInfo_getName, "Name of type without namespace")
		.add_property("pretty_name", &rg3::pybind::wrappers::TypeBaseInfo_getPrettyName, "Prettified name of type (included namespace and full form of name)")
	;

	class_<rg3::cpp::TypeStatement>("TypeStatement", "Base information about type instantiation usage (ie function decl)")
	    .add_property("type_ref", make_getter(&rg3::cpp::TypeStatement::sTypeRef), "Reference to type info")
		.add_property("type_info", make_getter(&rg3::cpp::TypeStatement::sBaseInfo), "Base information about type")
		.add_property("location", &rg3::pybind::wrappers::TypeStatement_getDefinitionLocation, "(optional) Where type statement info defined (not a type!)")
		.add_property("is_const", make_getter(&rg3::cpp::TypeStatement::bIsConst), "Is declaration constant")
		.add_property("is_const_ptr", make_getter(&rg3::cpp::TypeStatement::bIsPtrConst), "Is pointer type with const qualifier (unused when is_ptr is False)")
		.add_property("is_ptr", make_getter(&rg3::cpp::TypeStatement::bIsPointer), "Is declaration pointer")
		.add_property("is_ref", make_getter(&rg3::cpp::TypeStatement::bIsReference), "Is declaration reference")
		.add_property("is_template", make_getter(&rg3::cpp::TypeStatement::bIsTemplateSpecialization), "Is declaration presented via template specialization")
		.add_property("is_void", &rg3::cpp::TypeStatement::isVoid, "Is type C++ void or not")

		.def("get_name", &rg3::pybind::wrappers::TypeStatement_getTypeName, "Return name of the type")
	;

	class_<rg3::cpp::FunctionArgument>("FunctionArgument", "Single argument of the function")
		.add_property("type_info", make_getter(&rg3::cpp::FunctionArgument::sType), "Argument type info")
		.add_property("name", make_getter(&rg3::cpp::FunctionArgument::sArgumentName), "Name of argument")
		.add_property("has_default_value", make_getter(&rg3::cpp::FunctionArgument::bHasDefaultValue))
	    ;

	class_<rg3::cpp::ClassProperty>("CppClassProperty", "Class property")
		.add_property("name", make_getter(&rg3::cpp::ClassProperty::sName), "Name of property (original name from code)")
		.add_property("alias", make_getter(&rg3::cpp::ClassProperty::sAlias), "Alias name of property declared via 'property' tag")
		.add_property("visibility", make_getter(&rg3::cpp::ClassProperty::eVisibility), "Property visibility level")
		.add_property("tags", make_getter(&rg3::cpp::ClassProperty::vTags), "Tags of property")
		.add_property("type_info", make_getter(&rg3::cpp::ClassProperty::sTypeInfo), "Base type info")
		.def("__eq__", make_function(&rg3::cpp::ClassProperty::operator==, return_value_policy<return_by_value>()))
		.def("__ne__", make_function(&rg3::cpp::ClassProperty::operator!=, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::ClassFunction>("CppClassFunction", "Class function")
		.add_property("name", make_getter(&rg3::cpp::ClassFunction::sName), "Name of function")
		.add_property("owner", make_getter(&rg3::cpp::ClassFunction::sOwnerClassName), "Owner class name")
		.add_property("visibility", make_getter(&rg3::cpp::ClassFunction::eVisibility), "Visibility")
		.add_property("tags", make_getter(&rg3::cpp::ClassFunction::vTags), "Tags list")
		.add_property("is_static", make_getter(&rg3::cpp::ClassFunction::bIsStatic), "Is static method")
		.add_property("is_const", make_getter(&rg3::cpp::ClassFunction::bIsConst), "Is const method")
		.add_property("is_noexcept", make_getter(&rg3::cpp::ClassFunction::bIsNoExcept), "Is noexcept method")
		.add_property("return_type", make_getter(&rg3::cpp::ClassFunction::sReturnType), "Type of return type")
		.add_property("arguments", &rg3::pybind::wrappers::ClassFunction_getArgumentsList, "Arguments list")
		.def("__eq__", make_function(&rg3::cpp::ClassFunction::operator==))
		.def("__ne__", make_function(&rg3::cpp::ClassFunction::operator!=))
	;

	enum_<rg3::llvm::CxxStandard>("CppStandard", "A standard of C++ for internal compiler")
		.value("CXX_11", rg3::llvm::CxxStandard::CC_11)
		.value("CXX_14", rg3::llvm::CxxStandard::CC_14)
		.value("CXX_17", rg3::llvm::CxxStandard::CC_17)
		.value("CXX_20", rg3::llvm::CxxStandard::CC_20)
		.value("CXX_23", rg3::llvm::CxxStandard::CC_23)
		.value("CXX_DEFAULT", rg3::llvm::CxxStandard::CC_DEFAULT)
	;

	enum_<rg3::llvm::IncludeKind>("CppIncludeKind", "Kind of include. Only for internal compiler")
		.value("IK_PROJECT", rg3::llvm::IncludeKind::IK_PROJECT)
		.value("IK_SYSTEM", rg3::llvm::IncludeKind::IK_SYSTEM)
		.value("IK_SYSROOT", rg3::llvm::IncludeKind::IK_SYSROOT)
		.value("IK_THIRD_PARTY", rg3::llvm::IncludeKind::IK_THIRD_PARTY)
		.value("IK_DEFAULT", rg3::llvm::IncludeKind::IK_DEFAULT)
	;

	class_<rg3::llvm::IncludeInfo>("CppIncludeInfo", "Information about include (location and kind)")
		.def(init<std::string, rg3::llvm::IncludeKind>(args("path", "kind")))
		.add_property("path", &rg3::pybind::wrappers::CppIncludeInfo_getPath, "Path to C/C++ source header")
		.add_property("kind", make_getter(&rg3::llvm::IncludeInfo::eKind), "Kind of header")
	;

	enum_<rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind>("CppCompilerIssueKind", "Kind and level of compiler issue")
		.value("IK_NONE", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_NONE)
		.value("IK_WARNING", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_WARNING)
		.value("IK_INFO", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_INFO)
		.value("IK_ERROR", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR)
	;

	class_<rg3::llvm::AnalyzerResult::CompilerIssue>("CppCompilerIssue", "Information about compiler issue", no_init)
		.add_property("kind", make_getter(&rg3::llvm::AnalyzerResult::CompilerIssue::kind))
		.add_property("source_file", make_getter(&rg3::llvm::AnalyzerResult::CompilerIssue::sSourceFile))
		.add_property("message", make_getter(&rg3::llvm::AnalyzerResult::CompilerIssue::sMessage))
	;

	class_<rg3::pybind::PyTypeBase, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyTypeBase>>("CppBaseType", "A base type type info of C++ type", no_init)
	    .add_property("name", make_function(&rg3::pybind::PyTypeBase::pyGetName, return_value_policy<return_by_value>()), "Name of C++ type (without namespace and qualifiers)")
		.add_property("hash", &rg3::pybind::PyTypeBase::__hash__, "Return unique hash of type")
		.add_property("kind", make_function(&rg3::pybind::PyTypeBase::pyGetTypeKind, return_value_policy<return_by_value>()), "Kind of type")
		.add_property("namespace", make_function(&rg3::pybind::PyTypeBase::pyGetNamespace, return_value_policy<copy_const_reference>()), "Namespace where declared type")
		.add_property("location", make_function(&rg3::pybind::PyTypeBase::pyGetLocation, return_value_policy<copy_const_reference>()), "Location where type declared")
		.add_property("pretty_name", &rg3::pybind::PyTypeBase::pyGetPrettyName, "Pretty name of type with namespace")
		.add_property("tags", make_function(&rg3::pybind::PyTypeBase::pyGetTags, return_value_policy<copy_const_reference>()), "Tags presented in type")
		.add_property("is_forward_declarable", make_function(&rg3::pybind::PyTypeBase::pyIsForwardDeclarable, return_value_policy<return_by_value>()), "Is type could be forward declared")

		.def("__str__", make_function(&rg3::pybind::PyTypeBase::__str__, return_value_policy<copy_const_reference>()))
		.def("__repr__", make_function(&rg3::pybind::PyTypeBase::__repr__, return_value_policy<copy_const_reference>()))
		.def("__hash__", &rg3::pybind::PyTypeBase::__hash__)
		.def("__eq__", &rg3::pybind::PyTypeBase::__eq__)
		.def("__ne__", &rg3::pybind::PyTypeBase::__ne__)
	;

	class_<rg3::pybind::PyTypeEnum, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyTypeEnum>, boost::python::bases<rg3::pybind::PyTypeBase>>("CppEnum", "C++ enum information", no_init)
		.add_property("entries", make_function(&rg3::pybind::PyTypeEnum::pyGetEnumEntries, return_value_policy<copy_const_reference>()), "Entries of enum")
		.add_property("is_scoped", &rg3::pybind::PyTypeEnum::pyIsScoped, "Is enum scoped or not")
		.add_property("underlying_type", make_function(&rg3::pybind::PyTypeEnum::pyGetUnderlyingTypeStr, return_value_policy<copy_const_reference>()), "Internal enum type")
	;

	class_<rg3::pybind::PyTypeClass, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyTypeClass>, boost::python::bases<rg3::pybind::PyTypeBase>>("CppClass", "C++ class or struct information", no_init)
		.add_property("properties", make_function(&rg3::pybind::PyTypeClass::pyGetClassProperties, return_value_policy<copy_const_reference>()), "Class properties")
		.add_property("functions", make_function(&rg3::pybind::PyTypeClass::pyGetClassFunctions, return_value_policy<copy_const_reference>()), "Class functions")
		.add_property("is_struct", &rg3::pybind::PyTypeClass::pyIsStruct, "Is struct or class")
		.add_property("is_trivial_constructible", &rg3::pybind::PyTypeClass::pyIsTriviallyConstructible)
		.add_property("has_copy_constructor", &rg3::pybind::PyTypeClass::pyHasCopyConstructor)
		.add_property("has_copy_assign_operator", &rg3::pybind::PyTypeClass::pyHasCopyAssignOperator)
		.add_property("has_move_constructor", &rg3::pybind::PyTypeClass::pyHasMoveConstructor)
		.add_property("has_move_assign_operator", &rg3::pybind::PyTypeClass::pyHasMoveAssignOperator)
		.add_property("parent_types", make_function(&rg3::pybind::PyTypeClass::pyGetClassParentTypeRefs, return_value_policy<copy_const_reference>()))
	;

	class_<rg3::pybind::PyCodeAnalyzerBuilder, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyCodeAnalyzerBuilder>>("CodeAnalyzer", "A simple code analyzer. Possible to analyze file or code string", no_init)
		.def("make", &rg3::pybind::PyCodeAnalyzerBuilder::makeInstance)
		.staticmethod("make")

		.add_property("types", make_function(&rg3::pybind::PyCodeAnalyzerBuilder::getFoundTypes, return_value_policy<copy_const_reference>()), "A list of found types")
		.add_property("issues", make_function(&rg3::pybind::PyCodeAnalyzerBuilder::getFoundIssues, return_value_policy<copy_const_reference>()), "A list of found issues")
		.add_property("ignore_runtime",
					  &rg3::pybind::PyCodeAnalyzerBuilder::isNonRuntimeTypesAllowedToBeCollected,
					  &rg3::pybind::PyCodeAnalyzerBuilder::setAllowToCollectNonRuntimeTypes,
					  "Allow to ignore @runtime tag near type decl")
		.add_property("definitions",
					  &rg3::pybind::PyCodeAnalyzerBuilder::getCompilerDefinitions,
					  &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerDefinitions,
					  "A list of compiler definitions like -D option in cmake")

		.def("set_ignore_non_runtime_types", &rg3::pybind::PyCodeAnalyzerBuilder::setAllowToCollectNonRuntimeTypes)
		.def("is_non_runtime_types_ignored", &rg3::pybind::PyCodeAnalyzerBuilder::isNonRuntimeTypesAllowedToBeCollected)

		.def("set_code", &rg3::pybind::PyCodeAnalyzerBuilder::setSourceCode)
		.def("set_file", &rg3::pybind::PyCodeAnalyzerBuilder::setSourceFile)
		.def("set_cpp_standard", &rg3::pybind::PyCodeAnalyzerBuilder::setCppStandard)
		.def("set_compiler_args", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerArgs)
		.def("set_compiler_include_dirs", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerIncludeDirs)
		.def("add_include_dir", &rg3::pybind::PyCodeAnalyzerBuilder::addIncludeDir)
		.def("add_project_include_dir", &rg3::pybind::PyCodeAnalyzerBuilder::addProjectIncludeDir)
		.def("get_definitions", &rg3::pybind::PyCodeAnalyzerBuilder::getCompilerDefinitions)
		.def("set_definitions", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerDefinitions)
		.def("analyze", &rg3::pybind::PyCodeAnalyzerBuilder::analyze)
		.def("make_evaluator", &rg3::pybind::wrappers::PyCodeAnalyzerBuilder_makeEvaluator)
	;

	class_<rg3::pybind::PyAnalyzerContext, boost::noncopyable , boost::shared_ptr<rg3::pybind::PyAnalyzerContext>>("AnalyzerContext", "A multithreaded analyzer and scheduled which made to analyze a bunch of files at once. If you have more than few files you should use this class.", no_init)
		.def("make", &rg3::pybind::PyAnalyzerContext::makeInstance)
		.staticmethod("make")

		// Properties
		.add_property("workers_count", &rg3::pybind::PyAnalyzerContext::getWorkersCount, "Count of workers which will prepare incoming sources")
		.add_property("types", make_function(&rg3::pybind::PyAnalyzerContext::getFoundTypes, return_value_policy<copy_const_reference>()), "A list of found types")
		.add_property("issues", make_function(&rg3::pybind::PyAnalyzerContext::getFoundIssues, return_value_policy<copy_const_reference>()), "A list of found issues")
		.add_property("headers", &rg3::pybind::PyAnalyzerContext::getHeaders, "List of headers")
		.add_property("include_directories", &rg3::pybind::PyAnalyzerContext::getCompilerIncludeDirs, "Include directories for compiler instance")
		.add_property("cpp_standard", &rg3::pybind::PyAnalyzerContext::getCppStandard, &rg3::pybind::PyAnalyzerContext::setCppStandard, "Set C++ standard")
		.add_property("compiler_args", &rg3::pybind::PyAnalyzerContext::getCompilerArgs, "Set clang compiler arguments list")
		.add_property("ignore_runtime_tag", &rg3::pybind::PyAnalyzerContext::isRuntimeTagIgnored, &rg3::pybind::PyAnalyzerContext::setIgnoreRuntimeTag, "Should context ignore @runtime tag on 'collect types' stage")
		.add_property("compiler_defs", &rg3::pybind::PyAnalyzerContext::getCompilerDefs, "Compiler definitions")

		// Functions
		.def("set_workers_count", &rg3::pybind::PyAnalyzerContext::setWorkersCount)
		.def("set_headers", &rg3::pybind::PyAnalyzerContext::setHeaders)
		.def("set_include_directories", &rg3::pybind::PyAnalyzerContext::setCompilerIncludeDirs)
		.def("set_compiler_args", &rg3::pybind::PyAnalyzerContext::setCompilerArgs)
		.def("set_compiler_defs", &rg3::pybind::PyAnalyzerContext::setCompilerDefs)
		.def("analyze", &rg3::pybind::PyAnalyzerContext::analyze)
		.def("make_evaluator", &rg3::pybind::wrappers::PyAnalyzerContext_makeEvaluator)

		// Resolvers
		.def("get_type_by_reference", &rg3::pybind::PyAnalyzerContext::pyGetTypeOfTypeReference)
	;

	class_<rg3::pybind::PyClangRuntime, boost::noncopyable>("ClangRuntime", "Technical information about bundled Clang, LLVM and detected system paths")
	    .def("get_version", &rg3::pybind::PyClangRuntime::getRuntimeInfo)
		.staticmethod("get_version")

		.def("detect_system_include_sources", &rg3::pybind::PyClangRuntime::detectSystemIncludeSources)
		.staticmethod("detect_system_include_sources")
	;

	class_<rg3::llvm::CodeEvaluator, boost::noncopyable, boost::shared_ptr<rg3::llvm::CodeEvaluator>>("CodeEvaluator", "Eval constexpr C++ code and provide access to result values", boost::python::init<>())
	    .def("eval", &rg3::pybind::wrappers::CodeEvaluator_eval)
		.def("set_cpp_standard", &rg3::pybind::wrappers::CodeEvaluator_setCppStandard)
		.def("set_compiler_config", &rg3::pybind::wrappers::CodeEvaluator_setCompilerConfigFromDict)
		.def("get_cpp_standard", &rg3::pybind::wrappers::CodeEvaluator_getCppStandard)

		.def("make_from_system_env", &rg3::pybind::wrappers::CodeEvaluator_makeFromSystemEnv)
		.staticmethod("make_from_system_env")
	;
}