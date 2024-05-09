#include <RG3/PyBind/PyAnalyzerContext.h>
#include <RG3/PyBind/PyTypeBase.h>
#include <RG3/PyBind/PyTypeClass.h>
#include <RG3/PyBind/PyTypeEnum.h>
#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/LLVM/CompilerConfigDetector.h>
#include <RG3/Cpp/TransactionGuard.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeEnum.h>
#include <fmt/format.h>
#include <optional>
#include <variant>
#include <thread>
#include <deque>
#include <mutex>


namespace rg3::pybind
{
	/**
	 * Do nothing (default command)
	 */
	using NullTask = std::nullptr_t;

	/**
	 * Stop current thread. Just return from internal loop
	 */
	struct StopWorkerTask
	{;
	};

	/**
	 * Run analyze on specific file
	 */
	struct AnalyzeHeaderTask
	{
		std::filesystem::path headerPath;
		rg3::llvm::CompilerConfig compilerConfig;
	};

	using ContextTask = std::variant<NullTask, StopWorkerTask, AnalyzeHeaderTask>;

	class IRuntimeContextBaseOperations
	{
	 public:
		virtual ~IRuntimeContextBaseOperations() noexcept = default;

		virtual void clearTasks() = 0;
		virtual void pushTask(ContextTask&& task) = 0;
		virtual std::optional<ContextTask> takeTask() = 0;
	};

	struct PyGuard final : boost::noncopyable
	{
		PyGuard()
		{
			m_state = PyEval_SaveThread();
		}

		~PyGuard()
		{
			PyEval_RestoreThread(m_state);
			m_state = nullptr;
		}

	 private:
		PyThreadState* m_state { nullptr };
	};

	struct PyGILGuard final : boost::noncopyable
	{
		PyGILState_STATE m_gs;

		PyGILGuard()
		{
			m_gs = PyGILState_Ensure();
		}

		~PyGILGuard()
		{
			PyGILState_Release(m_gs);
		}
	};

	struct PyAnalyzerContext::RuntimeContext : public IRuntimeContextBaseOperations
	{
	 public:
		using Storage = std::deque<ContextTask>;

	 private:
		std::mutex lockMtx;
		Storage tasks;
		std::vector<std::thread> workers;
		std::optional<rg3::llvm::CompilerEnvironment> m_compilerEnv {};

		PyFoundSubjects* pAnalyzerStorage{ nullptr };

	 public:
		class Transaction : public cpp::TransactionGuard<std::mutex>, public IRuntimeContextBaseOperations
		{
			Storage& m_storage;

		 public:
			explicit Transaction(std::mutex& mutex, Storage& storage) : cpp::TransactionGuard<std::mutex>(mutex), m_storage(storage)
			{
			}

			~Transaction() = default;

			void clearTasks() override
			{
				m_storage.clear();
			}

			void pushTask(ContextTask&& task) override
			{
				m_storage.emplace_back(std::move(task));
			}

			std::optional<ContextTask> takeTask() override
			{
				if (m_storage.empty())
					return std::nullopt;

				auto task = m_storage.front();
				m_storage.pop_front();

				return std::make_optional(std::move(task));
			}
		};

	 public:
		explicit RuntimeContext(PyFoundSubjects* pSubject) : pAnalyzerStorage(pSubject) {}
		~RuntimeContext() = default;

		void clearTasks() override
		{
			std::lock_guard<std::mutex> guard { lockMtx };
			tasks.clear();
		}

		void pushTask(ContextTask&& task) override
		{
			std::lock_guard<std::mutex> guard { lockMtx };
			tasks.emplace_back(std::move(task));
		}

		std::optional<ContextTask> takeTask() override
		{
			std::lock_guard<std::mutex> guard { lockMtx };
			if (tasks.empty())
				return std::nullopt;

			auto task = tasks.front();
			tasks.pop_front();

			return std::make_optional(std::move(task));
		}

		Transaction startTransaction()
		{
			return Transaction(lockMtx, tasks);
		}

		void setCompilerEnvironment(const rg3::llvm::CompilerEnvironment& compilerEnv)
		{
			m_compilerEnv = compilerEnv;
		}

