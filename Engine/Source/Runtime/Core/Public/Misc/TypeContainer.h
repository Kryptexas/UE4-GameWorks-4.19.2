// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


#if PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES

/**
 * Enumerates the scopes for instance creation in type containers.
 */
enum class ETypeContainerScope
{
	/** Create a new instance each time. */
	Instance,

	/** One singleton for the entire process. */
	Process,

	/** One singleton per thread. */
	Thread,
};


/**
 * Implements a type container.
 *
 * Type containers allow for configuring objects and their dependencies without actually
 * creating these objects. They fulfill a role similar to class factories, but without
 * being locked to a particular type of class. When passed into class constructors or
 * methods, type containers can facilitate the Inversion of Control (IoC) pattern.
 *
 * Since UE4 neither uses run-time type information nor pre-processes plain old C++ classes,
 * type names need to be exposed using the Expose_TNameOf macro in order to make them
 * registrable with type containers, i.e. Expose_TNameOf(FMyClass).
 *
 * Once a type is registered with a container, instances of that type can be retrieved from it.
 * There are currently three life time scopes available for instance creation:
 *
 *   1. Unique instance per process (aka. singleton),
 *      using RegisterClass(ETypeContainerScope::Process) or RegisterInstance()
 *   2. Unique instance per thread (aka. thread singleton),
 *      using RegisterClass(ETypeContainerScope::Thread)
 *   3. Unique instance per call (aka. class factory),
 *      using RegisterClass(ETypeContainerScope::Instance) or RegisterFactory()
 *
 * See the file TypeContainerTest.cpp for detailed examples on how to use each of these
 * type registration methods.
 *
 * Note: Type containers depend on variadic templates and are therefore not available
 * on XboxOne at this time, which means they should only be used for desktop applications.
 */
class FTypeContainer
{
	/** Interface for object instance providers. */
	class IInstanceProvider
	{
	public:

		/** Virtual destructor. */
		virtual ~IInstanceProvider() { }

		/**
		 * Gets an instance of a class.
		 *
		 * @return Shared pointer to the instance (must be cast to TSharedPtr<R>).
		 */
		virtual TSharedPtr<void> GetInstance() = 0;
	};

	/** Implements an instance provider that forwards instance requests to a factory function. */
	template<class T>
	struct TFunctionInstanceProvider
		: public IInstanceProvider
	{
		/** Constructor. */
		TFunctionInstanceProvider(TFunction<TSharedPtr<void>()> InCreateFunc)
			: CreateFunc(InCreateFunc)
		{ }

		virtual ~TFunctionInstanceProvider() override { }

		virtual TSharedPtr<void> GetInstance() override
		{
			return CreateFunc();
		}

		/** Factory function that creates the instances. */
		TFunction<TSharedPtr<void>()> CreateFunc;
	};


	/** Implements an instance provider that returns the same instance for all threads. */
	template<class T>
	struct TSharedInstanceProvider
		: public IInstanceProvider
	{
		/** Creates and initializes a new instance. */
		TSharedInstanceProvider(const TSharedRef<T>& InInstance)
			: Instance(InInstance)
		{ }

		/** Virtual destructor. */
		virtual ~TSharedInstanceProvider() { }

		virtual TSharedPtr<void> GetInstance() override
		{
			return Instance;
		}

		/** The stored instance. */
		TSharedRef<T> Instance;
	};


	/** Base class for per-thread instances providers. */
	struct FThreadInstanceProvider
		: public IInstanceProvider
	{
		/** Constructor. */
		FThreadInstanceProvider(TFunction<TSharedPtr<void>()>&& InCreateFunc)
			: CreateFunc(MoveTemp(InCreateFunc))
			, TlsSlot(FPlatformTLS::AllocTlsSlot())
		{ }

		virtual ~FThreadInstanceProvider() override
		{
			FPlatformTLS::FreeTlsSlot(TlsSlot);
		}

		/** Factory function for creating instances. */
		TFunction<TSharedPtr<void>()> CreateFunc;

		/** Slot ID for thread-local storage of the instance. */
		uint32 TlsSlot;
	};


