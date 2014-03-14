// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogModuleManager, Log, All);

#if !IS_MONOLITHIC
	/** If true, we are reloading a class for HotReload */
	CORE_API bool			GIsHotReload							= false;
#endif


int32 FModuleManager::FModuleInfo::CurrentLoadOrder = 1;

namespace ModuleManagerDefs
{
	const static FString CompilationInfoConfigSection("ModuleFileTracking");

	// These strings should match the values of the enum EModuleCompileMethod in ModuleManager.h
	// and should be handled in ReadModuleCompilationInfoFromConfig() & WriteModuleCompilationInfoToConfig() below
	const static FString CompileMethodRuntime("Runtime");
	const static FString CompileMethodExternal("External");
	const static FString CompileMethodUnknown("Unknown");

	// Add one minute epsilon to timestamp comparision
	const static FTimespan TimeStampEpsilon(0, 1, 0);
}

FModuleManager& FModuleManager::Get()
{
	// FModuleManager is not thread-safe
	ensure(IsInGameThread());

	// NOTE: The reason we initialize to NULL here is due to an issue with static initialization of variables with
	// constructors/destructors across DLL boundaries, where a function called from a statically constructed object
	// calls a function in another module (such as this function) that creates a static variable.  A crash can occur
	// because the static initialization of this DLL has not yet happened, and the CRT's list of static destructors
	// cannot be written to because it has not yet been initialized fully.	(@todo UE4 DLL)
	static FModuleManager* ModuleManager = NULL;

	if( ModuleManager == NULL )
	{
		ModuleManager = new FModuleManager();
	}

	return *ModuleManager;
}


FModuleManager::~FModuleManager()
{
	// NOTE: It may not be safe to unload modules by this point (static deinitialization), as other
	//       DLLs may have already been unloaded, which means we can't safely call clean up methods
}


void FModuleManager::Tick()
{
	// We never want to block on a pending compile when checking compilation status during Tick().  We're
	// just checking so that we can fire callbacks if and when compilation has finished.
	const bool bWaitForCompletion = false;

	// Ignored output variables
	bool bCompileStillInProgress = false;
	bool bCompileSucceeded = false;
	FOutputDeviceNull NullOutput;
	CheckForFinishedModuleDLLCompile( bWaitForCompletion, bCompileStillInProgress, bCompileSucceeded, NullOutput );
}

void FModuleManager::FindModules(const TCHAR* WildcardWithoutExtension, TArray<FName>& OutModules)
{
	// @todo plugins: Try to convert existing use cases to use plugins, and get rid of this function
#if !IS_MONOLITHIC

	TMap<FName, FString> ModulePaths;
	FindModulePaths(WildcardWithoutExtension, ModulePaths);

	for(TMap<FName, FString>::TConstIterator Iter(ModulePaths); Iter; ++Iter)
	{
		if(CheckModuleCompatibility(*Iter.Value()))
		{
			OutModules.Add(Iter.Key());
		}
	}

#else
	FString Wildcard(WildcardWithoutExtension);
	for (FStaticallyLinkedModuleInitializerMap::TConstIterator It(StaticallyLinkedModuleInitializers); It; ++It)
	{
		if (It.Key().ToString().MatchesWildcard(Wildcard))
		{
			OutModules.Add(It.Key());
		}
	}
#endif
}

/**
 * Checks to see if the specified module is currently loaded.  This is an O(1) operation.
 *
 * @param	InModuleName		The base name of the module file.  Should not include path, extension or platform/configuration info.  This is just the "module name" part of the module file name.  Names should be globally unique.
 *
 * @return	True if module is currently loaded, otherwise false
 */
bool FModuleManager::IsModuleLoaded( const FName InModuleName ) const
{
	// Do we even know about this module?
	const TSharedRef< FModuleInfo >* ModuleInfoPtr = Modules.Find( InModuleName );
	if( ModuleInfoPtr != NULL )
	{
		const TSharedRef< FModuleInfo >& ModuleInfo( *ModuleInfoPtr );

		// Only if already loaded
		if( ModuleInfo->Module.IsValid()  )
		{
			// Module is loaded and ready
			return true;
		}
	}

	// Not loaded, or not fully initialized yet (StartupModule wasn't called)
	return false;
}

