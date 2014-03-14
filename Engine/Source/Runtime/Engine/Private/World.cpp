// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	World.cpp: UWorld implementation
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "EngineLevelScriptClasses.h"
#include "EngineUserInterfaceClasses.h"
#include "ParticleDefinitions.h"
#include "Database.h"
#include "Net/NetworkProfiler.h"
#include "PrecomputedLightVolume.h"
#include "UObjectAnnotation.h"
#include "RenderCore.h"
#include "ParticleHelper.h"
#include "TickTaskManagerInterface.h"
#include "FXSystem.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"
#include "SoundDefinitions.h"
#include "VisualLog.h"
#include "LevelUtils.h"
#if WITH_EDITOR
	#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
	#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
	#include "Slate.h"
#endif

#include "MallocProfiler.h"

DEFINE_LOG_CATEGORY_STATIC(LogWorld, Log, All);
DEFINE_LOG_CATEGORY(LogSpawn);

#define LOCTEXT_NAMESPACE "World"

/*-----------------------------------------------------------------------------
	UWorld implementation.
-----------------------------------------------------------------------------*/

/** Global world pointer */
UWorldProxy GWorld;

TMap<FName, EWorldType::Type> UWorld::WorldTypePreLoadMap;

FWorldDelegates::FWorldInitializationEvent FWorldDelegates::OnPreWorldInitialization;
FWorldDelegates::FWorldInitializationEvent FWorldDelegates::OnPostWorldInitialization;
FWorldDelegates::FWorldCleanupEvent FWorldDelegates::OnWorldCleanup;
FWorldDelegates::FWorldEvent FWorldDelegates::OnPreWorldFinishDestroy;
FWorldDelegates::FOnLevelChanged FWorldDelegates::LevelAddedToWorld;
FWorldDelegates::FOnLevelChanged FWorldDelegates::LevelRemovedFromWorld;

UWorld::UWorld( const class FPostConstructInitializeProperties& PCIP )
:	UObject(PCIP)
,	TickTaskLevel(FTickTaskManagerInterface::Get().AllocateTickTaskLevel())
,	NextTravelType(TRAVEL_Relative)
{
	TimerManager = new FTimerManager();
#if WITH_EDITOR
	bBroadcastSelectionChange = true; //Ed Only
#endif // WITH_EDITOR
}


UWorld::UWorld( const class FPostConstructInitializeProperties& PCIP,const FURL& InURL )
:	UObject(PCIP)
,	URL(InURL)
,	TickTaskLevel(FTickTaskManagerInterface::Get().AllocateTickTaskLevel())
,	NextTravelType(TRAVEL_Relative)
{
	SetFlags( RF_Transactional );
	TimerManager = new FTimerManager();
#if WITH_EDITOR
	bBroadcastSelectionChange = true;
#endif // WITH_EDITOR
}


void UWorld::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	Ar << PersistentLevel;

	Ar << EditorViews[0];
	Ar << EditorViews[1];
	Ar << EditorViews[2];
	Ar << EditorViews[3];
	Ar << SaveGameSummary_DEPRECATED;


	if( !Ar.IsLoading() && !Ar.IsSaving() )
	{
		Ar << Levels;
		Ar << CurrentLevel;
		Ar << URL;

		Ar << NetDriver;
		
		Ar << LineBatcher;
		Ar << PersistentLineBatcher;
		Ar << ForegroundLineBatcher;

		Ar << MyParticleEventManager;
		Ar << MusicComp;
		Ar << GameState;
		Ar << AuthorityGameMode;
		Ar << NetworkManager;
#if WITH_EDITORONLY_DATA
		Ar << LandscapeInfoMap;
		//@todo.CONSOLE: How to handle w/out having a cooker/pakcager?
#endif	//#if WITH_EDITORONLY_DATA

		Ar << BehaviorTreeManager;
		Ar << EnvironmentQueryManager;
		Ar << NavigationSystem;
		Ar << AvoidanceManager;
	}

	Ar << ExtraReferencedObjects;

	// Do not save streaming levels of a persistent world in a world composition
	if (Ar.IsSaving() && WorldComposition)
	{
		TArray<ULevelStreaming*> EmptyStreaming;
		Ar << EmptyStreaming;
	}
	else
	{
		Ar << StreamingLevels;
	}
	
	if (Ar.UE4Ver() < VER_UE4_REMOVE_CLIENTDESTROYEDACTORCONTENT)
	{
		TArray<class UObject*>	TempClientDestroyedActorContent;
		Ar << TempClientDestroyedActorContent;
	}

	// Mark archive and package as containing a map if we're serializing to disk.
	if( !HasAnyFlags( RF_ClassDefaultObject ) && Ar.IsPersistent() )
	{
		Ar.ThisContainsMap();
		GetOutermost()->ThisContainsMap();
	}

	// Serialize World composition for PIE
	if (Ar.GetPortFlags() & PPF_DuplicateForPIE)
	{
		Ar << WorldComposition;
		Ar << GlobalOriginOffset;
		Ar << RequestedGlobalOriginOffset;
	}
	
	// UWorlds loaded/duplicated for PIE must lose RF_Public and RF_Standalone since they should not be referenced by objects in other packages and they should be GCed normally
	if (GetOutermost()->PackageFlags & PKG_PlayInEditor)
	{
		ClearFlags(RF_Public|RF_Standalone);
	}
}

void UWorld::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{	
	UWorld* This = CastChecked<UWorld>(InThis);
#if WITH_EDITOR
	if( GIsEditor )
	{
		Collector.AddReferencedObject( This->PersistentLevel, This );
		Collector.AddReferencedObject( This->SaveGameSummary_DEPRECATED, This );

		for( int32 Index = 0; Index < This->Levels.Num(); Index++ )
		{
			Collector.AddReferencedObject( This->Levels[Index], This );
		}

		Collector.AddReferencedObject( This->CurrentLevel, This );
		Collector.AddReferencedObject( This->NetDriver, This );
		Collector.AddReferencedObject( This->LineBatcher, This );
		Collector.AddReferencedObject( This->PersistentLineBatcher, This );
		Collector.AddReferencedObject( This->ForegroundLineBatcher, This );
		Collector.AddReferencedObject( This->MyParticleEventManager, This );
		Collector.AddReferencedObject( This->MusicComp, This );
		Collector.AddReferencedObject( This->GameState, This );
		Collector.AddReferencedObject( This->AuthorityGameMode, This );
		Collector.AddReferencedObject( This->NetworkManager, This );
		Collector.AddReferencedObject( This->BehaviorTreeManager, This );
		Collector.AddReferencedObject( This->EnvironmentQueryManager, This );
		Collector.AddReferencedObject( This->NavigationSystem, This );
		Collector.AddReferencedObject( This->AvoidanceManager, This );

#if WITH_EDITORONLY_DATA
		for( auto It = This->LandscapeInfoMap.CreateIterator(); It; ++It )
		{
			Collector.AddReferencedObject( It.Value(), This );
		}
		//@todo.CONSOLE: How to handle w/out having a cooker/pakcager?
#endif	//#if WITH_EDITORONLY_DATA

		for( int32 Index = 0; Index < This->ExtraReferencedObjects.Num(); Index++ )
		{
			Collector.AddReferencedObject( This->ExtraReferencedObjects[Index], This );
		}
		for( int32 Index = 0; Index < This->StreamingLevels.Num(); Index++ )
		{
			Collector.AddReferencedObject( This->StreamingLevels[Index], This );
		}
	}
#endif
	Super::AddReferencedObjects( This, Collector );
}

void UWorld::FinishDestroy()
{
	// Avoid cleanup if the world hasn't been initialized, e.g., the default object or a world object that got loaded
	// due to level streaming.
	if (bIsWorldInitialized)
	{
		FWorldDelegates::OnPreWorldFinishDestroy.Broadcast(this);

		// Wait for Async Trace data to finish and reset global variable
		WaitForAllAsyncTraceTasks();

		if (NavigationSystem != NULL)
		{
			// NavigationSystem should be already cleaned by now, after call 
			// in UWorld::CleanupWorld, but it never hurts to call it again
			NavigationSystem->CleanUp();
		}

		if (FXSystem)
		{
			FFXSystemInterface::Destroy( FXSystem );
			FXSystem = NULL;
		}

		if (PhysicsScene)
		{
			delete PhysicsScene;
			PhysicsScene = NULL;
		}

		if (Scene)
		{
			Scene->Release();
			Scene = NULL;
		}
	}

	// Clear GWorld pointer if it's pointing to this object.
	if( GWorld == this )
	{
		GWorld = NULL;
	}
	FTickTaskManagerInterface::Get().FreeTickTaskLevel(TickTaskLevel);
	TickTaskLevel = NULL;

	if (TimerManager)
	{
		delete TimerManager;
	}

	Super::FinishDestroy();
}

void UWorld::PostLoad()
{
	EWorldType::Type * PreLoadWorldType = UWorld::WorldTypePreLoadMap.Find(GetOuter()->GetFName());
	if (PreLoadWorldType)
	{
		WorldType = *PreLoadWorldType;
	}

	Super::PostLoad();
	CurrentLevel = PersistentLevel;

	// let's setup navigation system for presistent level (world) only
	const UWorld* MyOwningWorld = ULevel::StreamedLevelsOwningWorld.FindRef(GetOutermost()->GetFName());
	if (MyOwningWorld == NULL	)
	{
		UNavigationSystem* NewNavSys = UNavigationSystem::CreateNavigationSystem(this);
		if (NewNavSys != NULL)
		{
#if WITH_EDITOR
			if (!UNavigationSystem::GetIsNavigationAutoUpdateEnabled() && !IsPlayInEditor())
			{
				UNavigationSystem::SetNavigationAutoUpdateEnabled(false, NewNavSys);
			}
#endif
			SetNavigationSystem(NewNavSys);
		}
	}

	// copy StreamingLevels from WorldSettings to this if exists
	AWorldSettings * WorldSettings = GetWorldSettings(false, false);
	if ( WorldSettings && WorldSettings->StreamingLevels_DEPRECATED.Num() > 0 )
	{
		StreamingLevels = WorldSettings->StreamingLevels_DEPRECATED;
		WorldSettings->StreamingLevels_DEPRECATED.Empty();
	}

	// this is from AWorldSettings::PostLoad, where it rearrange order
	// Make sure that 'always loaded' maps are first in the array
	TArray<ULevelStreaming*> AlwaysLoadedLevels;
	// Iterate over each LevelStreaming object
	for( int32 LevelIndex=StreamingLevels.Num()-1; LevelIndex>=0; LevelIndex-- )
	{
		// See if its an 'always loaded' one
		ULevelStreamingAlwaysLoaded* AlwaysLoadedLevel = Cast<ULevelStreamingAlwaysLoaded>( StreamingLevels[LevelIndex] );
		if(AlwaysLoadedLevel)
		{
			// If it is, add to out list (preserving order), and remove from main list
			AlwaysLoadedLevels.Insert(AlwaysLoadedLevel, 0);
			StreamingLevels.RemoveAt(LevelIndex);
		}
	}

	// Now make new array that starts with 'always loaded' levels, followed by the rest
	TArray<ULevelStreaming*> NewStreamingLevels = AlwaysLoadedLevels;
	NewStreamingLevels.Append( StreamingLevels );
	// And use that for the new StreamingLevels array
	StreamingLevels = NewStreamingLevels;

	// Make sure that the persistent level isn't in this world's list of streaming levels.  This should
	// never really happen, but was needed in at least one observed case of corrupt map data.
	if( PersistentLevel != NULL )
	{
		for( int32 LevelIndex = 0; LevelIndex < StreamingLevels.Num(); ++LevelIndex )
		{
			ULevelStreaming* const StreamingLevel = StreamingLevels[ LevelIndex ];
			if( StreamingLevel != NULL )
			{
				if( StreamingLevel->PackageName == PersistentLevel->GetOutermost()->GetFName() || StreamingLevel->GetLoadedLevel() == PersistentLevel )
				{
					// Remove this streaming level
					StreamingLevels.RemoveAt( LevelIndex );
					MarkPackageDirty();
					--LevelIndex;
				}
			}
		}
	}

#if WITH_EDITORONLY_DATA
	if( GIsEditor && !VisibleLayers_DEPRECATED.IsEmpty() )
	{
		TArray< FString > OldVisibleLayerArray; 
		VisibleLayers_DEPRECATED.ParseIntoArray( &OldVisibleLayerArray, TEXT(","), 0 );

		for( auto VisibleLayerIt = OldVisibleLayerArray.CreateConstIterator(); VisibleLayerIt; ++VisibleLayerIt )
		{
			FName VisibleLayerName = FName( *( *VisibleLayerIt ) );

			if( VisibleLayerName == TEXT("None") )
			{
				continue;
			}

			bool bFoundExistingLayer = false;
			for( auto LayerIt = Layers.CreateConstIterator(); LayerIt; ++LayerIt )
			{
				TWeakObjectPtr< ULayer > Layer = *LayerIt;
				if( VisibleLayerName == Layer->LayerName )
				{
					bFoundExistingLayer = true;
					break;
				}
			}

			if( !bFoundExistingLayer )
			{
				ULayer* NewLayer = ConstructObject< ULayer >( ULayer::StaticClass(), this, NAME_None, RF_Transactional );
				check( NewLayer != NULL );

				NewLayer->LayerName = VisibleLayerName;
				NewLayer->bIsVisible = true;

				Layers.Add( NewLayer );
			}
		}

		VisibleLayers_DEPRECATED.Empty();
	}
#endif // WITH_EDITORONLY_DATA

	// Add the garbage collection callbacks
	FLevelStreamingGCHelper::AddGarbageCollectorCallback();

	// Initially set up the parameter collection list. This may be run again in UWorld::InitWorld.
	SetupParameterCollectionInstances();

	if ( GetLinkerUE4Version() < VER_UE4_WORLD_NAMED_AFTER_PACKAGE )
	{
		const FString ShortPackageName = FPackageName::GetLongPackageAssetName( GetOutermost()->GetName() );
		if ( GetName() != ShortPackageName )
		{
			Rename(*ShortPackageName, NULL, REN_NonTransactional | REN_ForceNoResetLoaders);
		}
	}

	if ( GetLinkerUE4Version() < VER_UE4_PUBLIC_WORLDS )
	{
		// Worlds are assets so they need RF_Public and RF_Standalone (for the editor)
		if (!(GetOutermost()->PackageFlags & PKG_PlayInEditor))
		{
			SetFlags(RF_Public|RF_Standalone);
		}
	}
}


bool UWorld::PreSaveRoot(const TCHAR* Filename, TArray<FString>& AdditionalPackagesToCook)
{
	// add any streaming sublevels to the list of extra packages to cook
	for (int32 LevelIndex = 0; LevelIndex < StreamingLevels.Num(); LevelIndex++)
	{
		ULevelStreaming* StreamingLevel = StreamingLevels[LevelIndex];
		if (StreamingLevel)
		{
			// Load package if found.
			FString PackageFilename;
			if (FPackageName::DoesPackageExist(StreamingLevel->PackageName.ToString(), NULL, &PackageFilename))
			{
				AdditionalPackagesToCook.Add(StreamingLevel->PackageName.ToString());
			}
		}
	}
#if WITH_EDITOR
	// If this level has a level script, rebuild it now to ensure no stale data is stored in the level script actor
	if( !IsRunningCommandlet() && PersistentLevel->GetLevelScriptBlueprint(true) )
	{
		FKismetEditorUtilities::CompileBlueprint(PersistentLevel->GetLevelScriptBlueprint(true), true, true);
	}
#endif

	// Update components and keep track off whether we need to clean them up afterwards.
	bool bCleanupIsRequired = false;
	if(!PersistentLevel->bAreComponentsCurrentlyRegistered)
	{
		PersistentLevel->UpdateLevelComponents(true);
		bCleanupIsRequired = true;
	}

	return bCleanupIsRequired;
}

void UWorld::PostSaveRoot( bool bCleanupIsRequired )
{
	if( bCleanupIsRequired )
	{
		PersistentLevel->ClearLevelComponents();
	}
}

UWorld* UWorld::GetWorld() const
{
	// Arg... rather hacky, but it seems conceptually ok because the object passed in should be able to fetch the
	// non-const world it's part of.  That's not a mutable action (normally) on the object, as we haven't changed
	// anything.
	return const_cast<UWorld*>(this);
}

void UWorld::SetupParameterCollectionInstances()
{
	// Create an instance for each parameter collection in memory
	for (TObjectIterator<UMaterialParameterCollection> It; It; ++It)
	{
		UMaterialParameterCollection* CurrentCollection = *It;
		AddParameterCollectionInstance(CurrentCollection, false);
	}

	UpdateParameterCollectionInstances(false);
}

void UWorld::AddParameterCollectionInstance(UMaterialParameterCollection* Collection, bool bUpdateScene)
{
	int32 ExistingIndex = INDEX_NONE;

	for (int32 InstanceIndex = 0; InstanceIndex < ParameterCollectionInstances.Num(); InstanceIndex++)
	{
		if (ParameterCollectionInstances[InstanceIndex]->GetCollection() == Collection)
		{
			ExistingIndex = InstanceIndex;
			break;
		}
	}

	UMaterialParameterCollectionInstance* NewInstance = (UMaterialParameterCollectionInstance*)StaticConstructObject(UMaterialParameterCollectionInstance::StaticClass());
	NewInstance->SetCollection(Collection, this);

	if (ExistingIndex != INDEX_NONE)
	{
		// Overwrite an existing instance
		ParameterCollectionInstances[ExistingIndex] = NewInstance;
	}
	else
	{
		// Add a new instance
		ParameterCollectionInstances.Add(NewInstance);
	}

	if (bUpdateScene)
	{
		// Update the scene's list of instances, needs to happen to prevent a race condition with GC 
		// (rendering thread still uses the FMaterialParameterCollectionInstanceResource when GC deletes the UMaterialParameterCollectionInstance)
		// However, if UpdateParameterCollectionInstances is going to be called after many AddParameterCollectionInstance's, this can be skipped for now.
		UpdateParameterCollectionInstances(false);
	}
}

UMaterialParameterCollectionInstance* UWorld::GetParameterCollectionInstance(const UMaterialParameterCollection* Collection)
{
	for (int32 InstanceIndex = 0; InstanceIndex < ParameterCollectionInstances.Num(); InstanceIndex++)
	{
		if (ParameterCollectionInstances[InstanceIndex]->GetCollection() == Collection)
		{
			return ParameterCollectionInstances[InstanceIndex];
		}
	}

	// Instance should always exist due to SetupParameterCollectionInstances() and UMaterialParameterCollection::PostLoad()
	checkf(0);
	return NULL;
}

void UWorld::UpdateParameterCollectionInstances(bool bUpdateInstanceUniformBuffers)
{
	if (Scene)
	{
		TArray<FMaterialParameterCollectionInstanceResource*> InstanceResources;

		for (int32 InstanceIndex = 0; InstanceIndex < ParameterCollectionInstances.Num(); InstanceIndex++)
		{
			UMaterialParameterCollectionInstance* Instance = ParameterCollectionInstances[InstanceIndex];

			if (bUpdateInstanceUniformBuffers)
			{
				Instance->UpdateRenderState();
			}

			InstanceResources.Add(Instance->GetResource());
		}

		Scene->UpdateParameterCollections(InstanceResources);
	}
}

