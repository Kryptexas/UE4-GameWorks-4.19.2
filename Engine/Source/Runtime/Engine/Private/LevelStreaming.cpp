// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "LevelUtils.h"
#if WITH_EDITOR
	#include "Slate.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogLevelStreaming, Log, All);

#define LOCTEXT_NAMESPACE "World"

FStreamLevelAction::FStreamLevelAction(bool bIsLoading, const FName & InLevelName, bool bIsMakeVisibleAfterLoad, bool bIsShouldBlockOnLoad, const FLatentActionInfo& InLatentInfo, UWorld* World)
	: bLoading(bIsLoading)
	, bMakeVisibleAfterLoad(bIsMakeVisibleAfterLoad)
	, bShouldBlockOnLoad(bIsShouldBlockOnLoad)
	, LevelName(InLevelName)
	, LatentInfo(InLatentInfo)
{
	Level = FindAndCacheLevelStreamingObject( LevelName, World );
	ActivateLevel( Level );
}

void FStreamLevelAction::UpdateOperation(FLatentResponse& Response)
{
	ULevelStreaming* LevelStreamingObject = Level; // to avoid confusion.
	bool bIsOperationFinished = UpdateLevel( LevelStreamingObject );
	Response.FinishAndTriggerIf(bIsOperationFinished, LatentInfo.ExecutionFunction, LatentInfo.Linkage, LatentInfo.CallbackTarget);
}

#if WITH_EDITOR
FString FStreamLevelAction::GetDescription() const
{
	return FString::Printf(TEXT("Streaming Level in progress...(%s)"), *LevelName.ToString());
}
#endif

/**
* Helper function to potentially find a level streaming object by name
*
* @param	LevelName							Name of level to search streaming object for in case Level is NULL
* @return	level streaming object or NULL if none was found
*/
ULevelStreaming* FStreamLevelAction::FindAndCacheLevelStreamingObject( const FName LevelName, UWorld* InWorld )
{
	// Search for the level object by name.
	if( LevelName != NAME_None )
	{
		const FString SearchShortName = MakeSafeShortLevelName( LevelName, InWorld );

		// Iterate over all streaming level objects in world info to find one with matching name.
		for( int32 LevelIndex=0; LevelIndex < InWorld->StreamingLevels.Num(); LevelIndex++ )
		{
			ULevelStreaming* LevelStreamingObject = InWorld->StreamingLevels[LevelIndex];
			if( LevelStreamingObject
				&& FPackageName::GetShortName(LevelStreamingObject->PackageName) == SearchShortName)
			{
				// If we have an exact match go ahead and return it
				return LevelStreamingObject;
			}
		}
	}

	return NULL;
}

/**
 * Given a level name, returns a shoftlevel name that will work with Play on Editor or Play on Console
 *
 * @param	InLevelName		Raw level name (no UEDPIE or UED<console> prefix)
 * @param	InWorld			World in which to check for other instances of the name
 */
FString FStreamLevelAction::MakeSafeShortLevelName( const FName& InLevelName, UWorld* InWorld )
{
	// Special case for PIE, the PackageName gets mangled.
	return InWorld->StreamingLevelsPrefix + FPackageName::GetShortName(InLevelName);
}
/**
* Handles "Activated" for single ULevelStreaming object.
*
* @param	LevelStreamingObject	LevelStreaming object to handle "Activated" for.
*/
void FStreamLevelAction::ActivateLevel( ULevelStreaming* LevelStreamingObject )
{	
	if( LevelStreamingObject != NULL )
	{
		// Loading.
		if( bLoading )
		{
			UE_LOG(LogStreaming, Log, TEXT("Streaming in level %s (%s)..."),*LevelStreamingObject->GetName(),*LevelStreamingObject->PackageName.ToString());
			LevelStreamingObject->bShouldBeLoaded		= true;
			LevelStreamingObject->bShouldBeVisible		|= bMakeVisibleAfterLoad;
			LevelStreamingObject->bShouldBlockOnLoad	= bShouldBlockOnLoad;
		}
		// Unloading.
		else 
		{
			UE_LOG(LogStreaming, Log, TEXT("Streaming out level %s (%s)..."),*LevelStreamingObject->GetName(),*LevelStreamingObject->PackageName.ToString());
			LevelStreamingObject->bShouldBeLoaded		= false;
			LevelStreamingObject->bShouldBeVisible		= false;
		}

		UWorld* LevelWorld = CastChecked<UWorld>(LevelStreamingObject->GetOuter());
		// If we have a valid world
		if(LevelWorld)
		{
			// Notify players of the change
			for( FConstPlayerControllerIterator Iterator = LevelWorld->GetPlayerControllerIterator(); Iterator; ++Iterator )
			{
				APlayerController* PlayerController = *Iterator;

				UE_LOG(LogLevel, Log, TEXT("ActivateLevel %s %i %i %i"), 
					*LevelStreamingObject->PackageName.ToString(), 
					LevelStreamingObject->bShouldBeLoaded, 
					LevelStreamingObject->bShouldBeVisible, 
					LevelStreamingObject->bShouldBlockOnLoad );



				PlayerController->LevelStreamingStatusChanged( 
					LevelStreamingObject, 
					LevelStreamingObject->bShouldBeLoaded, 
					LevelStreamingObject->bShouldBeVisible,
					LevelStreamingObject->bShouldBlockOnLoad, 
					INDEX_NONE);

			}
		}
	}
	else
	{
		UE_LOG(LogLevel, Warning, TEXT("Failed to find streaming level object associated with '%s'"), *LevelName.ToString() );
	}
}

