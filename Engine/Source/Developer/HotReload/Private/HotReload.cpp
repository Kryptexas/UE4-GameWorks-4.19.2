// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HotReloadPrivatePCH.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetReinstanceUtilities.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "EngineAnalytics.h"
#include "ScopedTimers.h"

DEFINE_LOG_CATEGORY(LogHotReload);

#define LOCTEXT_NAMESPACE "HotReload"

/**
 * Module for HotReload support
 */
class FHotReloadModule : public IHotReloadModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** IHotReloadInterface implementation */
	virtual void AddHotReloadFunctionRemap(Native NewFunctionPointer, Native OldFunctionPointer) override;	
	virtual void RebindPackages(TArray< UPackage* > Packages, TArray< FName > DependentModules, const bool bWaitForCompletion, FOutputDevice &Ar) override;
	virtual void DoHotReloadFromEditor() override;
	virtual FHotReloadEvent& OnHotReload() { return HotReloadEvent; }	

private:
	
	/**
	 * Adds a callback to directory watcher for the game binaries folder.
	 */
	void InitHotReloadWatcher();

	/**
	 * Removes a directory watcher callback
	 */
	void ShutdownHotReloadWatcher();

	/**
	 * Performs hot-reload from IDE (when game DLLs change)
	 */
	void DoHotReloadFromIDE();

	/**
	* Performs internal module recompilation
	*/
	ECompilationResult::Type RebindPackagesInternal(TArray< UPackage* > Packages, TArray< FName > DependentModules, const bool bWaitForCompletion, FOutputDevice &Ar);

	/**
	 * Does the actual hot-reload, unloads old modules, loads new ones
	 */
	ECompilationResult::Type DoHotReloadInternal(bool bRecompileFinished, bool bRecompileSucceeded, TArray<UPackage*> Packages, TArray<FName> InDependentModules, FOutputDevice &HotReloadAr);

	/**
	* Callback for async ompilation
	*/
	void DoHotReloadCallback(bool bRecompileFinished, bool bRecompileSucceeded, TArray<UPackage*> Packages, TArray<FName> InDependentModules, FOutputDevice &HotReloadAr);

	/**
	 * Gets all currently loaded game module names.
	 */
	void GetGameModules(TArray<FString>& OutGameModules);

	/**
	 * Gets packages to re-bind and dependent modules.
	 */
	void GetPackagesToRebindAndDependentModules(const TArray<FString>& InGameModuleNames, TArray<UPackage*>& OutPackagesToRebind, TArray<FName>& OutDependentModules);

	/**
	 * Called from CoreUObject to re-instance hot-reloaded classes
	 */
	void ReinstanceClass(UClass* OldClass, UClass* NewClass);

	/**
	 * Tick function for FTicker: checks for re-loaded modules and does hot-reload from IDE
	 */
	bool Tick(float DeltaTime);

	/**
	 * Directory watcher callback
	 */
	void OnHotReloadBinariesChanged(const TArray<struct FFileChangeData>& FileChanges);

	/**
	 * Broadcasts that a hot reload just finished. 
	 *
	 * @param	bWasTriggeredAutomatically	True if the hot reload was invoked automatically by the hot reload system after detecting a changed DLL
	 */
	void BroadcastHotReload( bool bWasTriggeredAutomatically ) 
	{ 
		HotReloadEvent.Broadcast( bWasTriggeredAutomatically ); 
	}

	/**
	 * Sends analytics event about the re-load
	*/
	static void RecordAnalyticsEvent(const TCHAR* ReloadFrom, ECompilationResult::Type Result, double Duration, int32 PackageCount, int32 DependentModulesCount);

	/** FTicker delegate (hot-reload from IDE) */
	FTickerDelegate TickerDelegate;

	/** Callback when game binaries folder changes */
	IDirectoryWatcher::FDirectoryChanged BinariesFolderChangedDelegate;

	/** True if currently hot-reloading from editor (suppresses hot-reload from IDE) */
	bool bIsHotReloadingFromEditor;
	
	struct FRecompiledModule
	{
		FString Name;
		FString NewFilename;
		FRecompiledModule() {}
		FRecompiledModule(const FString& InName, const FString& InFilename)
			: Name(InName)
			, NewFilename(InFilename)
		{}
	};

	/** New module DLLs */
	TArray<FRecompiledModule> NewModules;

	/** Delegate broadcast when a module has been hot-reloaded */
	FHotReloadEvent HotReloadEvent;
};