void UWorld::InitWorld(const InitializationValues IVS)
{
	if (!ensure(!bIsWorldInitialized))
	{
		return;
	}

	FWorldDelegates::OnPreWorldInitialization.Broadcast(this, IVS);

	if (IVS.bInitializeScenes)
	{
		// Allocate the world's hash, navigation octree and scene.
		if (IVS.bCreateNavigation && NavigationSystem == NULL)
		{
			SetNavigationSystem(UNavigationSystem::CreateNavigationSystem(this));
		}

		if (IVS.bCreatePhysicsScene)
		{
			// Create the physics scene
			SetPhysicsScene(new FPhysScene());
		}

		bShouldSimulatePhysics = IVS.bShouldSimulatePhysics;
		
		// Save off the value used to create the scene, so this UWorld can recreate its scene later
		bRequiresHitProxies = IVS.bRequiresHitProxies;
		Scene = GetRendererModule().AllocateScene( this, bRequiresHitProxies );
		if ( !IsRunningDedicatedServer() && !IsRunningCommandlet() )
		{
			FXSystem = FFXSystemInterface::Create();
			Scene->SetFXSystem( FXSystem );
		}
		else
		{
			FXSystem = NULL;
			Scene->SetFXSystem(NULL);
		}
	}

	// Prepare AI systems
	if (GEngine->BehaviorTreeManagerClass != NULL)
	{
		BehaviorTreeManager = NewObject<UBehaviorTreeManager>(this, GEngine->BehaviorTreeManagerClass);
	}

	if (GEngine->EnvironmentQueryManagerClass != NULL)
	{
		EnvironmentQueryManager = NewObject<UEnvQueryManager>(this, GEngine->EnvironmentQueryManagerClass);
	}

	if (GEngine->AvoidanceManagerClass != NULL)
	{
		AvoidanceManager = NewObject<UAvoidanceManager>(this, GEngine->AvoidanceManagerClass);
	}

#if WITH_EDITOR
	bEnableTraceCollision = IVS.bEnableTraceCollision;
#endif

	SetupParameterCollectionInstances();

	if (PersistentLevel->GetOuter() != this)
	{
		// Move persistent level into world so the world object won't get garbage collected in the multi- level
		// case as it is still referenced via the level's outer. This is required for multi- level editing to work.
		PersistentLevel->Rename( *PersistentLevel->GetName(), this );
	}

	Levels.Empty(1);
	Levels.Add( PersistentLevel );
	PersistentLevel->OwningWorld = this;
	PersistentLevel->bIsVisible = true;

	// If for some reason we don't have a valid WorldSettings object go ahead and spawn one to avoid crashing.
	// This will generally happen if a map is being moved from a different project.
	const bool bNeedsExchange = PersistentLevel->Actors.Num() > 0;
	const bool bNeedsDestroy = bNeedsExchange && PersistentLevel->Actors[0] != NULL;

	if (PersistentLevel->Actors.Num() < 1 || PersistentLevel->Actors[0] == NULL || !PersistentLevel->Actors[0]->IsA(GEngine->WorldSettingsClass))
	{
		// Rename invalid WorldSettings to avoid name collisions
		if (bNeedsDestroy)
		{
			PersistentLevel->Actors[0]->Rename(NULL, PersistentLevel);
		}
		
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.bNoCollisionFail = true;
		SpawnInfo.Name = GEngine->WorldSettingsClass->GetFName();
		AWorldSettings* NewWorldSettings = SpawnActor<AWorldSettings>( GEngine->WorldSettingsClass, SpawnInfo );

		const int32 NewWorldSettingsActorIndex = PersistentLevel->Actors.Find( NewWorldSettings );

		if (bNeedsExchange)
		{
			// The world info must reside at index 0.
			Exchange(PersistentLevel->Actors[0],PersistentLevel->Actors[NewWorldSettingsActorIndex]);
		}

		// If there was an existing actor, copy its properties to the new actor and then destroy it
		if (bNeedsDestroy)
		{
			NewWorldSettings->UnregisterAllComponents();
			UEngine::CopyPropertiesForUnrelatedObjects(PersistentLevel->Actors[NewWorldSettingsActorIndex], NewWorldSettings);
			NewWorldSettings->RegisterAllComponents();
			PersistentLevel->Actors[NewWorldSettingsActorIndex]->Destroy();
		}

		// Re-sort actor list as we just shuffled things around.
		PersistentLevel->SortActorList();

	}
	check(GetWorldSettings());

	// initialize DefaultPhysicsVolume for the world
	// checked
	if (DefaultPhysicsVolume == NULL)
	{
	DefaultPhysicsVolume = SpawnActor<APhysicsVolume>(ADefaultPhysicsVolume::StaticClass());
	DefaultPhysicsVolume->Priority = -1000000;
	}

	// Find gravity
	if (GetPhysicsScene())
	{
		FVector Gravity = FVector( 0.f, 0.f, GetGravityZ() );
		GetPhysicsScene()->SetUpForFrame( &Gravity );
	}

	// Create physics collision handler, if we have a physics scene
	if(IVS.bCreatePhysicsScene)
	{
		AWorldSettings* WorldSettings = GetWorldSettings();
		// First look for world override
		TSubclassOf<UPhysicsCollisionHandler> PhysHandlerClass = WorldSettings->PhysicsCollisionHandlerClass;
		// Then fall back to engine default
		if(PhysHandlerClass == NULL)
		{
			PhysHandlerClass = GEngine->PhysicsCollisionHandlerClass;
		}

		if (PhysHandlerClass != NULL)
		{
			PhysicsCollisionHandler = NewObject<UPhysicsCollisionHandler>(this, PhysHandlerClass);
			PhysicsCollisionHandler->InitCollisionHandler();
		}
	}

	URL					= PersistentLevel->URL;
	CurrentLevel		= PersistentLevel;

	bAllowAudioPlayback = IVS.bAllowAudioPlayback;

	bDoDelayedUpdateCullDistanceVolumes = false;

#if WITH_EDITOR
	// See whether we're missing the default brush. It was possible in earlier builds to accidentally delete the default
	// brush of sublevels so we simply spawn a new one if we encounter it missing.
	ABrush* DefaultBrush = PersistentLevel->Actors.Num()<2 ? NULL : Cast<ABrush>(PersistentLevel->Actors[1]);
	if (GIsEditor)
	{
		if (!DefaultBrush || !DefaultBrush->IsStaticBrush() || DefaultBrush->BrushType!=Brush_Default || !DefaultBrush->BrushComponent || !DefaultBrush->Brush)
		{
			// Spawn the default brush.
			DefaultBrush = SpawnBrush();
			check(DefaultBrush->BrushComponent);
			DefaultBrush->Brush = new( GetOuter(), TEXT("Brush") )UModel( FPostConstructInitializeProperties(), DefaultBrush, 1 );
			DefaultBrush->BrushComponent->Brush = DefaultBrush->Brush;
			DefaultBrush->SetNotForClientOrServer();
			DefaultBrush->Brush->SetFlags( RF_Transactional );
			DefaultBrush->Brush->Polys->SetFlags( RF_Transactional );

			// Find the index in the array the default brush has been spawned at. Not necessarily
			// the last index as the code might spawn the default physics volume afterwards.
			const int32 DefaultBrushActorIndex = PersistentLevel->Actors.Find( DefaultBrush );

			// The default brush needs to reside at index 1.
			Exchange(PersistentLevel->Actors[1],PersistentLevel->Actors[DefaultBrushActorIndex]);

			// Re-sort actor list as we just shuffled things around.
			PersistentLevel->SortActorList();
		}
		else
		{
			// Ensure that the Brush and BrushComponent both point to the same model
			DefaultBrush->BrushComponent->Brush = DefaultBrush->Brush;
		}

		// Reset the lightmass settings on the default brush; they can't be edited by the user but could have
		// been tainted if the map was created during a window where the memory was uninitialized
		if (DefaultBrush->Brush != NULL)
		{
			UModel* Model = DefaultBrush->Brush;

			const FLightmassPrimitiveSettings DefaultSettings;

			for (int32 i = 0; i < Model->LightmassSettings.Num(); ++i)
			{
				Model->LightmassSettings[i] = DefaultSettings;
			}

			if (Model->Polys != NULL) 
			{
				for (int32 i = 0; i < Model->Polys->Element.Num(); ++i)
				{
					Model->Polys->Element[i].LightmassSettings = DefaultSettings;
				}
			}
		}
	}
#endif // WITH_EDITOR


	// update it's bIsDefaultLevel
	bIsDefaultLevel = (FPaths::GetBaseFilename(GetMapName()) == FPaths::GetBaseFilename(UGameMapsSettings::GetGameDefaultMap()));

	if (IVS.bCreateWorldComposition)
	{
		InitializeWorldComposition();
	}

	// We're initialized now.
	bIsWorldInitialized = true;

	//@TODO: Should this happen here, or below here?
	FWorldDelegates::OnPostWorldInitialization.Broadcast(this, IVS);

	PersistentLevel->PrecomputedVisibilityHandler.UpdateScene(Scene);
	PersistentLevel->PrecomputedVolumeDistanceField.UpdateScene(Scene);
	PersistentLevel->InitializeRenderingResources();

	if (GEngine->bStartWithMatineeCapture)
	{
		TArray<FString> VisibleLevels;
		GEngine->VisibleLevelsForMatineeCapture.ParseIntoArray(&VisibleLevels, TEXT("|"), true);
		for( int32 LevelIndex=0; LevelIndex<StreamingLevels.Num(); LevelIndex++ )
		{
			ULevelStreaming* StreamingLevel	= StreamingLevels[LevelIndex];
			if (StreamingLevel)
			{	
				for (int32 VisibleLevelIndex = 0; VisibleLevelIndex < VisibleLevels.Num(); ++VisibleLevelIndex)
				{
					FString LevelName = VisibleLevels[VisibleLevelIndex];
					FString FullPackageName = StreamingLevel->PackageName.ToString();
					FString ShortFullPackageName = FPackageName::GetLongPackageAssetName(FullPackageName);
					// Check for Play on PC.  That prefix is a bit special as it's only 5 characters. (All others are 6)
					if( ShortFullPackageName.StartsWith( FString( PLAYWORLD_CONSOLE_BASE_PACKAGE_PREFIX ) + TEXT( "PC") ) )
					{
						ShortFullPackageName = ShortFullPackageName.Right(ShortFullPackageName.Len() - 5);
					}
					else if( ShortFullPackageName.StartsWith( PLAYWORLD_CONSOLE_BASE_PACKAGE_PREFIX ) )
					{
						// This is a Play on Console map package prefix. (6 characters)
						ShortFullPackageName = ShortFullPackageName.Right(ShortFullPackageName.Len() - 6);
					}
					FString ShortLevelName = FPackageName::GetShortName( LevelName );
		
					if (ShortLevelName == ShortFullPackageName)
					{
						StreamingLevel->bShouldBeLoaded		= true;
						StreamingLevel->bShouldBeVisible	= true;
						break;
					}
				}
			}
		}
	}

	BroadcastLevelsChanged();
}

void UWorld::InitializeNewWorld(const InitializationValues IVS)
{
	if (!IVS.bTransactional)
	{
		ClearFlags(RF_Transactional);
	}

	PersistentLevel			= new( this, TEXT("PersistentLevel") ) ULevel(FPostConstructInitializeProperties(),FURL(NULL));
	PersistentLevel->Model	= new( PersistentLevel				 ) UModel(FPostConstructInitializeProperties(),NULL, 1 );
	PersistentLevel->OwningWorld = this;

	// Mark objects are transactional for undo/ redo.
	if (IVS.bTransactional)
	{
		PersistentLevel->SetFlags( RF_Transactional );
		PersistentLevel->Model->SetFlags( RF_Transactional );
	}
	else
	{
		PersistentLevel->ClearFlags( RF_Transactional );
		PersistentLevel->Model->ClearFlags( RF_Transactional );
	}

	// Need to associate current level so SpawnActor doesn't complain.
	CurrentLevel = PersistentLevel;

	// Create the WorldInfo actor.
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.bNoCollisionFail = true;
	// Set constant name for WorldSettings to make a network replication work between new worlds on host and client
	SpawnInfo.Name = GEngine->WorldSettingsClass->GetFName();
	SpawnActor( GEngine->WorldSettingsClass, NULL, NULL, SpawnInfo );
	check(GetWorldSettings());

	// Initialize the world
	InitWorld(IVS);

	// Update components.
	UpdateWorldComponents( true, false );
}


void UWorld::DestroyWorld( bool bInformEngineOfWorld )
{
	// Clean up existing world and remove it from root set so it can be garbage collected.
	bIsLevelStreamingFrozen = false;
	bShouldForceUnloadStreamingLevels = true;
	FlushLevelStreaming( NULL, true );
	CleanupWorld();

	check( NetworkActors.Num() == 0 );

	// Tell the engine we are destroying the world.(unless we are asked not to)
	if( ( GEngine ) && ( bInformEngineOfWorld == true ) )
	{
		GEngine->WorldDestroyed( this );
	}		
	RemoveFromRoot();
	ClearFlags(RF_Standalone);
	
	for (int32 LevelIndex=0; LevelIndex < GetNumLevels(); ++LevelIndex)
	{
		UWorld* World = CastChecked<UWorld>(GetLevel(LevelIndex)->GetOuter());
		if (World != this)
		{
			World->ClearFlags(RF_Standalone);
		}
	}
}

UWorld* UWorld::CreateWorld( const EWorldType::Type InWorldType, bool bInformEngineOfWorld )
{
	// Create a new package unless we're a commandlet in which case we keep the dummy world in the transient package.
	UPackage*	WorldPackage		= IsRunningCommandlet() ? GetTransientPackage() : CreatePackage( NULL, NULL );

	if (InWorldType == EWorldType::PIE)
	{
		WorldPackage->PackageFlags |= PKG_PlayInEditor;
	}

	// Mark the package as containing a world.  This has to happen here rather than at serialization time,
	// so that e.g. the referenced assets browser will work correctly.
	if ( WorldPackage != GetTransientPackage() )
	{
		WorldPackage->ThisContainsMap();
	}

	// Create new UWorld, ULevel and UModel.
	UWorld* NewWorld = new( WorldPackage				, TEXT("NewWorld")			) UWorld(FPostConstructInitializeProperties(),FURL(NULL));
	NewWorld->WorldType = InWorldType;
	NewWorld->InitializeNewWorld(UWorld::InitializationValues().ShouldSimulatePhysics(false).EnableTraceCollision(true));

	// Clear the dirty flag set during SpawnActor and UpdateLevelComponents
	WorldPackage->SetDirtyFlag(false);

	// Add to root set so it doesn't get garbage collected.
	NewWorld->AddToRoot();

	// Tell the engine we are adding a world (unless we are asked not to)
	if( ( GEngine ) && ( bInformEngineOfWorld == true ) )
	{
		GEngine->WorldAdded( NewWorld );
	}

	return NewWorld;
}

void UWorld::RemoveActor(AActor* Actor, bool bShouldModifyLevel)
{
	bool	bSuccessfulRemoval = false;
	ULevel* CheckLevel = Actor->GetLevel();
	int32 ActorListIndex = CheckLevel->Actors.Find( Actor );
	// Search the entire list.
	if( ActorListIndex != INDEX_NONE )
	{
		if ( bShouldModifyLevel && GUndo )
		{
			ModifyLevel( CheckLevel );
		}
		
		if (!HasBegunPlay())
		{
			CheckLevel->Actors.ModifyItem(ActorListIndex);
		}
		
		CheckLevel->Actors[ActorListIndex] = NULL;
		bSuccessfulRemoval = true;		
	}

	// Remove actor from network list
	RemoveNetworkActor( Actor );

	// TTP 281860: Callstack will hopefully indicate how the actors array ends up without the required default actors
	check(CheckLevel->Actors.Num() >= 2);

	if ( !bSuccessfulRemoval && !( Actor->GetFlags() & RF_Transactional ) )
	{
		//CheckLevel->Actors is a transactional array so it is very likely that non-transactional
		//actors could be missing from the array if the array was reverted to a state before they
		//existed. (but they won't be reverted since they are non-transactional)
		bSuccessfulRemoval = true;
	}

	if ( !bSuccessfulRemoval )
	{
		// TTP 270000: Trying to track down why certain actors aren't in the level actor list when saving.  If we're reinstancing, dump the list
		{
			UE_LOG(LogWorld, Log, TEXT("--- Actors Currently in %s ---"), *CheckLevel->GetPathName());
			for (int32 ActorIdx = 0; ActorIdx < CheckLevel->Actors.Num(); ActorIdx++)
			{
				AActor* CurrentActor = CheckLevel->Actors[ActorIdx];
				UE_LOG(LogWorld, Log, TEXT("  %s"), (CurrentActor ? *CurrentActor->GetPathName() : TEXT("NONE")));
			}
		}
		UE_LOG(LogWorld, Fatal, TEXT("Could not remove actor %s from world (check level is %s)"), *Actor->GetPathName(), *CheckLevel->GetPathName());
	}
	check(bSuccessfulRemoval);
}


bool UWorld::ContainsActor( AActor* Actor )
{
	return (Actor && Actor->GetWorld() == this);
}

bool UWorld::AllowAudioPlayback()
{
	return bAllowAudioPlayback;
}

#if WITH_EDITOR
void UWorld::ShrinkLevel()
{
	GetModel()->ShrinkModel();
}
#endif // WITH_EDITOR

void UWorld::ClearWorldComponents()
{
	for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels[LevelIndex];
		Level->ClearLevelComponents();
	}

	if(LineBatcher && LineBatcher->IsRegistered())
	{
		LineBatcher->UnregisterComponent();
	}

	if(PersistentLineBatcher && PersistentLineBatcher->IsRegistered())
	{
		PersistentLineBatcher->UnregisterComponent();
	}

	if(ForegroundLineBatcher && ForegroundLineBatcher->IsRegistered())
	{
		ForegroundLineBatcher->UnregisterComponent();
	}
}


void UWorld::UpdateWorldComponents(bool bRerunConstructionScripts, bool bCurrentLevelOnly)
{
	if ( !IsRunningDedicatedServer() )
	{
		if(!LineBatcher)
		{
			LineBatcher = ConstructObject<ULineBatchComponent>(ULineBatchComponent::StaticClass());
		}

		if(!LineBatcher->IsRegistered())
		{	
			LineBatcher->RegisterComponentWithWorld(this);
		}

		if(!PersistentLineBatcher)
		{
			PersistentLineBatcher = ConstructObject<ULineBatchComponent>(ULineBatchComponent::StaticClass());
		}

		if(!PersistentLineBatcher->IsRegistered())	
		{
			PersistentLineBatcher->RegisterComponentWithWorld(this);
		}

		if(!ForegroundLineBatcher)
		{
			ForegroundLineBatcher = ConstructObject<ULineBatchComponent>(ULineBatchComponent::StaticClass());
		}

		if(!ForegroundLineBatcher->IsRegistered())	
		{
			ForegroundLineBatcher->RegisterComponentWithWorld(this);
		}
	}

	if ( bCurrentLevelOnly )
	{
		check( CurrentLevel );
		CurrentLevel->UpdateLevelComponents(bRerunConstructionScripts);
	}
	else
	{
		for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel* Level = Levels[LevelIndex];
			ULevelStreaming* StreamingLevel = FLevelUtils::FindStreamingLevel(Level);
			// Update the level only if it is visible (or not a streamed level)
			if(!StreamingLevel || Level->bIsVisible)
			{
				Level->UpdateLevelComponents(bRerunConstructionScripts);
			}
		}
	}

	UpdateCullDistanceVolumes();
}


void UWorld::UpdateCullDistanceVolumes()
{
	// Map that will store new max draw distance for every primitive
	TMap<UPrimitiveComponent*,float> CompToNewMaxDrawMap;

	// Keep track of time spent.
	double Duration = 0;
	{
		SCOPE_SECONDS_COUNTER(Duration);

		// Establish base line of LD specified cull distances.
		for( FActorIterator It(this); It; ++It )
		{
			auto PrimitiveComponent = It->FindComponentByClass<UPrimitiveComponent>();
			if (PrimitiveComponent)
			{
				CompToNewMaxDrawMap.Add(PrimitiveComponent, PrimitiveComponent->LDMaxDrawDistance);
			}
		}

		// Iterate over all cull distance volumes and get new cull distances.
		for( FActorIterator It(this); It; ++It )
		{
			ACullDistanceVolume* CullDistanceVolume = Cast<ACullDistanceVolume>(*It);
			if( CullDistanceVolume )
			{
				CullDistanceVolume->GetPrimitiveMaxDrawDistances(CompToNewMaxDrawMap);
			}
		}

		// Finally, go over all primitives, and see if they need to change.
		// Only if they do do we reregister them, as thats slow.
		for ( TMap<UPrimitiveComponent*,float>::TIterator It(CompToNewMaxDrawMap); It; ++It )
		{
			UPrimitiveComponent* PrimComp = It.Key();
			float NewMaxDrawDist = It.Value();

			if( !FMath::IsNearlyEqual(PrimComp->CachedMaxDrawDistance, NewMaxDrawDist) )
			{
				PrimComp->CachedMaxDrawDistance = NewMaxDrawDist;
				PrimComp->RecreateRenderState_Concurrent();
			}
		}
	}

	if( Duration > 1.f )
	{
		UE_LOG(LogWorld, Log, TEXT("Updating cull distance volumes took %5.2f seconds"),Duration);
	}
}


void UWorld::ModifyLevel(ULevel* Level)
{
	if( Level && Level->HasAnyFlags(RF_Transactional))
	{
		Level->Modify( false );
		//Level->Actors.ModifyAllItems();
		Level->Model->Modify( false );
	}
}

void UWorld::EnsureCollisionTreeIsBuilt()
{
	if (bInTick)
	{
		// Current implementation of collision tree rebuild ticks physics scene and can not be called during world tick
		return;
	}

	if (GIsEditor && !IsPlayInEditor())
	{
		// Don't simulate physics in the editor
		return;
	}
	
	// Set physics to static loading mode
	if (PhysicsScene)
	{
		PhysicsScene->SetIsStaticLoading(true);
	}

	for (int Iteration = 0; Iteration < 6; ++Iteration)
	{
		SetupPhysicsTickFunctions(0.1f);
		PhysicsScene->StartFrame();
		PhysicsScene->WaitPhysScenes();
		PhysicsScene->EndFrame(NULL);
	}

	if (PhysicsScene)
	{
		PhysicsScene->SetIsStaticLoading(false);
	}
}

void UWorld::InvalidateModelGeometry(ULevel* InLevel)
{
	if ( InLevel )
	{
		InLevel->InvalidateModelGeometry();
	}
	else
	{
		for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel* Level = Levels[LevelIndex];
			Level->InvalidateModelGeometry();
		}
	}
}


void UWorld::InvalidateModelSurface(bool bCurrentLevelOnly)
{
	if ( bCurrentLevelOnly )
	{
		check( bCurrentLevelOnly );
		CurrentLevel->InvalidateModelSurface();
	}
	else
	{
		for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel* Level = Levels[LevelIndex];
			Level->InvalidateModelSurface();
		}
	}
}


void UWorld::CommitModelSurfaces()
{
	for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels[LevelIndex];
		Level->CommitModelSurfaces();
	}
}