	/** Implements an instance provider that returns the same instance per thread. */
	template<class T>
	struct TThreadInstanceProvider
		: public FThreadInstanceProvider
	{
		/** Constructor. */
		TThreadInstanceProvider(TFunction<TSharedPtr<void>()>&& InCreateFunc)
			: FThreadInstanceProvider(MoveTemp(InCreateFunc))
		{ }

		virtual ~TThreadInstanceProvider() override { }

		virtual TSharedPtr<void> GetInstance() override
		{
			void* Instance = FPlatformTLS::GetTlsValue(TlsSlot);

			if (Instance == nullptr)
			{
				Instance = new TSharedPtr<T>(StaticCastSharedPtr<T>(CreateFunc()));

				/** @todo gmp: this cannot possibly work, because the FRunnableThread is not yet registered.
					A better mechanism for executing code on thread shutdown is already being worked on.
					In the meantime we leak some memory here.
				FRunnableThread::GetThreadRegistry().Lock();
				{
					const uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
					FRunnableThread* RunnableThread = FRunnableThread::GetThreadRegistry().GetThread(ThreadId);

					if (RunnableThread != nullptr)
					{
						Instance = new TSharedPtr<T>((T*)CreateFunc());
						RunnableThread->OnThreadDestroyed().AddStatic(&TThreadInstance::HandleDestroyInstance, Instance);
					}
				}
				FRunnableThread::GetThreadRegistry().Unlock();*/

				FPlatformTLS::SetTlsValue(TlsSlot, Instance);
				// @see ITlsAutoCleanup
				//Instance->Register();
			}

			return *((TSharedPtr<void>*)Instance);
		}

		/** Callback for destroying per-thread instances. */
		static void HandleDestroyInstance(void* Instance)
		{
			auto InstancePtr = (TSharedPtr<T>*)Instance;
			delete InstancePtr;
		}
	};

public:

	/**
	 * Gets a shared pointer to an instance of the specified class.
	 *
	 * @param T The type of class to get an instance for.
	 * @param A shared reference to the instance.
	 * @see RegisterClass, RegisterDelegate, RegisterFactory, RegisterInstance
	 */
	template<class R>
	TSharedRef<R> GetInstance()
	{
		FScopeLock Lock(&CriticalSection);
		{
			const TSharedPtr<IInstanceProvider>& Provider = Providers.FindRef(TNameOf<R>::GetName());
			check(Provider.IsValid());

			return StaticCastSharedPtr<R>(Provider->GetInstance()).ToSharedRef();
		}
	}

	/**
	 * Registers a class for instances of the specified class.
	 *
	 * @param R The type of class to register an instance class for.
	 * @param T Tye type of class to register (must be the same as or derived from R).
	 * @param P The constructor parameters that the class requires.
	 * @param Scope The scope at which instances must be unique (default = Process).
	 * @see RegisterDelegate, RegisterInstance, Unregister
	 */
	template<class R, class T, typename... P>
	void RegisterClass(ETypeContainerScope Scope = ETypeContainerScope::Process)
	{
		static_assert(TPointerIsConvertibleFromTo<T, R>::Value, "T must implement R");

		TSharedPtr<IInstanceProvider> Provider;
		
		switch(Scope)
		{
		case ETypeContainerScope::Instance:
			Provider = MakeShareable(
				new TFunctionInstanceProvider<T>(
					[this]() -> TSharedPtr<void> {
						return MakeShareable(new T(GetInstance<P>()...));
					}
				)
			);
			break;

		case ETypeContainerScope::Thread:
			Provider = MakeShareable(
				new TThreadInstanceProvider<T>(
					[this]() -> TSharedPtr<void> {
						return MakeShareable(new T(GetInstance<P>()...));
					}
				)
			);
			break;

		default:
			Provider = MakeShareable(
				new TSharedInstanceProvider<T>(
					MakeShareable(new T(GetInstance<P>()...))
				)
			);
			break;
		}

		AddProvider(TNameOf<R>::GetName(), Provider);
	}

	/**
	 * Register a factory delegate for the specified class.
	 *
	 * @param R The type of class to register an instance class for.
	 * @param D Tye type of factory delegate to register.
	 * @param P The parameters that the delegate requires.
	 * @param Delegate The delegate to register.
	 */
	template<class R, class D, typename... P>
	void RegisterDelegate(D Delegate)
	{
		static_assert(TAreTypesEqual<TSharedRef<R>, typename D::RetValType>::Value, "Delegate return type must be TSharedPtr<R>");

		TSharedPtr<IInstanceProvider> Provider = MakeShareable(
			new TFunctionInstanceProvider<R>(
				[=]() -> TSharedPtr<void> {
					return Delegate.Execute(GetInstance<P>()...);
				}
			)
		);

		AddProvider(TNameOf<R>::GetName(), Provider);
	}