void FModuleManager::AddModule( const FName InModuleName )
{
	// Do we already know about this module?  If not, we'll create information for this module now.
	if( ensureMsg( InModuleName != NAME_None, TEXT( "FModuleManager::AddModule() was called with an invalid module name (empty string or 'None'.)  This is not allowed." ) ) &&
		!Modules.Contains( InModuleName ) )
	{
		// Add this module to the set of modules that we know about
		TSharedRef< FModuleInfo > ModuleInfo( new FModuleInfo() );

#if !IS_MONOLITHIC
		FString ModuleNameString = InModuleName.ToString();

		TMap<FName, FString> ModulePathMap;
		FindModulePaths(*ModuleNameString, ModulePathMap);

		if(ModulePathMap.Num() == 1)
		{
			// Add this module to the set of modules that we know about
			ModuleInfo->OriginalFilename = TMap<FName, FString>::TConstIterator(ModulePathMap).Value();
			ModuleInfo->Filename = ModuleInfo->OriginalFilename;

			// When iterating on code during development, it's possible there are multiple rolling versions of this
			// module's DLL file.  This can happen if the programmer is recompiling DLLs while the game is loaded.  In
			// this case, we want to load the newest iteration of the DLL file, so that behavior is the same after
			// restarting the application.
			{
				// NOTE: We leave this enabled in UE_BUILD_SHIPPING editor builds so module authors can iterate on custom modules
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) || (UE_BUILD_SHIPPING && WITH_EDITOR)
				// In some cases, sadly, modules may be loaded before appInit() is called.  We can't cleanly support rolling files for those modules.
				{
					// First, check to see if the module we added already exists on disk
					const FDateTime OriginalModuleFileTime = IFileManager::Get().GetTimeStamp( *ModuleInfo->OriginalFilename );
					if( OriginalModuleFileTime != FDateTime::MinValue() )
					{
						const FString ModuleName = *InModuleName.ToString();
						const int32 MatchPos = ModuleInfo->OriginalFilename.Find( ModuleName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
						if( ensureMsgf( MatchPos != INDEX_NONE, TEXT("Could not find module name '%s' in module filename '%s'"), *ModuleName, *ModuleInfo->OriginalFilename ) )
						{
							const int32 SuffixPos = MatchPos + ModuleName.Len();

							const FString Prefix = ModuleInfo->OriginalFilename.Left( SuffixPos );
							const FString Suffix = ModuleInfo->OriginalFilename.Right( ModuleInfo->OriginalFilename.Len() - SuffixPos );

							const FString ModuleFileSearchString = FString::Printf( TEXT( "%s-*%s" ), *Prefix, *Suffix);
							const FString ModuleFileSearchDirectory = FPaths::GetPath(ModuleFileSearchString);

							// Search for module files
							TArray<FString> FoundFiles;
							IFileManager::Get().FindFiles( FoundFiles, *ModuleFileSearchString, true, false );

							// Figure out what the newest module file is
							int32 NewestFoundFileIndex = INDEX_NONE;
							FDateTime NewestFoundFileTime = OriginalModuleFileTime;
							if( FoundFiles.Num() > 0 )
							{
								for( int32 CurFileIndex = 0; CurFileIndex < FoundFiles.Num(); ++CurFileIndex )
								{
									// FoundFiles contains file names with no directory information, but we need the full path up
									// to the file, so we'll prefix it back on if we have a path.
									const FString& FoundFile = FoundFiles[ CurFileIndex ];
									const FString FoundFilePath = ModuleFileSearchDirectory.IsEmpty() ? FoundFile : ( ModuleFileSearchDirectory / FoundFile );

									// need to reject some files here that are not numbered...release executables, do have a suffix, so we need to make sure we don't find the debug version
									check(FoundFilePath.Len() > Prefix.Len() + Suffix.Len());
									FString Center = FoundFilePath.Mid(Prefix.Len(), FoundFilePath.Len() - Prefix.Len() - Suffix.Len());
									check(Center.StartsWith(TEXT("-"))); // a minus sign is still considered numeric, so we can leave it.
									if (!Center.IsNumeric())
									{
										// this is a debug DLL or something, it is not a numbered hot DLL
										continue;
									}


									// Check the time stamp for this file
									const FDateTime FoundFileTime = IFileManager::Get().GetTimeStamp( *FoundFilePath );
									if( ensure( FoundFileTime != -1.0 ) )
									{
										// Was this file modified more recently than our others?
										if( FoundFileTime > NewestFoundFileTime )
										{
											NewestFoundFileIndex = CurFileIndex;
											NewestFoundFileTime = FoundFileTime;
										}
									}
									else
									{
										// File wasn't found, should never happen as we searched for these files just now
									}
								}


								// Did we find a variant of the module file that is newer than our original file?
								if( NewestFoundFileIndex != INDEX_NONE )
								{
									// Update the module working file name to the most recently-modified copy of that module
									const FString& NewestModuleFilename = FoundFiles[ NewestFoundFileIndex ];
									const FString NewestModuleFilePath = ModuleFileSearchDirectory.IsEmpty() ? NewestModuleFilename : ( ModuleFileSearchDirectory / NewestModuleFilename );
									ModuleInfo->Filename = NewestModuleFilePath;
								}
								else
								{
									// No variants were found that were newer than the original module file name, so
									// we'll continue to use that!
								}
							}
						}
					}
#endif	// !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				}
			}
		}
#endif	// !IS_MONOLITHIC

		// Update hash table
		Modules.Add( InModuleName, ModuleInfo );

		// List of known modules has changed.  Fire callbacks.
		ModulesChangedEvent.Broadcast( InModuleName, EModuleChangeReason::PluginDirectoryChanged );
	}
}

/**
 * Adds a module to our list of modules and tries to load it immediately
 *
 * @param	InModuleName	The base name of the module file.  Should not include path, extension or platform/configuration info.  This is just the "name" part of the module file name.  Names should be globally unique.
 */
TSharedPtr<IModuleInterface> FModuleManager::LoadModule( const FName InModuleName, bool bWasReloaded /*=false*/ )
{
	ELoadModuleFailureReason::Type FailureReason;
	return LoadModuleWithFailureReason(InModuleName, FailureReason, bWasReloaded );
}

TSharedPtr<IModuleInterface> FModuleManager::LoadModuleWithFailureReason( const FName InModuleName, ELoadModuleFailureReason::Type& OutFailureReason, bool bWasReloaded /*=false*/ )
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Module Load"), STAT_ModuleLoad, STATGROUP_LoadTime);

	TSharedPtr<IModuleInterface> LoadedModule;
	OutFailureReason = ELoadModuleFailureReason::Success;

	// Update our set of known modules, in case we don't already know about this module
	AddModule( InModuleName );

	// Grab the module info.  This has the file name of the module, as well as other info.
	TSharedRef< FModuleInfo > ModuleInfo = Modules.FindChecked( InModuleName );

	if( ModuleInfo->Module.IsValid() )
	{
		// Assign the already loaded module into the return value, otherwise the return value gives the impression the module failed load!
		LoadedModule = ModuleInfo->Module;
	}
	else
	{
		// Make sure this isn't a module that we had previously loaded, and then unloaded at shutdown time.
		//
		// If this assert goes off, your trying to load a module during the shutdown phase that was already
		// cleaned up.  The easiest way to fix this is to change your code to query for an already-loaded
		// module instead of trying to load it directly.
		checkf( ( !ModuleInfo->bWasUnloadedAtShutdown ), TEXT( "Attempted to load module '%s' that was already unloaded at shutdown.  FModuleManager::LoadModule() was called to load a module that was previously loaded, and was unloaded at shutdown time.  If this assert goes off, your trying to load a module during the shutdown phase that was already cleaned up.  The easiest way to fix this is to change your code to query for an already-loaded module instead of trying to load it directly." ), *InModuleName.ToString() );

		// Check if we're statically linked with the module.  Those modules register with the module manager using a static variable,
		// so hopefully we already know about the name of the module and how to initialize it.
		const FInitializeStaticallyLinkedModule* ModuleInitializerPtr = StaticallyLinkedModuleInitializers.Find( InModuleName );
		if( ModuleInitializerPtr != NULL )
		{
			const FInitializeStaticallyLinkedModule& ModuleInitializer( *ModuleInitializerPtr );

			// Initialize the module!
			ModuleInfo->Module = MakeShareable( ModuleInitializer.Execute() );

			if( ModuleInfo->Module.IsValid() )
			{
				// Startup the module
				ModuleInfo->Module->StartupModule();

				// Module was started successfully!  Fire callbacks.
				ModulesChangedEvent.Broadcast( InModuleName, EModuleChangeReason::ModuleLoaded );

				// Set the return parameter
				LoadedModule = ModuleInfo->Module;
			}
			else
			{
				UE_LOG(LogModuleManager, Warning, TEXT( "ModuleManager: Unable to load module '%s' because InitializeModule function failed (returned NULL pointer.)" ), *InModuleName.ToString() );
				OutFailureReason = ELoadModuleFailureReason::FailedToInitialize;
			}
		}
#if IS_MONOLITHIC
		else
		{
			// Monolithic builds that do not have the initializer were *not found* during the build step, so return FileNotFound
			// (FileNotFound is an acceptable error in some case - ie loading a content only project)
			UE_LOG(LogModuleManager, Warning, TEXT( "ModuleManager: Module '%s' not found - it's StaticallyLinkedModuleInitializers function is null." ), *InModuleName.ToString() );
			OutFailureReason = ELoadModuleFailureReason::FileNotFound;
		}
#endif
#if !IS_MONOLITHIC
		else
		{
			// Make sure that any UObjects that need to be registered were already processed before we go and
			// load another module.  We just do this so that we can easily tell whether UObjects are present
			// in the module being loaded.
			if( bCanProcessNewlyLoadedObjects )
			{
				ProcessLoadedObjectsCallback.Broadcast();
			}

			// Try to dynamically load the DLL

			UE_LOG(LogModuleManager, Verbose, TEXT( "ModuleManager: Load Module '%s' DLL '%s'" ), *InModuleName.ToString(), *ModuleInfo->Filename);

			// Determine which file to load for this module.
			const FString ModuleFileToLoad = ModuleInfo->Filename;

			// Clear the handle and set it again below if the module is successfully loaded
			ModuleInfo->Handle = NULL;

			// Skip this check if file manager has not yet been initialized
			if ( FPaths::FileExists(ModuleFileToLoad) && CheckModuleCompatibility(*ModuleFileToLoad))
			{
				ModuleInfo->Handle = FPlatformProcess::GetDllHandle(*ModuleFileToLoad);
				if( ModuleInfo->Handle != NULL )
				{
					// First things first.  If the loaded DLL has UObjects in it, then their generated code's
					// static initialization will have run during the DLL loading phase, and we'll need to
					// go in and make sure those new UObject classes are properly registered.
					{
						// Sometimes modules are loaded before even the UObject systems are ready.  We need to assume
						// these modules aren't using UObjects.
						if( bCanProcessNewlyLoadedObjects)
						{
							// OK, we've verified that loading the module caused new UObject classes to be
							// registered, so we'll treat this module as a module with UObjects in it.
							ProcessLoadedObjectsCallback.Broadcast();
						}
					}

					// Find our "InitializeModule" global function, which must exist for all module DLLs
					FInitializeModuleFunctionPtr InitializeModuleFunctionPtr =
						( FInitializeModuleFunctionPtr )FPlatformProcess::GetDllExport( ModuleInfo->Handle, TEXT( "InitializeModule" ) );
					if( InitializeModuleFunctionPtr != NULL )
					{
						// Initialize the module!
						ModuleInfo->Module = MakeShareable( InitializeModuleFunctionPtr() );

						if( ModuleInfo->Module.IsValid() )
						{
							// Startup the module
							ModuleInfo->Module->StartupModule();

							// Module was started successfully!  Fire callbacks.
							ModulesChangedEvent.Broadcast( InModuleName, EModuleChangeReason::ModuleLoaded );

							// Set the return parameter
							LoadedModule = ModuleInfo->Module;
						}
						else
						{
							UE_LOG(LogModuleManager, Warning, TEXT( "ModuleManager: Unable to load module '%s' because InitializeModule function failed (returned NULL pointer.)" ), *ModuleFileToLoad );

							FPlatformProcess::FreeDllHandle( ModuleInfo->Handle );
							ModuleInfo->Handle = NULL;
							OutFailureReason = ELoadModuleFailureReason::FailedToInitialize;
						}
					}
					else
					{
						UE_LOG(LogModuleManager, Warning, TEXT( "ModuleManager: Unable to load module '%s' because InitializeModule function was not found." ), *ModuleFileToLoad );

						FPlatformProcess::FreeDllHandle( ModuleInfo->Handle );
						ModuleInfo->Handle = NULL;
						OutFailureReason = ELoadModuleFailureReason::FailedToInitialize;
					}
				}
				else
				{
					UE_LOG(LogModuleManager, Warning, TEXT( "ModuleManager: Unable to load module '%s' because the file couldn't be loaded by the OS." ), *ModuleFileToLoad );
					OutFailureReason = ELoadModuleFailureReason::CouldNotBeLoadedByOS;
				}
			}
			else
			{
				UE_LOG(LogModuleManager, Warning, TEXT( "ModuleManager: Unable to load module '%s' because the file was not found." ), *ModuleFileToLoad );
				OutFailureReason = ELoadModuleFailureReason::FileNotFound;
			}
		}
#endif
	}

	return LoadedModule;
}


bool FModuleManager::UnloadModule( const FName InModuleName, bool bIsShutdown )
{
	// Do we even know about this module?
	TSharedRef< FModuleInfo >* ModuleInfoPtr = Modules.Find( InModuleName );
	if( ModuleInfoPtr != NULL )
	{
		TSharedRef< FModuleInfo >& ModuleInfo( *ModuleInfoPtr );

		// Only if already loaded
		if( ModuleInfo->Module.IsValid() )
		{
			// Shutdown the module
			ModuleInfo->Module->ShutdownModule();

			// Verify that we have the only outstanding reference to this module.  No one should still be 
			// referencing a module that is about to be destroyed!
			check( ModuleInfo->Module.IsUnique() );

			// Release reference to module interface.  This will actually destroy the module object.
			ModuleInfo->Module.Reset();

#if !IS_MONOLITHIC
			if( ModuleInfo->Handle != NULL )
			{
				// If we're shutting down then don't bother actually unloading the DLL.  We'll simply abandon it in memory
				// instead.  This makes it much less likely that code will be unloaded that could still be called by
				// another module, such as a destructor or other virtual function.  The module will still be unloaded by
				// the operating system when the process exits.
				if( !bIsShutdown )
				{
					// Unload the DLL
					FPlatformProcess::FreeDllHandle( ModuleInfo->Handle );
				}
				ModuleInfo->Handle = NULL;
			}
#endif

			// If we're shutting down, then we never want this module to be "resurrected" in this session.
			// It's gone for good!  So we'll mark it as such so that we can catch cases where a routine is
			// trying to load a module that we've unloaded/abandoned at shutdown.
			if( bIsShutdown )
			{
				ModuleInfo->bWasUnloadedAtShutdown = true;
			}

			// Don't bother firing off events while we're in the middle of shutting down.  These events
			// are designed for subsystems that respond to plugins dynamically being loaded and unloaded,
			// such as the ModuleUI -- but they shouldn't be doing work to refresh at shutdown.
			else
			{
				// A module was successfully unloaded.  Fire callbacks.
				ModulesChangedEvent.Broadcast( InModuleName, EModuleChangeReason::ModuleUnloaded );
			}

			return true;
		}
	}

	return false;
}


/**
 * Abandons a loaded module, leaving it loaded in memory but no longer tracking it in the module manager
 *
 * @param	InModuleName	The name of the module to abandon.  Should not include path, extension or platform/configuration info.  This is just the "module name" part of the module file name.
 */