void UWorld::TransferBlueprintDebugReferences(UWorld* NewWorld)
{
#if WITH_EDITOR
	// First create a list of blueprints that already exist in the new world
	TArray< FString > NewWorldExistingBlueprintNames;
	for (FBlueprintToDebuggedObjectMap::TIterator It(NewWorld->BlueprintObjectsBeingDebugged); It; ++It)
	{
		if (UBlueprint* TargetBP = It.Key().Get())
		{	
			NewWorldExistingBlueprintNames.AddUnique( TargetBP->GetName() );
		}
	}

	// Move debugging object associations across from the old world to the new world that are not already there
	for (FBlueprintToDebuggedObjectMap::TIterator It(BlueprintObjectsBeingDebugged); It; ++It)
	{
		if (UBlueprint* TargetBP = It.Key().Get())
		{	
			FString SourceName = TargetBP->GetName();
			// If this blueprint is not already listed in the ones bieng debugged in the new world, add it.
			if( NewWorldExistingBlueprintNames.Find( SourceName ) == INDEX_NONE )			
			{
				TWeakObjectPtr<UObject>& WeakTargetObject = It.Value();
				UObject* NewTargetObject = NULL;

				if (WeakTargetObject.IsValid())
				{
					UObject* OldTargetObject = WeakTargetObject.Get();
					check(OldTargetObject);

					NewTargetObject = FindObject<UObject>(NewWorld, *OldTargetObject->GetPathName(this));
				}

				if (NewTargetObject != NULL)
				{
					// Check to see if the object we found to transfer to is of a different class.  LevelScripts are always exceptions, because a new level may have been loaded in PIE, and we have special handling for LSA debugging objects
					if (!NewTargetObject->IsA(TargetBP->GeneratedClass) && !NewTargetObject->IsA(ALevelScriptActor::StaticClass()))
					{
						const FString BlueprintFullPath = TargetBP->GetPathName();

						if (BlueprintFullPath.StartsWith(TEXT("/Temp/Autosaves")) || BlueprintFullPath.StartsWith(TEXT("/Temp//Autosaves")))
						{
							// This map was an autosave for networked PIE; it's OK to fail to fix
							// up the blueprint object being debugged reference as the whole blueprint
							// is going away.
						}
						else
						{
							// Let the ensure fire
							UE_LOG(LogWorld, Warning, TEXT("Found object to debug in main world that isn't the correct type"));
							UE_LOG(LogWorld, Warning, TEXT("  TargetBP path is %s"), *TargetBP->GetPathName());
							UE_LOG(LogWorld, Warning, TEXT("  TargetBP gen class path is %s"), *TargetBP->GeneratedClass->GetPathName());
							UE_LOG(LogWorld, Warning, TEXT("  NewTargetObject path is %s"), *NewTargetObject->GetPathName());
							UE_LOG(LogWorld, Warning, TEXT("  NewTargetObject class path is %s"), *NewTargetObject->GetClass()->GetPathName());

							UObject* OldTargetObject = WeakTargetObject.Get();
							UE_LOG(LogWorld, Warning, TEXT("  OldObject path is %s"), *OldTargetObject->GetPathName());
							UE_LOG(LogWorld, Warning, TEXT("  OldObject class path is %s"), *OldTargetObject->GetClass()->GetPathName());

							ensureMsg(false, TEXT("Failed to find an appropriate object to debug back in the editor world"));
						}

						NewTargetObject = NULL;
					}
				}

				TargetBP->SetObjectBeingDebugged(NewTargetObject);
			}
		}
	}

	// Empty the map, anything useful got moved over the map in the new world
	BlueprintObjectsBeingDebugged.Empty();
#endif	//#if WITH_EDITOR
}


void UWorld::NotifyOfBlueprintDebuggingAssociation(class UBlueprint* Blueprint, UObject* DebugObject)
{
#if WITH_EDITOR
	TWeakObjectPtr<UBlueprint> Key(Blueprint);

	if (DebugObject)
	{
		BlueprintObjectsBeingDebugged.FindOrAdd(Key) = DebugObject;
	}
	else
	{
		BlueprintObjectsBeingDebugged.Remove(Key);
	}
#endif	//#if WITH_EDITOR
}

DEFINE_STAT(STAT_AddToWorldTime);
DEFINE_STAT(STAT_RemoveFromWorldTime);
DEFINE_STAT(STAT_UpdateLevelStreamingTime);

/**
 * Static helper function for AddToWorld to determine whether we've already spent all the allotted time.
 *
 * @param	CurrentTask		Description of last task performed
 * @param	StartTime		StartTime, used to calculate time passed
 * @param	Level			Level work has been performed on
 *
 * @return true if time limit has been exceeded, false otherwise
 */
static bool IsTimeLimitExceeded( const TCHAR* CurrentTask, double StartTime, ULevel* Level )
{
	bool bIsTimeLimitExceed = false;
	// We don't spread work across several frames in the Editor to avoid potential side effects.
	if( Level->OwningWorld->IsGameWorld() == true )
	{
		double TimeLimit = GEngine->LevelStreamingActorsUpdateTimeLimit;
		double CurrentTime	= FPlatformTime::Seconds();
		// Delta time in ms.
		double DeltaTime	= (CurrentTime - StartTime) * 1000;
		if( DeltaTime > TimeLimit )
		{
			// Log if a single event took way too much time.
			if( DeltaTime > 20 )
			{
				UE_LOG(LogStreaming, Log, TEXT("UWorld::AddToWorld: %s for %s took (less than) %5.2f ms"), CurrentTask, *Level->GetOutermost()->GetName(), DeltaTime );
			}
			bIsTimeLimitExceed = true;
		}
	}
	return bIsTimeLimitExceed;
}

#if PERF_TRACK_DETAILED_ASYNC_STATS

// Variables for tracking how long eachpart of the AddToWorld process takes
double MoveActorTime = 0.0;
double ShiftActorsTime = 0.0;
double UpdateComponentsTime = 0.0;
double InitBSPPhysTime = 0.0;
double InitActorPhysTime = 0.0;
double InitActorTime = 0.0;
double RouteActorInitializeTime = 0.0;
double CrossLevelRefsTime = 0.0;
double SequenceBeginPlayTime = 0.0;
double SortActorListTime = 0.0;

/** Helper class, to add the time between this objects creating and destruction to passed in variable. */
class FAddWorldScopeTimeVar
{
public:
	FAddWorldScopeTimeVar(double* Time)
	{
		TimeVar = Time;
		Start = FPlatformTime::Seconds();
	}

	~FAddWorldScopeTimeVar()
	{
		*TimeVar += (FPlatformTime::Seconds() - Start);
	}

private:
	/** Pointer to value to add to when object is destroyed */
	double* TimeVar;
	/** The time at which this object was created */
	double	Start;
};

/** Macro to create a scoped timer for the supplied var */
#define SCOPE_TIME_TO_VAR(V) FAddWorldScopeTimeVar TimeVar(V)

#else

/** Empty macro, when not doing timing */
#define SCOPE_TIME_TO_VAR(V)

#endif // PERF_TRACK_DETAILED_ASYNC_STATS

void UWorld::AddToWorld( ULevel* Level, const FTransform& LevelTransform )
{
	SCOPE_CYCLE_COUNTER(STAT_AddToWorldTime);
	
	check(Level);
	check(!Level->IsPendingKill());
	check(!Level->HasAnyFlags(RF_Unreachable));

	// Set flags to indicate that we are associating a level with the world to e.g. perform slower/ better octree insertion 
	// and such, as opposed to the fast path taken for run-time/ gameplay objects.
	Level->bIsAssociatingLevel = true;

	const double StartTime = FPlatformTime::Seconds();

	// Don't consider the time limit if the match hasn't started as we need to ensure that the levels are fully loaded
	const bool bConsiderTimeLimit = bMatchStarted;

	bool bExecuteNextStep = (CurrentLevelPendingVisibility == Level) || CurrentLevelPendingVisibility == NULL;
	bool bPerformedLastStep	= false;
	
	if( bExecuteNextStep && CurrentLevelPendingVisibility == NULL )
	{
		Level->OwningWorld = this;
		
		// Mark level as being the one in process of being made visible.
		CurrentLevelPendingVisibility = Level;

		// Add to the UWorld's array of levels, which causes it to be rendered et al.
		Levels.AddUnique( Level );
		
#if PERF_TRACK_DETAILED_ASYNC_STATS
		MoveActorTime = 0.0;
		ShiftActorsTime = 0.0;
		UpdateComponentsTime = 0.0;
		InitBSPPhysTime = 0.0;
		InitActorPhysTime = 0.0;
		InitActorTime = 0.0;
		RouteActorInitializeTime = 0.0;
		CrossLevelRefsTime = 0.0;
		SequenceBeginPlayTime = 0.0;
		SortActorListTime = 0.0;
#endif
	}

	if( bExecuteNextStep && !Level->bAlreadyMovedActors )
	{
		SCOPE_TIME_TO_VAR(&MoveActorTime);

		FLevelUtils::ApplyLevelTransform( Level, LevelTransform, false );

		Level->bAlreadyMovedActors = true;
		bExecuteNextStep = (!bConsiderTimeLimit || !IsTimeLimitExceeded( TEXT("moving actors"), StartTime, Level ));
	}

	if( bExecuteNextStep && !Level->bAlreadyShiftedActors )
	{
		SCOPE_TIME_TO_VAR(&ShiftActorsTime);

		// Notify world composition: will place level actors according to current world origin
		if (WorldComposition)
		{
			WorldComposition->OnLevelAddedToWorld(Level);
		}
		
		Level->bAlreadyShiftedActors = true;
		bExecuteNextStep = (!bConsiderTimeLimit || !IsTimeLimitExceeded( TEXT("shifting actors"), StartTime, Level ));
	}

	// Updates the level components (Actor components and UModelComponents).
	if( bExecuteNextStep && !Level->bAlreadyUpdatedComponents )
	{
		SCOPE_TIME_TO_VAR(&UpdateComponentsTime);

		// Make sure code thinks components are not currently attached.
		Level->bAreComponentsCurrentlyRegistered = false;

		// Incrementally update components.
		int32 NumActorsToUpdate = GEngine->LevelStreamingNumActorsToUpdate;
		do
		{
#if WITH_EDITOR
			// Pretend here that we are loading package to avoid package dirtying during components registration
			TGuardValue<bool> IsEditorLoadingPackage(GIsEditorLoadingPackage, (GIsEditor ? true : GIsEditorLoadingPackage));
#endif
			// We don't need to rerun construction scripts if we have cooked data or we are playing in editor unless the PIE world was loaded
			// from disk rather than duplicated
			const bool bRerunConstructionScript = !FPlatformProperties::RequiresCookedData() && (!IsPlayInEditor() || HasAnyFlags(RF_WasLoaded));
			Level->IncrementalUpdateComponents( (GIsEditor || IsRunningCommandlet()) ? 0 : NumActorsToUpdate, bRerunConstructionScript );
		}
		while( (!bConsiderTimeLimit || !IsTimeLimitExceeded( TEXT("updating components"), StartTime, Level )) && !Level->bAreComponentsCurrentlyRegistered );

		// We are done once all components are attached.
		Level->bAlreadyUpdatedComponents	= Level->bAreComponentsCurrentlyRegistered;
		bExecuteNextStep					= Level->bAreComponentsCurrentlyRegistered;
	}

	if( IsGameWorld() && HasBegunPlay() )
	{
		// Initialize all actors and start execution.
		if( bExecuteNextStep && !Level->bAlreadyInitializedActors )
		{
			SCOPE_TIME_TO_VAR(&InitActorTime);

			Level->InitializeActors();
			Level->bAlreadyInitializedActors = true;
			bExecuteNextStep = (!bConsiderTimeLimit || !IsTimeLimitExceeded( TEXT("initializing actors"), StartTime, Level ));
		}

		// Route various begin play functions and set volumes.
		if( bExecuteNextStep && !Level->bAlreadyRoutedActorInitialize )
		{
			SCOPE_TIME_TO_VAR(&RouteActorInitializeTime);
			bStartup = 1;
			Level->RouteActorInitialize();
			Level->bAlreadyRoutedActorInitialize = true;
			bStartup = 0;

			bExecuteNextStep = (!bConsiderTimeLimit || !IsTimeLimitExceeded( TEXT("routing BeginPlay on actors"), StartTime, Level ));
		}

		// Sort the actor list; can't do this on save as the relevant properties for sorting might have been changed by code
		if( bExecuteNextStep && !Level->bAlreadySortedActorList )
		{
			SCOPE_TIME_TO_VAR(&SortActorListTime);

			Level->SortActorList();
			Level->bAlreadySortedActorList = true;
			bExecuteNextStep = (!bConsiderTimeLimit || !IsTimeLimitExceeded( TEXT("sorting actor list"), StartTime, Level ));
			bPerformedLastStep = true;
		}
	}
	else
	{
		bPerformedLastStep = true;
	}

	Level->bIsAssociatingLevel = false;

	// We're done.
	if( bPerformedLastStep )
	{
#if PERF_TRACK_DETAILED_ASYNC_STATS
		// Log out all of the timing information
		double TotalTime = MoveActorTime + ShiftActorsTime + UpdateComponentsTime + InitBSPPhysTime + InitActorPhysTime + InitActorTime + RouteActorInitializeTime + CrossLevelRefsTime + SequenceBeginPlayTime + SortActorListTime;
		UE_LOG(LogStreaming, Log, TEXT("Detailed AddToWorld stats for '%s' - Total %6.2fms"), *Level->GetOutermost()->GetName(), TotalTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Move Actors             : %6.2f ms"), MoveActorTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Shift Actors            : %6.2f ms"), ShiftActorsTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Update Components       : %6.2f ms"), UpdateComponentsTime * 1000 );

		PrintSortedListFromMap(Level->UpdateComponentsTimePerActorClass);
		Level->UpdateComponentsTimePerActorClass.Empty();

		UE_LOG(LogStreaming, Log, TEXT("Init BSP Phys           : %6.2f ms"), InitBSPPhysTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Init Actor Phys         : %6.2f ms"), InitActorPhysTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Init Actors             : %6.2f ms"), InitActorTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("BeginPlay               : %6.2f ms"), RouteActorInitializeTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Cross Level Refs        : %6.2f ms"), CrossLevelRefsTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Sequence BeginPlay      : %6.2f ms"), SequenceBeginPlayTime * 1000 );
		UE_LOG(LogStreaming, Log, TEXT("Sort Actor List         : %6.2f ms"), SortActorListTime * 1000 );	
#endif // PERF_TRACK_DETAILED_ASYNC_STATS

		if ( IsGameWorld() )
		{
			Level->bAlreadyMovedActors = false; // If in the editor, actors are not removed during RemoveFromWorld, so will not need to be moved again when unhidden\AddToWorld is called a second time
		}
		Level->bAlreadyShiftedActors					= false;
		Level->bAlreadyUpdatedComponents				= false;
		Level->bAlreadyInitializedActors				= false;
		Level->bAlreadyRoutedActorInitialize			= false;
		Level->bAlreadySortedActorList					= false;

		// Finished making level visible - allow other levels to be added to the world.
		CurrentLevelPendingVisibility					= NULL;

		// notify server that the client has finished making this level visible
		for (FLocalPlayerIterator It(GEngine, this); It; ++It)
		{
			if (It->PlayerController != NULL)
			{
				// Remap packagename for PIE networking before sending out to server
				FName PackageName = Level->GetOutermost()->GetFName();
				FString PackageNameStr = PackageName.ToString();
				if (GEngine->NetworkRemapPath(this, PackageNameStr, false))
				{
					PackageName = FName(*PackageNameStr);
				}

				It->PlayerController->ServerUpdateLevelVisibility(PackageName, true);
			}
		}

		Level->InitializeRenderingResources();

		// Notify the texture streaming system now that everything is set up.
		GStreamingManager->AddLevel( Level );

		Level->bIsVisible = true;
	
		// send a callback that a level was added to the world
		FWorldDelegates::LevelAddedToWorld.Broadcast(Level, this);

		BroadcastLevelsChanged();

		ULevelStreaming::BroadcastLevelVisibleStatus(this, Level->GetOutermost()->GetFName(), true);
	}
}


void UWorld::RemoveFromWorld( ULevel* Level )
{
	SCOPE_CYCLE_COUNTER(STAT_RemoveFromWorldTime);
	check(Level);
	check(!Level->IsPendingKill());
	check(!Level->HasAnyFlags(RF_Unreachable));

	if( CurrentLevelPendingVisibility == NULL )
	{
		// Keep track of timing.
		double StartTime = FPlatformTime::Seconds();	

		for (int32 ActorIdx = 0; ActorIdx < Level->Actors.Num(); ActorIdx++)
		{
			AActor* Actor = Level->Actors[ActorIdx];
			if (Actor != NULL)
			{
				Actor->OnRemoveFromWorld();
				if (NetDriver)
				{
					NetDriver->NotifyActorLevelUnloaded(Actor);
				}
			}
		}

		// Remove any pawns from the pawn list that are about to be streamed out
		for( FConstPawnIterator Iterator = GetPawnIterator(); Iterator; ++Iterator )
		{
			APawn* Pawn = *Iterator;
			if (Pawn->IsInLevel(Level))
			{
				RemovePawn(Pawn);
				--Iterator;
			}
			else if (UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent()))
			{
				// otherwise force floor check in case the floor was streamed out from under it
				CharacterMovement->bForceNextFloorCheck = true;
			}
		}

		Level->ReleaseRenderingResources();

		// Remove from the world's level array and destroy actor components.
		GStreamingManager->RemoveLevel( Level );
		
		Level->ClearLevelComponents();

		// notify server that the client has removed this level
		for (FLocalPlayerIterator It(GEngine, this); It; ++It)
		{
			if (It->PlayerController != NULL)
			{
				// Remap packagename for PIE networking before sending out to server
				FName PackageName = Level->GetOutermost()->GetFName();
				FString PackageNameStr = PackageName.ToString();
				if (GEngine->NetworkRemapPath(this, PackageNameStr, false))
				{
					PackageName = FName(*PackageNameStr);
				}

				It->PlayerController->ServerUpdateLevelVisibility(PackageName, false);
			}
		}
		
		// Notify world composition: will place a level at original position
		if (WorldComposition)
		{
			WorldComposition->OnLevelRemovedFromWorld(Level);
		}

		// Make sure level always has OwningWorld in the editor
		if ( IsGameWorld() )
		{
			Levels.Remove(Level);
			Level->OwningWorld = NULL;
		}
				
		// Remove any actors in the network list
		for ( int i = 0; i < Level->Actors.Num(); i++ )
		{
			RemoveNetworkActor( Level->Actors[ i ] );
		}

		Level->bIsVisible = false;
		UE_LOG(LogStreaming, Log, TEXT("UWorld::RemoveFromWorld for %s took %5.2f ms"), *Level->GetOutermost()->GetName(), (FPlatformTime::Seconds()-StartTime) * 1000.0 );
			
		// let the universe know we have removed a level
		FWorldDelegates::LevelRemovedFromWorld.Broadcast(Level, this);
		BroadcastLevelsChanged();

		ULevelStreaming::BroadcastLevelVisibleStatus(this, Level->GetOutermost()->GetFName(), false);
	}
}

/************************************************************************/
/* FLevelStreamingGCHelper implementation                               */
/************************************************************************/
FLevelStreamingGCHelper::FOnGCStreamedOutLevelsEvent FLevelStreamingGCHelper::OnGCStreamedOutLevels;
TArray<TWeakObjectPtr<ULevel> > FLevelStreamingGCHelper::LevelsPendingUnload;
TArray<FName> FLevelStreamingGCHelper::LevelPackageNames;

void FLevelStreamingGCHelper::AddGarbageCollectorCallback()
{
	// Only register for garbage collection once
	static bool GarbageCollectAdded = false;
	if ( GarbageCollectAdded == false )
	{
		FCoreDelegates::PreGarbageCollect.AddStatic( FLevelStreamingGCHelper::PrepareStreamedOutLevelsForGC );
		FCoreDelegates::PostGarbageCollect.AddStatic( FLevelStreamingGCHelper::VerifyLevelsGotRemovedByGC );
		GarbageCollectAdded = true;
	}
}

void FLevelStreamingGCHelper::RequestUnload( ULevel* InLevel )
{
	if (!IsRunningCommandlet())
	{
		check( InLevel );
		check( InLevel->bIsVisible == false );
		LevelsPendingUnload.AddUnique( InLevel );
	}
}

void FLevelStreamingGCHelper::CancelUnloadRequest( ULevel* InLevel )
{
	LevelsPendingUnload.Remove( InLevel );
}

void FLevelStreamingGCHelper::PrepareStreamedOutLevelsForGC()
{
	if (LevelsPendingUnload.Num() > 0)
	{
		OnGCStreamedOutLevels.Broadcast();
	}
	
	// Iterate over all level objects that want to be unloaded.
	for( int32 LevelIndex=0; LevelIndex < LevelsPendingUnload.Num(); LevelIndex++ )
	{
		ULevel*	Level = LevelsPendingUnload[LevelIndex].Get();

		if( Level && (!GIsEditor || (Level->GetOutermost()->PackageFlags & PKG_PlayInEditor) ))
		{
			UPackage* LevelPackage = Cast<UPackage>(Level->GetOutermost());
			UE_LOG(LogStreaming, Log, TEXT("PrepareStreamedOutLevelsForGC called on '%s'"), *LevelPackage->GetName() );
			
			const TArray<FWorldContext>& WorldList = GEngine->GetWorldContexts();
			for (int32 ContextIdx = 0; ContextIdx < WorldList.Num(); ++ContextIdx)
			{
				UWorld* World = WorldList[ContextIdx].World();
				if (World)
				{
					// This can never ever be called during tick; same goes for GC in general.
					check( !World->bInTick );
					UNetDriver *NetDriver = World->GetNetDriver();
					if (NetDriver)
					{
						// The net driver must remove this level and its actors from the packagemap, or else
						// the client package map will keep hard refs to them and prevent GC
						NetDriver->NotifyStreamingLevelUnload(Level);
					}

					// Broadcast level unloaded event to blueprints through level streaming objects
					ULevelStreaming::BroadcastLevelLoadedStatus(World, LevelPackage->GetFName(), false);
				}
			}

			// Make sure that this package has been unloaded after GC pass.
			LevelPackageNames.Add( LevelPackage->GetFName() );

			// Mark level as pending kill so references to it get deleted.
			Level->GetOuter()->MarkPendingKill();
			Level->MarkPendingKill();
			if (LevelPackage->MetaData)
			{
				LevelPackage->MetaData->MarkPendingKill();
			}

			// Mark all model components as pending kill so GC deletes references to them.
			for( int32 ModelComponentIndex=0; ModelComponentIndex<Level->ModelComponents.Num(); ModelComponentIndex++ )
			{
				UModelComponent* ModelComponent = Level->ModelComponents[ModelComponentIndex];
				if( ModelComponent )
				{
					ModelComponent->MarkPendingKill();
				}
			}

			// Mark all actors and their components as pending kill so GC will delete references to them.
			for( int32 ActorIndex=0; ActorIndex<Level->Actors.Num(); ActorIndex++ )
			{
				AActor* Actor = Level->Actors[ActorIndex];
				if (Actor != NULL)
				{
					Actor->MarkComponentsAsPendingKill();
					Actor->MarkPendingKill();
				}
			}
		}
	}

	LevelsPendingUnload.Empty();
}