		bool runWorkers(int workersAmount)
		{
			if (workersAmount <= 1)
				return false;

			workers.clear();
			workers.reserve(workersAmount);

			for (int i = 0; i < workersAmount; i++)
			{
				std::thread worker {
					[this, iWorkerIndex = static_cast<size_t>(i)]() {
						workerEntryPoint(iWorkerIndex, m_compilerEnv);
					}
				};

				workers.emplace_back(std::move(worker));
			}

			return true;
		}

		void waitAll()
		{
			for (auto& worker : workers)
			{
				worker.join();
			}
			// now we've done
		}

	 private:
		void workerEntryPoint(size_t iWorkerId, const std::optional<rg3::llvm::CompilerEnvironment>& sCompilerEnvironment)
		{
			struct Visitor
			{
				bool* stopFlag;
				PyFoundSubjects* pAnalyzerStorage { nullptr };
				std::optional<rg3::llvm::CompilerEnvironment> sCompilerEnv { std::nullopt };

				void operator()(NullTask)
				{
					// Do nothing
				}

				void operator()(const StopWorkerTask& task)
				{
					// Stop current loop
					if (stopFlag != nullptr)
					{
						(*stopFlag) = true;
					}
				}

				void operator()(const AnalyzeHeaderTask& analyzeHeader)
				{
					// Do analyze stub
					rg3::llvm::CodeAnalyzer codeAnalyzer { analyzeHeader.headerPath, analyzeHeader.compilerConfig };
					if (sCompilerEnv.has_value())
					{
						// set environment from cache
						codeAnalyzer.setCompilerEnvironment(sCompilerEnv.value());
					}

					rg3::llvm::AnalyzerResult analyzeResult = codeAnalyzer.analyze();

					{
						// Write results (write lock)
						std::unique_lock<std::shared_mutex> guard { pAnalyzerStorage->lockMutex };
						PyGILGuard gilGuard {};

						for (const auto& issue : analyzeResult.vIssues)
						{
							pAnalyzerStorage->pyFoundIssues.append(issue);
						}

						// Iterate over types and trying to push 'em into types db
						for (auto&& type : analyzeResult.vFoundTypes)
						{
							// Note: here we need to assume that type is complete type without any issues, otherwise this type should be ignored!
							switch (type->getKind())
							{
								case cpp::TypeKind::TK_NONE:
									// Unsupported yet, lost, yep
									break;
								case cpp::TypeKind::TK_TRIVIAL:
								{
									auto object = boost::shared_ptr<PyTypeBase>(new PyTypeBase(std::move(type)));
									auto [_iter, bInserted] = pAnalyzerStorage->vFoundTypeInstances.try_emplace(object->getNative()->getPrettyName(), object);

									if (bInserted)
									{
										pAnalyzerStorage->pyFoundTypes.append(object);
									}
								}
								break;
								case cpp::TypeKind::TK_ENUM:
								{
									auto object = boost::shared_ptr<PyTypeEnum>(new PyTypeEnum(std::move(type)));
									auto [_iter, bInserted] = pAnalyzerStorage->vFoundTypeInstances.try_emplace(object->getNative()->getPrettyName(), object);

									if (bInserted)
									{
										pAnalyzerStorage->pyFoundTypes.append(object);
									}
								}
								break;
								case cpp::TypeKind::TK_STRUCT_OR_CLASS:
								{
									auto object = boost::shared_ptr<PyTypeClass>(new PyTypeClass(std::move(type)));
									auto [_iter, bInserted] = pAnalyzerStorage->vFoundTypeInstances.try_emplace(object->getNative()->getPrettyName(), object);

									if (bInserted)
									{
										pAnalyzerStorage->pyFoundTypes.append(object);
									}
								}
								break;
							}
						}
					}
				}
			};


			bool bShouldStop = false;
			Visitor v { &bShouldStop, pAnalyzerStorage, sCompilerEnvironment };

			while (!bShouldStop)
			{
				// Try to extract task or take null task to do nothing
				auto task = takeTask().value_or(NullTask());

				// Run task
				std::visit(v, task);
			}
		}
	};