void FModuleManager::AbandonModule( const FName InModuleName )
{
	// Do we even know about this module?
	TSharedRef< FModuleInfo >* ModuleInfoPtr = Modules.Find( InModuleName );
	if( ModuleInfoPtr != NULL )
	{
		TSharedRef< FModuleInfo >& ModuleInfo( *ModuleInfoPtr );

		// Only if already loaded
		if( ModuleInfo->Module.IsValid() )
		{
			// Release reference to module interface.  This will actually destroy the module object.
			// @todo UE4 DLL: Could be dangerous in some cases to reset the module interface while abandoning.  Currently not
			// a problem because script modules don't implement any functionality here.  Possible, we should keep these references
			// alive off to the side somewhere (intentionally leak)
			ModuleInfo->Module.Reset();

			// A module was successfully unloaded.  Fire callbacks.
			ModulesChangedEvent.Broadcast( InModuleName, EModuleChangeReason::ModuleUnloaded );
		}
	}
}


void FModuleManager::UnloadModulesAtShutdown()
{
	struct FModulePair
	{
		FName ModuleName;
		int32 LoadOrder;
		FModulePair(FName InModuleName, int32 InLoadOrder)
			: ModuleName(InModuleName)
			, LoadOrder(InLoadOrder)
		{
			check(LoadOrder > 0); // else was never initialized
		}
		bool operator<(const FModulePair& Other) const
		{
			return LoadOrder > Other.LoadOrder; //intentionally backwards, we want the last loaded module first
		}
	};
	TArray<FModulePair> ModulesToUnload;
	for( FModuleMap::TConstIterator ModuleIt( Modules ); ModuleIt; ++ModuleIt )
	{
		TSharedRef< FModuleInfo > ModuleInfo( ModuleIt.Value() );

		// Take this opportunity to update and write out the compile data before the editor shuts down
		UpdateModuleCompileData(ModuleIt.Key(), ModuleInfo);

		// Only if already loaded
		if( ModuleInfo->Module.IsValid() )
		{
			// Only if the module supports shutting down in this phase
			if( ModuleInfo->Module->SupportsAutomaticShutdown() )
			{
				new (ModulesToUnload) FModulePair(ModuleIt.Key(), ModuleIt.Value()->LoadOrder);
			}
		}
	}

	ModulesToUnload.Sort();

	for (int32 Index = 0; Index < ModulesToUnload.Num(); Index++)
	{
		UE_LOG(LogModuleManager, Log, TEXT( "Shutting down and abandoning module %s (%d)" ), *ModulesToUnload[Index].ModuleName.ToString(), ModulesToUnload[Index].LoadOrder );
		const bool bIsShutdown = true;
		UnloadModule( ModulesToUnload[Index].ModuleName, bIsShutdown );
	}
}


/**
 * Given a module name, returns a shared reference to that module's interface.  If the module is unknown or not loaded, this will assert!
 *
 * @param	InModuleName	Name of the module to return
 *
 * @return	A shared reference to the module
 */
TSharedRef< IModuleInterface > FModuleManager::GetModuleInterfaceRef( const FName InModuleName )
{
	// Do we even know about this module?
	TSharedRef< FModuleInfo > ModuleInfo = Modules.FindChecked( InModuleName );

	// Module must be loaded!
	checkf( ModuleInfo->Module.IsValid(), TEXT("Tried to use invalid module %s!"), *ModuleInfo->Filename );
	return ModuleInfo->Module.ToSharedRef();
}



/**
 * Given a module name, returns a reference to that module's interface.  If the module is unknown or not loaded, this will assert!
 *
 * @param	InModuleName	Name of the module to return
 *
 * @return	A reference to the module
 */
IModuleInterface& FModuleManager::GetModuleInterface( const FName InModuleName )
{
	// Do we even know about this module?
	TSharedRef< FModuleInfo > ModuleInfo = Modules.FindChecked( InModuleName );

	// Module must be loaded!
	checkf( ModuleInfo->Module.IsValid(), TEXT("Tried to use invalid module %s!"), *ModuleInfo->Filename );
	return *ModuleInfo->Module;
}



/**
 * Exec command
 *
 * @param	InWorld		World context
 * @param	Cmd		Command to execute
 * @param	Ar		Output device
 *
 * @return	True if command was handled
 */
bool FModuleManager::Exec( UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar )
{
#if !UE_BUILD_SHIPPING
	if ( FParse::Command( &Cmd, TEXT( "Module" ) ) )
	{
		// List
		if( FParse::Command( &Cmd, TEXT( "List" ) ) )
		{
			if( Modules.Num() > 0 )
			{
				Ar.Logf( TEXT( "Listing all %i known modules:\n" ), Modules.Num() );

				TArray< FString > StringsToDisplay;
				for( FModuleMap::TConstIterator ModuleIt( Modules ); ModuleIt; ++ModuleIt )
				{
					StringsToDisplay.Add(
						FString::Printf( TEXT( "    %s [File: %s] [Loaded: %s]" ),
							*ModuleIt.Key().ToString(),
							*ModuleIt.Value()->Filename,
							ModuleIt.Value()->Module.IsValid() != false? TEXT( "Yes" ) : TEXT( "No" ) ) );
				}

				// Sort the strings
				StringsToDisplay.Sort();

				// Display content
				for( TArray< FString >::TConstIterator StringIt( StringsToDisplay ); StringIt; ++StringIt )
				{
					Ar.Log( *StringIt );
				}
			}
			else
			{
				Ar.Logf( TEXT( "No modules are currently known." ), Modules.Num() );
			}

			return true;
		}

#if !IS_MONOLITHIC
		// Load <ModuleName>
		else if( FParse::Command( &Cmd, TEXT( "Load" ) ) )
		{
			const FString ModuleNameStr = FParse::Token( Cmd, 0 );
			if( !ModuleNameStr.IsEmpty() )
			{
				const FName ModuleName( *ModuleNameStr );
				if( !IsModuleLoaded( ModuleName ) )
				{
					Ar.Logf( TEXT( "Loading module" ) );
					LoadModuleWithCallback( ModuleName, Ar );
				}
				else
				{
					Ar.Logf( TEXT( "Module is already loaded." ) );
				}
			}
			else
			{
				Ar.Logf( TEXT( "Please specify a module name to load." ) );
			}

			return true;
		}


		// Unload <ModuleName>
		else if( FParse::Command( &Cmd, TEXT( "Unload" ) ) )
		{
			const FString ModuleNameStr = FParse::Token( Cmd, 0 );
			if( !ModuleNameStr.IsEmpty() )
			{
				const FName ModuleName( *ModuleNameStr );

				if( IsModuleLoaded( ModuleName ) )
				{
					Ar.Logf( TEXT( "Unloading module." ) );
					UnloadOrAbandonModuleWithCallback( ModuleName, Ar );
				}
				else
				{
					Ar.Logf( TEXT( "Module is not currently loaded." ) );
				}
			}
			else
			{
				Ar.Logf( TEXT( "Please specify a module name to unload." ) );
			}

			return true;
		}


		// Reload <ModuleName>
		else if( FParse::Command( &Cmd, TEXT( "Reload" ) ) )
		{
			const FString ModuleNameStr = FParse::Token( Cmd, 0 );
			if( !ModuleNameStr.IsEmpty() )
			{
				const FName ModuleName( *ModuleNameStr );

				if( IsModuleLoaded( ModuleName ) )
				{
					Ar.Logf( TEXT( "Reloading module.  (Module is currently loaded.)" ) );
					UnloadOrAbandonModuleWithCallback( ModuleName, Ar );
				}
				else
				{
					Ar.Logf( TEXT( "Reloading module.  (Module was not loaded.)" ) );
				}

				if( !IsModuleLoaded( ModuleName ) )
				{
					Ar.Logf( TEXT( "Reloading module" ) );
					LoadModuleWithCallback( ModuleName, Ar );
				}
			}

			return true;
		}


		// Recompile <ModuleName>
		else if( FParse::Command( &Cmd, TEXT( "Recompile" ) ) )
		{
			const FString ModuleNameStr = FParse::Token( Cmd, 0 );
			if( !ModuleNameStr.IsEmpty() )
			{
				const FName ModuleName( *ModuleNameStr );
				const bool bReloadAfterRecompile = true;
				RecompileModule( ModuleName, bReloadAfterRecompile, Ar);
			}

			return true;
		}
#endif // !IS_MONOLITHIC
	}
#endif // !UE_BUILD_SHIPPING

	return false;
}



/**
 * Queries information about a specific module name
 *
 * @param	InModuleName	Module to query status for
 * @param	OutModuleStatus	Status of the specified module
 *
 * @return	True if the module was found the OutModuleStatus is valid
 */
bool FModuleManager::QueryModule( const FName InModuleName, FModuleStatus& OutModuleStatus )
{
	// Do we even know about this module?
	const TSharedRef< FModuleInfo >* ModuleInfoPtr = Modules.Find( InModuleName );
	if( ModuleInfoPtr != NULL )
	{
		const TSharedRef< FModuleInfo >& ModuleInfo( *ModuleInfoPtr );

		OutModuleStatus.Name = InModuleName.ToString();
		OutModuleStatus.FilePath = FPaths::ConvertRelativePathToFull(ModuleInfo->Filename);
		OutModuleStatus.bIsLoaded = ( ModuleInfo->Module.IsValid() != false );

		if( OutModuleStatus.bIsLoaded )
		{
			OutModuleStatus.bIsGameModule = ModuleInfo->Module->IsGameModule();
		}

		if (!ModuleInfo->CompileData.bIsValid)
		{
			UpdateModuleCompileData(InModuleName, ModuleInfo);
		}

		FString CompileMethodString = ModuleManagerDefs::CompileMethodUnknown;
		if (ModuleInfo->CompileData.CompileMethod == EModuleCompileMethod::Runtime)
		{
			CompileMethodString = ModuleManagerDefs::CompileMethodRuntime;
		}
		else if (ModuleInfo->CompileData.CompileMethod == EModuleCompileMethod::External)
		{
			CompileMethodString = ModuleManagerDefs::CompileMethodExternal;
		}

		OutModuleStatus.CompilationMethod = CompileMethodString;

		return true;
	}

	// Not known to us
	return false;
}

	
/**
 * Queries information about all of the currently known modules
 *
 * @param	OutModuleStatuses	Status of all modules
 */