void FLevelStreamingGCHelper::VerifyLevelsGotRemovedByGC()
{
	if( !GIsEditor )
	{
#if DO_GUARD_SLOW
		int32 FailCount = 0;
		// Iterate over all objects and find out whether they reside in a GC'ed level package.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Object = *It;
			// Check whether object's outermost is in the list.
			if( LevelPackageNames.Find( Object->GetOutermost()->GetFName() ) != INDEX_NONE
			// But disregard package object itself.
			&&	!Object->IsA(UPackage::StaticClass()) )
			{
				UE_LOG(LogWorld, Log, TEXT("%s didn't get garbage collected! Trying to find culprit, though this might crash. Try increasing stack size if it does."), *Object->GetFullName());
				StaticExec(NULL, *FString::Printf(TEXT("OBJ REFS CLASS=%s NAME=%s shortest"),*Object->GetClass()->GetName(), *Object->GetPathName()));
				TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( Object, true, GARBAGE_COLLECTION_KEEPFLAGS );
				FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, Object );
				// Print out error message. We don't assert here as there might be multiple culprits.
				UE_LOG(LogWorld, Warning, TEXT("%s didn't get garbage collected!") LINE_TERMINATOR TEXT("%s"), *Object->GetFullName(), *ErrorString );
				FailCount++;
			}
		}
		if( FailCount > 0 )
		{
			UE_LOG(LogWorld, Fatal,TEXT("Streamed out levels were not completely garbage collected! Please see previous log entries."));
		}
#endif
	}

	LevelPackageNames.Empty();
}

int32 FLevelStreamingGCHelper::GetNumLevelsPendingPurge()
{
	return LevelsPendingUnload.Num();
}

FString UWorld::ConvertToPIEPackageName(const FString& PackageName, int32 PIEInstanceID)
{
	const FString Prefix = PLAYWORLD_PACKAGE_PREFIX;
	const FString PackageAssetName = FPackageName::GetLongPackageAssetName(PackageName);
	
	if (PackageAssetName.StartsWith(Prefix))
	{
		return PackageName;
	}
	else
	{
		check(PIEInstanceID != -1);
		const FString PackageAssetPath = FPackageName::GetLongPackagePath(PackageName);
		return FString::Printf(TEXT("%s/%s_%d_%s"), *PackageAssetPath, *Prefix, PIEInstanceID, *PackageAssetName );
	}
}

UWorld* UWorld::DuplicateWorldForPIE(const FString& PackageName, UWorld* OwningWorld)
{
	// Find the original (non-PIE) level package
	UPackage* EditorLevelPackage = FindObjectFast<UPackage>( NULL, FName(*PackageName));
	if( !EditorLevelPackage )
		return NULL;

	// Find world object and use its PersistentLevel pointer.
	UWorld* EditorLevelWorld = UWorld::FindWorldInPackage(EditorLevelPackage);
	if( !EditorLevelWorld )
		return NULL;

	FWorldContext WorldContext = GEngine->WorldContextFromWorld(OwningWorld);
	GPlayInEditorID = WorldContext.PIEInstance;

	FString PrefixedLevelName = ConvertToPIEPackageName(PackageName, WorldContext.PIEInstance);
	UWorld::WorldTypePreLoadMap.FindOrAdd(FName(*PrefixedLevelName)) = EWorldType::PIE;
	UPackage* PIELevelPackage = CastChecked<UPackage>(CreatePackage(NULL,*PrefixedLevelName));
	PIELevelPackage->PackageFlags |= PKG_PlayInEditor;
#if WITH_EDITOR
	PIELevelPackage->PIEInstanceID = WorldContext.PIEInstance;
#endif

	ULevel::StreamedLevelsOwningWorld.Add(PIELevelPackage->GetFName(), OwningWorld);
	UWorld* PIELevelWorld = CastChecked<UWorld>(StaticDuplicateObject(EditorLevelWorld, PIELevelPackage, *EditorLevelWorld->GetName(), RF_AllFlags, NULL, SDO_DuplicateForPie));

	{
		ULevel* EditorLevel = EditorLevelWorld->PersistentLevel;
		ULevel* PIELevel = PIELevelWorld->PersistentLevel;
#if WITH_EDITOR
		// Fixup model components. The index buffers have been created for the components in the EditorWorld and the order
		// in which components were post-loaded matters. So don't try to guarantee a particular order here, just copy the
		// elements over.
		if (PIELevel->Model != NULL
			&& PIELevel->Model == EditorLevel->Model
			&& PIELevel->ModelComponents.Num() == EditorLevel->ModelComponents.Num() )
		{
			PIELevel->Model->ClearLocalMaterialIndexBuffersData();
			for (int32 ComponentIndex = 0; ComponentIndex < PIELevel->ModelComponents.Num(); ++ComponentIndex)
			{
				UModelComponent* SrcComponent = EditorLevel->ModelComponents[ComponentIndex];
				UModelComponent* DestComponent = PIELevel->ModelComponents[ComponentIndex];
				DestComponent->CopyElementsFrom(SrcComponent);
			}
		}
#endif
		// We have to place PIELevel at his local position in case EditorLevel was visible
		// Correct placement will occur during UWorld::AddToWorld
		if (EditorLevel->OwningWorld->WorldComposition && EditorLevel->bIsVisible)
		{
			FIntPoint LevelOffset = FIntPoint::ZeroValue - EditorLevel->OwningWorld->WorldComposition->GetLevelOffset(EditorLevel);
			PIELevel->ApplyWorldOffset(FVector(LevelOffset, 0), false);
		}
	}

	PIELevelWorld->ClearFlags(RF_Standalone);
	EditorLevelWorld->TransferBlueprintDebugReferences(PIELevelWorld);

	UE_LOG(LogWorld, Log, TEXT("PIE: Copying PIE streaming level from %s to %s. OwningWorld: %s"),
		*EditorLevelWorld->GetPathName(),
		*PIELevelWorld->GetPathName(),
		*OwningWorld->GetPathName());

	return PIELevelWorld;
}

void UWorld::UpdateLevelStreamingInner( UWorld* PersistentWorld, FSceneViewFamily* ViewFamily )
{
	// Iterate over level collection to find out whether we need to load/ unload any levels.
	for (int32 LevelIndex = 0; LevelIndex < StreamingLevels.Num(); LevelIndex++)
	{
		ULevelStreaming* StreamingLevel	= StreamingLevels[LevelIndex];
		if( !StreamingLevel )
		{
			// This can happen when manually adding a new level to the array via the property editing code.
			continue;
		}
		
		// Don't bother loading sub-levels in PIE for levels that aren't visible in editor
		if( PersistentWorld->IsPlayInEditor() && GEngine->OnlyLoadEditorVisibleLevelsInPIE() )
		{
			// create a new streaming level helper object
			if( !StreamingLevel->bShouldBeVisibleInEditor )
			{
				continue;
			}
		}

		// Work performed to make a level visible is spread across several frames and we can't unload/ hide a level that is currently pending
		// to be made visible, so we fulfill those requests first.
		bool bHasVisibilityRequestPending	= StreamingLevel->GetLoadedLevel() && StreamingLevel->GetLoadedLevel() == PersistentWorld->CurrentLevelPendingVisibility;
		
		// Figure out whether level should be loaded, visible and block on load if it should be loaded but currently isn't.
		bool bShouldBeLoaded				= bHasVisibilityRequestPending || (!GEngine->bUseBackgroundLevelStreaming && !PersistentWorld->bShouldForceUnloadStreamingLevels && !StreamingLevel->bIsRequestingUnloadAndRemoval);
		bool bShouldBeVisible				= bHasVisibilityRequestPending || PersistentWorld->bShouldForceVisibleStreamingLevels;;
		bool bShouldBlockOnLoad				= StreamingLevel->bShouldBlockOnLoad;

		// Don't update if the code requested this level object to be unloaded and removed.
		if( !PersistentWorld->bShouldForceUnloadStreamingLevels && !StreamingLevel->bIsRequestingUnloadAndRemoval )
		{
			// Take all views into account if any were passed in.
			if( ViewFamily && ViewFamily->Views.Num() > 0 )
			{
				// Iterate over all views in collection.
				for( int32 ViewIndex=0; ViewIndex<ViewFamily->Views.Num(); ViewIndex++ )
				{
					const FSceneView* View		= ViewFamily->Views[ViewIndex];
					const FVector& ViewLocation	= View->ViewMatrices.ViewOrigin;
					bShouldBeLoaded	= bShouldBeLoaded  || ( StreamingLevel && (!IsGameWorld() || StreamingLevel->ShouldBeLoaded(ViewLocation)) );
					bShouldBeVisible= bShouldBeVisible || ( bShouldBeLoaded && StreamingLevel && StreamingLevel->ShouldBeVisible(ViewLocation) );
				}
			}
			// Or default to view location of 0,0,0
			else
			{
				FVector ViewLocation(0,0,0);
				bShouldBeLoaded		= bShouldBeLoaded  || ( StreamingLevel && (!IsGameWorld() || StreamingLevel->ShouldBeLoaded(ViewLocation)) );
				bShouldBeVisible	= bShouldBeVisible || ( bShouldBeLoaded && StreamingLevel && StreamingLevel->ShouldBeVisible(ViewLocation) );
			}
		}

		// Figure out whether there are any levels we haven't collected garbage yet.
		bool bAreLevelsPendingPurge	=	FLevelStreamingGCHelper::GetNumLevelsPendingPurge() > 0;
		// We want to give the garbage collector a chance to remove levels before we stream in more. We can't do this in the
		// case of a blocking load as it means those requests should be fulfilled right away. By waiting on GC before kicking
		// off new levels we potentially delay streaming in maps, but AllowLevelLoadRequests already looks and checks whether
		// async loading in general is active. E.g. normal package streaming would delay loading in this case. This is done
		// on purpose as well so the GC code has a chance to execute between consecutive loads of maps.
		//
		// NOTE: AllowLevelLoadRequests not an invariant as streaming might affect the result, do NOT pulled out of the loop.
		bool bAllowLevelLoadRequests =	(PersistentWorld->AllowLevelLoadRequests() && !bAreLevelsPendingPurge) || bShouldBlockOnLoad;

		// Request a 'soft' GC if there are levels pending purge and there are levels to be loaded. In the case of a blocking
		// load this is going to guarantee GC firing first thing afterwards and otherwise it is going to sneak in right before
		// kicking off the async load.
		if (bAreLevelsPendingPurge)
		{
			PersistentWorld->ForceGarbageCollection( false );
		}

		// See whether level is already loaded
		if (bShouldBeLoaded)
		{
			const bool bBlockOnLoad = (!IsGameWorld() || !GEngine->bUseBackgroundLevelStreaming);
			// Try to obtain level
			StreamingLevel->RequestLevel(PersistentWorld, bAllowLevelLoadRequests, bBlockOnLoad);
		}
		
		// Cache pointer for convenience. This cannot happen before this point as e.g. flushing async loaders
		// or such will modify StreamingLevel->LoadedLevel.
		ULevel* Level = StreamingLevel->GetLoadedLevel();

		// See whether we have a loaded level.
		if (Level)
		{
			// Update loaded level visibility
			if (bShouldBeVisible)
			{
				Level->bHasShowRequest = true;

				// Add loaded level to a world if it's not there yet
				if (!Level->bIsVisible)
				{
					PersistentWorld->AddToWorld(Level, StreamingLevel->LevelTransform);
				}

				// In case we have finished making level visible
				// immediately discard previous level
				if (Level->bIsVisible)
				{
					StreamingLevel->DiscardPendingUnloadLevel(PersistentWorld);
				}
			}
			else
			{
				Level->bHasHideRequest = true;
				// Discard previous LOD level
				StreamingLevel->DiscardPendingUnloadLevel(PersistentWorld);
			}

			if (!bShouldBeLoaded)
			{
				if (!Level->bIsVisible && 
					!Level->HasVisibilityRequestPending())
				{
					StreamingLevel->ClearLoadedLevel();
					StreamingLevel->DiscardPendingUnloadLevel(PersistentWorld);
				}
			}
		}
		else
		{
			StreamingLevel->DiscardPendingUnloadLevel(PersistentWorld);
		}
								
		// If requested, remove this level from iterated over array once it is unloaded.
		if (StreamingLevel->bIsRequestingUnloadAndRemoval)
		{
			if (StreamingLevel->HasLoadedLevel() == false && 
				StreamingLevel->bHasLoadRequestPending == false)
			{
				// The -- is required as we're forward iterating over the StreamingLevels array.
				StreamingLevels.RemoveAt( LevelIndex-- );
			}
		}	
	}
}

void UWorld::UpdateLevelStreaming( FSceneViewFamily* ViewFamily )
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateLevelStreamingTime);
	// do nothing if level streaming is frozen
	if (bIsLevelStreamingFrozen)
	{
		return;
	}
	
	if (WorldComposition && ViewFamily)
	{
		// May issue the world origin shift request depending on view location
		EvaluateWorldOriginLocation(*ViewFamily);
		// May add/remove streaming objects to persistent world depending on view location
		WorldComposition->UpdateStreamingState(ViewFamily);
	}

	// Store current count of pending unload levels
	const int32 NumLevelsPendingPurge = FLevelStreamingGCHelper::GetNumLevelsPendingPurge();
	// Store current count of async loading packages
	const int32	NumAsyncLoadingPackages = GetNumAsyncPackages(); 
	
	// In the editor update only streaming levels in the persistent world
	// "Levels Browser" does not currently correctly supports multiple streaming levels referring to the same level package
	if (!IsGameWorld())
	{
		UpdateLevelStreamingInner(this, ViewFamily);
	}
	else
	{
		// Make a copy of the current levels list 
		// because levels count can be changed in the loop below
		TArray<ULevel*> VisibileLevels = Levels;
		for (int32 LevelIndex = 0; LevelIndex < VisibileLevels.Num(); ++LevelIndex)
		{
			// Update streaming objects in all visible worlds including Persistent world
			UWorld* InnerWorld = Cast<UWorld>(VisibileLevels[LevelIndex]->GetOuter());
			InnerWorld->UpdateLevelStreamingInner(this, ViewFamily);
		}
	}
	
	// Force initial loading to be "bShouldBlockOnLoad".
	const bool bLevelsHaveLoadRequestPending = NumAsyncLoadingPackages < GetNumAsyncPackages();
	if (bLevelsHaveLoadRequestPending && (!HasBegunPlay() || bWorldWasLoadedThisTick ))
	{
		// Block till all async requests are finished.
		FlushAsyncLoading( NAME_None );
	}
		
	for (int LevelIndex = Levels.Num()-1; LevelIndex >= 1; LevelIndex--)
	{
		ULevel* Level = Levels[LevelIndex];
		
		// Hide level if we have request to hide it and no request to keep it visible
		if (Level->bIsVisible &&
			Level->bHasHideRequest && !Level->bHasShowRequest)
		{
			RemoveFromWorld(Level);
		}

		// Clear visibility request flags for next update
		Level->bHasShowRequest = false;
		Level->bHasHideRequest = false;
				
		// Process abandoned levels (visible levels with no streaming level objects referring them)
		if (Level->GetStreamingLevelRefs() <= 0)
		{
			if (Level->bIsVisible)
			{
				// Hide abandoned level
				RemoveFromWorld(Level);
			}

			if (!Level->bIsVisible &&
				!Level->HasVisibilityRequestPending())
			{
				// It's safe to request unload for hidden abandoned level
				FLevelStreamingGCHelper::RequestUnload(Level);
			}
		}
	}
	
	// In case more levels has been requested to unload, force GC on next tick 
	if (NumLevelsPendingPurge < FLevelStreamingGCHelper::GetNumLevelsPendingPurge())
	{
		ForceGarbageCollection(true); 
	}
}

void UWorld::EvaluateWorldOriginLocation(const FSceneViewFamily& ViewFamily)
{
	if ( IsGameWorld() )
	{
		FVector CentroidLocation(0,0,0);
		for (int32 ViewIndex = 0; ViewIndex < ViewFamily.Views.Num(); ViewIndex++)
		{
			CentroidLocation+= ViewFamily.Views[ViewIndex]->ViewMatrices.ViewOrigin;
		}

		CentroidLocation/= ViewFamily.Views.Num();
	
		// Request to shift world in case current view is quite far from current origin
		if (CentroidLocation.Size2D() > HALF_WORLD_MAX1*0.5f)
		{
			RequestNewWorldOrigin(FIntPoint(CentroidLocation.X, CentroidLocation.Y) + GlobalOriginOffset);
		}
	}
}

void UWorld::FlushLevelStreaming( FSceneViewFamily* ViewFamily, bool bOnlyFlushVisibility, FName ExcludeType)
{
	AWorldSettings* WorldSettings = GetWorldSettings();

	// Allow queuing multiple load requests if we're performing a full flush and disallow if we're just
	// flushing level visibility.
	int32 OldAllowLevelLoadOverride = AllowLevelLoadOverride;
	AllowLevelLoadOverride = bOnlyFlushVisibility ? 0 : 1;

	// Update internals with current loaded/ visibility flags.
	UpdateLevelStreaming();

	// Only flush async loading if we're performing a full flush.
	if( !bOnlyFlushVisibility )
	{
		// Make sure all outstanding loads are taken care of, other than ones associated with the excluded type
		FlushAsyncLoading(ExcludeType);
	}

	// Kick off making levels visible if loading finished by flushing.
	UpdateLevelStreaming();

	// Making levels visible is spread across several frames so we simply loop till it is done.
	bool bLevelsPendingVisibility = true;
	while( bLevelsPendingVisibility )
	{
		bLevelsPendingVisibility = IsVisibilityRequestPending();

		// Tick level streaming to make levels visible.
		if( bLevelsPendingVisibility )
		{
			// Only flush async loading if we're performing a full flush.
			if( !bOnlyFlushVisibility )
			{
				// Make sure all outstanding loads are taken care of...
				FlushAsyncLoading(NAME_None);
			}
	
			// Update level streaming.
			UpdateLevelStreaming( ViewFamily );
		}
	}
	
	// Update level streaming one last time to make sure all RemoveFromWorld requests made it.
	UpdateLevelStreaming( ViewFamily );

	// we need this, or the traces will be abysmally slow
	EnsureCollisionTreeIsBuilt();

	// We already blocked on async loading.
	if( !bOnlyFlushVisibility )
	{
		bRequestedBlockOnAsyncLoading = false;
	}

	AllowLevelLoadOverride = OldAllowLevelLoadOverride;
}

/**
 * Forces streaming data to be rebuilt for the current world.
 */
static void ForceBuildStreamingData()
{
	for (TObjectIterator<UWorld> ObjIt;  ObjIt; ++ObjIt)
	{
		UWorld* WorldComp = Cast<UWorld>(*ObjIt);
		if (WorldComp && WorldComp->PersistentLevel && WorldComp->PersistentLevel->OwningWorld == WorldComp)
		{
			ULevel::BuildStreamingData(WorldComp);
		}		
	}
}

static FAutoConsoleCommand ForceBuildStreamingDataCmd(
	TEXT("ForceBuildStreamingData"),
	TEXT("Forces streaming data to be rebuilt for the current world."),
	FConsoleCommandDelegate::CreateStatic(ForceBuildStreamingData)
	);


void UWorld::TriggerStreamingDataRebuild()
{
	bStreamingDataDirty = true;
	BuildStreamingDataTimer = FPlatformTime::Seconds() + 5.0;
}


void UWorld::ConditionallyBuildStreamingData()
{
	if ( bStreamingDataDirty && FPlatformTime::Seconds() > BuildStreamingDataTimer )
	{
		bStreamingDataDirty = false;
		ULevel::BuildStreamingData( this );
	}
}

bool UWorld::IsVisibilityRequestPending() const
{
	return (CurrentLevelPendingVisibility != NULL);
}

bool UWorld::AreAlwaysLoadedLevelsLoaded() const
{
	for (int32 LevelIndex = 0; LevelIndex < StreamingLevels.Num(); LevelIndex++)
	{
		ULevelStreaming* LevelStreaming = StreamingLevels[LevelIndex];

		// See whether there's a level with a pending request.
		if (LevelStreaming != NULL && LevelStreaming->ShouldBeAlwaysLoaded())
		{	
			const ULevel* LoadedLevel = LevelStreaming->GetLoadedLevel();

			if (LevelStreaming->bHasLoadRequestPending
				|| !LoadedLevel
				|| !LoadedLevel->bIsVisible)
			{
				return false;
			}
		}
	}

	return true;
}