	PyAnalyzerContext::PyAnalyzerContext()
	{
		m_pContext = std::make_unique<PyAnalyzerContext::RuntimeContext>(&m_pySubjects);
	}

	PyAnalyzerContext::~PyAnalyzerContext()
	{
	}

	void PyAnalyzerContext::setWorkersCount(int workersCount)
	{
		if (m_iWorkersAmount < 1 || m_bInProgress.load(std::memory_order_relaxed))
			return;

		m_iWorkersAmount = workersCount;
	}

	int PyAnalyzerContext::getWorkersCount() const
	{
		return m_iWorkersAmount;
	}

	void PyAnalyzerContext::setHeaders(const boost::python::list& headers)
	{
		if (m_bInProgress.load(std::memory_order_relaxed))
			return;

		m_headersToPrepare.clear();

		for (int i = 0; i < boost::python::len(headers); i++)
		{
			const std::string as_str = boost::python::extract<std::string>(headers[i]);
			m_headersToPrepare.emplace_back(as_str);
		}
	}

	boost::python::list PyAnalyzerContext::getHeaders() const
	{
		boost::python::list result;

		for (const auto& header : m_headersToPrepare)
		{
			result.append(header.string());
		}

		return result;
	}

	void PyAnalyzerContext::setCompilerIncludeDirs(const boost::python::list& includeDirs)
	{
		if (m_bInProgress.load(std::memory_order_relaxed))
			return;

		m_compilerConfig.vIncludes.clear();

		for (int i = 0; i < boost::python::len(includeDirs); ++i)
		{
			m_compilerConfig.vIncludes.emplace_back(boost::python::extract<rg3::llvm::IncludeInfo>(includeDirs[i]));
		}
	}

	boost::python::list PyAnalyzerContext::getCompilerIncludeDirs() const
	{
		boost::python::list result;

		for (const auto& includeInfo : m_compilerConfig.vIncludes)
		{
			result.append(includeInfo);
		}

		return result;
	}

	void PyAnalyzerContext::setCppStandard(rg3::llvm::CxxStandard standard)
	{
		if (m_bInProgress.load(std::memory_order_relaxed))
			return;

		m_compilerConfig.cppStandard = standard;
	}

	rg3::llvm::CxxStandard PyAnalyzerContext::getCppStandard() const
	{
		return m_compilerConfig.cppStandard;
	}

	void PyAnalyzerContext::setCompilerArgs(const boost::python::list& compilerArgs)
	{
		if (m_bInProgress.load(std::memory_order_relaxed))
			return;

		m_compilerConfig.vCompilerArgs.clear();

		for (int i = 0; i < boost::python::len(compilerArgs); i++)
		{
			m_compilerConfig.vCompilerArgs.emplace_back(boost::python::extract<std::string>(compilerArgs[i]));
		}
	}

	boost::python::list PyAnalyzerContext::getCompilerArgs() const
	{
		boost::python::list result;

		for (const auto& compilerArg : m_compilerConfig.vCompilerArgs)
		{
			result.append(compilerArg);
		}

		return result;
	}

	void PyAnalyzerContext::setCompilerDefs(const boost::python::list& compilerDefs)
	{
		if (m_bInProgress.load(std::memory_order_relaxed))
			return;

		m_compilerConfig.vCompilerDefs.clear();

		for (int i = 0; i < boost::python::len(compilerDefs); i++)
		{
			m_compilerConfig.vCompilerDefs.emplace_back(boost::python::extract<std::string>(compilerDefs[i]));
		}
	}

	boost::python::list PyAnalyzerContext::getCompilerDefs() const
	{
		boost::python::list result;

		for (const auto& compilerDef : m_compilerConfig.vCompilerDefs)
		{
			result.append(compilerDef);
		}

		return result;
	}

	void PyAnalyzerContext::setIgnoreRuntimeTag(bool bIgnoreRT)
	{
		if (m_bInProgress.load(std::memory_order_relaxed))
			return;

		m_bIgnoreRuntimeTag = bIgnoreRT;
	}