/**
* Handles "UpdateOp" for single ULevelStreaming object.
*
* @param	LevelStreamingObject	LevelStreaming object to handle "UpdateOp" for.
*
* @return true if operation has completed, false if still in progress
*/
bool FStreamLevelAction::UpdateLevel( ULevelStreaming* LevelStreamingObject )
{
	// No level streaming object associated with this sequence.
	if( LevelStreamingObject == NULL )
	{
		return true;
	}
	// Level is neither loaded nor should it be so we finished (in the sense that we have a pending GC request) unloading.
	else if( (LevelStreamingObject->GetLoadedLevel() == NULL) && !LevelStreamingObject->bShouldBeLoaded )
	{
		return true;
	}
	// Level shouldn't be loaded but is as background level streaming is enabled so we need to fire finished event regardless.
	else if( LevelStreamingObject->GetLoadedLevel() && !LevelStreamingObject->bShouldBeLoaded && !GEngine->bUseBackgroundLevelStreaming )
	{
		return true;
	}
	// Level is both loaded and wanted so we finished loading.
	else if(	LevelStreamingObject->GetLoadedLevel() && LevelStreamingObject->bShouldBeLoaded 
	// Make sure we are visible if we are required to be so.
	&&	(!bMakeVisibleAfterLoad || LevelStreamingObject->GetLoadedLevel()->bIsVisible) )
	{
		return true;
	}

	// Loading/ unloading in progress.
	return false;
}

/*-----------------------------------------------------------------------------
	ULevelStreaming* implementation.
-----------------------------------------------------------------------------*/

void ULevelStreaming::PostLoad()
{
	Super::PostLoad();

	const bool PIESession = GetWorld()->WorldType == EWorldType::PIE || (GetOutermost()->PackageFlags & PKG_PlayInEditor) != 0;

	// If this streaming level was saved with a short package name, try to convert it to a long package name
	if ( !PIESession && PackageName != NAME_None )
	{
		if ( FPackageName::IsShortPackageName(PackageName) == false )
		{
			if ( FPackageName::DoesPackageExist(PackageName.ToString()) == false )
			{
				UE_LOG(LogLevelStreaming, Display, TEXT("Failed to find streaming level package file: %s. This streaming level may not load or save properly."), *PackageName.ToString());
#if WITH_EDITOR
				if (GIsEditor)
				{
					// Launch notification to inform user of default change
					FFormatNamedArguments Args;
					Args.Add( TEXT("PackageName"), FText::FromName( PackageName ) );
					FNotificationInfo Info( FText::Format(LOCTEXT("LevelStreamingFailToStreamLevel","Failed to find streamed level {PackageName}, please fix the reference to it in the Level Browser"), Args ) );
					Info.ExpireDuration = 7.0f;

					FSlateNotificationManager::Get().AddNotification(Info);
				}
#endif // WITH_EDITOR
			}
		}
		else
		{
			UE_LOG(LogLevelStreaming, Display, TEXT("Invalid streaming level package name (%s). Only long package names are supported. This streaming level may not load or save properly."), *PackageName.ToString());
		}
	}
}