bool UWorld::AllowLevelLoadRequests()
{
	bool bAllowLevelLoadRequests = false;
	// Hold off requesting new loads if there is an active async load request.
	if( !GIsEditor )
	{
		// Let code choose.
		if( AllowLevelLoadOverride == 0 )
		{
			// There are pending queued requests and gameplay has already started so hold off queueing.
			if( IsAsyncLoading() && GetTimeSeconds() > 1.f )
			{
				bAllowLevelLoadRequests = false;
			}
			// No load requests or initial load so it's save to queue.
			else
			{
				bAllowLevelLoadRequests = true;
			}
		}
		// Use override, < 0 == don't allow, > 0 == allow
		else
		{
			bAllowLevelLoadRequests = AllowLevelLoadOverride > 0 ? true : false;
		}
	}
	// Always allow load request in the Editor.
	else
	{
		bAllowLevelLoadRequests = true;
	}
	return bAllowLevelLoadRequests;
}

bool UWorld::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( FParse::Command( &Cmd, TEXT("TRACETAG") ) )
	{
		return HandleTraceTagCommand( Cmd, Ar );
	}
	else if( FParse::Command( &Cmd, TEXT("FLUSHPERSISTENTDEBUGLINES") ) )
	{		
		return HandleFlushPersistentDebugLinesCommand( Cmd, Ar );
	}
	else if (FParse::Command(&Cmd, TEXT("LOGACTORCOUNTS")))
	{		
		return HandleLogActorCountsCommand( Cmd, Ar, InWorld );
	}
	else if( ExecPhysCommands( Cmd, &Ar, InWorld ) )
	{
		return HandleLogActorCountsCommand( Cmd, Ar, InWorld );
	}
	else 
	{
		return 0;
	}
}

bool UWorld::HandleTraceTagCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	FString TagStr;
	FParse::Token(Cmd, TagStr, 0);
	DebugDrawTraceTag = FName(*TagStr);
	return true;
}

bool UWorld::HandleFlushPersistentDebugLinesCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	PersistentLineBatcher->Flush();
	return true;
}

bool UWorld::HandleLogActorCountsCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld )
{
	Ar.Logf(TEXT("Num Actors: %i"), InWorld->GetActorCount());
	return true;
}


bool UWorld::SetGameMode(const FURL& InURL)
{
	if( IsServer() && !AuthorityGameMode )
	{
		// Init the game info.
		FString Options(TEXT(""));
		TCHAR GameParam[256]=TEXT("");
		FString	Error=TEXT("");
		AWorldSettings* Settings = GetWorldSettings();		
		for( int32 i=0; i<InURL.Op.Num(); i++ )
		{
			Options += TEXT("?");
			Options += InURL.Op[i];
			FParse::Value( *InURL.Op[i], TEXT("GAME="), GameParam, ARRAY_COUNT(GameParam) );
		}

		UGameEngine* const GameEngine = Cast<UGameEngine>(GEngine);

		// Get the GameMode class. Start by using the default game type specified in the map's worldsettings.  It may be overridden by settings below.
		UClass* GameClass = Settings->DefaultGameMode;

		// If there is a GameMode parameter in the URL, allow it to override the default game type
		if ( GameParam[0] )
		{
			FString const GameClassName = AGameMode::StaticGetFullGameClassName(FString(GameParam));

			// if the gamename was specified, we can use it to fully load the pergame PreLoadClass packages
			if (GameEngine)
			{
				GameEngine->LoadPackagesFully(this, FULLYLOAD_Game_PreLoadClass, *GameClassName);
			}

			GameClass = StaticLoadClass(AGameMode::StaticClass(), NULL, *GameClassName, NULL, LOAD_None, NULL);
		}

		if (!GameClass)
		{
			FString MapName = InURL.Map;
			FString MapNameNoPath = FPaths::GetBaseFilename(MapName);
			if (MapNameNoPath.StartsWith(PLAYWORLD_PACKAGE_PREFIX))
			{
				const int32 PrefixLen = FString(PLAYWORLD_PACKAGE_PREFIX).Len();
				MapNameNoPath = MapNameNoPath.Mid(PrefixLen);
			}
			
			// see if we have a per-prefix default specified
			for ( int32 Idx=0; Idx<Settings->DefaultMapPrefixes.Num(); Idx++ )
			{
				const FGameModePrefix& GTPrefix = Settings->DefaultMapPrefixes[Idx];
				if ( (GTPrefix.Prefix.Len() > 0) && MapNameNoPath.StartsWith(GTPrefix.Prefix) )
				{
					GameClass = StaticLoadClass(AGameMode::StaticClass(), NULL, *GTPrefix.GameMode, NULL, LOAD_None, NULL) ;
					if (GameClass)
					{
						// found a good class for this prefix, done
						break;
					}
				}
			}
		}

		if ( !GameClass )
		{
			if( GIsAutomationTesting )
			{
				// fall back to raw GameMode if Automation Testing, as the shared engine maps were not designed to use what could be any developer default GameMode
				GameClass = AGameMode::StaticClass();
			}
			else
			{
				// fall back to overall default game type
				GameClass = StaticLoadClass(AGameMode::StaticClass(), NULL, *UGameMapsSettings::GetGlobalDefaultGameMode(), NULL, LOAD_None, NULL);
			}
		}

		if ( !GameClass ) 
		{
			// fall back to raw GameMode
			GameClass = AGameMode::StaticClass();
		}
		else
		{
			// Remove any directory path from the map for the purpose of setting the game type
			GameClass = GameClass->GetDefaultObject<AGameMode>()->SetGameMode(FPaths::GetBaseFilename(InURL.Map), Options, *InURL.Portal);
		}

		// no matter how the game was specified, we can use it to load the PostLoadClass packages
		if (GameEngine)
		{
			GameEngine->LoadPackagesFully(this, FULLYLOAD_Game_PostLoadClass, GameClass->GetPathName());
			GameEngine->LoadPackagesFully(this, FULLYLOAD_Game_PostLoadClass, TEXT("LoadForAllGameModes"));
		}

		// Spawn the GameMode.
		UE_LOG(LogWorld, Log,  TEXT("Game class is '%s'"), *GameClass->GetName() );
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.bNoCollisionFail = true;
		AuthorityGameMode = SpawnActor<AGameMode>( GameClass, SpawnInfo );
		
		if( AuthorityGameMode != NULL )
		{
			return true;
		}
		else
		{
			UE_LOG(LogWorld, Error, TEXT("Failed to spawn GameMode actor."));
			return false;
		}
	}

	return false;
}


void UWorld::BeginPlay(const FURL& InURL, bool bResetTime)
{
	check(bIsWorldInitialized);
	double StartTime = FPlatformTime::Seconds();

	// Don't reset time for seamless world transitions.
	if (bResetTime)
	{
		TimeSeconds = 0.0f;
		RealTimeSeconds = 0.0f;
		AudioTimeSeconds = 0.0f;
	}

	// Get URL Options
	FString Options(TEXT(""));
	FString	Error(TEXT(""));
	for( int32 i=0; i<InURL.Op.Num(); i++ )
	{
		Options += TEXT("?");
		Options += InURL.Op[i];
	}

	// Set level info.
	if( !InURL.GetOption(TEXT("load"),NULL) )
	{
		URL = InURL;
	}

	// Update world and the components of all levels.	
	// We don't need to rerun construction scripts if we have cooked data or we are playing in editor unless the PIE world was loaded
	// from disk rather than duplicated
	const bool bRerunConstructionScript = !FPlatformProperties::RequiresCookedData() && (!IsPlayInEditor() || HasAnyFlags(RF_WasLoaded));
	UpdateWorldComponents( bRerunConstructionScript, true );

	// Reset indices till we have a chance to rearrange actor list at the end of this function.
	for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels[LevelIndex];
		Level->iFirstNetRelevantActor	= 0;
	}

	// Init level gameplay info.
	if( !HasBegunPlay() )
	{
		// Check that paths are valid
		if ( !IsNavigationRebuilt() )
		{
			UE_LOG(LogWorld, Warning, TEXT("*** WARNING - PATHS MAY NOT BE VALID ***"));
		}

		// Lock the level.
		if(WorldType == EWorldType::Preview)
		{
			UE_LOG(LogWorld, Verbose,  TEXT("Bringing preview %s up for play (max tick rate %i) at %s"), *GetFullName(), FMath::Round(GEngine->GetMaxTickRate(0,false)), *FDateTime::Now().ToString() );
		}
		else
		{
			UE_LOG(LogWorld, Log,  TEXT("Bringing %s up for play (max tick rate %i) at %s"), *GetFullName(), FMath::Round(GEngine->GetMaxTickRate(0,false)), *FDateTime::Now().ToString() );
		}

		// Initialize all actors and start execution.
		for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel*	const Level = Levels[LevelIndex];
			Level->InitializeActors();
		}

		// Enable actor script calls.
		bStartup	= 1;
		bBegunPlay	= 1;

		// Spawn serveractors
		ENetMode CurNetMode = GEngine != NULL ? GEngine->GetNetMode(this) : NM_Standalone;

		if (CurNetMode == NM_ListenServer || CurNetMode == NM_DedicatedServer)
		{
			GEngine->SpawnServerActors(this);
		}

		// Init the game mode.
		if (AuthorityGameMode && !AuthorityGameMode->bActorInitialized)
		{
			AuthorityGameMode->InitGame( FPaths::GetBaseFilename(InURL.Map), Options, Error );
		}

		// Route various begin play functions and set volumes.
		for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel*	const Level = Levels[LevelIndex];
			Level->RouteActorInitialize();
		}

		bStartup = 0;
	}

	// Rearrange actors: static not net relevant actors first, then static net relevant actors and then others.
	check( Levels.Num() );
	check( PersistentLevel );
	check( Levels[0] == PersistentLevel );
	for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel*	Level = Levels[LevelIndex];
		Level->SortActorList();
	}

	// update the auto-complete list for the console
	UConsole* ViewportConsole = (GEngine->GameViewport != NULL) ? GEngine->GameViewport->ViewportConsole : NULL;
	if (ViewportConsole != NULL)
	{
		ViewportConsole->BuildRuntimeAutoCompleteList();
	}

	// let all subsystems/managers know:
	// @note if UWorld starts to host more of these it might a be a good idea to create a generic IManagerInterface of some sort
	if (NavigationSystem != NULL)
	{
		NavigationSystem->OnBeginPlay();
	}

	for( int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel*	Level = Levels[LevelIndex];
		GStreamingManager->AddLevel( Level );
	}

	if(WorldType == EWorldType::Preview)
	{
		UE_LOG(LogWorld, Verbose, TEXT("Bringing up preview level for play took: %f"), FPlatformTime::Seconds() - StartTime );
	}
	else
	{
		UE_LOG(LogWorld, Log, TEXT("Bringing up level for play took: %f"), FPlatformTime::Seconds() - StartTime );
	}
}



bool UWorld::IsNavigationRebuilt() const
{
	return GetNavigationSystem() == NULL || GetNavigationSystem()->IsNavigationBuilt(GetWorldSettings());
}

void UWorld::CleanupWorld(bool bSessionEnded, bool bCleanupResources)
{
	check(IsVisibilityRequestPending() == false);

	FWorldDelegates::OnWorldCleanup.Broadcast(this, bSessionEnded, bCleanupResources);

	if (NavigationSystem != NULL && bCleanupResources == true)
	{
		NavigationSystem->CleanUp();
	}

	NetworkActors.Empty();

#if WITH_EDITORONLY_DATA
	// If we're server traveling, we need to break the reference dependency here (caused by levelscript)
	// to avoid a GC crash for not cleaning up the gameinfo referenced by levelscript
	if( IsGameWorld() && !GIsEditor && !IsRunningCommandlet() && bSessionEnded && bCleanupResources )
	{
		if( ULevelScriptBlueprint* LSBP = PersistentLevel->GetLevelScriptBlueprint(true) )
		{
			if( LSBP->SkeletonGeneratedClass )
			{
				LSBP->SkeletonGeneratedClass->ClassGeneratedBy = NULL; 
			}

			if( LSBP->GeneratedClass )
			{
				LSBP->GeneratedClass->ClassGeneratedBy = NULL; 
			}
		}
	}
#endif //WITH_EDITORONLY_DATA

#if ENABLE_VISUAL_LOG
	FVisualLog::Get()->Cleanup();
#endif // ENABLE_VISUAL_LOG	

	// Tell actors to remove their components from the scene.
	ClearWorldComponents();

	if (bCleanupResources && PersistentLevel)
	{
		PersistentLevel->ReleaseRenderingResources();
	}

	// Clear standalone flag when switching maps in the Editor. This causes resources placed in the map
	// package to be garbage collected together with the world.
	if( GIsEditor && !IsTemplate() )
	{
		TArray<UObject*> WorldObjects;

		// Iterate over all objects to find ones that reside in the same package as the world.
		GetObjectsWithOuter(GetOutermost(), WorldObjects);
		for( int32 ObjectIndex=0; ObjectIndex < WorldObjects.Num(); ++ObjectIndex )
		{
			UObject* CurrentObject = WorldObjects[ObjectIndex];
			if ( CurrentObject != this )
			{
				CurrentObject->ClearFlags( RF_Standalone );
			}
		}
	}

	for (int32 LevelIndex=0; LevelIndex < GetNumLevels(); ++LevelIndex)
	{
		UWorld* World = CastChecked<UWorld>(GetLevel(LevelIndex)->GetOuter());
		if (World != this)
		{
			World->CleanupWorld(bSessionEnded, bCleanupResources);
		}
	}
}

FConstControllerIterator UWorld::GetControllerIterator() const
{
	return ControllerList.CreateConstIterator();
}

FConstPlayerControllerIterator UWorld::GetPlayerControllerIterator() const
{
	return PlayerControllerList.CreateConstIterator();
}

APlayerController* UWorld::GetFirstPlayerController() const
{
	APlayerController* PlayerController = NULL;
	if( GetPlayerControllerIterator() )
	{
		PlayerController = *GetPlayerControllerIterator();
	}
	return PlayerController;
}

ULocalPlayer* UWorld::GetFirstLocalPlayerFromController() const
{
	for( FConstPlayerControllerIterator Iterator = GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;
		if( PlayerController )
		{
			ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerController->Player);
			if( LocalPlayer )
			{
				return LocalPlayer;
			}
		}
	}
	return NULL;
}

void UWorld::AddController( AController* Controller )
{	
	check( Controller );
	ControllerList.AddUnique( Controller );	
	if( Cast<APlayerController>(Controller) )
	{
		PlayerControllerList.AddUnique( Cast<APlayerController>(Controller) );
	}
}


void UWorld::RemoveController( AController* Controller )
{
	check( Controller );
	if( ControllerList.Remove( Controller ) > 0 )
	{
		if( Cast<APlayerController>(Controller) )
		{
			PlayerControllerList.Remove( Cast<APlayerController>(Controller) );
		}
	}
}

FConstPawnIterator UWorld::GetPawnIterator() const
{
	return PawnList.CreateConstIterator();
}

void UWorld::AddPawn( APawn* Pawn )
{
	check( Pawn );
	PawnList.AddUnique( Pawn );
}


void UWorld::RemovePawn( APawn* Pawn )
{
	check( Pawn );
	
	AController* Controller = Pawn->GetController();
	if (Controller)
	{
		Controller->UnPossess();
	}

	PawnList.Remove( Pawn );	
}

void UWorld::AddNetworkActor( AActor * Actor )
{
	if ( Actor == NULL )
	{
		return;
	}

	if ( Actor->IsPendingKill() ) 
	{
		return;
	}

#if 0
	if ( ( !NetDriver || NetDriver->ServerConnection ) && !GIsEditor )
	{
		return;		// Clients or standalone games don't use this list
	}
#endif

	NetworkActors.AddUnique( Actor );
}

void UWorld::RemoveNetworkActor( AActor * Actor )
{
	if ( Actor == NULL )
	{
		return;
	}

	NetworkActors.RemoveSingleSwap( Actor );
}

void UWorld::AddOnActorSpawnedHandler( const FOnActorSpawned::FDelegate& InHandler )
{
	OnActorSpawned.Add(InHandler);
}

void UWorld::RemoveOnActorSpawnedHandler( const FOnActorSpawned::FDelegate& InHandler )
{
	OnActorSpawned.Remove(InHandler);
}

ABrush* UWorld::GetBrush() const
{
	check(PersistentLevel);
	return PersistentLevel->GetBrush();
}


bool UWorld::HasBegunPlay() const
{
	return PersistentLevel && PersistentLevel->Actors.Num() && bBegunPlay;
}


float UWorld::GetTimeSeconds() const
{
	return TimeSeconds;
}


float UWorld::GetRealTimeSeconds() const
{
	checkSlow(IsInGameThread());
	return RealTimeSeconds;
}


float UWorld::GetAudioTimeSeconds() const
{
	return AudioTimeSeconds;
}


float UWorld::GetDeltaSeconds() const
{
	return DeltaTimeSeconds;
}

float UWorld::TimeSince( float Time ) const
{
	return GetTimeSeconds() - Time;
}

void UWorld::SetPhysicsScene(FPhysScene* InScene)
{ 
	// Clear world pointer in old FPhysScene (if there is one)
	if(PhysicsScene != NULL)
	{
		PhysicsScene->OwningWorld = NULL;
	}

	// Assign pointer
	PhysicsScene = InScene;

	// Set pointer in scene to know which world its coming from
	if(PhysicsScene != NULL)
	{
		PhysicsScene->OwningWorld = this;
	}
}


APhysicsVolume* UWorld::GetDefaultPhysicsVolume() const
{
	return DefaultPhysicsVolume;
}


ALevelScriptActor* UWorld::GetLevelScriptActor( ULevel* OwnerLevel ) const
{
	if( OwnerLevel == NULL )
	{
		OwnerLevel = CurrentLevel;
	}
	check(OwnerLevel);
	return OwnerLevel->GetLevelScriptActor();
}


AWorldSettings* UWorld::GetWorldSettings( bool bCheckStreamingPesistent, bool bChecked ) const
{
	checkSlow(IsInGameThread());
	AWorldSettings* WorldSettings = NULL;
	if (PersistentLevel)
	{
		if (bChecked)
		{
			checkSlow(PersistentLevel->Actors.Num());
			checkSlow(PersistentLevel->Actors[0]);
			checkSlow(PersistentLevel->Actors[0]->IsA(AWorldSettings::StaticClass()));

			WorldSettings = (AWorldSettings*)PersistentLevel->Actors[0];
		}
		else if (PersistentLevel->Actors.Num() > 0)
		{
			WorldSettings = Cast<AWorldSettings>(PersistentLevel->Actors[0]);
		}

		if( bCheckStreamingPesistent )
		{
			if( StreamingLevels.Num() > 0 &&
				StreamingLevels[0] &&
				StreamingLevels[0]->IsA(ULevelStreamingPersistent::StaticClass()) )
			{
				ULevel* Level = StreamingLevels[0]->GetLoadedLevel();
				if (Level != NULL)
				{
					WorldSettings = Level->GetWorldSettings();
				}
			}
		}
	}
	return WorldSettings;
}


UModel* UWorld::GetModel() const
{
	check(CurrentLevel);
	return CurrentLevel->Model;
}


float UWorld::GetGravityZ() const
{
	AWorldSettings* WorldSettings = GetWorldSettings();
	return (WorldSettings != NULL) ? WorldSettings->GetGravityZ() : 0.f;
}


float UWorld::GetDefaultGravityZ() const
{
	AWorldSettings* WorldSettings = GetWorldSettings();
	return (WorldSettings != NULL) ? WorldSettings->DefaultGravityZ : 0.f;
}

/** This is our global function for retrieving the current MapName **/
ENGINE_API const FString GetMapNameStatic()
{
	FString Retval;

	FWorldContext const* ContextToUse = NULL;
	if ( GEngine )
	{
		const TArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();

		// We're going to look through the WorldContexts and pull any Game context we find
		// If there isn't a Game context, we'll take the first PIE we find
		// and if none of those we'll use an Editor
		for (int32 WorldIndex = 0; WorldIndex < WorldContexts.Num(); ++WorldIndex)
		{
			const FWorldContext& WorldContext = WorldContexts[WorldIndex];

			if (WorldContext.WorldType == EWorldType::Game)
			{
				ContextToUse = &WorldContext;
				break;
			}
			else if (WorldContext.WorldType == EWorldType::PIE && (ContextToUse == NULL || ContextToUse->WorldType != EWorldType::PIE))
			{
				ContextToUse = &WorldContext;
			}
			else if (WorldContext.WorldType == EWorldType::Editor && ContextToUse == NULL)
			{	
				ContextToUse = &WorldContext;
			}
		}
	}

	if( ContextToUse != NULL )
	{
		Retval = ContextToUse->World()->GetMapName();
	}
	else if ( Internal::GObjInitialized )
	{
		Retval = appGetStartupMap( FCommandLine::Get() );
	}

	return Retval;
}

const FString UWorld::GetMapName() const
{
	// Default to the world's package as the map name.
	FString MapName = GetOutermost()->GetName();
	
	// In the case of a seamless world check to see whether there are any persistent levels in the levels
	// array and use its name if there is one.
	for( int32 LevelIndex=0; LevelIndex<StreamingLevels.Num(); LevelIndex++ )
	{
		ULevelStreamingPersistent* PersistentStreamingLevel = Cast<ULevelStreamingPersistent>( StreamingLevels[LevelIndex] );
		// Use the name of the first found persistent level.
		if( PersistentStreamingLevel )
		{
			MapName = PersistentStreamingLevel->PackageName.ToString();
			break;
		}
	}

	// Just return the name of the map, not the rest of the path
	MapName = FPackageName::GetLongPackageAssetName(MapName);

	return MapName;
}