IMPLEMENT_MODULE(FHotReloadModule, HotReload);

void FHotReloadModule::StartupModule()
{
	bIsHotReloadingFromEditor = false;

	// Register re-instancing delegate (Core)
	FCoreUObjectDelegates::ReplaceHotReloadClassDelegate.BindRaw(this, &FHotReloadModule::ReinstanceClass);
	
	// Register directory watcher delegate
	InitHotReloadWatcher();

	// Register hot-reload from IDE ticker
	TickerDelegate = FTickerDelegate::CreateRaw(this, &FHotReloadModule::Tick);
	FTicker::GetCoreTicker().AddTicker(TickerDelegate);
}

void FHotReloadModule::ShutdownModule()
{
	FTicker::GetCoreTicker().RemoveTicker(TickerDelegate);
	ShutdownHotReloadWatcher();
}

/** Type hash for a UObject Function Pointer, maybe not a great choice, but it should be sufficient for the needs here. **/
inline uint32 GetTypeHash(Native A)
{
	return *(uint32*)&A;
}

/** Map from old function pointer to new function pointer for hot reload. */
static TMap<Native, Native> HotReloadFunctionRemap;

/** Adds and entry for the UFunction native pointer remap table */
void FHotReloadModule::AddHotReloadFunctionRemap(Native NewFunctionPointer, Native OldFunctionPointer)
{
	Native OtherNewFunction = HotReloadFunctionRemap.FindRef(OldFunctionPointer);
	check(!OtherNewFunction || OtherNewFunction == NewFunctionPointer);
	check(NewFunctionPointer);
	check(OldFunctionPointer);
	HotReloadFunctionRemap.Add(OldFunctionPointer, NewFunctionPointer);
}

void FHotReloadModule::DoHotReloadFromEditor()
{
	TArray<FString> GameModuleNames;
	TArray<UPackage*> PackagesToRebind;
	TArray<FName> DependentModules;
	GetGameModules(GameModuleNames);
	ECompilationResult::Type Result = ECompilationResult::Unsupported;
	// Analytics
	double Duration = 0.0;

	if (GameModuleNames.Num() > 0)
	{
		FScopedDurationTimer Timer(Duration);


		GetPackagesToRebindAndDependentModules(GameModuleNames, PackagesToRebind, DependentModules);

		const bool bWaitForCompletion = false;	// Don't wait -- we want compiling to happen asynchronously
		Result = RebindPackagesInternal(PackagesToRebind, DependentModules, bWaitForCompletion, *GLog);
	}

	RecordAnalyticsEvent(TEXT("Editor"), Result, Duration, PackagesToRebind.Num(), DependentModules.Num());
}

void FHotReloadModule::DoHotReloadCallback(bool bRecompileFinished, bool bRecompileSucceeded, TArray<UPackage*> Packages, TArray< FName > InDependentModules, FOutputDevice &HotReloadAr)
{
	DoHotReloadInternal(bRecompileFinished, bRecompileSucceeded, Packages, InDependentModules, HotReloadAr);
}