UWorld* ULevelStreaming::GetWorld() const
{
	// Fail gracefully if a CDO
	if(IsTemplate())
	{
		return NULL;
	}
	// Otherwise 
	else
	{
		return CastChecked<UWorld>(GetOuter());
	}
}

void ULevelStreaming::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);
	
	if (Ar.IsLoading())
	{
#if WITH_EDITOR
		if ((GetOutermost()->PackageFlags & PKG_PlayInEditor) != 0 && GetOutermost()->PIEInstanceID != INDEX_NONE)
		{
			RenameForPIE(GetOutermost()->PIEInstanceID);
		}
#endif
	}
}

int32 ULevelStreaming::GetLODIndex(UWorld* PersistentWorld) const
{
	check(PersistentWorld);
	auto Entry = PersistentWorld->StreamingLevelsLOD.Find(PackageName);
	return Entry ? *Entry : INDEX_NONE;
}

void ULevelStreaming::SetLODIndex(UWorld* PersistentWorld, int32 LODIndex)
{
	check(PersistentWorld);
	PersistentWorld->StreamingLevelsLOD.Add(PackageName, LODIndex);
}

FName ULevelStreaming::GetLODPackageName(UWorld* PersistentWorld) const
{
	int32 LODCurrentIdx = GetLODIndex(PersistentWorld);
	return LODPackageNames.IsValidIndex(LODCurrentIdx) ? LODPackageNames[LODCurrentIdx] : PackageName;
}

FName ULevelStreaming::GetLODPackageNameToLoad(UWorld* PersistentWorld) const
{
	int32 LODCurrentIdx = GetLODIndex(PersistentWorld);

	if (LODPackageNames.IsValidIndex(LODCurrentIdx))
	{
		return LODPackageNamesToLoad.IsValidIndex(LODCurrentIdx) ? LODPackageNamesToLoad[LODCurrentIdx] : NAME_None;
	}
	else
	{
		return PackageNameToLoad;
	}
}

void ULevelStreaming::DiscardPendingUnloadLevel(UWorld* PersistentWorld)
{
	if (PendingUnloadLevel)
	{
		if (PendingUnloadLevel->bIsVisible)
		{
			PersistentWorld->RemoveFromWorld(PendingUnloadLevel);
		}

		if (!PendingUnloadLevel->bIsVisible)
		{
			SetPendingUnloadLevel(NULL);
		}
	}
}