void FModuleManager::QueryModules( TArray< FModuleStatus >& OutModuleStatuses )
{
	OutModuleStatuses.Reset();

	for( FModuleMap::TConstIterator ModuleIt( Modules ); ModuleIt; ++ModuleIt )
	{
		const FName CurModuleFName = ModuleIt.Key();
		const TSharedRef< FModuleInfo > CurModule = ModuleIt.Value();

		FModuleStatus ModuleStatus;
		{
			ModuleStatus.Name = CurModuleFName.ToString();
			ModuleStatus.FilePath = FPaths::ConvertRelativePathToFull(CurModule->Filename);
			ModuleStatus.bIsLoaded = CurModule->Module.IsValid();

			if( ModuleStatus.bIsLoaded  )
			{
				ModuleStatus.bIsGameModule = CurModule->Module->IsGameModule();
			}

			if (!CurModule->CompileData.bIsValid)
			{
				UpdateModuleCompileData(CurModuleFName, CurModule);
			}

			FString CompileMethodString = ModuleManagerDefs::CompileMethodUnknown;
			if (CurModule->CompileData.CompileMethod == EModuleCompileMethod::Runtime)
			{
				CompileMethodString = ModuleManagerDefs::CompileMethodRuntime;
			}
			else if (CurModule->CompileData.CompileMethod == EModuleCompileMethod::External)
			{
				CompileMethodString = ModuleManagerDefs::CompileMethodExternal;
			}

			ModuleStatus.CompilationMethod = CompileMethodString;
		}
		OutModuleStatuses.Add( ModuleStatus );
	}
}

bool FModuleManager::IsSolutionFilePresent()
{
	return !GetSolutionFilepath().IsEmpty();
}

FString FModuleManager::GetSolutionFilepath()
{
#if PLATFORM_MAC
	const TCHAR* Postfix = TEXT(".xcodeproj/project.pbxproj");
#else
	const TCHAR* Postfix = TEXT(".sln");
#endif

	FString SolutionFilepath;

	if ( FPaths::IsProjectFilePathSet() )
	{
		// When using game specific uproject files, the solution is named after the game and in the uproject folder
		SolutionFilepath = FPaths::GameDir() / FPaths::GetBaseFilename(FPaths::GetProjectFilePath()) + Postfix;
		if ( !FPaths::FileExists(SolutionFilepath) )
		{
			SolutionFilepath = TEXT("");
		}
	}

	if ( SolutionFilepath.IsEmpty() )
	{
		// Otherwise, it is simply titled UE4.sln
		SolutionFilepath = FPaths::RootDir() / FString(TEXT("UE4")) + Postfix ;
		if ( !FPaths::FileExists(SolutionFilepath) )
		{
			SolutionFilepath = TEXT("");
		}
	}

	return SolutionFilepath;
}

bool FModuleManager::RecompileModule( const FName InModuleName, const bool bReloadAfterRecompile, FOutputDevice &Ar )
{
#if !IS_MONOLITHIC
	const bool bShowProgressDialog = true;
	const bool bShowCancelButton = false;

	FFormatNamedArguments Args;
	Args.Add( TEXT("CodeModuleName"), FText::FromName( InModuleName ) );
	const FText StatusUpdate = FText::Format( NSLOCTEXT("ModuleManager", "Recompile_SlowTaskName", "Compiling {CodeModuleName}..."), Args );

	GWarn->BeginSlowTask( StatusUpdate, bShowProgressDialog, bShowCancelButton );

	ModuleCompilerStartedEvent.Broadcast();

	// Update our set of known modules, in case we don't already know about this module
	AddModule( InModuleName );

	// Only use rolling module names if the module was already loaded into memory.  This allows us to try compiling
	// the module without actually having to unload it first.
	const bool bWasModuleLoaded = IsModuleLoaded( InModuleName );
	const bool bUseRollingModuleNames = bWasModuleLoaded;
	TSharedRef< FModuleInfo > Module = Modules.FindChecked( InModuleName );

	bool bWasSuccessful = true;
	FString NewModuleFileNameOnSuccess = Module->Filename;
	if( bUseRollingModuleNames )
	{
		// First, try to compile the module.  If the module is already loaded, we won't unload it quite yet.  Instead
		// make sure that it compiles successfully.

		// Find a unique file name for the module
		FString UniqueSuffix;
		FString UniqueModuleFileName;
		MakeUniqueModuleFilename( InModuleName, UniqueSuffix, UniqueModuleFileName );

		// If the recompile succeeds, we'll update our cached file name to use the new unique file name
		// that we setup for the module
		NewModuleFileNameOnSuccess = UniqueModuleFileName;

		TArray< FModuleToRecompile > ModulesToRecompile;
		FModuleToRecompile ModuleToRecompile;
		ModuleToRecompile.ModuleName = InModuleName.ToString();
		ModuleToRecompile.ModuleFileSuffix = UniqueSuffix;
		ModuleToRecompile.NewModuleFilename = UniqueModuleFileName;
		ModulesToRecompile.Add( ModuleToRecompile );
		bWasSuccessful = RecompileModuleDLLs( ModulesToRecompile, Ar );
	}

	if( bWasSuccessful )
	{
		// Shutdown the module if it's already running
		if( bWasModuleLoaded )
		{
			Ar.Logf( TEXT( "Unloading module before compile." ) );
			UnloadOrAbandonModuleWithCallback( InModuleName, Ar );
		}

		if( !bUseRollingModuleNames )
		{
			// Try to recompile the DLL
			TArray< FModuleToRecompile > ModulesToRecompile;
			FModuleToRecompile ModuleToRecompile;
			ModuleToRecompile.ModuleName = InModuleName.ToString();
			ModulesToRecompile.Add( ModuleToRecompile );
			bWasSuccessful = RecompileModuleDLLs( ModulesToRecompile, Ar );
		}

		// Reload the module if it was loaded before we recompiled
		if( bWasSuccessful && bWasModuleLoaded && bReloadAfterRecompile )
		{
			Ar.Logf( TEXT( "Reloading module after successful compile." ) );
			bWasSuccessful = LoadModuleWithCallback( InModuleName, Ar );
		}
	}

	GWarn->EndSlowTask();
	return bWasSuccessful;
#else
	return false;
#endif // !IS_MONOLITHIC
}



bool FModuleManager::IsCurrentlyCompiling() const
{
	return ModuleCompileProcessHandle.IsValid();
}


FString FModuleManager::MakeUBTArgumentsForModuleCompiling()
{
	FString AdditionalArguments;
	if ( FPaths::IsProjectFilePathSet() )
	{
		// We have to pass FULL paths to UBT
		FString FullProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());

		// @todo projectdirs: Currently non-Rocket projects that exist under the UE4 root are compiled by UBT with no .uproject file
		//     name passed in (see bIsProjectTarget in VCProject.cs), which causes intermediate libraries to be saved to the Engine
		//     intermediate folder instead of the project's intermediate folder.  We're emulating this behavior here for module
		//     recompiling, so that compiled modules will be able to find their import libraries in the original folder they were compiled.
		if( FRocketSupport::IsRocket() || !FullProjectPath.StartsWith( FPaths::ConvertRelativePathToFull( FPaths::RootDir() ) ) )
		{
			const FString ProjectFilenameWithQuotes = FString::Printf(TEXT("\"%s\""), *FullProjectPath);
			AdditionalArguments += FString::Printf(TEXT("%s "), *ProjectFilenameWithQuotes);
		}

		if (FRocketSupport::IsRocket())
		{
			AdditionalArguments += TEXT("-rocket ");
		}
	}

	return AdditionalArguments;
}

void FModuleManager::OnModuleCompileSucceeded(FName ModuleName, TSharedRef<FModuleManager::FModuleInfo> ModuleInfo)
{
#if !IS_MONOLITHIC && WITH_EDITOR
	// UpdateModuleCompileData() should have been run before compiling so the
	// data in ModuleInfo should be correct for the pre-compile dll file.
	if (ensure(ModuleInfo->CompileData.bIsValid))
	{
		FDateTime FileTimeStamp;
		bool bGotFileTimeStamp = GetModuleFileTimeStamp(ModuleInfo, FileTimeStamp);

		ModuleInfo->CompileData.bHasFileTimeStamp = bGotFileTimeStamp;
		ModuleInfo->CompileData.FileTimeStamp = FileTimeStamp;

		if (ModuleInfo->CompileData.bHasFileTimeStamp)
		{
			ModuleInfo->CompileData.CompileMethod = EModuleCompileMethod::Runtime;
		}
		else
		{
			ModuleInfo->CompileData.CompileMethod = EModuleCompileMethod::Unknown;
		}
		WriteModuleCompilationInfoToConfig(ModuleName, ModuleInfo);
	}
#endif
}

void FModuleManager::UpdateModuleCompileData(FName ModuleName, TSharedRef<FModuleManager::FModuleInfo> ModuleInfo)
{
	// reset the compile data before updating it
	ModuleInfo->CompileData.bHasFileTimeStamp = false;
	ModuleInfo->CompileData.FileTimeStamp = FDateTime(0);
	ModuleInfo->CompileData.CompileMethod = EModuleCompileMethod::Unknown;
	ModuleInfo->CompileData.bIsValid = true;

#if !IS_MONOLITHIC && WITH_EDITOR
	ReadModuleCompilationInfoFromConfig(ModuleName, ModuleInfo);

	FDateTime FileTimeStamp;
	bool bGotFileTimeStamp = GetModuleFileTimeStamp(ModuleInfo, FileTimeStamp);

	if (!bGotFileTimeStamp)
	{
		// File missing? Reset the cached timestamp and method to defaults and save them.
		ModuleInfo->CompileData.bHasFileTimeStamp = false;
		ModuleInfo->CompileData.FileTimeStamp = FDateTime(0);
		ModuleInfo->CompileData.CompileMethod = EModuleCompileMethod::Unknown;
		WriteModuleCompilationInfoToConfig(ModuleName, ModuleInfo);
	}
	else
	{
		if (ModuleInfo->CompileData.bHasFileTimeStamp)
		{
			if (FileTimeStamp > ModuleInfo->CompileData.FileTimeStamp + ModuleManagerDefs::TimeStampEpsilon)
			{
				// The file is newer than the cached timestamp
				// The file must have been compiled externally
				ModuleInfo->CompileData.FileTimeStamp = FileTimeStamp;
				ModuleInfo->CompileData.CompileMethod = EModuleCompileMethod::External;
				WriteModuleCompilationInfoToConfig(ModuleName, ModuleInfo);
			}
		}
		else
		{
			// The cached timestamp and method are default value so this file has no history yet
			// We can only set its timestamp and save
			ModuleInfo->CompileData.bHasFileTimeStamp = true;
			ModuleInfo->CompileData.FileTimeStamp = FileTimeStamp;
			WriteModuleCompilationInfoToConfig(ModuleName, ModuleInfo);
		}
	}
#endif
}