ECompilationResult::Type FHotReloadModule::DoHotReloadInternal(bool bRecompileFinished, bool bRecompileSucceeded, TArray<UPackage*> Packages, TArray< FName > InDependentModules, FOutputDevice &HotReloadAr)
{
	ECompilationResult::Type Result = ECompilationResult::Unsupported;
#if !IS_MONOLITHIC
	if (bRecompileSucceeded)
	{
		FFeedbackContext& ErrorsFC = UClass::GetDefaultPropertiesFeedbackContext();
		ErrorsFC.Errors.Empty();
		ErrorsFC.Warnings.Empty();
		// Rebind the hot reload DLL 
		TGuardValue<bool> GuardIsHotReload(GIsHotReload, true);
		TGuardValue<bool> GuardIsInitialLoad(GIsInitialLoad, true);
		HotReloadFunctionRemap.Empty(); // redundant

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS); // we create a new CDO in the transient package...this needs to go away before we try again.

		// Load the new modules up
		bool bReloadSucceeded = false;
		for (TArray<UPackage*>::TConstIterator CurPackageIt(Packages); CurPackageIt; ++CurPackageIt)
		{
			UPackage* Package = *CurPackageIt;
			FName ShortPackageName = FPackageName::GetShortFName(Package->GetFName());

			// Abandon the old module.  We can't unload it because various data structures may be living
			// that have vtables pointing to code that would become invalidated.
			FModuleManager::Get().AbandonModule(ShortPackageName);

			// Module should never be loaded at this point
			check(!FModuleManager::Get().IsModuleLoaded(ShortPackageName));

			// Load the newly-recompiled module up (it will actually have a different DLL file name at this point.)
			FModuleManager::Get().LoadModule(ShortPackageName);
			bReloadSucceeded = FModuleManager::Get().IsModuleLoaded(ShortPackageName);
			if (!bReloadSucceeded)
			{
				HotReloadAr.Logf(ELogVerbosity::Warning, TEXT("HotReload failed, reload failed %s."), *Package->GetName());
				Result = ECompilationResult::OtherCompilationError;
				break;
			}
		}
		// Load dependent modules.
		for (int32 Nx = 0; Nx < InDependentModules.Num(); ++Nx)
		{
			const FName ModuleName = InDependentModules[Nx];

			FModuleManager::Get().UnloadOrAbandonModuleWithCallback(ModuleName, HotReloadAr);
			const bool bLoaded = FModuleManager::Get().LoadModuleWithCallback(ModuleName, HotReloadAr);
			if (!bLoaded)
			{
				HotReloadAr.Logf(ELogVerbosity::Warning, TEXT("Unable to reload module %s"), *ModuleName.GetPlainNameString());
			}
		}

		if (ErrorsFC.Errors.Num() || ErrorsFC.Warnings.Num())
		{
			TArray<FString> All;
			All = ErrorsFC.Errors;
			All += ErrorsFC.Warnings;

			ErrorsFC.Errors.Empty();
			ErrorsFC.Warnings.Empty();

			FString AllInOne;
			for (int32 Index = 0; Index < All.Num(); Index++)
			{
				AllInOne += All[Index];
				AllInOne += TEXT("\n");
			}
			HotReloadAr.Logf(ELogVerbosity::Warning, TEXT("Some classes could not be reloaded:\n%s"), *AllInOne);
		}

		if (bReloadSucceeded)
		{
			int32 Count = 0;
			// Remap all native functions (and gather scriptstructs)
			TArray<UScriptStruct*> ScriptStructs;
			for (FRawObjectIterator It; It; ++It)
			{
				if (UFunction* Function = Cast<UFunction>(*It))
				{
					if (Native NewFunction = HotReloadFunctionRemap.FindRef(Function->GetNativeFunc()))
					{
						Count++;
						Function->SetNativeFunc(NewFunction);
					}
				}

				if (UScriptStruct* ScriptStruct = Cast<UScriptStruct>(*It))
				{
					if (Packages.ContainsByPredicate([=](UPackage* Package) { return ScriptStruct->IsIn(Package); }) && ScriptStruct->GetCppStructOps())
					{
						ScriptStructs.Add(ScriptStruct);
					}
				}
			}
			// now let's set up the script structs...this relies on super behavior, so null them all, then set them all up. Internally this sets them up hierarchically.
			for (int32 ScriptIndex = 0; ScriptIndex < ScriptStructs.Num(); ScriptIndex++)
			{
				ScriptStructs[ScriptIndex]->ClearCppStructOps();
			}
			for (int32 ScriptIndex = 0; ScriptIndex < ScriptStructs.Num(); ScriptIndex++)
			{
				ScriptStructs[ScriptIndex]->PrepareCppStructOps();
				check(ScriptStructs[ScriptIndex]->GetCppStructOps());
			}
			HotReloadAr.Logf(ELogVerbosity::Warning, TEXT("HotReload successful (%d functions remapped  %d scriptstructs remapped)"), Count, ScriptStructs.Num());

			HotReloadFunctionRemap.Empty();
			Result = ECompilationResult::Succeeded;
		}


		const bool bWasTriggeredAutomatically = !bIsHotReloadingFromEditor;
		BroadcastHotReload( bWasTriggeredAutomatically );
	}
	else if (bRecompileFinished)
	{
		HotReloadAr.Logf(ELogVerbosity::Warning, TEXT("HotReload failed, recompile failed"));
		Result = ECompilationResult::OtherCompilationError;
	}
