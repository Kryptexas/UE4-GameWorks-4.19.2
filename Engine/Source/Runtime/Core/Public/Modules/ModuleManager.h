// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "ModuleInterface.h"
#include "Delegate.h"
#include "Boilerplate/ModuleBoilerplate.h"

#if !IS_MONOLITHIC
	/** If true, we are reloading a class for HotReload */
	extern CORE_API bool		GIsHotReload;
#endif

namespace ELoadModuleFailureReason
{
	enum Type
	{
		Success,
		FileNotFound,
		FileIncompatible,
		CouldNotBeLoadedByOS,
		FailedToInitialize
	};
}

namespace EModuleCompileMethod
{
	enum Type
	{
		Runtime,
		External,
		Unknown
	};
}

namespace EModuleChangeReason
{
	/** Reasons that an FModuleManager::OnModuleChanged() delegate will be called */
	enum Type
	{
		/** A module has been loaded and is ready to be used */
		ModuleLoaded,

		/* A module has been unloaded and should no longer be used */
		ModuleUnloaded,

		/** The paths controlling which plugins are loaded have been changed and the given module has been found, but not yet loaded */
		PluginDirectoryChanged
	};
}

/**
 * Module Manager is used to load and unload modules, as well as to keep track of all of the
 * modules that are currently loaded.  You can access this singleton using FModuleManager::Get().
 */
class CORE_API FModuleManager : private FSelfRegisteringExec
{

public:

	/**
	 * Status of a module.  Used only for reporting information to external objects.
	 */
	struct FModuleStatus
	{
		/** Default constructor. */
		FModuleStatus()
			: bIsLoaded( false )
			, bIsGameModule( false )
		{}

		/** Short name for this module */
		FString Name;

		/** Full path to this module file on disk */
		FString FilePath;

		/** Whether the module is currently loaded or not */
		bool bIsLoaded;

		/** Whether this module contains gameplay code */
		bool bIsGameModule;

		/** The compilation method of this module */
		FString CompilationMethod;
	};

	/** Declare delegate that for binding functions to be called when modules are loaded, or unloaded, or
	    our set of known modules changes. Passes in the name of the module that changed, and the reason. */
	DECLARE_EVENT_TwoParams( FModuleManager, FModulesChangedEvent, FName, EModuleChangeReason::Type );
	FModulesChangedEvent& OnModulesChanged() { return ModulesChangedEvent; }

	/** Delegate for binding functions to be called when starting the module compiling */
	DECLARE_EVENT( FModuleManager, FModuleCompilerStartedEvent );
	FModuleCompilerStartedEvent& OnModuleCompilerStarted() { return ModuleCompilerStartedEvent; }

	/** Delegate for binding functions to be called when the module compiler finishes,
		passing in the compiler output and a boolean which indicates success or failure
		as well as a second boolean which determines if the log should be shown */
	DECLARE_EVENT_ThreeParams( FModuleManager, FModuleCompilerFinishedEvent, const FString&, bool, bool );
	FModuleCompilerFinishedEvent& OnModuleCompilerFinished() { return ModuleCompilerFinishedEvent; }

	/** Called after a module recompile finishes.  First argument specifies whether the compilation has finished, 
		Second argument specifies whether the compilation was successful or not */
	DECLARE_DELEGATE_TwoParams( FRecompileModulesCallback, bool, bool );

	/** Called when any loaded objects need to be processed. */
	FSimpleMulticastDelegate& OnProcessLoadedObjectsCallback() { return ProcessLoadedObjectsCallback; }

	/** When module manager is linked against an application that supports UObjects, this delegate will be primed
	    at startup to provide information about whether a UObject package is loaded into memory. */
	DECLARE_DELEGATE_RetVal_OneParam( bool, FIsPackageLoadedCallback, FName );
	FIsPackageLoadedCallback& IsPackageLoadedCallback() { return IsPackageLoaded; }


	/**
	 * Static: Access single instance of module manager
	 *
	 * @return	Reference to the module manager singleton object
	 */
	static FModuleManager& Get();


	/** Destructor */
	~FModuleManager();