void FModuleManager::ReadModuleCompilationInfoFromConfig(FName ModuleName, TSharedRef<FModuleManager::FModuleInfo> ModuleInfo)
{
	FString DateTimeString;
	if (GConfig->GetString(*ModuleManagerDefs::CompilationInfoConfigSection, *FString::Printf(TEXT("%s.TimeStamp"), *ModuleName.ToString()), DateTimeString, GEditorUserSettingsIni))
	{
		FDateTime TimeStamp;
		if (!DateTimeString.IsEmpty() && FDateTime::Parse(DateTimeString, TimeStamp))
		{
			ModuleInfo->CompileData.bHasFileTimeStamp = true;
			ModuleInfo->CompileData.FileTimeStamp = TimeStamp;

			FString CompileMethodString;
			if (GConfig->GetString(*ModuleManagerDefs::CompilationInfoConfigSection, *FString::Printf(TEXT("%s.LastCompileMethod"), *ModuleName.ToString()), CompileMethodString, GEditorUserSettingsIni))
			{
				if (CompileMethodString.Equals(ModuleManagerDefs::CompileMethodRuntime, ESearchCase::IgnoreCase))
				{
					ModuleInfo->CompileData.CompileMethod = EModuleCompileMethod::Runtime;
				}
				else if (CompileMethodString.Equals(ModuleManagerDefs::CompileMethodExternal, ESearchCase::IgnoreCase))
				{
					ModuleInfo->CompileData.CompileMethod = EModuleCompileMethod::External;
				}
			}
		}
	}
}

void FModuleManager::WriteModuleCompilationInfoToConfig(FName ModuleName, TSharedRef<FModuleManager::FModuleInfo> ModuleInfo)
{
	if (ensure(ModuleInfo->CompileData.bIsValid))
	{
		FString DateTimeString;
		if (ModuleInfo->CompileData.bHasFileTimeStamp)
		{
			DateTimeString = ModuleInfo->CompileData.FileTimeStamp.ToString();
		}

		GConfig->SetString(*ModuleManagerDefs::CompilationInfoConfigSection, *FString::Printf(TEXT("%s.TimeStamp"), *ModuleName.ToString()), *DateTimeString, GEditorUserSettingsIni);

		FString CompileMethodString = ModuleManagerDefs::CompileMethodUnknown;
		if (ModuleInfo->CompileData.CompileMethod == EModuleCompileMethod::Runtime)
		{
			CompileMethodString = ModuleManagerDefs::CompileMethodRuntime;
		}
		else if (ModuleInfo->CompileData.CompileMethod == EModuleCompileMethod::External)
		{
			CompileMethodString = ModuleManagerDefs::CompileMethodExternal;
		}

		GConfig->SetString(*ModuleManagerDefs::CompilationInfoConfigSection, *FString::Printf(TEXT("%s.LastCompileMethod"), *ModuleName.ToString()), *CompileMethodString, GEditorUserSettingsIni);
	}
}

bool FModuleManager::GetModuleFileTimeStamp(TSharedRef<const FModuleInfo> ModuleInfo, FDateTime& OutFileTimeStamp)
{
	if (IFileManager::Get().FileSize(*ModuleInfo->Filename) > 0)
	{
		OutFileTimeStamp = FDateTime(IFileManager::Get().GetTimeStamp(*ModuleInfo->Filename));
		return true;
	}
	return false;
}

void FModuleManager::FindModulePaths(const TCHAR* NamePattern, TMap<FName, FString> &OutModulePaths)
{
	// Search through the engine directory
	FindModulePathsInDirectory(FPlatformProcess::GetModulesDirectory(), false, NamePattern, OutModulePaths);

	// Search any engine directories
	for (int Idx = 0; Idx < EngineBinariesDirectories.Num(); Idx++)
	{
		FindModulePathsInDirectory(EngineBinariesDirectories[Idx], false, NamePattern, OutModulePaths);
	}

	// Search any game directories
	for (int Idx = 0; Idx < GameBinariesDirectories.Num(); Idx++)
	{
		FindModulePathsInDirectory(GameBinariesDirectories[Idx], true, NamePattern, OutModulePaths);
	}
}

void FModuleManager::FindModulePathsInDirectory(const FString& InDirectoryName, bool bIsGameDirectory, const TCHAR* NamePattern, TMap<FName, FString> &OutModulePaths)
{
	// Get the module configuration for this directory type
	#if UE_BUILD_DEBUG
		const TCHAR *ConfigSuffix = TEXT("-Debug");
	#elif UE_BUILD_DEVELOPMENT
		static bool bUsingDebugGame = FParse::Param(FCommandLine::Get(), TEXT("debug"));
		const TCHAR *ConfigSuffix = (bIsGameDirectory && bUsingDebugGame) ? TEXT("-DebugGame") : NULL;
	#elif UE_BUILD_TEST
		const TCHAR *ConfigSuffix = TEXT("-Test");
	#elif UE_BUILD_SHIPPING
		const TCHAR *ConfigSuffix = TEXT("-Shipping");
	#else
		#error "Unknown configuration type"
	#endif

	// Get the base name for modules of this application
	FString ModulePrefix = FPaths::GetBaseFilename(FPlatformProcess::ExecutableName());
	if (ModulePrefix.Contains(TEXT("-")))
	{
		ModulePrefix = ModulePrefix.Left(ModulePrefix.Find(TEXT("-")) + 1);
	}
	else
	{
		ModulePrefix += TEXT("-");
	}

	// Get the suffix for each module
	FString ModuleSuffix;
	if (ConfigSuffix != NULL)
	{
		ModuleSuffix += TEXT("-");
		ModuleSuffix += FPlatformProcess::GetBinariesSubdirectory();
		ModuleSuffix += ConfigSuffix;
	}
	ModuleSuffix += TEXT(".");
	ModuleSuffix += FPlatformProcess::GetModuleExtension();

	// Find all the files
	TArray<FString> FullFileNames;
	IFileManager::Get().FindFilesRecursive(FullFileNames, *InDirectoryName, *(ModulePrefix + NamePattern + ModuleSuffix), true, false);

	// Parse all the matching module names
	for (int32 Idx = 0; Idx < FullFileNames.Num(); Idx++)
	{
		const FString &FullFileName = FullFileNames[Idx];

		FString FileName = FPaths::GetCleanFilename(FullFileName);
		if (FileName.StartsWith(ModulePrefix) && FileName.EndsWith(ModuleSuffix))
		{
			FString ModuleName = FileName.Mid(ModulePrefix.Len(), FileName.Len() - ModulePrefix.Len() - ModuleSuffix.Len());
			if (ConfigSuffix != NULL || (!ModuleName.EndsWith("-Debug") && !ModuleName.EndsWith("-Shipping") && !ModuleName.EndsWith("-Test") && !ModuleName.EndsWith("-DebugGame")))
			{
				OutModulePaths.Add(FName(*ModuleName), FullFileName);
			}
		}
	}
}

bool FModuleManager::RecompileModulesAsync( const TArray< FName > ModuleNames, const FRecompileModulesCallback& InRecompileModulesCallback, const bool bWaitForCompletion, FOutputDevice &Ar )
{
#if !IS_MONOLITHIC
	// NOTE: This method of recompiling always using a rolling file name scheme, since we never want to unload before
	// we start recompiling, and we need the output DLL to be unlocked before we invoke the compiler

	ModuleCompilerStartedEvent.Broadcast();

	TArray< FModuleToRecompile > ModulesToRecompile;

	for( TArray< FName >::TConstIterator CurModuleIt( ModuleNames ); CurModuleIt; ++CurModuleIt )
	{
		const FName CurModuleName = *CurModuleIt;

		// Update our set of known modules, in case we don't already know about this module
		AddModule( CurModuleName );

		TSharedRef< FModuleInfo > Module = Modules.FindChecked( CurModuleName );

		FString NewModuleFileNameOnSuccess = Module->Filename;

		// Find a unique file name for the module
		FString UniqueSuffix;
		FString UniqueModuleFileName;
		MakeUniqueModuleFilename( CurModuleName, UniqueSuffix, UniqueModuleFileName );

		// If the recompile succeeds, we'll update our cached file name to use the new unique file name
		// that we setup for the module
		NewModuleFileNameOnSuccess = UniqueModuleFileName;

		FModuleToRecompile ModuleToRecompile;
		ModuleToRecompile.ModuleName = CurModuleName.ToString();
		ModuleToRecompile.ModuleFileSuffix = UniqueSuffix;
		ModuleToRecompile.NewModuleFilename = UniqueModuleFileName;
		ModulesToRecompile.Add( ModuleToRecompile );
	}

	// Kick off compilation!
	const FString AdditionalArguments = MakeUBTArgumentsForModuleCompiling();
	bool bWasSuccessful = StartCompilingModuleDLLs( FApp::GetGameName(), ModulesToRecompile, InRecompileModulesCallback, Ar, true, AdditionalArguments );
	if (bWasSuccessful)
	{
		// Go ahead and check for completion right away.  This is really just so that we can handle the case
		// where the user asked us to wait for the compile to finish before returning.
		bool bCompileStillInProgress = false;
		bool bCompileSucceeded = false;
		FOutputDeviceNull NullOutput;
		CheckForFinishedModuleDLLCompile( bWaitForCompletion, bCompileStillInProgress, bCompileSucceeded, NullOutput );
		if( !bCompileStillInProgress && !bCompileSucceeded )
		{
			bWasSuccessful = false;
		}
	}

	return bWasSuccessful;
#else
	return false;
#endif // !IS_MONOLITHIC
}