bool ULevelStreaming::RequestLevel(UWorld* PersistentWorld, bool bAllowLevelLoadRequests, bool bBlockOnLoad)
{
	// Quit early in case load request already issued
	if (bHasLoadRequestPending)
	{
		return true;
	}

	// Previous attempts have failed, no reason to try again
	if (bFailedToLoad)
	{
		return false;
	}
	
	// Package name we want to load
	FName DesiredPackageName = PersistentWorld->IsGameWorld() ? GetLODPackageName(PersistentWorld) : PackageName;
	FName DesiredPackageNameToLoad = PersistentWorld->IsGameWorld() ? GetLODPackageNameToLoad(PersistentWorld) : PackageNameToLoad;

	// Check if currently loaded level is what we want right now
	if (LoadedLevel != NULL && 
		LoadedLevel->GetOutermost()->GetFName() == DesiredPackageName)
	{
		return true;
	}

	// Can not load new level now, there is still level pending unload
	if (PendingUnloadLevel != NULL)
	{
		return false;
	}
		
	// Try to find the [to be] loaded package.
	UPackage* LevelPackage = (UPackage*) StaticFindObjectFast(UPackage::StaticClass(), NULL, DesiredPackageName, 0, 0, RF_PendingKill);

	// Package is already or still loaded.
	if (LevelPackage)
	{
		// Find world object and use its PersistentLevel pointer.
		UWorld* World = UWorld::FindWorldInPackage(LevelPackage);

		// Check for a redirector. Follow it, if found.
		if ( !World )
		{
			World = UWorld::FollowWorldRedirectorInPackage(LevelPackage);
			if ( World )
			{
				LevelPackage = World->GetOutermost();
			}
		}

		if (World != NULL)
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (World->PersistentLevel == NULL)
			{
				UE_LOG(LogLevelStreaming, Log, TEXT("World exists but PersistentLevel doesn't for %s, most likely caused by reference to world of unloaded level and GC setting reference to NULL while keeping world object"), *World->GetOutermost()->GetName());
				// print out some debug information...
				StaticExec(World, *FString::Printf(TEXT("OBJ REFS CLASS=WORLD NAME=%s shortest"), *World->GetPathName()));
				TMap<UObject*,UProperty*> Route = FArchiveTraceRoute::FindShortestRootPath( World, true, GARBAGE_COLLECTION_KEEPFLAGS );
				FString ErrorString = FArchiveTraceRoute::PrintRootPath( Route, World );
				UE_LOG(LogLevelStreaming, Log, TEXT("%s"), *ErrorString);
				// before asserting
				checkf(World->PersistentLevel,TEXT("Most likely caused by reference to world of unloaded level and GC setting reference to NULL while keeping world object"));
				return false;
			}
#endif
			SetLoadedLevel(World->PersistentLevel);
			
			// Broadcast level loaded event to blueprints
			OnLevelLoaded.Broadcast();
			
			return true;
		}
	}

	int32 PIEInstanceID = INDEX_NONE;

	// copy streaming level on demand if we are in PIE
	// (the world is already loaded for the editor, just find it and copy it)
	if ( PersistentWorld->IsPlayInEditor() )
	{
#if WITH_EDITOR
		PIEInstanceID = PersistentWorld->GetOutermost()->PIEInstanceID;
#endif
		const FString PrefixedLevelName = DesiredPackageName.ToString();
		const FString ShortPrefixedLevelName = FPackageName::GetLongPackageAssetName(PrefixedLevelName);
		const FString LevelPath = FPackageName::GetLongPackagePath(PrefixedLevelName);
		// Rebuild the original NonPrefixedLevelName so we can find and duplicate it
		const FString NonPrefixedLevelName = LevelPath + "/" + ShortPrefixedLevelName.RightChop(PersistentWorld->StreamingLevelsPrefix.Len());
					
		// Do the duplication
		UWorld* PIELevelWorld = UWorld::DuplicateWorldForPIE(NonPrefixedLevelName, PersistentWorld);
		if (PIELevelWorld)
		{
			PIELevelWorld->PersistentLevel->bAlreadyMovedActors = true; // As we have duplicated the world, the actors will already have been transformed
			check(PendingUnloadLevel == NULL);
			SetLoadedLevel(PIELevelWorld->PersistentLevel);

			// Broadcast level loaded event to blueprints
			OnLevelLoaded.Broadcast();

			return true;
		}
		else if (PersistentWorld->WorldComposition == NULL) // In world composition streaming levels are not loaded by default
		{
			if ( bAllowLevelLoadRequests )
			{
				UE_LOG(LogLevelStreaming, Log, TEXT("World to duplicate for PIE '%s' not found. Attempting load."), *NonPrefixedLevelName);
			}
			else
			{
				UE_LOG(LogLevelStreaming, Warning, TEXT("Unable to duplicate PIE World: '%s'"), *NonPrefixedLevelName);
				for (TObjectIterator<UWorld> It; It; ++It)
				{
					UWorld *W = *It;
					UE_LOG(LogLevelStreaming, Warning, TEXT("    Loaded World: %s"), *W->GetPathName() );
				}
			}
		}
	}

	// Async load package if world object couldn't be found and we are allowed to request a load.
	if (bAllowLevelLoadRequests)
	{
		FString PackageNameToLoadFrom = DesiredPackageName.ToString();
		if (DesiredPackageNameToLoad != NAME_None)
		{
			PackageNameToLoadFrom = DesiredPackageNameToLoad.ToString();
		}

		if (GUseSeekFreeLoading)
		{
			// Only load localized package if it exists as async package loading doesn't handle errors gracefully.
			FString LocalizedPackageName = PackageNameToLoadFrom + LOCALIZED_SEEKFREE_SUFFIX;
			FString LocalizedFileName;
			if (FPackageName::DoesPackageExist(LocalizedPackageName, NULL, &LocalizedFileName))
			{
				// Load localized part of level first in case it exists. We don't need to worry about GC or completion 
				// callback as we always kick off another async IO for the level below.
				LoadPackageAsync(*(PackageName.ToString() + LOCALIZED_SEEKFREE_SUFFIX), NULL, NAME_None, *LocalizedPackageName)
					.SetPIEInstanceID(PIEInstanceID);
			}
		}

		if (FPackageName::DoesPackageExist(PackageNameToLoadFrom, NULL, NULL))
		{
			bHasLoadRequestPending = true;
			
			ULevel::StreamedLevelsOwningWorld.Add(DesiredPackageName, PersistentWorld);
			UWorld::WorldTypePreLoadMap.FindOrAdd(DesiredPackageName) = PersistentWorld->WorldType;

			// Kick off async load request.
			LoadPackageAsync(*DesiredPackageName.ToString(), 
				FLoadPackageAsyncDelegate::CreateUObject(this, &ULevelStreaming::AsyncLevelLoadComplete), 
				NULL, NAME_None, *PackageNameToLoadFrom
				).SetPIEInstanceID(PIEInstanceID);

			// Are we sublevel of a persistent level or a sublevel of a streamed-in level?
			const bool bInnerLevel = PersistentWorld != GetOuter(); 

			// streamingServer: server loads everything?
			// Editor immediately blocks on load and we also block if background level streaming is disabled.
			if (bBlockOnLoad || (!bInnerLevel && ShouldBeAlwaysLoaded()))
			{
				// Finish all async loading.
				FlushAsyncLoading( NAME_None );
			}
		}
		else
		{
			UE_LOG(LogStreaming, Error,TEXT("Couldn't find file for package %s."), *PackageNameToLoadFrom);
			bFailedToLoad = true;
			return false;
		}
	}

	return true;
}