	/**
	 * Module manager ticking is only used to check for asynchronously compiled modules that may need to be reloaded
	 */
	void Tick();


	/**
	 * Looks on the disk for loadable modules matching a wildcard
	 *
	 * @param	WildcardWithoutExtension		Filename part (no path, no extension, no build config info) to search for
	 * @param	OutModules					List of modules found
	 */
	void FindModules(const TCHAR* WildcardWithoutExtension, TArray<FName>& OutModules);

	/**
	 * Checks to see if the specified module is currently loaded.  This is an O(1) operation.
	 *
	 * @param	InModuleName		The base name of the module file.  Should not include path, extension or platform/configuration info.  This is just the "module name" part of the module file name.  Names should be globally unique.
	 *
	 * @return	True if module is currently loaded, otherwise false
	 */
	bool IsModuleLoaded( const FName InModuleName ) const;


	/**
	 * Adds a module to our list of modules and tries to load it immediately
	 *
	 * @param	InModuleName	The base name of the module file.  Should not include path, extension or platform/configuration info.  This is just the "module name" part of the module file name.  Names should be globally unique.
	 * @param bWasReloaded	Indicates that the modue has been reloaded.
	 *
	 * @return	The loaded module (null if the load operation failed)
	 */
	TSharedPtr<IModuleInterface> LoadModule( const FName InModuleName, const bool bWasReloaded=false );
	TSharedPtr<IModuleInterface> LoadModuleWithFailureReason( const FName InModuleName, ELoadModuleFailureReason::Type& OutFailureReason, const bool bWasReloaded=false );


	/**
	 * Unloads a specific module
	 *
	 * @param	InModuleName	The name of the module to unload.  Should not include path, extension or platform/configuration info.  This is just the "module name" part of the module file name.
	 * @param   bIsShutdown		Is this unload module call occurring at shutdown
	 *
	 * @return	True if module was unloaded successfully
	 */
	bool UnloadModule( const FName InModuleName, bool bIsShutdown=false );


	/**
	 * Abandons a loaded module, leaving it loaded in memory but no longer tracking it in the module manager
	 *
	 * @param	InModuleName	The name of the module to abandon.  Should not include path, extension or platform/configuration info.  This is just the "module name" part of the module file name.
	 */
	void AbandonModule( const FName InModuleName );


	/**
	 * Unloads modules during the shutdown process.  Usually called at various points while
	 * while exiting the application.
	 */
	void UnloadModulesAtShutdown();


	/**
	 * Given a module name, returns a shared reference to that module's interface.  If the module is unknown or not loaded, this will assert!
	 *
	 * @param	InModuleName	Name of the module to return
	 *
	 * @return	A shared reference to the module
	 */
	TSharedRef< IModuleInterface > GetModuleInterfaceRef( const FName InModuleName );


	/**
	 * Given a module name, returns a reference to that module's interface.  If the module is unknown or not loaded, this will assert!
	 *
	 * @param	InModuleName	Name of the module to return
	 *
	 * @return	A reference to the module
	 */
	IModuleInterface& GetModuleInterface( const FName InModuleName );


	/**
	  * Static: Finds a module by name, checking to ensure it exists.
	  * The return value is the loaded module, casted to the template parameter type (this is an unchecked cast)
	  *
	  * @param	ModuleName	The module to find
	  *
	  * @return	Returns the module interface, casted to the specified typename
	  */
	template<typename TModuleInterface>
	static TModuleInterface& GetModuleChecked( const FName ModuleName )
	{
		FModuleManager& ModuleManager = FModuleManager::Get();

		// Make sure the module is loaded now.
		checkf( ModuleManager.IsModuleLoaded(ModuleName), TEXT("Tried to get module interface for unloaded module: '%s'"), *(ModuleName.ToString()));
		return (TModuleInterface&)(ModuleManager.GetModuleInterface(ModuleName));
	}