#endif
	bIsHotReloadingFromEditor = false;
	return Result;
}

void FHotReloadModule::RebindPackages(TArray<UPackage*> InPackages, TArray<FName> DependentModules, const bool bWaitForCompletion, FOutputDevice &Ar)
{
	ECompilationResult::Type Result = ECompilationResult::Unknown;
	double Duration = 0.0;
	{
		FScopedDurationTimer RebindTimer(Duration);
		Result = RebindPackagesInternal(InPackages, DependentModules, bWaitForCompletion, Ar);
	}
	RecordAnalyticsEvent(TEXT("Rebind"), Result, Duration, InPackages.Num(), DependentModules.Num());
}

ECompilationResult::Type FHotReloadModule::RebindPackagesInternal(TArray<UPackage*> InPackages, TArray<FName> DependentModules, const bool bWaitForCompletion, FOutputDevice &Ar)
{
	ECompilationResult::Type Result = ECompilationResult::Unsupported;
#if !IS_MONOLITHIC
	bool bCanRebind = InPackages.Num() > 0;

	// Verify that we're going to be able to rebind the specified packages
	if (bCanRebind)
	{
		for (UPackage* Package : InPackages)
		{
			check(Package);

			if (Package->GetOuter() != NULL)
			{
				Ar.Logf(ELogVerbosity::Warning, TEXT("Could not rebind package for %s, package is either not bound yet or is not a DLL."), *Package->GetName());
				bCanRebind = false;
				break;
			}
		}
	}

	// We can only proceed if a compile isn't already in progress
	if (FModuleManager::Get().IsCurrentlyCompiling())
	{
		Ar.Logf(ELogVerbosity::Warning, TEXT("Could not rebind package because a module compile is already in progress."));
		bCanRebind = false;
	}

	if (bCanRebind)
	{
		bIsHotReloadingFromEditor = true;

		const double StartTime = FPlatformTime::Seconds();

		TArray< FName > ModuleNames;
		for (UPackage* Package : InPackages)
		{
			// Attempt to recompile this package's module
			FName ShortPackageName = FPackageName::GetShortFName(Package->GetFName());
			ModuleNames.Add(ShortPackageName);
		}

		// Add dependent modules.
		ModuleNames.Append(DependentModules);

		// Start compiling modules
		const bool bCompileStarted = FModuleManager::Get().RecompileModulesAsync(
			ModuleNames,
			FModuleManager::FRecompileModulesCallback::CreateRaw< FHotReloadModule, TArray<UPackage*>, TArray<FName>, FOutputDevice& >(this, &FHotReloadModule::DoHotReloadCallback, InPackages, DependentModules, Ar),
			bWaitForCompletion,
			Ar);

		if (bCompileStarted)
		{
			if (bWaitForCompletion)
			{
				Ar.Logf(ELogVerbosity::Warning, TEXT("HotReload operation took %4.1fs."), float(FPlatformTime::Seconds() - StartTime));
			}
			else
			{
				Ar.Logf(ELogVerbosity::Warning, TEXT("Starting HotReload took %4.1fs."), float(FPlatformTime::Seconds() - StartTime));
			}
			Result = ECompilationResult::Succeeded;
		}
		else
		{
			Ar.Logf(ELogVerbosity::Warning, TEXT("RebindPackages failed because the compiler could not be started."));
			Result = ECompilationResult::OtherCompilationError;
		}
	}
	else
#endif
	{
		Ar.Logf(ELogVerbosity::Warning, TEXT("RebindPackages not possible for specified packages (or application was compiled in monolithic mode.)"));
	}
	return Result;
}