void ULevelStreaming::AsyncLevelLoadComplete( const FString& InPackageName, UPackage* InLoadedPackage ) 
{
	bHasLoadRequestPending = false;
	
	if( InLoadedPackage )
	{
		UPackage* LevelPackage = InLoadedPackage;
		
		// Try to find a UWorld object in the level package.
		UWorld* World = UWorld::FindWorldInPackage(LevelPackage);

		if ( World )
		{
			ULevel* Level = World->PersistentLevel;
			if( Level )
			{
				check(PendingUnloadLevel == NULL);
				SetLoadedLevel(Level);

				// Broadcast level loaded event to blueprints
				OnLevelLoaded.Broadcast();

				// Make sure this level will start to render only when it will be fully added to the world
				if (LODPackageNames.Num() > 0)
				{
					Level->bRequireFullVisibilityToRender = true;
				}
			
				// In the editor levels must be in the levels array regardless of whether they are visible or not
				if (ensure(Level->OwningWorld) && Level->OwningWorld->WorldType == EWorldType::Editor)
				{
					Level->OwningWorld->AddLevel(Level);
#if WITH_EDITOR
					// We should also at this point, apply the level's editor transform
					if (!Level->bAlreadyMovedActors)
					{
						FLevelUtils::ApplyEditorTransform(this, false);
						Level->bAlreadyMovedActors = true;
					}
#endif // WITH_EDITOR
				}
			}
			else
			{
				UE_LOG(LogLevelStreaming, Warning, TEXT("Couldn't find ULevel object in package '%s'"), *InPackageName );
			}
		}
		else
		{
			// There could have been a redirector in the package. Attempt to follow it.
			UObjectRedirector* WorldRedirector = nullptr;
			UWorld* DestinationWorld = UWorld::FollowWorldRedirectorInPackage(LevelPackage, &WorldRedirector);
			if (DestinationWorld)
			{
				// To follow the world redirector for level streaming...
				// 1) Update all globals that refer to the redirector package by name
				// 2) Update the PackageNameToLoad to refer to the new package location
				// 3) If the package name to load was the same as the destination package name...
				//         ... update the package name to the new package and let the next RequestLevel try this process again.
				//    If the package name to load was different...
				//         ... it means the specified package name was explicit and we will just load from another file.

				FName OldDesiredPackageName = FName(*InPackageName);
				UWorld** OwningWorldPtr = ULevel::StreamedLevelsOwningWorld.Find(OldDesiredPackageName);
				UWorld* OwningWorld = OwningWorldPtr ? *OwningWorldPtr : NULL;
				ULevel::StreamedLevelsOwningWorld.Remove(OldDesiredPackageName);

				// Try again with the destination package to load.
				// IMPORTANT: check this BEFORE changing PackageNameToLoad, otherwise you wont know if the package name was supposed to be different.
				const bool bLoadingIntoDifferentPackage = (PackageName != PackageNameToLoad) && (PackageNameToLoad != NAME_None);

				// ... now set PackageNameToLoad
				PackageNameToLoad = DestinationWorld->GetOutermost()->GetFName();

				if ( PackageNameToLoad != OldDesiredPackageName )
				{
					EWorldType::Type* OldPackageWorldType = UWorld::WorldTypePreLoadMap.Find(OldDesiredPackageName);
					if ( OldPackageWorldType )
					{
						UWorld::WorldTypePreLoadMap.FindOrAdd(PackageNameToLoad) = *OldPackageWorldType;
						UWorld::WorldTypePreLoadMap.Remove(OldDesiredPackageName);
					}
				}

				// Now determine if we are loading into the package explicitly or if it is okay to just load the other package.
				if ( bLoadingIntoDifferentPackage )
				{
					// Loading into a new custom package explicitly. Load the destination world directly into the package.
					// Detach the linker to load from a new file into the same package.
					ULinkerLoad* PackageLinker = ULinkerLoad::FindExistingLinkerForPackage(LevelPackage);
					if (PackageLinker)
					{
						PackageLinker->Detach(false);
					}

					// Make sure the redirector is not in the way of the new world.
					if (WorldRedirector->GetFName() == DestinationWorld->GetFName())
					{
						// Pass NULL as the name to make a new unique name and NULL for the outer to not change the outer.
						WorldRedirector->Rename(NULL, NULL, REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders | REN_NonTransactional);
					}
				}
				else
				{
					// Loading the requested package normally. Fix up the destination world then update the requested package to the destination.
					if (OwningWorld)
					{
						if (DestinationWorld->PersistentLevel)
						{
							DestinationWorld->PersistentLevel->OwningWorld = OwningWorld;
						}

						// In some cases, BSP render data is not created because the OwningWorld was not set correctly.
						// Regenerate that render data here
						DestinationWorld->PersistentLevel->InvalidateModelSurface();
						DestinationWorld->PersistentLevel->CommitModelSurfaces();
					}
						
					PackageName = PackageNameToLoad;
				}
			}
		}
	}
	else
	{
		UE_LOG(LogLevelStreaming, Warning, TEXT("Failed to load package '%s'"), *InPackageName );
	}
}