bool FModuleManager::CompileGameProject( const FString& ProjectFilename, FOutputDevice &Ar )
{
	bool bCompileSucceeded = false;

#if !IS_MONOLITHIC
	ModuleCompilerStartedEvent.Broadcast();

	// Leave an empty list of modules to recompile to indicate that all modules in the specified project file should be compiled
	TArray< FModuleToRecompile > ModulesToRecompile;	
	const FString GameName = FPaths::GetBaseFilename(ProjectFilename);
	FString AdditionalCmdLineArgs = FString::Printf(TEXT("\"%s\""), *ProjectFilename);
	if (FRocketSupport::IsRocket())
	{
		AdditionalCmdLineArgs += TEXT(" -rocket");
	}

	if (StartCompilingModuleDLLs(GameName, ModulesToRecompile, FRecompileModulesCallback(), Ar, false, AdditionalCmdLineArgs))
	{
		const bool bWaitForCompletion = true;	// Always wait
		bool bCompileStillInProgress = false;
		FOutputDeviceNull NullOutput;
		CheckForFinishedModuleDLLCompile(bWaitForCompletion, bCompileStillInProgress, bCompileSucceeded, NullOutput);
	}
#endif // !IS_MONOLITHIC
	return bCompileSucceeded;
}

bool FModuleManager::CompileGameProjectEditor( const FString& GameProjectFilename, FOutputDevice &Ar )
{
	bool bCompileSucceeded = false;
#if !IS_MONOLITHIC
	ModuleCompilerStartedEvent.Broadcast();

	// Leave an empty list of modules to recompile to indicate that all modules in the specified project file should be compiled
	TArray< FModuleToRecompile > ModulesToRecompile;

	FString GameTargetName = FPaths::GetBaseFilename(GameProjectFilename);
	//@NOTE: We assume 'MyProject'Editor is the target name for the editor...
	// Since we generate the targets, this should be fine - but worth noting.
	GameTargetName += TEXT("Editor");
	FString AdditionalCmdLineArgs = FString::Printf(TEXT("\"%s\""), *GameProjectFilename);
	if (FRocketSupport::IsRocket())
	{
		AdditionalCmdLineArgs += TEXT(" -rocket");
	}

	if (StartCompilingModuleDLLs(GameTargetName, ModulesToRecompile, FRecompileModulesCallback(), Ar, false, AdditionalCmdLineArgs))
	{
		const bool bWaitForCompletion = true;	// Always wait
		bool bCompileStillInProgress = false;
		FOutputDeviceNull NullOutput;
		CheckForFinishedModuleDLLCompile(bWaitForCompletion, bCompileStillInProgress, bCompileSucceeded, NullOutput);
	}

#endif // !IS_MONOLITHIC
	return bCompileSucceeded;
}

bool FModuleManager::GenerateCodeProjectFiles( const FString& ProjectFilename, FOutputDevice &Ar )
{
	bool bCompileSucceeded = false;
#if !IS_MONOLITHIC
	// Leave an empty list of modules to recompile to indicate that all modules in the specified project file should be compiled
	TArray< FModuleToRecompile > ModulesToRecompile;

#if PLATFORM_MAC
	FString CmdLineParams = TEXT("-xcodeprojectfile");
#else
	FString CmdLineParams = TEXT("-projectfiles");
#endif
	if ( !ProjectFilename.IsEmpty() )
	{
		bool bIsInRootFolder = false;
		if ( !FRocketSupport::IsRocket() )
		{
			// This is a non-rocket GenerateCodeProjectFiles call. If we are in the UE4 root, just generate all project files.
			FString AbsoluteProjectParentFolder = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::GetPath(FPaths::GetPath(ProjectFilename)));
			FString AbsoluteRootPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::RootDir());
			
			if ( AbsoluteProjectParentFolder.Right(1) != TEXT("/") )
			{
				AbsoluteProjectParentFolder += TEXT("/");
			}

			if ( AbsoluteRootPath.Right(1) != TEXT("/") )
			{
				AbsoluteRootPath += TEXT("/");
			}

			bIsInRootFolder = (AbsoluteProjectParentFolder == AbsoluteRootPath);
		}

		// @todo Rocket: New projects created under the root don't need a full path to the .uproject name (but eventually, they probably should always have this for consistency.)
		const bool bNeedsUProjectFilePath = FRocketSupport::IsRocket() || !bIsInRootFolder;
		if( bNeedsUProjectFilePath )
		{
			CmdLineParams += FString::Printf(TEXT(" \"%s\""), *IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*ProjectFilename));
		}

		if ( !FRocketSupport::IsRocket() )
		{
			// This is a non-rocket GenerateCodeProjectFiles call. If we are in the UE4 root, just generate all project files.
			if ( !bIsInRootFolder )
			{
				CmdLineParams += TEXT(" -game -engine");
			}
		}
        else
        {
            CmdLineParams += TEXT(" -rocket");
        }
	}

	if( InvokeUnrealBuildTool( CmdLineParams, Ar ) )
	{
		const bool bWaitForCompletion = true;	// Always wait
		bool bCompileStillInProgress = false;
		const FText SlowTaskOverrideText = NSLOCTEXT("FModuleManager", "GenerateCodeProjectsStatusMessage", "Generating code projects...");
		const bool bFireEvents = false;
		CheckForFinishedModuleDLLCompile( bWaitForCompletion, bCompileStillInProgress, bCompileSucceeded, Ar, SlowTaskOverrideText, bFireEvents );
	}
	else
	{
		Ar.Logf(TEXT("Failed invoke UnrealBuildTool when generating code project files."));
	}
#else
	Ar.Logf(TEXT("Cannot generate code project files from monolithic builds."));
#endif

	return bCompileSucceeded;
}

bool FModuleManager::IsUnrealBuildToolAvailable()
{
	// If using Rocket and the Rocket unreal build tool executable exists, then UBT is available
	if ( FRocketSupport::IsRocket() )
	{
		return FPaths::FileExists(GetUnrealBuildToolExecutableFilename());
	}
	else
	{
		// If not using Rocket, check to make sure UBT can be built, since it is an intermediate.
		// We are simply checking for the existence of source code files, which is a heuristic to determine if UBT can be built
		TArray<FString> Filenames;
		const FString UBTSourcePath = GetUnrealBuildToolSourceCodePath();
		const FString SearchPattern = TEXT("*.cs");
		const bool bFiles = true;
		const bool bDirectories = false;
		const bool bClearFileNames = false;
		IFileManager::Get().FindFilesRecursive(Filenames, *UBTSourcePath, *SearchPattern, bFiles, bDirectories, bClearFileNames);

		return Filenames.Num() > 0;
	}
}


void FModuleManager::UnloadOrAbandonModuleWithCallback( const FName InModuleName, FOutputDevice &Ar )
{
	TSharedRef< FModuleInfo > Module = Modules.FindChecked( InModuleName );
	
	Module->Module->PreUnloadCallback();

	const bool bIsHotReloadable = DoesLoadedModuleHaveUObjects( InModuleName );
	if( !bIsHotReloadable && Module->Module->SupportsDynamicReloading() )
	{
		if( !UnloadModule( InModuleName ))
		{
			Ar.Logf(TEXT("Module couldn't be unloaded, and so can't be recompiled while the engine is running."));
		}
	}
	else
	{
		Ar.Logf( TEXT( "Modile being reloaded does not support dynamic unloading -- abandoning existing loaded module so that we can load the recompiled version!" ) );
		AbandonModule( InModuleName );
	}
}

bool FModuleManager::LoadModuleWithCallback( const FName InModuleName, FOutputDevice &Ar )
{
	TSharedPtr<IModuleInterface> LoadedModule = LoadModule( InModuleName, true );
	bool bWasSuccessful = IsModuleLoaded( InModuleName );

	if (bWasSuccessful && LoadedModule.IsValid())
	{
		LoadedModule->PostLoadCallback();
	}
	else
	{
		Ar.Logf(TEXT("Module couldn't be loaded."));
	}

	return bWasSuccessful;
}