void FHotReloadModule::ReinstanceClass(UClass* OldClass, UClass* NewClass)
{
	UE_LOG(LogHotReload, Log, TEXT("Re-instancing %s after hot-reload."), *NewClass->GetName());
	FBlueprintCompileReinstancer ReinstanceHelper(NewClass, OldClass);
	ReinstanceHelper.ReinstanceObjects();
}

void FHotReloadModule::GetGameModules(TArray<FString>& OutGameModules)
{
	// Ask the module manager for a list of currently-loaded gameplay modules
	TArray< FModuleStatus > ModuleStatuses;
	FModuleManager::Get().QueryModules(ModuleStatuses);

	for (TArray< FModuleStatus >::TConstIterator ModuleStatusIt(ModuleStatuses); ModuleStatusIt; ++ModuleStatusIt)
	{
		const FModuleStatus& ModuleStatus = *ModuleStatusIt;

		// We only care about game modules that are currently loaded
		if (ModuleStatus.bIsLoaded && ModuleStatus.bIsGameModule)
		{
			OutGameModules.AddUnique(ModuleStatus.Name);
		}
	}
}

void FHotReloadModule::OnHotReloadBinariesChanged(const TArray<struct FFileChangeData>& FileChanges)
{
	if (bIsHotReloadingFromEditor)
	{
		// DO NOTHING, this case is handled by RebindPackages
		return;
	}

	TArray< FString > GameModuleNames;
	GetGameModules(GameModuleNames);

	if (GameModuleNames.Num() > 0)
	{
		// Check if any of the game DLLs has been added
		for (auto& Change : FileChanges)
		{
			if (Change.Action == FFileChangeData::FCA_Added)
			{
				const FString Filename = FPaths::GetCleanFilename(Change.Filename);
				if (Filename.EndsWith(FPlatformProcess::GetModuleExtension()))
				{
					for (auto& GameModule : GameModuleNames)
					{
						if (Filename.Contains(GameModule) && !NewModules.ContainsByPredicate([&](const FRecompiledModule& Module){ return Module.Name == GameModule; }))
						{
							// Add to queue. We do not hot-reload here as there may potentially be other modules being compiled.
							NewModules.Add(FRecompiledModule(GameModule, Change.Filename));
							UE_LOG(LogHotReload, Log, TEXT("New module detected: %s"), *Filename);
						}
					}
				}
			}
		}
	}
}

void FHotReloadModule::InitHotReloadWatcher()
{
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();
	if (DirectoryWatcher)
	{
		// Watch the game binaries folder for new files
		FString BinariesPath = FPaths::ConvertRelativePathToFull(FPaths::GameDir() / TEXT("Binaries") / FPlatformProcess::GetBinariesSubdirectory());
		BinariesFolderChangedDelegate = IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FHotReloadModule::OnHotReloadBinariesChanged);
		DirectoryWatcher->RegisterDirectoryChangedCallback(BinariesPath, BinariesFolderChangedDelegate);
	}
}

void FHotReloadModule::ShutdownHotReloadWatcher()
{
	FDirectoryWatcherModule* DirectoryWatcherModule = FModuleManager::LoadModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	if( DirectoryWatcherModule != nullptr )
	{
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule->Get();
		if (DirectoryWatcher)
		{
			FString BinariesPath = FPaths::ConvertRelativePathToFull(FPaths::GameDir() / TEXT("Binaries") / FPlatformProcess::GetBinariesSubdirectory());
			DirectoryWatcher->UnregisterDirectoryChangedCallback(BinariesPath, BinariesFolderChangedDelegate);
		}
	}
}