bool ULevelStreaming::IsLevelVisible() const
{
	return LoadedLevel != NULL && LoadedLevel->bIsVisible;
}

bool ULevelStreaming::IsLevelLoaded() const
{
	return LoadedLevel != NULL;
}

ULevelStreaming* ULevelStreaming::CreateInstance(FString InstanceUniqueName)
{
	ULevelStreaming* StreamingLevelInstance = nullptr;
	
	UWorld* InWorld = GetWorld();
	if (InWorld)
	{
		// Create instance long package name 
		FString InstanceShortPackageName = InWorld->StreamingLevelsPrefix + FPackageName::GetShortName(InstanceUniqueName);
		FString InstancePackagePath = FPackageName::GetLongPackagePath(PackageName.ToString()) + TEXT("/");
		FName	InstanceUniquePackageName = FName(*(InstancePackagePath + InstanceShortPackageName));

		// check if instance name is unique among existing streaming level objects
		const bool bUniqueName = (InWorld->StreamingLevels.FindMatch(ULevelStreaming::FPackageNameMatcher(InstanceUniquePackageName)) == INDEX_NONE);
				
		if (bUniqueName)
		{
			StreamingLevelInstance = Cast<ULevelStreaming>(StaticConstructObject(GetClass(), InWorld, NAME_None, RF_Transient, NULL));
			// new level streaming instance will load the same map package as this object
			StreamingLevelInstance->PackageNameToLoad = (PackageNameToLoad == NAME_None ? PackageName : PackageNameToLoad);
			// under a provided unique name
			StreamingLevelInstance->PackageName = InstanceUniquePackageName;
			StreamingLevelInstance->bShouldBeLoaded = false;
			StreamingLevelInstance->bShouldBeVisible = false;
			StreamingLevelInstance->LevelTransform = LevelTransform;

			// add a new instance to streaming level list
			InWorld->StreamingLevels.Add(StreamingLevelInstance);
		}
		else
		{
			UE_LOG(LogStreaming, Warning, TEXT("Provided streaming level instance name is not unique: %s"), *InstanceUniquePackageName.ToString());
		}
	}
	
	return StreamingLevelInstance;
}