	/**
	  * Static: Finds a module by name, checking to ensure it exists and loading it if not already loaded.
	  * The return value is the loaded module, casted to the template parameter type (this is an unchecked cast)
	  *
	  * @param	ModuleName	The module to find and load
	  *
	  * @return	Returns the module interface, casted to the specified typename
	  */
	template<typename TModuleInterface>
	static TModuleInterface& LoadModuleChecked( const FName ModuleName )
	{
		FModuleManager& ModuleManager = FModuleManager::Get();

		if (!ModuleManager.IsModuleLoaded(ModuleName))
		{
			// Ensure the module is loaded.
			ModuleManager.LoadModule(ModuleName);
		}

		return (TModuleInterface&)(ModuleManager.GetModuleInterface(ModuleName));
	}

	/**
	  * Static: Finds a module by name, checking to ensure it exists and loading it if not already loaded.
	  * The return value is the loaded module, casted to the template parameter pointer type (this is an unchecked cast)
	  *
	  * @param	ModuleName	The module to find and load
	  *
	  * @return	Returns the module interface, casted to the specified typename, or NULL if the module could not be loaded
	  */
	template<typename TModuleInterface>
	static TModuleInterface* LoadModulePtr( const FName ModuleName )
	{
		FModuleManager& ModuleManager = FModuleManager::Get();

		if (!ModuleManager.IsModuleLoaded(ModuleName))
		{
			ModuleManager.LoadModule(ModuleName);
			if (!ModuleManager.IsModuleLoaded(ModuleName))
			{
				return NULL;
			}
		}
		return (TModuleInterface*)(&ModuleManager.GetModuleInterface(ModuleName));
	}

	/**
	 * Queries information about a specific module name
	 *
	 * @param	InModuleName	Module to query status for
	 * @param	OutModuleStatus	Status of the specified module
	 *
	 * @return	True if the module was found the OutModuleStatus is valid
	 */
	bool QueryModule( const FName InModuleName, FModuleStatus& OutModuleStatus );


	/**
	 * Queries information about all of the currently known modules
	 *
	 * @param	OutModuleStatuses	Status of all modules
	 */
	void QueryModules( TArray< FModuleStatus >& OutModuleStatuses );

	/**
	 * Query to determine current module count
	 *
	 * @return	the number of modules
	 */
	int32 GetModuleCount() const { return Modules.Num(); }

	/**
	 * Checks for the solution file using the hard-coded location on disk
	 * Used to determine whether source code is potentially available for recompiles
	 * 
	 * @return	True if the solution file is found (source code MAY BE available)
	 */
	bool IsSolutionFilePresent();

	/**
	 * Returns the full path of the solution file
	 * 
	 * @return	SolutionFilepath
	 */
	FString GetSolutionFilepath();

	/**
	 * Tries to recompile the specified module.  If the module is loaded, it will first be unloaded (then reloaded after,
	 * if the recompile was successful.)
	 *
	 * @param	InModuleName			Name of the module to recompile
	 * @param	bReloadAfterRecompile	If true, the module will automatically be reloaded after a successful compile.  Otherwise, you'll need to load it yourself after.
	 * @param	Ar						Output device for logging compilation status
	 *
	 * @return	Returns true if the module was successfully recompiled (and reloaded, if it was previously loaded.)
	 */
	bool RecompileModule( const FName InModuleName, const bool bReloadAfterRecompile, FOutputDevice &Ar );

	/** @return	Returns true if an asynchronous compile is currently in progress */
	bool IsCurrentlyCompiling() const;

	/**
	 * Tries to recompile the specified modules in the background.  When recompiling finishes, the specified callback
	 * delegate will be triggered, passing along a bool that tells you whether the compile action succeeded.  This
	 * function never tries to unload modules or to reload the modules after they finish compiling.  You should do
	 * that yourself in the recompile completion callback!
	 *
	 * @param	ModuleNames					Names of the modules to recompile
	 * @param	RecompileModulesCallback	Callback function to execute after compilation finishes (whether successful or not.)
	 * @param	bWaitForCompletion			True if the function should not return until recompilation attempt has finished and callbacks have fired
	 * @param	Ar							Output device for logging compilation status
	 *
	 * @return	True if the recompile action was kicked off successfully.  If this returns false, then the recompile callback will never fire.  In the case where bWaitForCompletion=false, this will also return false if the compilation failed for any reason.
	 */
	bool RecompileModulesAsync( const TArray< FName > ModuleNames, const FRecompileModulesCallback& RecompileModulesCallback, const bool bWaitForCompletion, FOutputDevice &Ar );