bool FHotReloadModule::Tick(float DeltaTime)
{
	if (NewModules.Num())
	{
		// We have new modules in the queue, but make sure UBT has finished compiling all of them
		if (!FPlatformProcess::IsApplicationRunning(TEXT("UnrealBuildTool")))
		{
			DoHotReloadFromIDE();
			NewModules.Empty();
		}
		else
		{
			UE_LOG(LogHotReload, Verbose, TEXT("Detected %d reloaded modules but UnrealBuildTool is still running"), NewModules.Num());
		}
	}
	return true;
}

void FHotReloadModule::GetPackagesToRebindAndDependentModules(const TArray<FString>& InGameModuleNames, TArray<UPackage*>& OutPackagesToRebind, TArray<FName>& OutDependentModules)
{
	for (auto& GameModuleName : InGameModuleNames)
	{
		FString PackagePath(FString(TEXT("/Script/")) + GameModuleName);
		UPackage* Package = FindPackage(NULL, *PackagePath);
		if (Package != NULL)
		{
			OutPackagesToRebind.Add(Package);
		}
		else
		{
			OutDependentModules.Add(*GameModuleName);
		}
	}
}

void FHotReloadModule::DoHotReloadFromIDE()
{
	TArray<FString> GameModuleNames;
	TArray<UPackage*> PackagesToRebind;
	TArray<FName> DependentModules;
	double Duration = 0.0;
	ECompilationResult::Type Result = ECompilationResult::Unsupported;

	GetGameModules(GameModuleNames);

	if (GameModuleNames.Num() > 0)
	{		
		FScopedDurationTimer Timer(Duration);

		UE_LOG(LogHotReload, Log, TEXT("Starting Hot-Reload from IDE"));

		GWarn->BeginSlowTask(LOCTEXT("CompilingGameCode", "Compiling Game Code"), true);

		// Update compile data before we start compiling
		for (auto& NewModule : NewModules)
		{
			FModuleManager::Get().UpdateModuleCompileData(*NewModule.Name);
			FModuleManager::Get().OnModuleCompileSucceeded(*NewModule.Name, NewModule.NewFilename);
		}


		GetPackagesToRebindAndDependentModules(GameModuleNames, PackagesToRebind, DependentModules);

		check(PackagesToRebind.Num() || DependentModules.Num())
		{
			const bool bRecompileFinished = true;
			const bool bRecompileSucceeded = true;
			Result = DoHotReloadInternal(bRecompileFinished, bRecompileSucceeded, PackagesToRebind, DependentModules, *GLog);
		}

		GWarn->EndSlowTask();
	}

	RecordAnalyticsEvent(TEXT("IDE"), Result, Duration, PackagesToRebind.Num(), DependentModules.Num());
}

void FHotReloadModule::RecordAnalyticsEvent(const TCHAR* ReloadFrom, ECompilationResult::Type Result, double Duration, int32 PackageCount, int32 DependentModulesCount)
{
	if (FEngineAnalytics::IsAvailable())
	{
		TArray< FAnalyticsEventAttribute > ReloadAttribs;
		ReloadAttribs.Add(FAnalyticsEventAttribute(TEXT("ReloadFrom"), ReloadFrom));
		ReloadAttribs.Add(FAnalyticsEventAttribute(TEXT("Result"), ECompilationResult::ToString(Result)));
		ReloadAttribs.Add(FAnalyticsEventAttribute(TEXT("Duration"), FString::Printf(TEXT("%.4lf"), Duration)));
		ReloadAttribs.Add(FAnalyticsEventAttribute(TEXT("Packages"), FString::Printf(TEXT("%d"), PackageCount)));
		ReloadAttribs.Add(FAnalyticsEventAttribute(TEXT("DependentModules"), FString::Printf(TEXT("%d"), DependentModulesCount)));
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.HotReload"), ReloadAttribs);
	}
}

#undef LOCTEXT_NAMESPACE