EAcceptConnection::Type UWorld::NotifyAcceptingConnection()
{
	check(NetDriver);
	if( NetDriver->ServerConnection )
	{
		// We are a client and we don't welcome incoming connections.
		UE_LOG(LogNet, Log, TEXT("NotifyAcceptingConnection: Client refused") );
		return EAcceptConnection::Reject;
	}
	else if( NextURL!=TEXT("") )
	{
		// Server is switching levels.
		UE_LOG(LogNet, Log, TEXT("NotifyAcceptingConnection: Server %s refused"), *GetName() );
		return EAcceptConnection::Ignore;
	}
	else
	{
		// Server is up and running.
		UE_LOG(LogNet, Log, TEXT("NotifyAcceptingConnection: Server %s accept"), *GetName() );
		return EAcceptConnection::Accept;
	}
}

void UWorld::NotifyAcceptedConnection( UNetConnection* Connection )
{
	check(NetDriver!=NULL);
	check(NetDriver->ServerConnection==NULL);
	UE_LOG(LogNet, Log, TEXT("Open %s %s %s"), *GetName(), FPlatformTime::StrTimestamp(), *Connection->LowLevelGetRemoteAddress() );
	NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("OPEN"), *(GetName() + TEXT(" ") + Connection->LowLevelGetRemoteAddress())));
}

bool UWorld::NotifyAcceptingChannel( UChannel* Channel )
{
	check(Channel);
	check(Channel->Connection);
	check(Channel->Connection->Driver);
	UNetDriver* Driver = Channel->Connection->Driver;

	if( Driver->ServerConnection )
	{
		// We are a client and the server has just opened up a new channel.
		//UE_LOG(LogWorld, Log,  "NotifyAcceptingChannel %i/%i client %s", Channel->ChIndex, Channel->ChType, *GetName() );
		if( Channel->ChType==CHTYPE_Actor )
		{
			// Actor channel.
			//UE_LOG(LogWorld, Log,  "Client accepting actor channel" );
			return 1;
		}
		else
		{
			// Unwanted channel type.
			UE_LOG(LogNet, Log, TEXT("Client refusing unwanted channel of type %i"), (uint8)Channel->ChType );
			return 0;
		}
	}
	else
	{
		// We are the server.
		if( Channel->ChIndex==0 && Channel->ChType==CHTYPE_Control )
		{
			// The client has opened initial channel.
			UE_LOG(LogNet, Log, TEXT("NotifyAcceptingChannel Control %i server %s: Accepted"), Channel->ChIndex, *GetFullName() );
			return 1;
		}
		else if( Channel->ChType==CHTYPE_File )
		{
			// The client is going to request a file.
			UE_LOG(LogNet, Log, TEXT("NotifyAcceptingChannel File %i server %s: Accepted"), Channel->ChIndex, *GetFullName() );
			return 1;
		}
		else
		{
			// Client can't open any other kinds of channels.
			UE_LOG(LogNet, Log, TEXT("NotifyAcceptingChannel %i %i server %s: Refused"), (uint8)Channel->ChType, Channel->ChIndex, *GetFullName() );
			return 0;
		}
	}
}

void UWorld::WelcomePlayer(UNetConnection* Connection)
{
	check(CurrentLevel);
	Connection->SendPackageMap();
	
	FString LevelName = CurrentLevel->GetOutermost()->GetName();
	if (WorldComposition != NULL)
	{
		LevelName+= TEXT("?worldcomposition");
	}

	Connection->ClientWorldPackageName = CurrentLevel->GetOutermost()->GetFName();

	FString GameName;
	if (AuthorityGameMode != NULL)
	{
		GameName = AuthorityGameMode->GetClass()->GetPathName();
	}

	FNetControlMessage<NMT_Welcome>::Send(Connection, LevelName, GameName);
	Connection->FlushNet();
	// don't count initial join data for netspeed throttling
	// as it's unnecessary, since connection won't be fully open until it all gets received, and this prevents later gameplay data from being delayed to "catch up"
	Connection->QueuedBytes = 0;
	Connection->SetClientLoginState( EClientLoginState::Welcomed );		// Client is fully logged in
}

bool UWorld::DestroySwappedPC(UNetConnection* Connection)
{
	for( FConstPlayerControllerIterator Iterator = GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;
		if (PlayerController->Player == NULL && PlayerController->PendingSwapConnection == Connection)
		{
			DestroyActor(PlayerController);
			return true;
		}
	}

	return false;
}

void UWorld::NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, class FInBunch& Bunch)
{
	if( NetDriver->ServerConnection )
	{
		check(Connection == NetDriver->ServerConnection);

		// We are the client, traveling to a new map with the same server
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		UE_LOG(LogNet, Verbose, TEXT("Level client received: %s"), FNetControlMessageInfo::GetName(MessageType));
#endif
		switch (MessageType)
		{
			case NMT_Failure:
			{
				// our connection attempt failed for some reason, for example a synchronization mismatch (bad GUID, etc) or because the server rejected our join attempt (too many players, etc)
				// here we can further parse the string to determine the reason that the server closed our connection and present it to the user
				FString EntryURL = TEXT("?failed");

				FString ErrorMsg;
				FNetControlMessage<NMT_Failure>::Receive(Bunch, ErrorMsg);
				if (ErrorMsg.IsEmpty())
				{
					ErrorMsg = NSLOCTEXT("NetworkErrors", "GenericConnectionFailed", "Connection Failed.").ToString();
				}

				GEngine->BroadcastNetworkFailure(this, NetDriver, ENetworkFailure::FailureReceived, ErrorMsg);
				if (Connection)
				{
					Connection->Close();
				}
				break;
			}
			case NMT_Uses:
			{
				// Dependency information.
				FPackageInfo Info(NULL);
				Connection->ParsePackageInfo(Bunch, Info);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				UE_LOG(LogNet, Verbose, TEXT(" ---> Package: %s, GUID: %s, Generation: %i, BasePkg: %s"), *Info.PackageName.ToString(), *Info.Guid.ToString(), Info.RemoteGeneration, *Info.ForcedExportBasePackageName.ToString());
#endif
				// it's important to verify packages in order so that we're not reshuffling replicated indices during gameplay, so don't try if there are already others pending
				if (Connection->PendingPackageInfos.Num() > 0 || !NetDriver->VerifyPackageInfo(Info))
				{
					// we can't verify the package until we have finished async loading
					Connection->PendingPackageInfos.Add(Info);
				}
				break;
			}
			case NMT_Unload:
			{
				break;
			}
			case NMT_DebugText:
			{
				// debug text message
				FString Text;
				FNetControlMessage<NMT_DebugText>::Receive(Bunch,Text);

				UE_LOG(LogNet, Log, TEXT("%s received NMT_DebugText Text=[%s] Desc=%s DescRemote=%s"),
					*Connection->Driver->GetDescription(),*Text,*Connection->LowLevelDescribe(),*Connection->LowLevelGetRemoteAddress());

				break;
			}
			case NMT_NetGUIDAssign:
			{
				FNetworkGUID NetGUID;
				FString Path;
				FNetControlMessage<NMT_NetGUIDAssign>::Receive(Bunch, NetGUID, Path);

				UE_LOG(LogNet, Verbose, TEXT("NMT_NetGUIDAssign  NetGUID %s. Path: %s. "), *NetGUID.ToString(), *Path );
				Connection->PackageMap->NetGUIDAssign(NetGUID, Path, NULL);
				break;
			}
		}
	}
	else
	{
		// We are the server.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		UE_LOG(LogNet, Verbose, TEXT("Level server received: %s"), FNetControlMessageInfo::GetName(MessageType));
#endif
		if ( !Connection->IsClientMsgTypeValid( MessageType ) )
		{
			// If we get here, either code is mismatched on the client side, or someone could be spoofing the client address
			UE_LOG( LogNet, Error, TEXT( "IsClientMsgTypeValid FAILED (%i): Remote Address = %s" ), (int)MessageType, *Connection->LowLevelGetRemoteAddress() );
			Bunch.SetError();
			return;
		}
		
		switch (MessageType)
		{
			case NMT_Hello:
			{
				uint8 IsLittleEndian;
				int32 RemoteMinVer, RemoteVer;
				FGuid RemoteGameGUID;
				FNetControlMessage<NMT_Hello>::Receive(Bunch, IsLittleEndian, RemoteMinVer, RemoteVer, RemoteGameGUID);

				if (!IsNetworkCompatible(Connection->Driver->RequireEngineVersionMatch, RemoteVer, RemoteMinVer))
				{
					UE_LOG(LogNet, Log, TEXT("Requesting client to upgrade or downgrade to GEngineMinNetVersion=%d, GEngineNetVersion=%d"), GEngineMinNetVersion, GEngineNetVersion);
					FNetControlMessage<NMT_Upgrade>::Send(Connection, GEngineMinNetVersion, GEngineNetVersion);
					Connection->FlushNet(true);
					Connection->Close();
				}
				else
				{
					Connection->NegotiatedVer = FMath::Min(RemoteVer, GEngineNetVersion);

					// Make sure the server has the same GameGUID as we do
					if( RemoteGameGUID != GetDefault<UGeneralProjectSettings>()->ProjectID )
					{
						FString ErrorMsg = NSLOCTEXT("NetworkErrors", "ServerHostingDifferentGame", "Incompatible game connection.").ToString();
						FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
						Connection->FlushNet(true);
						Connection->Close();
						break;
					}

					Connection->Challenge = FString::Printf(TEXT("%08X"), FPlatformTime::Cycles());
					Connection->SetExpectedClientLoginMsgType( NMT_Login );
					FNetControlMessage<NMT_Challenge>::Send(Connection, Connection->NegotiatedVer, Connection->Challenge);
					Connection->FlushNet();
				}
				break;
			}

			case NMT_Netspeed:
			{
				int32 Rate;
				FNetControlMessage<NMT_Netspeed>::Receive(Bunch, Rate);
				Connection->CurrentNetSpeed = FMath::Clamp(Rate, 1800, NetDriver->MaxClientRate);
				UE_LOG(LogNet, Log, TEXT("Client netspeed is %i"), Connection->CurrentNetSpeed);
				break;
			}
			case NMT_Have:
			{
				// Client specifying his generation.
				FGuid Guid;
				int32 RemoteGeneration;
				FNetControlMessage<NMT_Have>::Receive(Bunch, Guid, RemoteGeneration);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				UE_LOG(LogNet, Verbose, TEXT(" ---> GUID: %s, Generation: %i"), *Guid.ToString(), RemoteGeneration);
#endif
				bool bFound = true;
				if (!bFound)
				{
					UE_LOG(LogNet, Log, TEXT("NOT FOUND GUID: %s, Generation: %i"), *Guid.ToString(), RemoteGeneration);
					// the client specified a package with an incorrect GUID or one that it should not be using; kick it out
					FString ErrorMsg = NSLOCTEXT("NetworkErrors", "IncompatiblePackage", "Incompatible package.").ToString();
					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
					Connection->Close();
				}
				break;
			}
			case NMT_Abort:
			{
				break;
			}
			case NMT_Skip:
			{
				
				break;
			}
			case NMT_Login:
			{
				FUniqueNetIdRepl UniqueIdRepl;

				// Admit or deny the player here.
				FNetControlMessage<NMT_Login>::Receive(Bunch, Connection->ClientResponse, Connection->RequestURL, UniqueIdRepl);
				UE_LOG(LogNet, Log, TEXT("Login request: %s userId: %s"), *Connection->RequestURL, UniqueIdRepl.IsValid() ? *UniqueIdRepl->ToString() : TEXT("Invalid"));


				// Compromise for passing splitscreen playercount through to gameplay login code,
				// without adding a lot of extra unnecessary complexity throughout the login code.
				// NOTE: This code differs from NMT_JoinSplit, by counting + 1 for SplitscreenCount
				//			(since this is the primary connection, not counted in Children)
				FURL InURL( NULL, *Connection->RequestURL, TRAVEL_Absolute );

				if ( !InURL.Valid )
				{
					UE_LOG( LogNet, Error, TEXT( "NMT_Login: Invalid URL %s" ), *Connection->RequestURL );
					Bunch.SetError();
					break;
				}

				uint8 SplitscreenCount = FMath::Min(Connection->Children.Num() + 1, 255);

				// Don't allow clients to specify this value
				InURL.RemoveOption(TEXT("SplitscreenCount"));
				InURL.AddOption(*FString::Printf(TEXT("SplitscreenCount=%i"), SplitscreenCount));

				Connection->RequestURL = InURL.ToString();


				// skip to the first option in the URL
				const TCHAR* Tmp = *Connection->RequestURL;
				for (; *Tmp && *Tmp != '?'; Tmp++);


				// keep track of net id for player associated with remote connection
				Connection->PlayerId = UniqueIdRepl.GetUniqueNetId();

				// ask the game code if this player can join
				FString ErrorMsg;
				AuthorityGameMode->PreLogin( Tmp, Connection->LowLevelGetRemoteAddress(), Connection->PlayerId, ErrorMsg);
				if (!ErrorMsg.IsEmpty())
				{
					UE_LOG(LogNet, Log, TEXT("PreLogin failure: %s"), *ErrorMsg);
					NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("RRELOGIN FAILURE"), *ErrorMsg));
					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
					Connection->FlushNet(true);
					//@todo sz - can't close the connection here since it will leave the failure message 
					// in the send buffer and just close the socket. 
					//Connection->Close();
				}
				else
				{
					WelcomePlayer(Connection);
				}
				break;
			}
			case NMT_Join:
			{
				if (Connection->PlayerController == NULL)
				{
					// Spawn the player-actor for this network player.
					FString ErrorMsg;
					UE_LOG(LogNet, Log, TEXT("Join request: %s"), *Connection->RequestURL);

					FURL InURL( NULL, *Connection->RequestURL, TRAVEL_Absolute );

					if ( !InURL.Valid )
					{
						UE_LOG( LogNet, Error, TEXT( "NMT_Login: Invalid URL %s" ), *Connection->RequestURL );
						Bunch.SetError();
						break;
					}

					Connection->PlayerController = SpawnPlayActor( Connection, ROLE_AutonomousProxy, InURL, Connection->PlayerId, ErrorMsg );
					if (Connection->PlayerController == NULL)
					{
						// Failed to connect.
						UE_LOG(LogNet, Log, TEXT("Join failure: %s"), *ErrorMsg);
						NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("JOIN FAILURE"), *ErrorMsg));
						FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
						Connection->FlushNet(true);
						//@todo sz - can't close the connection here since it will leave the failure message 
						// in the send buffer and just close the socket. 
						//Connection->Close();
					}
					else
					{
						// Successfully in game.
						UE_LOG(LogNet, Log, TEXT("Join succeeded: %s"), *Connection->PlayerController->PlayerState->PlayerName);
						NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("JOIN"), *Connection->PlayerController->PlayerState->PlayerName));
						// if we're in the middle of a transition or the client is in the wrong world, tell it to travel
						FString LevelName;
						FSeamlessTravelHandler &SeamlessTravelHandler = GEngine->SeamlessTravelHandlerForWorld( this );

						if (SeamlessTravelHandler.IsInTransition())
						{
							// tell the client to go to the destination map
							LevelName = SeamlessTravelHandler.GetDestinationMapName();
						}
						else if (!Connection->PlayerController->HasClientLoadedCurrentWorld())
						{
							// tell the client to go to our current map
							//LevelName = FString(GWorld->GetOutermost()->GetName());
						}
						if (LevelName != TEXT(""))
						{
							Connection->PlayerController->ClientTravel(LevelName, TRAVEL_Relative, true);
						}

						// @TODO FIXME - TEMP HACK? - clear queue on join
						Connection->QueuedBytes = 0;
					}
				}
				break;
			}
			case NMT_JoinSplit:
			{
				// Handle server-side request for spawning a new controller using a child connection.
				FString SplitRequestURL;
				FUniqueNetIdRepl UniqueIdRepl;
				FNetControlMessage<NMT_JoinSplit>::Receive(Bunch, SplitRequestURL, UniqueIdRepl);

				// Compromise for passing splitscreen playercount through to gameplay login code,
				// without adding a lot of extra unnecessary complexity throughout the login code.
				// NOTE: This code differs from NMT_Login, by counting + 2 for SplitscreenCount
				//			(once for pending child connection, once for primary non-child connection)
				FURL InURL( NULL, *SplitRequestURL, TRAVEL_Absolute );

				if ( !InURL.Valid )
				{
					UE_LOG( LogNet, Error, TEXT( "NMT_JoinSplit: Invalid URL %s" ), *SplitRequestURL );
					Bunch.SetError();
					break;
				}

				uint8 SplitscreenCount = FMath::Min(Connection->Children.Num() + 2, 255);

				// Don't allow clients to specify this value
				InURL.RemoveOption(TEXT("SplitscreenCount"));
				InURL.AddOption(*FString::Printf(TEXT("SplitscreenCount=%i"), SplitscreenCount));

				SplitRequestURL = InURL.ToString();


				// skip to the first option in the URL
				const TCHAR* Tmp = *SplitRequestURL;
				for (; *Tmp && *Tmp != '?'; Tmp++);


				// keep track of net id for player associated with remote connection
				Connection->PlayerId = UniqueIdRepl.GetUniqueNetId();

				// go through the same full login process for the split player even though it's all in the same frame
				FString ErrorMsg;
				AuthorityGameMode->PreLogin(Tmp, Connection->LowLevelGetRemoteAddress(), Connection->PlayerId, ErrorMsg);
				if (!ErrorMsg.IsEmpty())
				{
					// if any splitscreen viewport fails to join, all viewports on that client also fail
					UE_LOG(LogNet, Log, TEXT("PreLogin failure: %s"), *ErrorMsg);
					NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("PRELOGIN FAILURE"), *ErrorMsg));
					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
					Connection->FlushNet(true);
					//@todo sz - can't close the connection here since it will leave the failure message 
					// in the send buffer and just close the socket. 
					//Connection->Close();
				}
				else
				{
					// create a child network connection using the existing connection for its parent
					check(Connection->GetUChildConnection() == NULL);
					check(CurrentLevel);

					UChildConnection* ChildConn = NetDriver->CreateChild(Connection);
					ChildConn->PlayerId = Connection->PlayerId;
					ChildConn->RequestURL = SplitRequestURL;
					ChildConn->ClientWorldPackageName = CurrentLevel->GetOutermost()->GetFName();

					// create URL from string
					FURL JoinSplitURL(NULL, *SplitRequestURL, TRAVEL_Absolute);

					UE_LOG(LogNet, Log, TEXT("JOINSPLIT: Join request: URL=%s"), *JoinSplitURL.ToString());
					APlayerController* PC = SpawnPlayActor(ChildConn, ROLE_AutonomousProxy, JoinSplitURL, ChildConn->PlayerId, ErrorMsg, uint8(Connection->Children.Num()));
					if (PC == NULL)
					{
						// Failed to connect.
						UE_LOG(LogNet, Log, TEXT("JOINSPLIT: Join failure: %s"), *ErrorMsg);
						NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("JOINSPLIT FAILURE"), *ErrorMsg));
						// remove the child connection
						Connection->Children.Remove(ChildConn);
						// if any splitscreen viewport fails to join, all viewports on that client also fail
						FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
						Connection->FlushNet(true);
						//@todo sz - can't close the connection here since it will leave the failure message 
						// in the send buffer and just close the socket. 
						//Connection->Close();
					}
					else
					{
						// Successfully spawned in game.
						UE_LOG(LogNet, Log, TEXT("JOINSPLIT: Succeeded: %s PlayerId: %s"), 
							*ChildConn->PlayerController->PlayerState->PlayerName,
							ChildConn->PlayerController->PlayerState->UniqueId.IsValid() ? *ChildConn->PlayerController->PlayerState->UniqueId->ToDebugString() : TEXT("INVALID"));
					}
				}
				break;
			}
			case NMT_PCSwap:
			{
				UNetConnection* SwapConnection = Connection;
				int32 ChildIndex;
				FNetControlMessage<NMT_PCSwap>::Receive(Bunch, ChildIndex);
				if (ChildIndex >= 0)
				{
					SwapConnection = Connection->Children.IsValidIndex(ChildIndex) ? Connection->Children[ChildIndex] : NULL;
				}
				bool bSuccess = false;
				if (SwapConnection != NULL)
				{
					bSuccess = DestroySwappedPC(SwapConnection);
				}
				
				if (!bSuccess)
				{
					UE_LOG(LogNet, Log, TEXT("Received invalid swap message with child index %i"), ChildIndex);
				}
				break;
			}
			case NMT_DebugText:
			{
				// debug text message
				FString Text;
				FNetControlMessage<NMT_DebugText>::Receive(Bunch,Text);

				UE_LOG(LogNet, Log, TEXT("%s received NMT_DebugText Text=[%s] Desc=%s DescRemote=%s"),
					*Connection->Driver->GetDescription(),*Text,*Connection->LowLevelDescribe(),*Connection->LowLevelGetRemoteAddress());
				break;
			}
		}
	}
}