	/** Request that any current compilation operation be abandoned */
	void RequestStopCompilation()
	{
		bRequestCancelCompilation = true;
	}

	/**
	 * Tries to compile the specified game project. Not used for recompiling modules that are already loaded.
	 *
	 * @param	GameProjectFilename		The filename (including path) of the game project to compile
	 * @param	Ar						Output device for logging compilation status
	 *
	 * @return	Returns true if the project was successfully compiled
	 */
	bool CompileGameProject( const FString& GameProjectFilename, FOutputDevice &Ar );

	/**
	 * Tries to compile the specified game projects editor. Not used for recompiling modules that are already loaded.
	 *
	 * @param	GameProjectFilename		The filename (including path) of the game project to compile
	 * @param	Ar						Output device for logging compilation status
	 *
	 * @return	Returns true if the project was successfully compiled
	 */
	bool CompileGameProjectEditor( const FString& GameProjectFilename, FOutputDevice &Ar );

	/**
	 * Tries to compile the specified game project. Not used for recompiling modules that are already loaded.
	 *
	 * @param	GameProjectFilename		The filename (including path) of the game project for which to generate code projects
	 * @param	Ar						Output device for logging compilation status
	 *
	 * @return	Returns true if the project was successfully compiled
	 */
	bool GenerateCodeProjectFiles( const FString& GameProjectFilename, FOutputDevice &Ar );

	/**
	 * @return true if the UBT executable exists (in Rocket) or source code is available to compile it (in non-Rocket)
	 */
	bool IsUnrealBuildToolAvailable();


	/** Delegate that's used by the module manager to initialize a registered module that we statically linked with (monolithic only) */
	DECLARE_DELEGATE_RetVal( IModuleInterface*, FInitializeStaticallyLinkedModule )

	/**
	 * Registers an initializer for a module that is statically linked
	 *
	 * @param	InModuleName	The name of this module
	 * @param	InInitializerDelegate	The delegate that will be called to initialize an instance of this module
	 */
	void RegisterStaticallyLinkedModule( const FName InModuleName, const FInitializeStaticallyLinkedModule& InInitializerDelegate )
	{
		StaticallyLinkedModuleInitializers.Add( InModuleName, InInitializerDelegate );
	}


	/** Called by the engine at startup to let the Module Manager know that it's now safe to process
	    new UObjects discovered by loading C++ modules */
	void StartProcessingNewlyLoadedObjects();

	/** Adds an engine binaries directory. */
	void AddBinariesDirectory(const TCHAR *InDirectory, bool bIsGameDirectory);

	/**
	 *	Set the game binaries directory
	 *
	 *	@param	InDirectory		The game binaries directory
	 */
	void SetGameBinariesDirectory(const TCHAR* InDirectory);


	/**
	 * Adds a module to our list of modules, unless it's already known
	 *
	 * @param	InModuleName			The base name of the module file.  Should not include path, extension or platform/configuration info.  This is just the "name" part of the module file name.  Names should be globally unique.
	 * @param	InBinariesDirectory		The directory where to find this file, or an empty string to search in the default locations.  This parameter is used by the plugin system to locate plugin binaries.
	 */
	void AddModule( const FName InModuleName );

	/**
	 * Determines whether the specified module contains UObjects.  The module must already be loaded into
	 * memory before calling this function.
	 *
	 * @param	ModuleName	Name of the loaded module to check
	 *
	 * @return	True if the module was found to contain UObjects, or false if it did not (or wasn't loaded.)
	 */
	bool DoesLoadedModuleHaveUObjects( const FName ModuleName );

private:

	/** Private constructor.  Singleton instance is always constructed on demand. */
	FModuleManager()
		: bCanProcessNewlyLoadedObjects( false )
//		,  ModuleCompileProcessHandle( NULL )
		, ModuleCompileReadPipe( NULL )
		, bRequestCancelCompilation(false)
	{
	}

	/**
	 * Helper structure to hold on to module state while asynchronously recompiling DLLs
	 */
	struct FModuleToRecompile
	{
		/** Name of the module */
		FString ModuleName;