void ULevelStreaming::BroadcastLevelLoadedStatus(UWorld* PersistentWorld, FName LevelPackageName, bool bLoaded)
{
	for (auto It = PersistentWorld->StreamingLevels.CreateIterator(); It; ++It)
	{
		if ((*It)->PackageName == LevelPackageName)
		{
			if (bLoaded)
			{
				(*It)->OnLevelLoaded.Broadcast();
			}
			else
			{
				(*It)->OnLevelUnloaded.Broadcast();
			}
		}
	}

	// traverse inner worlds as well, skip persistent level
	for (int32 LevelIdx = 1; LevelIdx < PersistentWorld->GetNumLevels(); ++LevelIdx)
	{
		UWorld* InnerWorld = Cast<UWorld>(PersistentWorld->GetLevel(LevelIdx)->GetOuter());
		BroadcastLevelLoadedStatus(InnerWorld, LevelPackageName, bLoaded);
	}
}
	
void ULevelStreaming::BroadcastLevelVisibleStatus(UWorld* PersistentWorld, FName LevelPackageName, bool bVisible)
{
	for (auto It = PersistentWorld->StreamingLevels.CreateIterator(); It; ++It)
	{
		if ((*It)->PackageName == LevelPackageName)
		{
			if (bVisible)
			{
				(*It)->OnLevelShown.Broadcast();
			}
			else
			{
				(*It)->OnLevelHidden.Broadcast();
			}
		}
	}

	// traverse inner worlds as well, skip persistent level
	for (int32 LevelIdx = 1; LevelIdx < PersistentWorld->GetNumLevels(); ++LevelIdx)
	{
		UWorld* InnerWorld = Cast<UWorld>(PersistentWorld->GetLevel(LevelIdx)->GetOuter());
		BroadcastLevelVisibleStatus(InnerWorld, LevelPackageName, bVisible);
	}
}

void ULevelStreaming::RenameForPIE(int32 PIEInstanceID)
{
	// Apply PIE prefix so this level references
	if (PackageName != NAME_None)
	{
		// Store original name 
		if (PackageNameToLoad == NAME_None)
		{
			PackageNameToLoad = PackageName;
		}
		const FString PlayWorldSteamingLevelName = UWorld::ConvertToPIEPackageName(PackageName.ToString(), PIEInstanceID);
		PackageName = FName(*PlayWorldSteamingLevelName);
	}
	
	// Rename LOD levels if any
	if (LODPackageNames.Num() > 0)
	{
		LODPackageNamesToLoad.Empty();
		for (auto It = LODPackageNames.CreateIterator(); It; ++It)
		{
			// Store LOD level original package name
			LODPackageNamesToLoad.Add(*It); 
			// Apply PIE prefix to package name
			*It = FName(*UWorld::ConvertToPIEPackageName((*It).ToString(), PIEInstanceID)); 
		}
	}
}

bool ULevelStreaming::ShouldBeLoaded( const FVector& ViewLocation )
{
	return true;
}

bool ULevelStreaming::ShouldBeVisible( const FVector& ViewLocation )
{
	if( GetWorld()->IsGameWorld() )
	{
		// Game and play in editor viewport codepath.
		return bShouldBeVisible && ShouldBeLoaded( ViewLocation );
	}
	else
	{
		// Editor viewport codepath.
		return bShouldBeVisibleInEditor;
	}
}

FBox ULevelStreaming::GetStreamingVolumeBounds()
{
	FBox Bounds(0);

	// Iterate over each volume associated with this LevelStreaming object
	for(int32 VolIdx=0; VolIdx<EditorStreamingVolumes.Num(); VolIdx++)
	{
		ALevelStreamingVolume* StreamingVol = EditorStreamingVolumes[VolIdx];
		if(StreamingVol && StreamingVol->BrushComponent)
		{
			Bounds += StreamingVol->BrushComponent->BrushBodySetup->AggGeom.CalcAABB(StreamingVol->BrushComponent->ComponentToWorld);
		}
	}

	return Bounds;
}