bool UWorld::Listen( FURL& InURL )
{
	if( NetDriver )
	{
		GEngine->BroadcastNetworkFailure(this, NetDriver, ENetworkFailure::NetDriverAlreadyExists);
		return false;
	}

	// Create net driver.
	if (GEngine->CreateNamedNetDriver(this, NAME_GameNetDriver, NAME_GameNetDriver))
	{
		NetDriver = GEngine->FindNamedNetDriver(this, NAME_GameNetDriver);
		NetDriver->SetWorld(this);
	}

	if (NetDriver == NULL)
	{
		GEngine->BroadcastNetworkFailure(this, NULL, ENetworkFailure::NetDriverCreateFailure);
		return false;
	}

	FString Error;
	if( !NetDriver->InitListen( this, InURL, false, Error ) )
	{
		GEngine->BroadcastNetworkFailure(this, NetDriver, ENetworkFailure::NetDriverListenFailure, Error);
		UE_LOG(LogWorld, Log,  TEXT("Failed to listen: %s"), *Error );
		NetDriver->SetWorld(NULL);
		NetDriver = NULL;
		return false;
	}
	static bool LanPlay = FParse::Param(FCommandLine::Get(),TEXT("lanplay"));
	if ( !LanPlay && (NetDriver->MaxInternetClientRate < NetDriver->MaxClientRate) && (NetDriver->MaxInternetClientRate > 2500) )
	{
		NetDriver->MaxClientRate = NetDriver->MaxInternetClientRate;
	}

	// Create the Navigation system if required
	if (GetNavigationSystem() == NULL)
	{
		SetNavigationSystem(UNavigationSystem::CreateNavigationSystem(this)); 
	}
	NextSwitchCountdown = NetDriver->ServerTravelPause;
	return true;
}

bool UWorld::IsClient()
{
	return GIsClient;
}

bool UWorld::IsServer()
{
	// @todo ONLINE move this into net driver since there will no longer be one netdriver
	return (!NetDriver || NetDriver->IsServer());
}

void UWorld::PrepareMapChange(const TArray<FName>& LevelNames)
{
		// Kick off async loading request for those maps.
	if( !GEngine->PrepareMapChange(this, LevelNames) )
		{
		UE_LOG(LogWorld, Warning,TEXT("Preparing map change via %s was not successful: %s"), *GetFullName(), *GEngine->GetMapChangeFailureDescription(this) );
	}
}

bool UWorld::IsPreparingMapChange()
{
	return GEngine->IsPreparingMapChange(this);
}

bool UWorld::IsMapChangeReady()
{
	return GEngine->IsReadyForMapChange(this);
}

void UWorld::CancelPendingMapChange()
{
	GEngine->CancelPendingMapChange(this);
}

void UWorld::CommitMapChange()
{
		if( IsPreparingMapChange() )
		{
		GEngine->SetShouldCommitPendingMapChange(this, true);
		}
		else
		{
			UE_LOG(LogWorld, Log, TEXT("AWorldSettings::CommitMapChange being called without a pending map change!"));
		}
	}

void UWorld::InitializeWorldComposition()
{
	WorldComposition = NULL;
	//
	FString RootPackageName = GetOutermost()->GetName();
	if (FPackageName::DoesPackageExist(RootPackageName))
	{
		WorldComposition = ConstructObject<UWorldComposition>(UWorldComposition::StaticClass(), this);
		// Get the root directory
		FString RootPathName = FPaths::GetPath(RootPackageName) + TEXT("/");
		// Open world composition from root directory
		WorldComposition->OpenWorldRoot(RootPathName);
	}
}

void UWorld::RequestNewWorldOrigin(const FIntPoint& InNewOrigin)
{
	RequestedGlobalOriginOffset = InNewOrigin;
}

bool UWorld::SetNewWorldOrigin(const FIntPoint& InNewOrigin)
{
	if (GlobalOriginOffset == InNewOrigin) 
	{
		return true;
	}
	
	// We cannot shift world origin while Level is in the process to be added to a world
	// it will cause wrong positioning for this level 
	if (IsVisibilityRequestPending())
	{
		return false;
	}
	
	UE_LOG(LogLevel, Log, TEXT("WORLD TRANSLATION BEGIN {%d, %d} -> {%d, %d}"), GlobalOriginOffset.X, GlobalOriginOffset.Y, InNewOrigin.X, InNewOrigin.Y);
	const double MoveStartTime = FPlatformTime::Seconds();

	// Broadcast that we going to shift world to the new origin
	FCoreDelegates::PreWorldOriginOffset.Broadcast(this, GlobalOriginOffset, InNewOrigin);

	FVector Offset(GlobalOriginOffset - InNewOrigin, 0);

	// Local players
	for (auto It = GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = *It;
		if (PlayerController)
		{
			ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerController->Player);
			if (LocalPlayer)
			{
				LocalPlayer->ApplyWorldOffset(Offset);
			}
		}
	}
	
	// Send offset command to rendering thread
	Scene->ApplyWorldOffset(Offset);

	// Shift physics scene
	if (PhysicsScene && FPhysScene::SupportsOriginShifting())
	{
		PhysicsScene->ApplyWorldOffset(Offset);
	}
		
	// Apply offset to all visible levels
	for(int32 LevelIndex = 0; LevelIndex < Levels.Num(); LevelIndex++)
	{
		ULevel* LevelToShift = Levels[LevelIndex];
		
		// Only visible levels need to be shifted
		if (LevelToShift->bIsVisible)
		{
			// We don't want to shift always loaded levels, because world scrolling mechanic does not work for them
			bool bIsAlwaysLoaded = (LevelToShift != PersistentLevel && 
									WorldComposition != NULL && 
									WorldComposition->IsLevelAlwaysLoaded(LevelToShift));
					
			if (!bIsAlwaysLoaded)
			{
				LevelToShift->ApplyWorldOffset(Offset, true);
			}
			else
			{
				LevelToShift->ApplyWorldOffset(FVector::ZeroVector, true);
			}
		}
	}
			
	// Shift navigation meshes
	if (NavigationSystem)
	{
		NavigationSystem->ApplyWorldOffset(Offset, true);
	}

	FIntPoint PreviosWorldOrigin = GlobalOriginOffset;
	// Set new world origin
	GlobalOriginOffset = InNewOrigin;
	RequestedGlobalOriginOffset = InNewOrigin;
	
	// Propagate event to a level blueprints
	for(int32 LevelIndex = 0; LevelIndex < Levels.Num(); LevelIndex++)
	{
		ULevel* Level = Levels[LevelIndex];
		if (Level->bIsVisible && 
			Level->LevelScriptActor)
		{
			Level->LevelScriptActor->WorldOriginChanged(PreviosWorldOrigin, GlobalOriginOffset);
		}
	}

	// Broadcast that have finished world shifting
	FCoreDelegates::PostWorldOriginOffset.Broadcast(this, PreviosWorldOrigin, GlobalOriginOffset);

	const double CurrentTime = FPlatformTime::Seconds();
	const float TimeTaken = CurrentTime - MoveStartTime;
	UE_LOG(LogLevel, Log, TEXT("WORLD TRANSLATION END {%d, %d} took %.4f"), GlobalOriginOffset.X, GlobalOriginOffset.Y, TimeTaken);
	
	return true;
}

void UWorld::NavigateTo(FIntPoint InPosition)
{
	check(WorldComposition != NULL);

	SetNewWorldOrigin(InPosition);
	WorldComposition->UpdateStreamingState(FVector::ZeroVector);
	FlushLevelStreaming();
}

void UWorld::GetMatineeActors(TArray<class AMatineeActor*>& OutMatineeActors)
{
	check( IsGameWorld() && GetCurrentLevel());

	ULevel* CurLevel = GetCurrentLevel();
	for( int32 ActorIndex = 0; ActorIndex < CurLevel->Actors.Num(); ++ActorIndex )
	{
		AActor* Actor = CurLevel->Actors[ ActorIndex ];
		AMatineeActor* MatineeActor = Cast<AMatineeActor>( Actor );
		if( MatineeActor )
		{
			OutMatineeActors.Add( MatineeActor );
		}
	}
}

/*-----------------------------------------------------------------------------
	Seamless world traveling
-----------------------------------------------------------------------------*/

extern void LoadGametypeContent(FWorldContext &Context, const FURL& URL);

void FSeamlessTravelHandler::SetHandlerLoadedData(UObject* InLevelPackage, UWorld* InLoadedWorld)
{
	LoadedPackage = InLevelPackage;
	LoadedWorld = InLoadedWorld;
	if (LoadedWorld != NULL)
	{
		LoadedWorld->AddToRoot();
	}

}

/** callback sent to async loading code to inform us when the level package is complete */
void FSeamlessTravelHandler::SeamlessTravelLoadCallback(const FString& PackageName, UPackage* LevelPackage)
{
	// defer until tick when it's safe to perform the transition
	if (IsInTransition())
	{
		SetHandlerLoadedData(LevelPackage, UWorld::FindWorldInPackage(LevelPackage));
	}
}

bool FSeamlessTravelHandler::StartTravel(UWorld* InCurrentWorld, const FURL& InURL, const FGuid& InGuid)
{
	FWorldContext &Context = GEngine->WorldContextFromWorld(InCurrentWorld);
	WorldContextHandle = Context.ContextHandle;

	if (!InURL.Valid)
	{
		UE_LOG(LogWorld, Error, TEXT("Invalid travel URL specified"));
		return false;
	}
	else
	{
		UE_LOG(LogWorld, Log, TEXT("SeamlessTravel to: %s"), *InURL.Map);
		if (!FPackageName::DoesPackageExist(InURL.Map, InGuid.IsValid() ? &InGuid : NULL))
		{
			UE_LOG(LogWorld, Error, TEXT("Unable to travel to '%s' - file not found"), *InURL.Map);
			return false;
			// @todo: might have to handle this more gracefully to handle downloading (might also need to send GUID and check it here!)
		}
		else
		{
			CurrentWorld = InCurrentWorld;

			bool bCancelledExisting = false;
			if (IsInTransition())
			{
				if (PendingTravelURL.Map == InURL.Map)
				{
					// we are going to the same place so just replace the options
					PendingTravelURL = InURL;
					return true;
				}
				UE_LOG(LogWorld, Warning, TEXT("Cancelling travel to '%s' to go to '%s' instead"), *PendingTravelURL.Map, *InURL.Map);
				CancelTravel();
				bCancelledExisting = true;
			}

			checkSlow(LoadedPackage == NULL);
			checkSlow(LoadedWorld == NULL);

			PendingTravelURL = InURL;
			PendingTravelGuid = InGuid;
			bSwitchedToDefaultMap = false;
			bTransitionInProgress = true;
			bPauseAtMidpoint = false;
			bNeedCancelCleanUp = false;
			
			if (CurrentWorld->NetDriver)
			{
				// Lock the packagemap from acking any new NetGUIDs during server travel.
				// While locked, packagemap will serialize everything as full name/path pair due to unreliability of 
				// when exactly GC will be executed. Once clients are fully connected, packagemap is unlocked and returns to
				// 'optimized' mode.
				CurrentWorld->NetDriver->LockPackageMaps();
			}

			FString TransitionMap = GetDefault<UGameMapsSettings>()->TransitionMap;
			FName DefaultMapFinalName(*TransitionMap);

			// if we're already in the default map, skip loading it and just go to the destination
			if (DefaultMapFinalName == CurrentWorld->GetOutermost()->GetFName() ||
				DefaultMapFinalName == FName(*PendingTravelURL.Map))
			{
				UE_LOG(LogWorld, Log, TEXT("Already in default map or the default map is the destination, continuing to destination"));
				bSwitchedToDefaultMap = true;
				if (bCancelledExisting)
				{
					// we need to fully finishing loading the old package and GC it before attempting to load the new one
					bPauseAtMidpoint = true;
					bNeedCancelCleanUp = true;
				}
				else
				{
					StartLoadingDestination();
				}
			}
			else if (TransitionMap.IsEmpty())
			{
				// If a default transition map doesn't exist, create a dummy World to use as the transition
				SetHandlerLoadedData(NULL, UWorld::CreateWorld(EWorldType::None, false));
			}
			else
			{
				// first, load the entry level package
				LoadPackageAsync(TransitionMap, 
					FLoadPackageAsyncDelegate::CreateRaw(this, &FSeamlessTravelHandler::SeamlessTravelLoadCallback)
					);
			}

			return true;
		}
	}
}

/** cancels transition in progress */
void FSeamlessTravelHandler::CancelTravel()
{
	LoadedPackage = NULL;
	if (LoadedWorld != NULL)
	{
		LoadedWorld->RemoveFromRoot();
		LoadedWorld->ClearFlags(RF_Standalone);
		LoadedWorld = NULL;
	}
	if (bTransitionInProgress)
	{
		bTransitionInProgress = false;
		UE_LOG(LogWorld, Log, TEXT("----SeamlessTravel is cancelled!------"));
	}
}

void FSeamlessTravelHandler::SetPauseAtMidpoint(bool bNowPaused)
{
	if (!bTransitionInProgress)
	{
		UE_LOG(LogWorld, Warning, TEXT("Attempt to pause seamless travel when no transition is in progress"));
	}
	else if (bSwitchedToDefaultMap && bNowPaused)
	{
		UE_LOG(LogWorld, Warning, TEXT("Attempt to pause seamless travel after started loading final destination"));
	}
	else
	{
		bPauseAtMidpoint = bNowPaused;
		if (!bNowPaused && bSwitchedToDefaultMap)
		{
			// load the final destination now that we're done waiting
			StartLoadingDestination();
		}
	}
}

void FSeamlessTravelHandler::StartLoadingDestination()
{
	if (bTransitionInProgress && bSwitchedToDefaultMap)
	{
		if (GUseSeekFreeLoading)
		{
			// load gametype specific resources
			if (GEngine->bCookSeparateSharedMPGameContent)
			{
				UE_LOG(LogWorld, Log,  TEXT("LoadMap: %s: issuing load request for shared GameMode resources"), *PendingTravelURL.ToString());
				LoadGametypeContent(GEngine->WorldContextFromHandle(WorldContextHandle), PendingTravelURL);
			}

			// Only load localized package if it exists as async package loading doesn't handle errors gracefully.
			FString LocalizedPackageName = PendingTravelURL.Map + LOCALIZED_SEEKFREE_SUFFIX;
			if (FPackageName::DoesPackageExist(LocalizedPackageName))
			{
				// Load localized part of level first in case it exists. We don't need to worry about GC or completion 
				// callback as we always kick off another async IO for the level below.
				LoadPackageAsync(*LocalizedPackageName);
			}
		}

		LoadPackageAsync(PendingTravelURL.Map, 
			FLoadPackageAsyncDelegate::CreateRaw(this, &FSeamlessTravelHandler::SeamlessTravelLoadCallback), 
			PendingTravelGuid.IsValid() ? &PendingTravelGuid : NULL
			);
	}
	else
	{
		UE_LOG(LogWorld, Error, TEXT("Called StartLoadingDestination() when not ready! bTransitionInProgress: %u bSwitchedToDefaultMap: %u"), bTransitionInProgress, bSwitchedToDefaultMap);
		checkSlow(0);
	}
}

void FSeamlessTravelHandler::CopyWorldData()
{
	UNetDriver* const NetDriver = CurrentWorld->GetNetDriver();
	LoadedWorld->SetNetDriver(NetDriver);
	if (NetDriver != NULL)
	{
		CurrentWorld->SetNetDriver(NULL);
		NetDriver->SetWorld(LoadedWorld);
	}
	LoadedWorld->WorldType = CurrentWorld->WorldType;

	if (!bSwitchedToDefaultMap)
	{
		LoadedWorld->CopyGameState(CurrentWorld->GetAuthGameMode(), CurrentWorld->GameState);
	}
	LoadedWorld->TimeSeconds = CurrentWorld->TimeSeconds;
	LoadedWorld->RealTimeSeconds = CurrentWorld->RealTimeSeconds;
	LoadedWorld->AudioTimeSeconds = CurrentWorld->AudioTimeSeconds;

	if (NetDriver != NULL)
	{
		LoadedWorld->NextSwitchCountdown = NetDriver->ServerTravelPause;
	}
}

UWorld* FSeamlessTravelHandler::Tick()
{
	bool bWorldChanged = false;
	if (bNeedCancelCleanUp)
	{
		if (IsAsyncLoading())
		{
			// allocate more time for async loading so we can clean up faster
			ProcessAsyncLoading(true, 0.003f);
		}
		if (!IsAsyncLoading())
		{
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
			bNeedCancelCleanUp = false;
			SetPauseAtMidpoint(false);
		}
	}
	else if (IsInTransition())
	{
		float ExtraTime = 0.003f;

		// on console, we're locked to 30 FPS, so if we actually spent less time than that put it all into loading faster
		if (FPlatformProperties::SupportsWindowedMode() == false)
		{
			// Calculate the maximum time spent, EXCLUDING idle time. We don't use DeltaSeconds as it includes idle time waiting
			// for VSYNC so we don't know how much "buffer" we have.
			float FrameTime	= FPlatformTime::ToSeconds(FMath::Max<uint32>(GRenderThreadTime, GGameThreadTime));
			ExtraTime = FMath::Max<float>(ExtraTime, 0.0333f - FrameTime);
		}

		// allocate more time for async loading during transition
		ProcessAsyncLoading(true, ExtraTime);
	}
	//@fixme: wait for client to verify packages before finishing transition. Is this the right fix?
	// Once the default map is loaded, go ahead and start loading the destination map
	// Once the destination map is loaded, wait until all packages are verified before finishing transition

	UNetDriver* NetDriver = CurrentWorld->GetNetDriver();

	if ( (LoadedPackage != NULL || LoadedWorld != NULL) && CurrentWorld->NextURL == TEXT("") &&
		( NetDriver == NULL || NetDriver->ServerConnection == NULL || NetDriver->ServerConnection->PendingPackageInfos.Num() == 0 ||
		(!bSwitchedToDefaultMap && NetDriver->ServerConnection->PendingPackageInfos[0].LoadingPhase == 1) ) )
	{
		// First some validity checks		
		if( CurrentWorld == LoadedWorld )
		{
			// We are not going anywhere - this is the same world. 
			FString Error = FString::Printf(TEXT("Travel aborted - new world is the same as current world" ) );
			UE_LOG(LogWorld, Error, TEXT("%s"), *Error);
			// abort
			CancelTravel();			
		}
		else if ( LoadedWorld->PersistentLevel == NULL)
		{
			// Package isnt a level
			FString Error = FString::Printf(TEXT("Unable to travel to '%s' - package is not a level"), *LoadedPackage->GetName());
			UE_LOG(LogWorld, Error, TEXT("%s"), *Error);
			// abort
			CancelTravel();
			GEngine->BroadcastTravelFailure(CurrentWorld, ETravelFailure::NoLevel, Error);
		}
		else
		{
			// Make sure there are no pending visibility requests.
			CurrentWorld->FlushLevelStreaming( NULL, true );
			// only consider session ended if we're making the final switch so that HUD, etc. UI elements stay around until the end
			CurrentWorld->CleanupWorld(bSwitchedToDefaultMap); 
			CurrentWorld->RemoveFromRoot();
			CurrentWorld->ClearFlags(RF_Standalone);
			// Stop all audio to remove references to old world
			if (GEngine != NULL && GEngine->GetAudioDevice() != NULL)
			{
				GEngine->GetAudioDevice()->Flush( NULL );
			}

			// mark actors we want to keep
			FUObjectAnnotationSparseBool KeepAnnotation;

			// keep GameMode if traveling to entry
			if (!bSwitchedToDefaultMap && CurrentWorld->GetAuthGameMode() != NULL)
			{
				KeepAnnotation.Set(CurrentWorld->GetAuthGameMode());
			}
			// always keep Controllers that belong to players
			if (CurrentWorld->GetNetMode() == NM_Client)
			{
				for (FLocalPlayerIterator It(GEngine, CurrentWorld); It; ++It)
				{
					if (It->PlayerController != NULL)
					{
						KeepAnnotation.Set(It->PlayerController);
					}
				}
			}
			else
			{
				for( FConstControllerIterator Iterator = CurrentWorld->GetControllerIterator(); Iterator; ++Iterator )
				{
					AController* Player = *Iterator;
					if (Player->PlayerState || Cast<APlayerController>(Player) != NULL)
					{
						KeepAnnotation.Set(Player);
					}
				}
			}

			// ask the game class what else we should keep
			TArray<AActor*> KeepActors;
			if (CurrentWorld->GetAuthGameMode() != NULL)
			{
				CurrentWorld->GetAuthGameMode()->GetSeamlessTravelActorList(!bSwitchedToDefaultMap, KeepActors);
			}
			// ask players what else we should keep
			for (FLocalPlayerIterator It(GEngine, CurrentWorld); It; ++It)
			{
				if (It->PlayerController != NULL)
				{
					It->PlayerController->GetSeamlessTravelActorList(!bSwitchedToDefaultMap, KeepActors);
				}
			}
			// mark all valid actors specified
			for (int32 i = 0; i < KeepActors.Num(); i++)
			{
				if (KeepActors[i] != NULL)
				{
					KeepAnnotation.Set(KeepActors[i]);
				}
			} 

			// rename dynamic actors in the old world's PersistentLevel that we want to keep into the new world
			for (FActorIterator It(CurrentWorld); It; ++It)
			{
				AActor* TheActor = *It;
				bool bSameLevel = TheActor->GetLevel() == CurrentWorld->PersistentLevel;
				bool bShouldKeep = KeepAnnotation.Get(TheActor);
				bool bDormant = false;

				if (NetDriver && NetDriver->ServerConnection)
				{
					bDormant = NetDriver->ServerConnection->DormantActors.Contains(TheActor);
				}

				// keep if it's dynamic and has been marked or we don't control it
				if (bSameLevel && (bShouldKeep || (TheActor->Role < ROLE_Authority && !bDormant && !TheActor->IsNetStartupActor())) && !TheActor->IsA(ALevelScriptActor::StaticClass()))
				{
					It.ClearCurrent(); //@warning: invalidates *It until next iteration
					KeepAnnotation.Clear(TheActor);
					TheActor->Rename(NULL, LoadedWorld->PersistentLevel);
					// if it's a Controller or a Pawn, add it to the appropriate list in the new world's WorldSettings
					if (Cast<AController>(TheActor))
					{
						LoadedWorld->AddController(Cast<AController>(TheActor) );

						if (Cast<APlayerController>(TheActor))
						{
							Cast<APlayerController>(TheActor)->SeamlessTravelCount++;
						}
					}
					else if (Cast<APawn>(TheActor))
					{
						LoadedWorld->AddPawn(Cast<APawn>(TheActor));
					}
					// add to new world's actor list and remove from old
					LoadedWorld->PersistentLevel->Actors.Add(TheActor);

					TheActor->bActorSeamlessTraveled = true;
				}
				else
				{
					// otherwise, set to be deleted
					KeepAnnotation.Clear(TheActor);
					TheActor->MarkPendingKill();
					TheActor->MarkComponentsAsPendingKill();
					// close any channels for this actor
					if (NetDriver != NULL)
					{
						NetDriver->NotifyActorLevelUnloaded(TheActor);
					}
				}
			}

			CopyWorldData(); // This copies the net driver too (LoadedWorld now has whatever NetDriver was previously held by CurrentWorld)

			// Copy the standby cheat status
			bool bHasStandbyCheatTriggered = (CurrentWorld->NetworkManager) ? CurrentWorld->NetworkManager->bHasStandbyCheatTriggered : false;

			// the new world should not be garbage collected
			LoadedWorld->AddToRoot();
			
			// Update the FWorldContext to point to the newly loaded world
			FWorldContext &CurrentContext = GEngine->WorldContextFromWorld(CurrentWorld);
			CurrentContext.SetCurrentWorld(LoadedWorld);

			LoadedWorld->WorldType = CurrentContext.WorldType;
			if (CurrentContext.WorldType == EWorldType::PIE)
			{
				UPackage * WorldPackage = LoadedWorld->GetOutermost();
				check(WorldPackage);
				WorldPackage->PackageFlags |= PKG_PlayInEditor;
			}

			// Clear any world specific state from NetDriver before switching World
			if (NetDriver)
			{
				NetDriver->PreSeamlessTravelGarbageCollect();
			}

			GWorld = NULL;

			// mark everything else contained in the world to be deleted
			for (TObjectIterator<UObject> It; It; ++It)
			{
				if (It->IsIn(CurrentWorld))
				{
					It->MarkPendingKill();
				}
			}

			CurrentWorld = NULL;

			// collect garbage to delete the old world
			// because we marked everything in it pending kill, references will be NULL'ed so we shouldn't end up with any dangling pointers
			CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS, true );

			if (GIsEditor)
			{
				CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS, true );
			}

			appDefragmentTexturePool();
			appDumpTextureMemoryStats(TEXT(""));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			// verify that we successfully cleaned up the old world
			GEngine->VerifyLoadMapWorldCleanup();
#endif
			// Clean out NetDriver's Packagemaps, since they may have a lot of NULL object ptrs rotting in the lookup maps.
			if (NetDriver)
			{
				NetDriver->PostSeamlessTravelGarbageCollect();
			}

			// set GWorld to the new world and initialize it
			GWorld = LoadedWorld;
			if (!LoadedWorld->bIsWorldInitialized)
			{
				LoadedWorld->InitWorld();
			}
			bWorldChanged = true;
			// Track session change on seamless travel.
			NETWORK_PROFILER(GNetworkProfiler.TrackSessionChange(true, LoadedWorld->URL));

			// if we've already switched to entry before and this is the transition to the new map, re-create the gameinfo
			if (bSwitchedToDefaultMap && LoadedWorld->GetNetMode() != NM_Client)
			{
				LoadedWorld->SetGameMode(PendingTravelURL);

				if( GEngine && GEngine->GetAudioDevice() )
				{
					GEngine->GetAudioDevice()->SetDefaultBaseSoundMix( LoadedWorld->GetWorldSettings()->DefaultBaseSoundMix );
				}

				// Copy cheat flags if the game info is present
				// @todo UE4 FIXMELH - see if this exists, it should not since it's created in GameMode or it's garbage info
				if (LoadedWorld->NetworkManager != NULL)
				{
					LoadedWorld->NetworkManager->bHasStandbyCheatTriggered = bHasStandbyCheatTriggered;
				}
			}

			// call begin play functions on everything that wasn't carried over from the old world
			LoadedWorld->BeginPlay(PendingTravelURL, false);

			// send loading complete notifications for all local players
			for (FLocalPlayerIterator It(GEngine, LoadedWorld); It; ++It)
			{
				if (It->PlayerController != NULL)
				{
					It->PlayerController->NotifyLoadedWorld(LoadedWorld->GetOutermost()->GetFName(), bSwitchedToDefaultMap);
					It->PlayerController->ServerNotifyLoadedWorld(LoadedWorld->GetOutermost()->GetFName());
				}
			}

			// we've finished the transition
			LoadedWorld->bWorldWasLoadedThisTick = true;
			
			if (bSwitchedToDefaultMap)
			{
				// we've now switched to the final destination, so we're done
				
				// remember the last used URL
				CurrentContext.LastURL = PendingTravelURL;

				// Flag our transition as completed before we call PostSeamlessTravel.  This 
				// allows for chaining of maps.

				bTransitionInProgress = false;
				UE_LOG(LogWorld, Log, TEXT("----SeamlessTravel finished------") );

				AGameMode* const GameMode = LoadedWorld->GetAuthGameMode();
				if (GameMode)
				{
					if (LoadedWorld->GetNavigationSystem() != NULL)
					{
						LoadedWorld->GetNavigationSystem()->OnWorldInitDone(NavigationSystem::GameMode);
					}

					// inform the new GameMode so it can handle players that persisted
					GameMode->PostSeamlessTravel();
				}

				FCoreDelegates::PostLoadMap.Broadcast();
			}
			else
			{
				bSwitchedToDefaultMap = true;
				CurrentWorld = LoadedWorld;
				if (!bPauseAtMidpoint)
				{
					StartLoadingDestination();
				}
			}			
		}		
	}
	UWorld* OutWorld = NULL;
	if( bWorldChanged )
	{
		OutWorld = LoadedWorld;
		// Cleanup the old pointers
		LoadedPackage = NULL;
		LoadedWorld = NULL;
	}
	
	return OutWorld;
}