	/**
	 * Register a factory function for the specified class.
	 *
	 * @param R The type of class to register the instance for.
	 * @param P Additional parameters that the factory function requires.
	 * @param CreateFunc The factory function that will create instances.
	 * @see RegisterClass, RegisterInstance, Unregister
	 */
	template<class R>
	void RegisterFactory(const TFunction<TSharedRef<R>()>& CreateFunc)
	{
		TSharedPtr<IInstanceProvider> Provider = MakeShareable(
			new TFunctionInstanceProvider<R>(
				[=]() -> TSharedPtr<void> {
					return CreateFunc();
				}
			)
		);

		AddProvider(TNameOf<R>::GetName(), Provider);
	}

	/**
	 * Register a factory function for the specified class.
	 *
	 * @param R The type of class to register the instance for.
	 * @param P0 The first parameter that the factory function requires.
	 * @param P Additional parameters that the factory function requires.
	 * @param CreateFunc The factory function that will create instances.
	 * @see RegisterClass, RegisterInstance, Unregister
	 */
	template<class R, typename P0, typename... P>
	void RegisterFactory(const TFunction<TSharedRef<R>(TSharedRef<P0>, TSharedRef<P>...)>& CreateFunc)
	{
		TSharedPtr<IInstanceProvider> Provider = MakeShareable(
			new TFunctionInstanceProvider<R>(
				[=]() -> TSharedPtr<void> {
					return CreateFunc(GetInstance<P0>(), GetInstance<P>()...);
				}
			)
		);

		AddProvider(TNameOf<R>::GetName(), Provider);
	}

#ifdef __clang__
	/**
	 * Register a factory function for the specified class.
	 *
	 * This is a Clang specific overload for handling raw function pointers.
	 *
	 * @param R The type of class to register the instance for.
	 * @param P Additional parameters that the factory function requires.
	 * @param CreateFunc The factory function that will create instances.
	 * @see RegisterClass, RegisterInstance, Unregister
	 */
	template<class R, typename... P>
	void RegisterFactory(TSharedRef<R> (*CreateFunc)(TSharedRef<P>...))
	{
		TSharedPtr<IInstanceProvider> Provider = MakeShareable(
			new TFunctionInstanceProvider<R>(
				[=]() -> TSharedPtr<void> {
					return CreateFunc(GetInstance<P>()...);
				}
			)
		);

		AddProvider(TNameOf<R>::GetName(), Provider);
	}
#endif

	/**
	 * Registers an existing instance for the specified class.
	 *
	 * @param R The type of class to register the instance for.
	 * @param T The type of the instance to register (must be the same as or derived from R).
	 * @param Instance The instance of the class to register.
	 * @see RegisterClass, RegisterDelegate, Unregister
	 */
	template<class R, class T>
	void RegisterInstance(const TSharedRef<T>& Instance)
	{
		static_assert(TPointerIsConvertibleFromTo<T, R>::Value, "T must implement R");

		AddProvider(TNameOf<R>::GetName(), MakeShareable(new TSharedInstanceProvider<R>(Instance)));
	}

	/**
	 * Unregisters the instance or class for a previously registered class.
	 *
	 * @param R The type of class to unregister.
	 * @see RegisterClass, RegisterDelegate, RegisterInstance
	 */
	template<class R>
	void Unregister()
	{
		FScopeLock Lock(&CriticalSection);
		{
			Providers.Remove(TNameOf<R>::GetName());
		}
	}

protected:

	/**
	 * Add an instance provider to the container.
	 *
	 * @param Name The name of the type to add the provider for.
	 * @param Provider The instance provider.
	 */
	void AddProvider(const TCHAR* Name, const TSharedPtr<IInstanceProvider>& Provider)
	{
		FScopeLock Lock(&CriticalSection);
		{
			Providers.Add(Name, Provider);
		}
	}

private:

	/** Critical section for synchronizing access. */
	FCriticalSection CriticalSection;

	/** Maps class names to instance providers. */
	TMap<const TCHAR*, TSharedPtr<IInstanceProvider>> Providers;
};


#endif //PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