		/** Desired module file name suffix, or empty string if not needed */
		FString ModuleFileSuffix;

		/** The module file name to use after a compilation succeeds, or an empty string if not changing */
		FString NewModuleFilename;
	};

	/**
	 * Helper structure to store the compile time and method for a module
	 */
	struct FModuleCompilationData
	{
		/** A flag set when the data it updated - loaded modules don't update this info until they are compiled or just before they unload */
		bool bIsValid;

		/** Has a timestamp been set for the .dll file */
		bool bHasFileTimeStamp;

		/** Last known timestamp for the .dll file */
		FDateTime FileTimeStamp;

		/** Last known compilation method of the .dll file */
		EModuleCompileMethod::Type CompileMethod;

		FModuleCompilationData()
			: bIsValid(false)
			, bHasFileTimeStamp(false)
			, CompileMethod(EModuleCompileMethod::Unknown)
		{
		}
	};

	/**
	 * Information about a single module (may or may not be loaded.)
	 */
	class FModuleInfo
	{
	public:
		/** The original file name of the module, without any suffixes added */
		FString OriginalFilename;

		/** File name of this module (.dll file name) */
		FString Filename;

		/** Handle to this module (DLL handle), if it's currently loaded */
		void* Handle;

		/** The module object for this module.  We actually *own* this module, so it's lifetime is controlled
		    by the scope of this shared pointer. */
		TSharedPtr< IModuleInterface > Module;

		/** True if this module was unloaded at shutdown time, and we never want it to be loaded again */
		bool bWasUnloadedAtShutdown;

		/** Arbitrary number that encodes the load order of this module, so we can shut them down in reverse order. */
		int32 LoadOrder;

		/** Last know compilation data for this module - undefined if CompileData.bIsValid is false */
		FModuleCompilationData CompileData;

		/** static that tracks the current load number. Incremented whenever we add a new module*/
		static int32 CurrentLoadOrder;

	public:

		/** Constructor */
		FModuleInfo()
			: Handle( NULL ),
			  bWasUnloadedAtShutdown( false ),
			  LoadOrder(CurrentLoadOrder++)
		{
		}
	};

	/**
	 * Tries to recompile the specified DLL using UBT. Does not interact with modules. This is a low level routine.
	 *
	 * @param	ModuleNames			List of modules to recompile, including the module name and optional file suffix
	 * @param	Ar					Output device for logging compilation status
	 */
	bool RecompileModuleDLLs( const TArray< FModuleToRecompile >& ModuleNames, FOutputDevice &Ar );

public:
	/** Calls PreUnload then either unloads or abandons a module in memory, depending on whether the module supports unloading */;
	void UnloadOrAbandonModuleWithCallback( const FName InModuleName, FOutputDevice &Ar );

	/** Loads a module in memory then calls PostLoad */;
	bool LoadModuleWithCallback( const FName InModuleName, FOutputDevice &Ar );

private:
	/** Generates a unique file name for the specified module name by adding a random suffix and checking for file collisions */
	void MakeUniqueModuleFilename( const FName InModuleName, FString& UniqueSuffix, FString& UniqueModuleFileName );

	/** 
	 *	Starts compiling DLL files for one or more modules 
	 *
	 *	@param	GameName							The name of the game
	 *	@param	ModuleNames							The list of modules to compile
	 *	@param	InRecompileModulesCallback			Callback function to make when module recompiles
	 *	@param	Ar									
	 *	@param	bInFailIfGeneratedCodeChanges		If true, fail the compilation if generated headers change
	 *	@param	InAdditionalCmdLineArgs				Additional arguments to pass to UBT
	 *
	 *	@return	bool								true if successful, false if not
	 */
	bool StartCompilingModuleDLLs( const FString& GameName, const TArray< FModuleToRecompile >& ModuleNames, 
		const FRecompileModulesCallback& InRecompileModulesCallback, FOutputDevice& Ar, bool bInFailIfGeneratedCodeChanges, 
		const FString& InAdditionalCmdLineArgs = FString() );

	/** Returns the path to the unreal build tool source code */
	FString GetUnrealBuildToolSourceCodePath();