/** seamlessly travels to the given URL by first loading the entry level in the background,
 * switching to it, and then loading the specified level. Does not disrupt network communication or disconnect clients.
 * You may need to implement GameMode::GetSeamlessTravelActorList(), PlayerController::GetSeamlessTravelActorList(),
 * GameMode::PostSeamlessTravel(), and/or GameMode::HandleSeamlessTravelPlayer() to handle preserving any information
 * that should be maintained (player teams, etc)
 * This codepath is designed for worlds that use little or no level streaming and GameModes where the game state
 * is reset/reloaded when transitioning. (like UT)
 * @param URL the URL to travel to; must be relative to the current URL (same server)
 * @param bAbsolute (opt) - if true, URL is absolute, otherwise relative
 * @param MapPackageGuid (opt) - the GUID of the map package to travel to - this is used to find the file when it has been autodownloaded,
 * 				so it is only needed for clients
 */
void UWorld::SeamlessTravel(const FString& SeamlessTravelURL, bool bAbsolute, FGuid MapPackageGuid)
{
	// construct the URL
	FURL NewURL(&GEngine->LastURLFromWorld(this), *SeamlessTravelURL, bAbsolute ? TRAVEL_Absolute : TRAVEL_Relative);
	if (!NewURL.Valid)
	{
		const FString Error = FText::Format( NSLOCTEXT("Engine", "InvalidUrl", "Invalid URL: {0}"), FText::FromString( SeamlessTravelURL ) ).ToString();
		GEngine->BroadcastTravelFailure(this, ETravelFailure::InvalidURL, Error);
	}
	else
	{
		if (NewURL.HasOption(TEXT("Restart")))
		{
			//@todo url cleanup - we should merge the two URLs, not completely replace it
			NewURL = GEngine->LastURLFromWorld(this);
		}
		// tell the handler to start the transition
		FSeamlessTravelHandler &SeamlessTravelHandler = GEngine->SeamlessTravelHandlerForWorld( this );
		if (!SeamlessTravelHandler.StartTravel(this, NewURL, MapPackageGuid) && !SeamlessTravelHandler.IsInTransition())
		{
			const FString Error = FText::Format( NSLOCTEXT("Engine", "InvalidUrl", "Invalid URL: {0}"), FText::FromString( SeamlessTravelURL ) ).ToString();
			GEngine->BroadcastTravelFailure(this, ETravelFailure::InvalidURL, Error);
		}
	}
}

/** @return whether we're currently in a seamless transition */
bool UWorld::IsInSeamlessTravel()
{
	FSeamlessTravelHandler &SeamlessTravelHandler = GEngine->SeamlessTravelHandlerForWorld( this );
	return SeamlessTravelHandler.IsInTransition();
}

/** this function allows pausing the seamless travel in the middle,
 * right before it starts loading the destination (i.e. while in the transition level)
 * this gives the opportunity to perform any other loading tasks before the final transition
 * this function has no effect if we have already started loading the destination (you will get a log warning if this is the case)
 * @param bNowPaused - whether the transition should now be paused
 */
void UWorld::SetSeamlessTravelMidpointPause(bool bNowPaused)
{
	FSeamlessTravelHandler &SeamlessTravelHandler = GEngine->SeamlessTravelHandlerForWorld( this );
	SeamlessTravelHandler.SetPauseAtMidpoint(bNowPaused);
}

int32 UWorld::GetDetailMode()
{
	return GetCachedScalabilityCVars().DetailMode;
}

void UWorld::UpdateMusicTrack(FMusicTrackStruct NewMusicTrack)
{
	if (MusicComp != NULL)
	{
		// If attempting to play same track, don't.
		if (NewMusicTrack.Sound == CurrentMusicTrack.Sound)
		{
			return;
		}
		else
		{
			// otherwise fade out the current track
			MusicComp->FadeOut(CurrentMusicTrack.FadeOutTime,CurrentMusicTrack.FadeOutVolumeLevel);
			MusicComp = NULL;
		}
	}

	{
		// create a new audio component to play music
		MusicComp = FAudioDevice::CreateComponent( NewMusicTrack.Sound, this, NULL, false );
		if (MusicComp != NULL)
		{
			// update the new component with the correct settings
			MusicComp->bAutoDestroy = true;
			MusicComp->bShouldRemainActiveIfDropped = true;
			MusicComp->bIsMusic = true;
			MusicComp->bAutoActivate = NewMusicTrack.bAutoPlay;
			MusicComp->bIgnoreForFlushing = NewMusicTrack.bPersistentAcrossLevels;

			// and finally fade in the new track
			MusicComp->FadeIn( NewMusicTrack.FadeInTime, NewMusicTrack.FadeInVolumeLevel );
		}
	}

	// set the properties for future fades as well as replication to clients
	CurrentMusicTrack = NewMusicTrack;
	GetWorldSettings()->ReplicatedMusicTrack = NewMusicTrack;
}

/**
 * Updates all physics constraint actor joint locations.
 */
void UWorld::UpdateConstraintActors()
{
	if( bAreConstraintsDirty )
	{
		for( FActorIterator It(this); It; ++It )
		{
			APhysicsConstraintActor* ConstraintActor = Cast<APhysicsConstraintActor>( *It );
			if( ConstraintActor && ConstraintActor->ConstraintComp)
			{
				ConstraintActor->ConstraintComp->UpdateConstraintFrames();
			}
		}
		bAreConstraintsDirty = false;
	}
}

int32 UWorld::GetProgressDenominator()
{
	return GetActorCount();
}

int32 UWorld::GetActorCount()
{
	int32 TotalActorCount = 0;
	for( int32 LevelIndex=0; LevelIndex<GetNumLevels(); LevelIndex++ )
	{
		ULevel* Level = GetLevel(LevelIndex);
		TotalActorCount += Level->Actors.Num();
	}
	return TotalActorCount;
}

int32 UWorld::GetNetRelevantActorCount()
{
	int32 TotalActorCount = 0;
	for( int32 LevelIndex=0; LevelIndex<GetNumLevels(); LevelIndex++ )
	{
		ULevel* Level = GetLevel(LevelIndex);
		TotalActorCount += Level->Actors.Num() - Level->iFirstNetRelevantActor;
	}
	return TotalActorCount;
}

FConstLevelIterator	UWorld::GetLevelIterator() const
{
	return Levels.CreateConstIterator();
}

ULevel* UWorld::GetLevel( int32 InLevelIndex ) const
{
	check( InLevelIndex < Levels.Num() );
	check(Levels[ InLevelIndex ]);
	return Levels[ InLevelIndex ];
}

bool UWorld::ContainsLevel( ULevel* InLevel ) const
{
	return Levels.Find( InLevel ) != INDEX_NONE;
}

int32 UWorld::GetNumLevels() const
{
	return Levels.Num();
}

const TArray<class ULevel*>& UWorld::GetLevels() const
{
	return Levels;
}

bool UWorld::AddLevel( ULevel* InLevel )
{
	bool bAddedLevel = false;
	if(ensure(InLevel))
	{
		bAddedLevel = true;
		Levels.AddUnique( InLevel );
		BroadcastLevelsChanged();
	}
	return bAddedLevel;
}

bool UWorld::RemoveLevel( ULevel* InLevel )
{
	bool bRemovedLevel = false;
	if(ContainsLevel( InLevel ) == true )
	{
		bRemovedLevel = true;
		
#if WITH_EDITOR
		if( IsLevelSelected( InLevel ))
		{
			DeSelectLevel( InLevel );
		}
#endif //WITH_EDITOR
		Levels.Remove( InLevel );
		BroadcastLevelsChanged();
	}
	return bRemovedLevel;
}


FString UWorld::GetLocalURL() const
{
	return URL.ToString();
}

/** Returns whether script is executing within the editor. */
bool UWorld::IsPlayInEditor() const
{
	return WorldType == EWorldType::PIE;
}

bool UWorld::IsPlayInPreview() const
{
	return FParse::Param(FCommandLine::Get(), TEXT("PIEVIACONSOLE"));
}


bool UWorld::IsPlayInMobilePreview() const
{
	return FParse::Param(FCommandLine::Get(), TEXT("simmobile"));
}

bool UWorld::IsGameWorld() const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FString UWorld::GetAddressURL() const
{
	return FString::Printf( TEXT("%s:%i"), *URL.Host, URL.Port );
}

FString UWorld::RemovePIEPrefix(const FString &Source)
{
	// PIE prefix is: UEDPIE_X_MapName (where X is 0-9)
	const FString LookFor = PLAYWORLD_PACKAGE_PREFIX;
	FString FixedName;

	int32 idx = Source.Find(LookFor);
	if (idx >= 0)
	{
		FixedName = Source.Left( idx );

		FString Blah = Source.Right( idx );
		FString Blah2 = Source.Right( Source.Len() - idx );
		FString RightS = Source.Right( Source.Len() - (idx + LookFor.Len() + 3) );
		
		FixedName += RightS;
	}

	return FixedName;
}

UWorld* UWorld::FindWorldInPackage(UPackage* Package)
{
	UWorld* RetVal = NULL;
	TArray<UObject*> PotentialWorlds;
	GetObjectsWithOuter(Package, PotentialWorlds, false);
	for ( auto ObjIt = PotentialWorlds.CreateConstIterator(); ObjIt; ++ObjIt )
	{
		RetVal = Cast<UWorld>(*ObjIt);
		if ( RetVal )
		{
			break;
		}
	}

	return RetVal;
}

#if WITH_EDITOR


void UWorld::BroadcastSelectedLevelsChanged() 
{ 
	if( bBroadcastSelectionChange )
	{
		SelectedLevelsChangedEvent.Broadcast(); 
	}
}


void UWorld::SelectLevel( ULevel* InLevel )
{
	check( InLevel );
	if( IsLevelSelected( InLevel ) == false )
	{
		SelectedLevels.AddUnique( InLevel );
		BroadcastSelectedLevelsChanged();
	}
}

void UWorld::DeSelectLevel( ULevel* InLevel )
{
	check( InLevel );
	if( IsLevelSelected( InLevel ) == true )
	{
		SelectedLevels.Remove( InLevel );
		BroadcastSelectedLevelsChanged();
	}
}

bool UWorld::IsLevelSelected( ULevel* InLevel ) const
{
	return SelectedLevels.Find( InLevel ) != INDEX_NONE;
}

int32 UWorld::GetNumSelectedLevels() const 
{
	return SelectedLevels.Num();
}

TArray<class ULevel*>& UWorld::GetSelectedLevels()
{
	return SelectedLevels;
}

ULevel* UWorld::GetSelectedLevel( int32 InLevelIndex ) const
{
	check(SelectedLevels[ InLevelIndex ]);
	return SelectedLevels[ InLevelIndex ];
}

void UWorld::SetSelectedLevels( const TArray<class ULevel*>& InLevels )
{
	// Disable the broadcasting of selection changes - we will send a single broadcast when we have finished
	bBroadcastSelectionChange = false;
	SelectedLevels.Empty();
	for (int32 iSelected = 0; iSelected <  InLevels.Num(); iSelected++)
	{
		SelectLevel( InLevels[ iSelected ]);
	}
	// Enable the broadcasting of selection changes
	bBroadcastSelectionChange = true;
	// Broadcast we have change the selections
	BroadcastSelectedLevelsChanged();
}
#endif // WITH_EDITOR

/**
 * Jumps the server to new level.  If bAbsolute is true and we are using seemless traveling, we
 * will do an absolute travel (URL will be flushed).
 *
 * @param URL the URL that we are traveling to
 * @param bAbsolute whether we are using relative or absolute travel
 * @param bShouldSkipGameNotify whether to notify the clients/game or not
 */
void UWorld::ServerTravel(const FString& FURL, bool bAbsolute, bool bShouldSkipGameNotify)
{
	if (FURL.Contains(TEXT("%")) )
	{
		UE_LOG(LogWorld, Log, TEXT("FURL %s Contains illegal character '%'."), *FURL);
		return;
	}

	if (FURL.Contains(TEXT(":")) || FURL.Contains(TEXT("\\")) )
	{
		UE_LOG(LogWorld, Log, TEXT("FURL %s blocked"), *FURL);
	}

	FString MapName;
	int32 OptionStart = FURL.Find(TEXT("?"));
	if (OptionStart == INDEX_NONE)
	{
		MapName = FURL;
	}
	else
	{
		MapName = FURL.Left(OptionStart);
	}

	// Check for invalid package names.
	FText InvalidPackageError;
	if (MapName.StartsWith(TEXT("/")) && !FPackageName::IsValidLongPackageName(MapName, true, &InvalidPackageError))
	{
		UE_LOG(LogWorld, Log, TEXT("FURL %s blocked (%s)"), *FURL, *InvalidPackageError.ToString());
		return;
	}

	// Check for an error in the server's connection
	if (AuthorityGameMode && AuthorityGameMode->bHasNetworkError)
	{
		UE_LOG(LogWorld, Log, TEXT("Not traveling because of network error"));
		return;
	}

	// Set the next travel type to use
	NextTravelType = bAbsolute ? TRAVEL_Absolute : TRAVEL_Relative;

	// if we're not already in a level change, start one now
	// If the bShouldSkipGameNotify is there, then don't worry about seamless travel recursion
	// and accept that we really want to travel
	if (NextURL.IsEmpty() && (!IsInSeamlessTravel() || bShouldSkipGameNotify))
	{
		NextURL = FURL;
		if (AuthorityGameMode != NULL)
		{
			// Skip notifying clients if requested
			if (!bShouldSkipGameNotify)
			{
				AuthorityGameMode->ProcessServerTravel(FURL, bAbsolute);
			}
		}
		else
		{
			NextSwitchCountdown = 0;
		}
	}
}

void UWorld::SetNavigationSystem( UNavigationSystem* InNavigationSystem)
{
	if (NavigationSystem != NULL && NavigationSystem != InNavigationSystem)
	{
		NavigationSystem->CleanUp();
	}

	NavigationSystem = InNavigationSystem;
}

/** Set the CurrentLevel for this world. **/
bool UWorld::SetCurrentLevel( class ULevel* InLevel )
{
	bool bChanged = false;
	if( CurrentLevel != InLevel )
	{
		CurrentLevel = InLevel;
		bChanged = true;
	}
	return bChanged;
}

/** Get the CurrentLevel for this world. **/
ULevel* UWorld::GetCurrentLevel() const
{
	return CurrentLevel;
}

ENetMode UWorld::GetNetMode() const
{
	return NetDriver ? NetDriver->GetNetMode() : NM_Standalone;
}

void UWorld::CopyGameState(AGameMode* FromGameMode, AGameState* FromGameState)
{
	AuthorityGameMode = FromGameMode;
	GameState = FromGameState;
}

/**
* Dump visible actors in current world.
*/
static void DumpVisibleActors()
{
	UE_LOG(LogWorld, Log, TEXT("------ START DUMP VISIBLE ACTORS ------"));
	for (FActorIterator ActorIterator(GWorld); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (Actor && Actor->GetLastRenderTime() > (GWorld->GetTimeSeconds() - 0.05f))
		{
			UE_LOG(LogWorld, Log, TEXT("Visible Actor : %s"), *Actor->GetFullName());
		}
	}
	UE_LOG(LogWorld, Log, TEXT("------ END DUMP VISIBLE ACTORS ------"));
}

static FAutoConsoleCommand DumpVisibleActorsCmd(
	TEXT("DumpVisibleActors"),
	TEXT("Dump visible actors in current world."),
	FConsoleCommandDelegate::CreateStatic(DumpVisibleActors)
	);

#undef LOCTEXT_NAMESPACE 