	bool PyAnalyzerContext::isRuntimeTagIgnored() const
	{
		return m_bIgnoreRuntimeTag;
	}

	boost::python::object PyAnalyzerContext::pyGetTypeOfTypeReference(const rg3::cpp::TypeReference& typeReference)
	{
		// Try to find by type name
		if (auto it = m_pySubjects.vFoundTypeInstances.find(typeReference.getRefName()); it != m_pySubjects.vFoundTypeInstances.end())
		{
			return boost::python::object(it->second);
		}

		// Idk, None itself
		return {};
	}

	const boost::python::list& PyAnalyzerContext::getFoundIssues() const
	{
		if (!isFinished())
		{
			static boost::python::list s_List;
			return s_List;
		}

		return m_pySubjects.pyFoundIssues;
	}

	const boost::python::list& PyAnalyzerContext::getFoundTypes() const
	{
		if (!isFinished())
		{
			static boost::python::list s_List;
			return s_List;
		}

		return m_pySubjects.pyFoundTypes;
	}

	bool PyAnalyzerContext::analyze()
	{
		if (m_bInProgress)
			return false;

		bool bResult = false;

		m_bInProgress = true;
		bResult = runAnalyze();
		m_bInProgress = false;

		return bResult;
	}

	bool PyAnalyzerContext::isFinished() const
	{
		return m_bInProgress == false;
	}

	bool PyAnalyzerContext::runAnalyze()
	{
		// Cleanup known types
		m_pySubjects.pyFoundTypes = {};
		m_pySubjects.pyFoundIssues = {};
		m_pySubjects.vFoundTypeInstances.clear();
		bool bResult = false;

		// Collect compiler environment
		auto environmentExtractResult = rg3::llvm::CompilerConfigDetector::detectSystemCompilerEnvironment();
		if (rg3::llvm::CompilerEnvError* pError = std::get_if<rg3::llvm::CompilerEnvError>(&environmentExtractResult))
		{
			rg3::llvm::AnalyzerResult::CompilerIssue issue;
			issue.kind = rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR;
			issue.sSourceFile = "RG3_GLOBAL_SCOPE";
			issue.sMessage = fmt::format("RG3|Detect compiler environment failed: {}", pError->message);

			m_pySubjects.pyFoundIssues.append(issue);
			return false;
		}

		// Set environment to minimize future clang invocations
		m_pContext->setCompilerEnvironment(*std::get_if<rg3::llvm::CompilerEnvironment>(&environmentExtractResult));

		// Create tasks
		{
			PyGuard pyGuard {};

			{
				auto transaction = m_pContext->startTransaction();
				transaction.clearTasks();

				// Spawn worker tasks
				for (const auto& header : m_headersToPrepare)
				{
					transaction.pushTask(AnalyzeHeaderTask{header, m_compilerConfig});
				}

				// And spawn 'stop' tasks. +2 should be enough
				for (size_t i = 0; i < m_iWorkersAmount; i++)
				{
					transaction.pushTask(StopWorkerTask{});
				}
			}

			// Re-create workers and run analyze
			if (m_pContext->runWorkers(m_iWorkersAmount))
			{
				m_pContext->waitAll();
#if 0  // This stage marked as 'deprecated' and will be removed later.
				// Everything is fine
				bResult = resolveTypeReferences();
#endif
			}
		}

		return bResult;
	}

	void PyAnalyzerContext::pushResolverIssue(const ResolverContext& context, std::string&& errorMessage)
	{
		auto spaceToString = [](ResolverContext::ContextSpace eSpace) -> std::string
		{
			switch (eSpace)
			{
				case ResolverContext::ContextSpace::CS_UNDEFINED: return "UNDEFINED";
				case ResolverContext::ContextSpace::CS_TYPE: return "CXX_TYPE";
				case ResolverContext::ContextSpace::CS_PROPERTY: return "CXX_PROPERTY";
				case ResolverContext::ContextSpace::CS_FUNCTION: return "CXX_TYPE_FUNC";
			}

			return "UNKNOWN";
		};

		rg3::llvm::AnalyzerResult::CompilerIssue issue;
		issue.kind = rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR;
		issue.sSourceFile = context.pOwner->getDefinition().getPath();
		issue.sMessage = fmt::format("RG3|ResolveTypeREF failed: {} (space {})", errorMessage, spaceToString(context.eSpace));

		m_pySubjects.pyFoundIssues.append(issue);
	}