	/** Returns the filename for UBT including the path */
	FString GetUnrealBuildToolExecutableFilename();

	/** Builds unreal build tool using a compiler specific to the currently running platform */
	bool BuildUnrealBuildTool(FOutputDevice &Ar);

	/** Launches UnrealBuildTool with the specified command line parameters */
	bool InvokeUnrealBuildTool(const FString& InCmdLineParams, FOutputDevice &Ar);

	/** Checks to see if a pending compilation action has completed and optionally waits for it to finish.  If completed, fires any appropriate callbacks and reports status provided bFireEvents is true. */
	void CheckForFinishedModuleDLLCompile( const bool bWaitForCompletion, bool& bCompileStillInProgress, bool& bCompileSucceeded, FOutputDevice& Ar, const FText& SlowTaskOverrideTest = FText::GetEmpty(), bool bFireEvents = true );

	/** Compares file versions between the current executing engine version and the specified dll */
	static bool CheckModuleCompatibility(const TCHAR *Filename);

	// Begin FExec interface.
	virtual bool Exec( UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE;
	// End of FExec interface.

	/** @returns Static: Returns arguments to pass to UnrealBuildTool when compiling modules */
	static FString MakeUBTArgumentsForModuleCompiling();

	/** Called during CheckForFinishedModuleDLLCompile() for each successfully recomplied module */
	void OnModuleCompileSucceeded(FName ModuleName, TSharedRef<FModuleInfo> ModuleInfo);

	/** Called when the compile data for a module need to be update in memory and written to config */
	void UpdateModuleCompileData(FName ModuleName, TSharedRef<FModuleInfo> ModuleInfo);

	/** Called when a new module is added to the manager to get the saved compile data from config */
	static void ReadModuleCompilationInfoFromConfig(FName ModuleName, TSharedRef<FModuleInfo> ModuleInfo);

	/** Saves the module's compile data to config */
	static void WriteModuleCompilationInfoToConfig(FName ModuleName, TSharedRef<FModuleInfo> ModuleInfo);

	/** Access the module's file and read the timestamp from the file system. Returns true if the timestamp was read successfully. */
	static bool GetModuleFileTimeStamp(TSharedRef<const FModuleInfo> ModuleInfo, FDateTime& OutFileTimeStamp);

	/** Finds modules matching a given name wildcard. */
	void FindModulePaths(const TCHAR *NamePattern, TMap<FName, FString> &OutModulePaths);

	/** Finds modules matching a given name wildcard within a given directory. */
	void FindModulePathsInDirectory(const FString &DirectoryName, bool bIsGameDirectory, const TCHAR *NamePattern, TMap<FName, FString> &OutModulePaths);

private:

	/** Map of all modules.  Maps the case-insensitive module name to information about that module, loaded or not. */
	typedef TMap< FName, TSharedRef< FModuleInfo > > FModuleMap;
	FModuleMap Modules;


	/** Map of module names to a delegate that can initialize each respective statically linked module */
	typedef TMap< FName, FInitializeStaticallyLinkedModule > FStaticallyLinkedModuleInitializerMap;
	FStaticallyLinkedModuleInitializerMap StaticallyLinkedModuleInitializers;

	/** True if module manager should automatically register new UObjects discovered while loading C++ modules */
	bool bCanProcessNewlyLoadedObjects;

	/** Multicast delegate that will broadcast a notification when modules are loaded, unloaded, or
	    our set of known modules changes */
	FModulesChangedEvent ModulesChangedEvent;
	
	/** Multicast delegate which will broadcast a notification when the compiler starts */
	FModuleCompilerStartedEvent ModuleCompilerStartedEvent;
	
	/** Multicast delegate which will broadcast a notification when the compiler finishes */
	FModuleCompilerFinishedEvent ModuleCompilerFinishedEvent;

	/** Multicast delegate called to process any new loaded objects. */
	FSimpleMulticastDelegate ProcessLoadedObjectsCallback;

	///
	/// Async module recompile
	///

	/** When compiling a module using an external application, stores the handle to the process that is running */
	FProcHandle ModuleCompileProcessHandle;

	/** When compiling a module using an external application, this is the process read pipe handle */
	void* ModuleCompileReadPipe;

	/** When compiling a module using an external application, this is the text that was read from the read pipe handle */
	FString ModuleCompileReadPipeText;

	/** Callback to execute after an asynchronous recompile has completed (whether successful or not.) */
	FRecompileModulesCallback RecompileModulesCallback;

	/** When module manager is linked against an application that supports UObjects, this delegate will be primed
	    at startup to provide information about whether a UObject package is loaded into memory. */
	FIsPackageLoadedCallback IsPackageLoaded;

	/** Array of modules that we're currently recompiling */
	TArray< FModuleToRecompile > ModulesBeingCompiled;

	/** Array of modules that we're going to recompile */
	TArray< FModuleToRecompile > ModulesThatWereBeingRecompiled;

	/** true if we should attempt to cancel the current async compilation */
	bool bRequestCancelCompilation;

	/** Array of engine binaries directories. */
	TArray<FString> EngineBinariesDirectories;

	/** Array of game binaries directories. */
	TArray<FString> GameBinariesDirectories;
};

/**
 * Utility class for registering modules that are statically linked
 */
template< class ModuleClass >
class FStaticallyLinkedModuleRegistrant
{

public:

	/**
	 * Explicit constructor that registers a statically linked module
	 */
	FStaticallyLinkedModuleRegistrant( const ANSICHAR* InModuleName )
	{
		// Create a delegate to our InitializeModule method
		FModuleManager::FInitializeStaticallyLinkedModule InitializerDelegate = FModuleManager::FInitializeStaticallyLinkedModule::CreateRaw(
				this, &FStaticallyLinkedModuleRegistrant<ModuleClass>::InitializeModule );

		// Register this module
		FModuleManager::Get().RegisterStaticallyLinkedModule(
			FName( InModuleName ),	// Module name
			InitializerDelegate );	// Initializer delegate
	}

	
	/** Called by the module manager (via delegate above) to initialize this statically-linked module */
	IModuleInterface* InitializeModule()
	{
		return new ModuleClass();
	}
};


/**
 * Function pointer type for InitializeModule().  All modules must have an InitializeModule() function.
 * Usually this is declared automatically using the IMPLEMENT_MODULE macro below.
 *
 * The function must be declared using as 'extern "C"' so that the name remains undecorated.
 *
 * The object returned will be "owned" by the caller, and will be deleted by the caller before the module is unloaded.
 */
typedef IModuleInterface* ( *FInitializeModuleFunctionPtr )( void );


/**
 * A default minimal implementation of a module that does nothing at startup and shutdown
 */
class FDefaultModuleImpl
	: public IModuleInterface
{
};


/**
 * Default minimal module class for gameplay modules.  Does nothing at startup and shutdown.
 */
class FDefaultGameModuleImpl
	: public FDefaultModuleImpl
{
	/**
	 * Returns true if this module hosts gameplay code
	 *
	 * @return True for "gameplay modules", or false for engine code modules, plugins, etc.
	 */
	virtual bool IsGameModule() const OVERRIDE
	{
		return true;
	}
};

/**
 * The IMPLEMENT_MODULE macro is used to expose your module's main class to the rest of the engine.
 * You must use this macro in one of your modules C++ modules, in order for the 'InitializeModule'
 * function to be declared in such a way that the engine can find it.  Also, this macro will handle
 * the case where a module is statically linked with the engine instead of dynamically loaded.
 *
 * Usage:   IMPLEMENT_MODULE( <My Module Class>, <Module name string> )
 */

// If we're linking monolithically we assume all modules are linked in with the main binary.
#if IS_MONOLITHIC

	#define IMPLEMENT_MODULE( ModuleImplClass, ModuleName ) \
		/** Global registrant object for this module when linked statically */ \
		static FStaticallyLinkedModuleRegistrant< ModuleImplClass > ModuleRegistrant##ModuleName( #ModuleName ); \
		/** Implement an empty function so that if this module is built as a statically linked lib, */ \
		/** static initialization for this lib can be forced by referencing this symbol */ \
		void EmptyLinkFunctionForStaticInitialization##ModuleName(){} \
		PER_MODULE_BOILERPLATE_ANYLINK(ModuleImplClass, ModuleName)

#else	// IS_MONOLITHIC