#if WITH_EDITOR
void ULevelStreaming::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if ( PropertyChangedEvent.PropertyChain.Num() > 0 )
	{
		UProperty* OutermostProperty = PropertyChangedEvent.PropertyChain.GetHead()->GetValue();
		if ( OutermostProperty != NULL )
		{
			const FName PropertyName = OutermostProperty->GetFName();
			if ( PropertyName == TEXT("LevelTransform") )
			{
				GetWorld()->UpdateLevelStreaming();
			}

			if (PropertyName == GET_MEMBER_NAME_CHECKED(ULevelStreaming, EditorStreamingVolumes))
			{
				RemoveStreamingVolumeDuplicates();
			}

			else if( PropertyName == TEXT( "DrawColor" ) )
			{
				// Make sure the level's DrawColor change is applied immediately by reregistering the
				// components of the actor's in the level
				if( LoadedLevel != NULL )
				{
					UPackage* Package = LoadedLevel->GetOutermost();
					for( TObjectIterator<UActorComponent> It; It; ++It )
					{
						if( It->IsIn( Package ) )
						{
							UActorComponent* ActorComponent = Cast<UActorComponent>( *It );
							if( ActorComponent )
							{
								ActorComponent->RecreateRenderState_Concurrent();
							}
						}
					}
				}
			}
		}
	}

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void ULevelStreaming::RemoveStreamingVolumeDuplicates()
{
	for (int32 VolumeIdx = EditorStreamingVolumes.Num()-1; VolumeIdx >= 0; VolumeIdx--)
	{
		ALevelStreamingVolume* Volume = EditorStreamingVolumes[VolumeIdx];
		
		int32 DuplicateIdx = EditorStreamingVolumes.Find(Volume);
		check(DuplicateIdx != INDEX_NONE);
		if (DuplicateIdx != VolumeIdx)
		{
			EditorStreamingVolumes.RemoveAt(VolumeIdx);
		}
	}
}

#endif // WITH_EDITOR

ULevelStreaming::ULevelStreaming(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bShouldBeVisibleInEditor = true;
	DrawColor = FColor(255, 255, 255, 255);
	LevelTransform = FTransform::Identity;
	MinTimeBetweenVolumeUnloadRequests = 2.0f;
	bDrawOnLevelStatusMap = true;
}

#if WITH_EDITOR
void ULevelStreaming::PreEditUndo()
{
	FLevelUtils::RemoveEditorTransform(this, false);
}

void ULevelStreaming::PostEditUndo()
{
	FLevelUtils::ApplyEditorTransform(this, false);
}
#endif // WITH_EDITOR


/*-----------------------------------------------------------------------------
	ULevelStreamingPersistent implementation.
-----------------------------------------------------------------------------*/
ULevelStreamingPersistent::ULevelStreamingPersistent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/*-----------------------------------------------------------------------------
	ULevelStreamingKismet implementation.
-----------------------------------------------------------------------------*/
ULevelStreamingKismet::ULevelStreamingKismet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void ULevelStreamingKismet::PostLoad()
{
	Super::PostLoad();

	// Initialize startup state of the streaming level
	if ( GetWorld()->IsGameWorld() )
	{
		bShouldBeLoaded = bInitiallyLoaded;
		bShouldBeVisible = bInitiallyVisible;
	}
}

bool ULevelStreamingKismet::ShouldBeVisible( const FVector& ViewLocation )
{
	return bShouldBeVisible || (bShouldBeVisibleInEditor && !FApp::IsGame());
}

bool ULevelStreamingKismet::ShouldBeLoaded( const FVector& ViewLocation )
{
	return bShouldBeLoaded;
}

/*-----------------------------------------------------------------------------
	ULevelStreamingAlwaysLoaded implementation.
-----------------------------------------------------------------------------*/
ULevelStreamingAlwaysLoaded::ULevelStreamingAlwaysLoaded(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bShouldBeVisible = true;
}

bool ULevelStreamingAlwaysLoaded::ShouldBeLoaded( const FVector& ViewLocation )
{
	return true;
}

/*-----------------------------------------------------------------------------
	ULevelStreamingBounds implementation.
-----------------------------------------------------------------------------*/
ULevelStreamingBounds::ULevelStreamingBounds(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool ULevelStreamingBounds::ShouldBeVisible( const FVector& ViewLocation )
{
	bShouldBeVisible = ShouldBeLoaded(ViewLocation);
	return bShouldBeVisible;
}

bool ULevelStreamingBounds::ShouldBeLoaded( const FVector& ViewLocation )
{
	bShouldBeLoaded = false;
	
	UWorld* MyWorld = Cast<UWorld>(GetOuter());
	if (MyWorld && MyWorld->PersistentLevel->LevelBoundsActor.IsValid())
	{
		return MyWorld->PersistentLevel->LevelBoundsActor.Get()->GetComponentsBoundingBox().IsInside(ViewLocation);
	}
	
	return bShouldBeLoaded;
}

#undef LOCTEXT_NAMESPACE