	bool PyAnalyzerContext::resolveTypeReferences()
	{
		// So, here we need to resolve cross-references between types.
		// Known places where we have X-refs:
		// 1. Tags - each tag could have full qualified type reference. Example: /// @my_cool_ref(@ecs::Entity)
		// 2. ClassProperties - has own tags collection
		// 3. ClassFunction - has own tags collection
		//
		for (const auto& [typeName, typeInstance] : m_pySubjects.vFoundTypeInstances)
		{
			const boost::shared_ptr<rg3::cpp::TypeBase> pNative = typeInstance->getNative();
			ResolverContext resolverContext {};

			// Switch to type
			resolverContext.eSpace = ResolverContext::ContextSpace::CS_TYPE;
			resolverContext.pOwner = pNative;

			// Resolve tags
			if (!resolveTags(resolverContext, pNative->getTags()))
			{
				return false;
			}

			// Ok, tags resolved. Now we've need to check what kind of type do we have
			if (pNative->getKind() == cpp::TypeKind::TK_STRUCT_OR_CLASS)
			{
				auto pClassNative = boost::static_pointer_cast<cpp::TypeClass>(pNative);

				// Oh, here we need to resolve 'abstract' references to parent types, aren't we?

				// Resolve properties
				for (auto& classProperty : pClassNative->getProperties())
				{
					// Push 'properties' space
					resolverContext.eSpace = ResolverContext::ContextSpace::CS_PROPERTY;

					if (!resolveTags(resolverContext, classProperty.vTags))
					{
						return false;
					}
				}

				// Resolve functions
				for (auto& classFunction : pClassNative->getFunctions())
				{
					// Push 'functions' space
					resolverContext.eSpace = ResolverContext::ContextSpace::CS_FUNCTION;

					if (!resolveTags(resolverContext, classFunction.vTags))
					{
						return false;
					}
				}

				// Resolve parent type refs
				for (auto& parentType : pClassNative->getParentTypes())
				{
					resolverContext.eSpace = ResolverContext::ContextSpace::CS_TYPE;

					if (auto it = m_pySubjects.vFoundTypeInstances.find(parentType.rParentType.getRefName()); it != m_pySubjects.vFoundTypeInstances.end())
					{
						// parent type found
						parentType.rParentType.setResolvedType(it->second->getNative().get());
					}
					else
					{
						pushResolverIssue(resolverContext, fmt::format("Failed to find parent type '{}'", parentType.rParentType.getRefName()));
						return false;
					}
				}

				// Push context back
				resolverContext.eSpace = ResolverContext::ContextSpace::CS_TYPE;
			}
		}

		// Everything is fine
		return true;
	}

	bool PyAnalyzerContext::resolveTags(const ResolverContext& context, rg3::cpp::Tags& tagsToResolve)
	{
		for (auto& [tagName, tag] : tagsToResolve.getTags())
		{
			if (!tag.hasArguments())
				continue;

			for (auto& tagArg : tag.getArguments())
			{
				if (auto pTypeRef = tagArg.asTypeRefMutable(); pTypeRef && pTypeRef->get() == nullptr)
				{
					// Ok, it holds type reference - need resolve if not resolved yet
					if (auto it = m_pySubjects.vFoundTypeInstances.find(pTypeRef->getRefName()); it != m_pySubjects.vFoundTypeInstances.end())
					{
						// Resolved, push pointer to typeRef
						pTypeRef->setResolvedType(it->second->getNative().get());
					}
					else
					{
						// Type reference not found in context of ...
						pushResolverIssue(context, fmt::format("Reference '{}' not found in types db", pTypeRef->getRefName()));
						return false;
					}
				}
			}
		}

		return true;
	}
}