	#define IMPLEMENT_MODULE( ModuleImplClass, ModuleName ) \
		\
		/**/ \
		/* InitializeModule function, called by module manager after this module's DLL has been loaded */ \
		/**/ \
		/* @return	Returns an instance of this module */ \
		/**/ \
		extern "C" DLLEXPORT IModuleInterface* InitializeModule() \
		{ \
			return new ModuleImplClass(); \
		} \
		PER_MODULE_BOILERPLATE \
		PER_MODULE_BOILERPLATE_ANYLINK(ModuleImplClass, ModuleName)

#endif	// !IS_MONOLITHIC

/** IMPLEMENT_GAME_MODULE is used for modules that contain gameplay code */
#define IMPLEMENT_GAME_MODULE( ModuleImplClass, ModuleName ) \
	IMPLEMENT_MODULE( ModuleImplClass, ModuleName )

#if IS_PROGRAM

	#if IS_MONOLITHIC

		#define IMPLEMENT_APPLICATION( ModuleName, GameName ) \
			/* For monolithic builds, we must statically define the game's name string (See Core.h) */ \
			TCHAR GGameName[64] = TEXT( GameName ); \
			IMPLEMENT_GAME_MODULE( FDefaultGameModuleImpl, ModuleName ) \
			PER_MODULE_BOILERPLATE \
			FEngineLoop GEngineLoop;

	#else		

		#define IMPLEMENT_APPLICATION( ModuleName, GameName ) \
			/* For non-monolithic programs, we must set the game's name string before main starts (See Core.h) */ \
			struct FAutoSet##ModuleName \
			{ \
				FAutoSet##ModuleName() \
				{ \
					FCString::Strncpy(GGameName, TEXT( GameName ), ARRAY_COUNT(GGameName)); \
				} \
			} AutoSet##ModuleName; \
			PER_MODULE_BOILERPLATE \
			PER_MODULE_BOILERPLATE_ANYLINK(FDefaultGameModuleImpl, ModuleName) \
			FEngineLoop GEngineLoop;
	#endif

#else

/** IMPLEMENT_PRIMARY_GAME_MODULE must be used for at least one game module in your game.  It sets the "name"
    your game when compiling in monolithic mode. */
#if IS_MONOLITHIC
	#if PLATFORM_DESKTOP

		#define IMPLEMENT_PRIMARY_GAME_MODULE( ModuleImplClass, ModuleName, GameName ) \
			/* For monolithic builds, we must statically define the game's name string (See Core.h) */ \
			TCHAR GGameName[64] = TEXT( GameName ); \
			/* Implement the GIsGameAgnosticExe variable (See Core.h). */ \
			bool GIsGameAgnosticExe = false; \
			IMPLEMENT_GAME_MODULE( ModuleImplClass, ModuleName ) \
			PER_MODULE_BOILERPLATE \
			void UELinkerFixupCheat() \
			{ \
				extern void UELinkerFixups(); \
				UELinkerFixups(); \
			}

	#else	//PLATFORM_DESKTOP

		#define IMPLEMENT_PRIMARY_GAME_MODULE( ModuleImplClass, ModuleName, GameName ) \
			/* For monolithic builds, we must statically define the game's name string (See Core.h) */ \
			TCHAR GGameName[64] = TEXT( GameName ); \
			PER_MODULE_BOILERPLATE \
			IMPLEMENT_GAME_MODULE( ModuleImplClass, ModuleName ) \
			/* Implement the GIsGameAgnosticExe variable (See Core.h). */ \
			bool GIsGameAgnosticExe = false; \

	#endif	//PLATFORM_DESKTOP

#else	//IS_MONOLITHIC

	#define IMPLEMENT_PRIMARY_GAME_MODULE( ModuleImplClass, ModuleName, GameName ) \
		/* Nothing special to do for modular builds.  The game name will be set via the command-line */ \
		IMPLEMENT_GAME_MODULE( ModuleImplClass, ModuleName )
#endif	//IS_MONOLITHIC

#endif