void FModuleManager::MakeUniqueModuleFilename( const FName InModuleName, FString& UniqueSuffix, FString& UniqueModuleFileName )
{
	TSharedRef< FModuleInfo > Module = Modules.FindChecked( InModuleName );
	do
	{
		// Use a random number as the unique file suffix, but mod it to keep it of reasonable length
		UniqueSuffix = FString::FromInt( FMath::Rand() % 10000 );

		const FString ModuleName = *InModuleName.ToString();
		const int32 MatchPos = Module->OriginalFilename.Find( ModuleName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if( ensure( MatchPos != INDEX_NONE ) )
		{
			const int32 SuffixPos = MatchPos + ModuleName.Len();
			UniqueModuleFileName = FString::Printf( TEXT( "%s-%s%s" ),
				*Module->OriginalFilename.Left( SuffixPos ),
				*UniqueSuffix,
				*Module->OriginalFilename.Right( Module->OriginalFilename.Len() - SuffixPos ) );
		}
	}
	while (IFileManager::Get().GetFileAgeSeconds(*UniqueModuleFileName) != -1.0);
}


bool FModuleManager::StartCompilingModuleDLLs(const FString& GameName, const TArray< FModuleToRecompile >& ModuleNames, 
	const FRecompileModulesCallback& InRecompileModulesCallback, FOutputDevice& Ar, bool bInFailIfGeneratedCodeChanges, 
	const FString& InAdditionalCmdLineArgs )
{
#if PLATFORM_DESKTOP && !IS_MONOLITHIC
	// Keep track of what we're compiling
	ModulesBeingCompiled = ModuleNames;
	ModulesThatWereBeingRecompiled = ModulesBeingCompiled;

	const TCHAR* BuildPlatformName = FPlatformMisc::GetUBTPlatform();

	FString BuildConfigurationName = 
#if UE_BUILD_DEBUG
		TEXT( "Debug");
#elif UE_BUILD_SHIPPING
		TEXT( "Shipping" );
#else
		FParse::Param(FCommandLine::Get(), TEXT("debug"))? TEXT( "DebugGame" ) : TEXT( "Development" );
#endif

	RecompileModulesCallback = InRecompileModulesCallback;

	// Pass a module file suffix to UBT if we have one
	FString ModuleArg;
	for( int32 CurModuleIndex = 0; CurModuleIndex < ModuleNames.Num(); ++CurModuleIndex )
	{
		if( !ModuleNames[ CurModuleIndex ].ModuleFileSuffix.IsEmpty() )
		{
			ModuleArg += FString::Printf( TEXT( " -ModuleWithSuffix %s %s" ), *ModuleNames[ CurModuleIndex ].ModuleName, *ModuleNames[ CurModuleIndex ].ModuleFileSuffix );
		}
		else
		{
			ModuleArg += FString::Printf( TEXT( " -Module %s" ), *ModuleNames[ CurModuleIndex ].ModuleName );
		}
		Ar.Logf( TEXT( "Recompiling %s..." ), *ModuleNames[ CurModuleIndex ].ModuleName );

		FName ModuleFName(*ModuleNames[ CurModuleIndex ].ModuleName);
		if (ensure(Modules.Contains(ModuleFName)))
		{
			// prepare the compile info in the FModuleInfo so that it can be compared after compiling
			TSharedRef< FModuleInfo > ModuleInfo = Modules.FindChecked(ModuleFName);
			UpdateModuleCompileData(ModuleFName, ModuleInfo);
		}
	}

	FString ExtraArg;
#if UE_EDITOR
	// NOTE: When recompiling from the editor, we're passed the game target name, not the editor target name, but we'll
	//       pass "-editorrecompile" to UBT which tells UBT to figure out the editor target to use for this game, since
	//       we can't possibly know what the target is called from within the engine code.
	ExtraArg = TEXT( "-editorrecompile " );
#endif

	if (bInFailIfGeneratedCodeChanges)
	{
		// Additional argument to let UHT know that we can only compile the module if the generated code didn't change
		ExtraArg += TEXT( "-FailIfGeneratedCodeChanges " );
	}

	FString CmdLineParams = FString::Printf( TEXT( "%s%s %s %s %s%s" ), 
		*GameName, *ModuleArg, 
		BuildPlatformName, *BuildConfigurationName, 
		*ExtraArg, *InAdditionalCmdLineArgs );

	const bool bInvocationSuccessful = InvokeUnrealBuildTool(CmdLineParams, Ar);
	if ( !bInvocationSuccessful )
	{
		// No longer compiling modules
		ModulesBeingCompiled.Empty();

		ModuleCompilerFinishedEvent.Broadcast(FString(), false, false);

		// Fire task completion delegate 
		
		RecompileModulesCallback.ExecuteIfBound( false, false );
		RecompileModulesCallback.Unbind();
	}

	return bInvocationSuccessful;
#else
	return false;
#endif
}

FString FModuleManager::GetUnrealBuildToolSourceCodePath()
{
	return FPaths::Combine(*FPaths::EngineDir(), TEXT("Source"), TEXT("Programs"), TEXT("UnrealBuildTool"));
}

FString FModuleManager::GetUnrealBuildToolExecutableFilename()
{
	const FString UBTPath = FString::Printf(TEXT("%sBinaries/DotNET/"), *FPaths::EngineDir());
	const FString UBTExe = TEXT("UnrealBuildTool.exe");

	FString ExecutableFileName = UBTPath / UBTExe;
	ExecutableFileName = FPaths::ConvertRelativePathToFull(ExecutableFileName);
	return ExecutableFileName;
}

bool FModuleManager::BuildUnrealBuildTool(FOutputDevice &Ar)
{
#if !IS_MONOLITHIC
	if ( FRocketSupport::IsRocket() )
	{
		// We may not build UBT in rocket
		return false;
	}

	FString CompilerExecutableFilename;
	FString CmdLineParams;
#if PLATFORM_WINDOWS
	// To build UBT for windows, we must assemble a batch file that first registers the environment variable necessary to run msbuild then run it
	// This can not be done in a single invocation of CMD.exe because the environment variables do not transfer between subsequent commands when using the "&" syntax
	// devenv.exe can be used to build as well but it takes several seconds to start up so it is not desirable

	// First determine the appropriate vcvars batch file to launch
	FString VCVarsBat;
	{
#if _MSC_VER >= 1800
 		TCHAR VS12Path[MAX_PATH];
 		FPlatformMisc::GetEnvironmentVariable(TEXT("VS120COMNTOOLS"), VS12Path, ARRAY_COUNT(VS12Path));
 		VCVarsBat = VS12Path;
 		VCVarsBat += TEXT("/../../VC/bin/x86_amd64/vcvarsx86_amd64.bat");
#else
		TCHAR VS11Path[MAX_PATH];
		FPlatformMisc::GetEnvironmentVariable(TEXT("VS110COMNTOOLS"), VS11Path, ARRAY_COUNT(VS11Path));
		VCVarsBat = VS11Path;
		VCVarsBat += TEXT("/../../VC/bin/x86_amd64/vcvarsx86_amd64.bat");
#endif
	}

	// Check to make sure we found one.
	if ( VCVarsBat.IsEmpty() || !FPaths::FileExists(VCVarsBat) )
	{
		// VCVars doesn't exist, we can not build UBT
		CompilerExecutableFilename = TEXT("");
	}
	else
	{
		// Now make a batch file in the intermediate directory to invoke the vcvars batch then msbuild
		FString BuildBatchFile = FPaths::EngineIntermediateDir() / TEXT("Build") / TEXT("UnrealBuildTool") / TEXT("BuildUBT.bat");
		BuildBatchFile.ReplaceInline(TEXT("/"), TEXT("\\"));

		const FString CsProjLocation = FPaths::ConvertRelativePathToFull(GetUnrealBuildToolSourceCodePath()) / TEXT("UnrealBuildTool.csproj");
		
		FString BatchFileContents;
		BatchFileContents = FString::Printf(TEXT("call \"%s\"") LINE_TERMINATOR, *VCVarsBat);
		BatchFileContents += FString::Printf(TEXT("msbuild /nologo /verbosity:quiet %s /property:Configuration=Development /property:Platform=AnyCPU"), *CsProjLocation);
		FFileHelper::SaveStringToFile(BatchFileContents, *BuildBatchFile);

		{
			TCHAR CmdExePath[MAX_PATH];
			FPlatformMisc::GetEnvironmentVariable(TEXT("ComSpec"), CmdExePath, ARRAY_COUNT(CmdExePath));
			CompilerExecutableFilename = CmdExePath;
		}

		CmdLineParams = TEXT("/c ");
		CmdLineParams += BuildBatchFile;
	}
#elif PLATFORM_MAC
	const FString CsProjLocation = FPaths::ConvertRelativePathToFull(GetUnrealBuildToolSourceCodePath()) / TEXT("UnrealBuildTool_Mono.csproj");
	FString ScriptPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Build/BatchFiles/Mac/RunXBuild.sh"));
	CompilerExecutableFilename = TEXT("/bin/sh");
	CmdLineParams = FString::Printf(TEXT("\"%s\" /property:Configuration=Development %s"), *ScriptPath, *CsProjLocation);
#else
	Ar.Log( TEXT( "Unknown platform, unable to build UnrealBuildTool." ) );
#endif

	// If a compiler executable was provided, try to build now
	if ( !CompilerExecutableFilename.IsEmpty() )
	{
		const bool bLaunchDetached = false;
		const bool bLaunchHidden = true;
		const bool bLaunchReallyHidden = bLaunchHidden;
		FProcHandle ProcHandle = FPlatformProcess::CreateProc( *CompilerExecutableFilename, *CmdLineParams, bLaunchDetached, bLaunchHidden, bLaunchReallyHidden, NULL, 0, NULL, NULL );
		if ( ProcHandle.IsValid() )
		{
			FPlatformProcess::WaitForProc( ProcHandle );
			ProcHandle.Close();
		}

		// If the executable appeared where we expect it, then we were successful
		return FPaths::FileExists(GetUnrealBuildToolExecutableFilename());
	}
	else
#endif // !IS_MONOLITHIC
	{
		return false;
	}
}

bool FModuleManager::InvokeUnrealBuildTool( const FString& InCmdLineParams, FOutputDevice &Ar )
{
#if PLATFORM_DESKTOP && !IS_MONOLITHIC
	FString CmdLineParams = InCmdLineParams;

	if ( FRocketSupport::IsRocket() )
	{
		CmdLineParams += TEXT(" -rocket");
	}

	// Make sure we're not already compiling something!
	check( !IsCurrentlyCompiling() );

	// UnrealBuildTool is currently always located in the Binaries/DotNET folder
	FString ExecutableFileName = GetUnrealBuildToolExecutableFilename();

	// Rocket never builds UBT, UnrealBuildTool should already exist
	if ( !FRocketSupport::IsRocket() )
	{
		// When not using rocket, we should attempt to build UBT to make sure it is up to date
		// Only do this if we have not already successfully done it once during this session.
		static bool bSuccessfullyBuiltUBTOnce = false;
		if ( !bSuccessfullyBuiltUBTOnce )
		{
			Ar.Log( TEXT( "Building UnrealBuildTool..." ) );
			if ( BuildUnrealBuildTool(Ar) )
			{
				bSuccessfullyBuiltUBTOnce = true;
			}
			else
			{
				// Failed to build UBT
				Ar.Log( TEXT( "Failed to build UnrealBuildTool." ) );
				return false;
			}
		}
	}

	Ar.Logf( TEXT( "Launching UnrealBuildTool... [%s %s]" ), *ExecutableFileName, *CmdLineParams );

#if PLATFORM_MAC
	// On Mac we launch UBT with Mono
	FString ScriptPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Build/BatchFiles/Mac/RunMono.sh"));
	CmdLineParams = FString::Printf(TEXT("\"%s\" \"%s\" %s"), *ScriptPath, *ExecutableFileName, *CmdLineParams);
	ExecutableFileName = TEXT( "/bin/sh" );
#endif

	// Run UnrealBuildTool
	const bool bLaunchDetached = false;
	const bool bLaunchHidden = true;
	const bool bLaunchReallyHidden = bLaunchHidden;

	// Setup output redirection pipes, so that we can harvest compiler output and display it ourselves
#if PLATFORM_LINUX
	int pipefd[2];
	pipe(pipefd);
	void* PipeRead = &pipefd[0];
	void* PipeWrite = &pipefd[1];
#else
	void* PipeRead = NULL;
	void* PipeWrite = NULL;
#endif

	verify(FPlatformProcess::CreatePipe(PipeRead, PipeWrite));
	ModuleCompileReadPipeText = TEXT("");

	FProcHandle ProcHandle = FPlatformProcess::CreateProc( *ExecutableFileName, *CmdLineParams, bLaunchDetached, bLaunchHidden, bLaunchReallyHidden, NULL, 0, NULL, PipeWrite );
	if( !ProcHandle.IsValid() )
	{
		Ar.Logf( TEXT( "Failed to launch Unreal Build Tool. (%s)" ), *ExecutableFileName );

		// We're done with the process handle now
		ModuleCompileProcessHandle.Reset();
		ModuleCompileReadPipe = NULL;
	}
	else
	{
		ModuleCompileProcessHandle = ProcHandle;
		ModuleCompileReadPipe = PipeRead;
	}

	// We no longer need the Write pipe so close it.
	// We DO need the Read pipe however...
#if PLATFORM_LINUX
	close(*(int*)PipeWrite);
#else
	FPlatformProcess::ClosePipe(0, PipeWrite);
#endif

	return ProcHandle.IsValid();
#else
	return false;
#endif // PLATFORM_DESKTOP && !IS_MONOLITHIC
}


void FModuleManager::CheckForFinishedModuleDLLCompile( const bool bWaitForCompletion, bool& bCompileStillInProgress, bool& bCompileSucceeded, FOutputDevice& Ar, const FText& SlowTaskOverrideText, bool bFireEvents )
{
#if PLATFORM_DESKTOP && !IS_MONOLITHIC
	bCompileStillInProgress = false;
	bCompileSucceeded = false;

	// Is there a compilation in progress?
	if( IsCurrentlyCompiling() )
	{
		bCompileStillInProgress = true;

		// Ensure slow task messages are seen.
		GWarn->PushStatus();

		// Update the slow task dialog if we were summoned from a synchronous recompile path
		if (GIsSlowTask)
		{
			if ( !SlowTaskOverrideText.IsEmpty() )
			{
				GWarn->StatusUpdate(-1, -1, SlowTaskOverrideText);
			}
			else
			{
				FText StatusUpdate;
				if ( ModulesBeingCompiled.Num() > 0 )
				{
					FFormatNamedArguments Args;
					Args.Add( TEXT("CodeModuleName"), FText::FromString( ModulesBeingCompiled[0].ModuleName ) );
					StatusUpdate = FText::Format( NSLOCTEXT("FModuleManager", "CompileSpecificModuleStatusMessage", "{CodeModuleName}: Compiling modules..."), Args );
				}
				else
				{
					StatusUpdate = NSLOCTEXT("FModuleManager", "CompileStatusMessage", "Compiling modules...");
				}

				GWarn->StatusUpdate(-1, -1, StatusUpdate);
			}
		}

		// Check to see if the compile has finished yet
		int32 ReturnCode = 1;
		while (bCompileStillInProgress)
		{
			if( FPlatformProcess::GetProcReturnCode( ModuleCompileProcessHandle, &ReturnCode ) )
			{
				bCompileStillInProgress = false;
			}
			
			if (bRequestCancelCompilation)
			{
				FPlatformProcess::TerminateProc(ModuleCompileProcessHandle);
				bCompileStillInProgress = bRequestCancelCompilation = false;
			}

			if( bCompileStillInProgress )
			{
				ModuleCompileReadPipeText += FPlatformProcess::ReadPipe(ModuleCompileReadPipe);

				if( !bWaitForCompletion )
				{
					// We haven't finished compiling, but we were asked to return immediately

					break;
				}

				// Give up a small timeslice if we haven't finished recompiling yet
				FPlatformProcess::Sleep( 0.01f );
			}
		}
		
		bRequestCancelCompilation = false;

		// Restore any status from before the loop - see PushStatus() above.
		GWarn->PopStatus();

		if( !bCompileStillInProgress )		
		{
			// Compilation finished, now we need to grab all of the text from the output pipe
			ModuleCompileReadPipeText += FPlatformProcess::ReadPipe(ModuleCompileReadPipe);

			// Was the compile successful?
			bCompileSucceeded = ( ReturnCode == 0 );

			// If compilation succeeded for all modules, go back to the modules and update their module file names
			// in case we recompiled the modules to a new unique file name.  This is needed so that when the module
			// is reloaded after the recompile, we load the new DLL file name, not the old one
			if( bCompileSucceeded )
			{
				for( int32 CurModuleIndex = 0; CurModuleIndex < ModulesThatWereBeingRecompiled.Num(); ++CurModuleIndex )
				{
					const FModuleToRecompile& CurModule = ModulesThatWereBeingRecompiled[ CurModuleIndex ];

					// Were we asked to assign a new file name for this module?
					if( !CurModule.NewModuleFilename.IsEmpty() )
					{
						// Find the module
						const FName CurModuleName = FName( *CurModule.ModuleName );
						if( ensure( Modules.Contains( CurModuleName ) ) )
						{
							TSharedRef< FModuleInfo > ModuleInfo = Modules.FindChecked( CurModuleName );

							// If the compile succeeded, update the module info entry with the new file name for this module
							ModuleInfo->Filename = CurModule.NewModuleFilename;

							OnModuleCompileSucceeded(FName(*CurModule.ModuleName), ModuleInfo);
						}
					}
				}

				ModulesThatWereBeingRecompiled.Empty();
			}

			// We're done with the process handle now
			ModuleCompileProcessHandle.Close();
			ModuleCompileProcessHandle.Reset();

#if PLATFORM_LINUX
			close(*(int *)ModuleCompileReadPipe);
#else
			FPlatformProcess::ClosePipe(ModuleCompileReadPipe, 0);
#endif

			Ar.Log(*ModuleCompileReadPipeText);
			const FString FinalOutput = ModuleCompileReadPipeText;
			ModuleCompileReadPipe = NULL;
			ModuleCompileReadPipeText = TEXT("");

			// No longer compiling modules
			ModulesBeingCompiled.Empty();

			if ( bFireEvents )
			{
				const bool bShowLogOnSuccess = false;
				ModuleCompilerFinishedEvent.Broadcast(FinalOutput, bCompileSucceeded, !bCompileSucceeded || bShowLogOnSuccess);

				// Fire task completion delegate 
				RecompileModulesCallback.ExecuteIfBound( true, bCompileSucceeded );
				RecompileModulesCallback.Unbind();
			}
		}
		else
		{
			Ar.Logf(TEXT("Error: CheckForFinishedModuleDLLCompile: Compilation is still in progress"));
		}
	}
	else
	{
		Ar.Logf(TEXT("Error: CheckForFinishedModuleDLLCompile: There is no compilation in progress right now"));
	}
#endif // PLATFORM_DESKTOP && !IS_MONOLITHIC
}

bool FModuleManager::CheckModuleCompatibility(const TCHAR* Filename)
{
	static FBinaryFileVersion ExeVersion = FPlatformProcess::GetBinaryFileVersion(FPlatformProcess::ExecutableName(/*bRemoveExtension=*/false));
	FBinaryFileVersion DLLVersion = FPlatformProcess::GetBinaryFileVersion(Filename);

	if (ExeVersion.IsValid() && DLLVersion.IsValid() && ExeVersion == DLLVersion)
	{
		return true;
	}
	else
	{
		FString ExeVersionString;
		FString DLLVersionString;
		ExeVersion.ToString(ExeVersionString);
		DLLVersion.ToString(DLLVersionString);
		UE_LOG(LogModuleManager, Warning, TEXT("Found module file %s (version %s), but it was incompatible with the current engine version (%s). This is likely a stale module that must be recompiled."), Filename, *DLLVersionString, *ExeVersionString);
		return false;
	}
}


bool FModuleManager::RecompileModuleDLLs( const TArray< FModuleToRecompile >& ModuleNames, FOutputDevice& Ar )
{
	bool bCompileSucceeded = false;
#if !IS_MONOLITHIC
	const FString AdditionalArguments = MakeUBTArgumentsForModuleCompiling();
	if( StartCompilingModuleDLLs( FApp::GetGameName(), ModuleNames, FRecompileModulesCallback(), Ar, true, AdditionalArguments ) )
	{
		const bool bWaitForCompletion = true;	// Always wait
		bool bCompileStillInProgress = false;
		CheckForFinishedModuleDLLCompile( bWaitForCompletion, bCompileStillInProgress, bCompileSucceeded, Ar );
	}
#endif
	return bCompileSucceeded;
}

void FModuleManager::StartProcessingNewlyLoadedObjects()
{
	// Only supposed to be called once
	ensure( bCanProcessNewlyLoadedObjects == false );	
	bCanProcessNewlyLoadedObjects = true;
}

void FModuleManager::AddBinariesDirectory(const TCHAR *InDirectory, bool bIsGameDirectory)
{
	if (bIsGameDirectory)
	{
		GameBinariesDirectories.Add(InDirectory);
	}
	else
	{
		EngineBinariesDirectories.Add(InDirectory);
	}
}

void FModuleManager::SetGameBinariesDirectory(const TCHAR* InDirectory)
{
#if !IS_MONOLITHIC
	// Before loading game DLLs, make sure that the DLL files can be located by the OS by adding the
	// game binaries directory to the OS DLL search path.  This is so that game module DLLs which are
	// statically loaded as dependencies of other game modules can be located by the OS.
	FPlatformProcess::PushDllDirectory(InDirectory);

	// Add it to the list of game directories to search
	GameBinariesDirectories.Add(InDirectory);
#endif
}

bool FModuleManager::DoesLoadedModuleHaveUObjects( const FName ModuleName )
{
	if( IsModuleLoaded( ModuleName ) && IsPackageLoaded.IsBound() )
	{
		return IsPackageLoaded.Execute( *FString( FString( TEXT("/Script/") ) + ModuleName.ToString() ) );
	}

	return false;
}
