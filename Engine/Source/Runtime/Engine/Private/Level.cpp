// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
Level.cpp: Level-related functions
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "SoundDefinitions.h"
#include "EngineMaterialClasses.h"
#include "PrecomputedLightVolume.h"
#include "TickTaskManagerInterface.h"
#include "BlueprintUtilities.h"
#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#endif
#include "EngineLevelScriptClasses.h"
#include "Runtime/Engine/Classes/MovieScene/RuntimeMovieScenePlayerInterface.h"
#include "LevelUtils.h"
#include "TargetPlatform.h"

DEFINE_LOG_CATEGORY(LogLevel);

/*-----------------------------------------------------------------------------
ULevelBase implementation.
-----------------------------------------------------------------------------*/

ULevelBase::ULevelBase( const class FPostConstructInitializeProperties& PCIP,const FURL& InURL )
	: UObject(PCIP)
	, URL( InURL )
	, Actors( this )
{

}

void ULevelBase::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{	
	ULevelBase* This = CastChecked<ULevelBase>(InThis);
	// Let GC know that we're referencing some AActor objects
	for (auto& Actor : This->Actors)
	{
		Collector.AddReferencedObject(Actor, This);
	}
	UObject* ActorsOwner = This->Actors.GetOwner();
	Collector.AddReferencedObject(ActorsOwner, This);
	Super::AddReferencedObjects(This, Collector);
}

void ULevelBase::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);
	Ar << Actors;
	Ar << URL;
}


/*-----------------------------------------------------------------------------
ULevel implementation.
-----------------------------------------------------------------------------*/


/** Called when a level package has been dirtied. */
FSimpleMulticastDelegate ULevel::LevelDirtiedEvent;

//@deprecated with VER_SPLIT_SOUND_FROM_TEXTURE_STREAMING
struct FStreamableResourceInstanceDeprecated
{
	FSphere BoundingSphere;
	float TexelFactor;
	friend FArchive& operator<<( FArchive& Ar, FStreamableResourceInstanceDeprecated& ResourceInstance )
	{
		Ar << ResourceInstance.BoundingSphere;
		Ar << ResourceInstance.TexelFactor;
		return Ar;
	}
};
//@deprecated with VER_SPLIT_SOUND_FROM_TEXTURE_STREAMING
struct FStreamableResourceInfoDeprecated
{
	UObject* Resource;
	TArray<FStreamableResourceInstanceDeprecated> ResourceInstances;
	friend FArchive& operator<<( FArchive& Ar, FStreamableResourceInfoDeprecated& ResourceInfo )
	{
		Ar << ResourceInfo.Resource;
		Ar << ResourceInfo.ResourceInstances;
		return Ar;
	}
};
//@deprecated with VER_RENDERING_REFACTOR
struct FStreamableSoundInstanceDeprecated
{
	FSphere BoundingSphere;
	friend FArchive& operator<<( FArchive& Ar, FStreamableSoundInstanceDeprecated& SoundInstance )
	{
		Ar << SoundInstance.BoundingSphere;
		return Ar;
	}
};
//@deprecated with VER_RENDERING_REFACTOR
struct FStreamableSoundInfoDeprecated
{
	UDEPRECATED_SoundNodeWave*	SoundNodeWave;
	TArray<FStreamableSoundInstanceDeprecated> SoundInstances;
	friend FArchive& operator<<( FArchive& Ar, FStreamableSoundInfoDeprecated& SoundInfo )
	{
		Ar << SoundInfo.SoundNodeWave;
		Ar << SoundInfo.SoundInstances;
		return Ar;
	}
};
//@deprecated with VER_RENDERING_REFACTOR
struct FStreamableTextureInfoDeprecated
{
	UTexture*							Texture;
	TArray<FStreamableTextureInstance>	TextureInstances;
	friend FArchive& operator<<( FArchive& Ar, FStreamableTextureInfoDeprecated& TextureInfo )
	{
		Ar << TextureInfo.Texture;
		Ar << TextureInfo.TextureInstances;
		return Ar;
	}
};

int32 FPrecomputedVisibilityHandler::NextId = 0;

/** Updates visibility stats. */
void FPrecomputedVisibilityHandler::UpdateVisibilityStats(bool bAllocating) const
{
	if (bAllocating)
	{
		INC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets.GetAllocatedSize());
		for (int32 BucketIndex = 0; BucketIndex < PrecomputedVisibilityCellBuckets.Num(); BucketIndex++)
		{
			INC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets[BucketIndex].Cells.GetAllocatedSize());
			INC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets[BucketIndex].CellDataChunks.GetAllocatedSize());
			for (int32 ChunkIndex = 0; ChunkIndex < PrecomputedVisibilityCellBuckets[BucketIndex].CellDataChunks.Num(); ChunkIndex++)
			{
				INC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets[BucketIndex].CellDataChunks[ChunkIndex].Data.GetAllocatedSize());
			}
		}
	}
	else
	{
		DEC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets.GetAllocatedSize());
		for (int32 BucketIndex = 0; BucketIndex < PrecomputedVisibilityCellBuckets.Num(); BucketIndex++)
		{
			DEC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets[BucketIndex].Cells.GetAllocatedSize());
			DEC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets[BucketIndex].CellDataChunks.GetAllocatedSize());
			for (int32 ChunkIndex = 0; ChunkIndex < PrecomputedVisibilityCellBuckets[BucketIndex].CellDataChunks.Num(); ChunkIndex++)
			{
				DEC_DWORD_STAT_BY(STAT_PrecomputedVisibilityMemory, PrecomputedVisibilityCellBuckets[BucketIndex].CellDataChunks[ChunkIndex].Data.GetAllocatedSize());
			}
		}
	}
}

/** Sets this visibility handler to be actively used by the rendering scene. */
void FPrecomputedVisibilityHandler::UpdateScene(FSceneInterface* Scene) const
{
	if (Scene && PrecomputedVisibilityCellBuckets.Num() > 0)
	{
		Scene->SetPrecomputedVisibility(this);
	}
}

/** Invalidates the level's precomputed visibility and frees any memory used by the handler. */
void FPrecomputedVisibilityHandler::Invalidate(FSceneInterface* Scene)
{
	Scene->SetPrecomputedVisibility(NULL);
	// Block until the renderer no longer references this FPrecomputedVisibilityHandler so we can delete its data
	FlushRenderingCommands();
	UpdateVisibilityStats(false);
	PrecomputedVisibilityCellBucketOriginXY = FVector2D(0,0);
	PrecomputedVisibilityCellSizeXY = 0;
	PrecomputedVisibilityCellSizeZ = 0;
	PrecomputedVisibilityCellBucketSizeXY = 0;
	PrecomputedVisibilityNumCellBuckets = 0;
	PrecomputedVisibilityCellBuckets.Empty();
	// Bump the Id so FSceneViewState will know to discard its cached visibility data
	Id = NextId;
	NextId++;
}

FArchive& operator<<( FArchive& Ar, FPrecomputedVisibilityHandler& D )
{
	Ar << D.PrecomputedVisibilityCellBucketOriginXY;
	Ar << D.PrecomputedVisibilityCellSizeXY;
	Ar << D.PrecomputedVisibilityCellSizeZ;
	Ar << D.PrecomputedVisibilityCellBucketSizeXY;
	Ar << D.PrecomputedVisibilityNumCellBuckets;
	Ar << D.PrecomputedVisibilityCellBuckets;
	if (Ar.IsLoading())
	{
		D.UpdateVisibilityStats(true);
	}
	return Ar;
}


/** Sets this volume distance field to be actively used by the rendering scene. */
void FPrecomputedVolumeDistanceField::UpdateScene(FSceneInterface* Scene) const
{
	if (Scene && Data.Num() > 0)
	{
		Scene->SetPrecomputedVolumeDistanceField(this);
	}
}

/** Invalidates the level's volume distance field and frees any memory used by it. */
void FPrecomputedVolumeDistanceField::Invalidate(FSceneInterface* Scene)
{
	if (Scene && Data.Num() > 0)
	{
		Scene->SetPrecomputedVolumeDistanceField(NULL);
		// Block until the renderer no longer references this FPrecomputedVolumeDistanceField so we can delete its data
		FlushRenderingCommands();
		Data.Empty();
	}
}

FArchive& operator<<( FArchive& Ar, FPrecomputedVolumeDistanceField& D )
{
	Ar << D.VolumeMaxDistance;
	Ar << D.VolumeBox;
	Ar << D.VolumeSizeX;
	Ar << D.VolumeSizeY;
	Ar << D.VolumeSizeZ;
	Ar << D.Data;

	return Ar;
}

TMap<FName, UWorld*> ULevel::StreamedLevelsOwningWorld;

ULevel::ULevel( const class FPostConstructInitializeProperties& PCIP )
	:	ULevelBase( PCIP )
	,	OwningWorld(NULL)
	,	TickTaskLevel(FTickTaskManagerInterface::Get().AllocateTickTaskLevel())
{
	PrecomputedLightVolume = new FPrecomputedLightVolume();
}

ULevel::ULevel( const class FPostConstructInitializeProperties& PCIP,const FURL& InURL )
	:	ULevelBase( PCIP, InURL )
	,	OwningWorld(NULL)
	,	TickTaskLevel(FTickTaskManagerInterface::Get().AllocateTickTaskLevel())
{
	PrecomputedLightVolume = new FPrecomputedLightVolume();
}

ULevel::~ULevel()
{
	FTickTaskManagerInterface::Get().FreeTickTaskLevel(TickTaskLevel);
	TickTaskLevel = NULL;
}


void ULevel::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	ULevel* This = CastChecked<ULevel>(InThis);

	// Let GC know that we're referencing some UTexture2D objects
	for( auto& It : This->TextureToInstancesMap )
	{
		UTexture2D* Texture2D = It.Key;
		Collector.AddReferencedObject( Texture2D, This );
	}

	// Let GC know that we're referencing some UTexture2D objects
	for( auto& It : This->DynamicTextureInstances )
	{
		UPrimitiveComponent* Primitive = It.Key.Get();
		const TArray<FDynamicTextureInstance>& TextureInstances = It.Value;

		for ( FDynamicTextureInstance& Instance : It.Value )
		{
			Collector.AddReferencedObject( Instance.Texture, This );
		}
	}

	// Let GC know that we're referencing some UTexture2D objects
	for( auto& It : This->ForceStreamTextures )
	{
		UTexture2D* Texture2D = It.Key;
		Collector.AddReferencedObject( Texture2D, This );
	}

	Super::AddReferencedObjects( This, Collector );
}

// Compatibility classes
struct FOldGuidPair
{
public:
	FGuid	Guid;
	uint32	RefId;

	friend FArchive& operator<<( FArchive& Ar, FOldGuidPair& GP )
	{
		Ar << GP.Guid << GP.RefId;
		return Ar;
	}
};

struct FLegacyCoverIndexPair
{
	TLazyObjectPtr<class ACoverLink> CoverRef;
	uint8	SlotIdx;

	friend FArchive& operator<<( FArchive& Ar, struct FLegacyCoverIndexPair& IP )
	{
		Ar << IP.CoverRef << IP.SlotIdx;
		return Ar;
	}
};

void ULevel::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	Ar << Model;

	Ar << ModelComponents;

	if(Ar.UE4Ver() < VER_UE4_REMOVE_USEQUENCE)
	{
		TArray<UObject*> DummySequences;
		Ar << DummySequences;
	}

	if(!Ar.IsFilterEditorOnly() || (Ar.UE4Ver() < VER_UE4_EDITORONLY_BLUEPRINTS) )
	{
#if WITH_EDITORONLY_DATA
		// Skip serializing the LSBP if this is a world duplication for PIE/SIE, as it is not needed, and it causes overhead in startup times
		if( (Ar.GetPortFlags() & PPF_DuplicateForPIE ) == 0 )
		{
			Ar << LevelScriptBlueprint;
		}
		else
#endif //WITH_EDITORONLY_DATA
		{
			UObject* DummyBP = NULL;
			Ar << DummyBP;
		}
	}

	if( !Ar.IsTransacting() )
	{
		Ar << LevelScriptActor;

		if( ( Ar.IsLoading() && !FPlatformProperties::SupportsTextureStreaming() ) ||
			( Ar.IsCooking() && !Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::TextureStreaming) ) )
		{
			// Strip for unsupported platforms
			TMap< UTexture2D*, TArray< FStreamableTextureInstance > >		Dummy0;
			TMap< UPrimitiveComponent*, TArray< FDynamicTextureInstance > >	Dummy1;
			Ar << Dummy0;
			Ar << Dummy1;
		}
		else
		{
			Ar << TextureToInstancesMap;
			Ar << DynamicTextureInstances;
		}

		bool bIsCooked = Ar.IsCooking();
		if (Ar.UE4Ver() >= VER_UE4_REBUILD_TEXTURE_STREAMING_DATA_ON_LOAD)
		{
			Ar << bIsCooked;
		}
		if (Ar.UE4Ver() >= VER_UE4_EXPLICIT_STREAMING_TEXTURE_BUILT && Ar.UE4Ver() < VER_UE4_REBUILD_TEXTURE_STREAMING_DATA_ON_LOAD)
		{
			bool bTextureStreamingBuiltDummy = bIsCooked;
			Ar << bTextureStreamingBuiltDummy;
		}
		if (Ar.IsLoading())
		{
			// Always rebuild texture streaming data after loading
			bTextureStreamingBuilt = false;
		}

		//@todo legacy, useless
		if (Ar.IsLoading())
		{
			uint32 Size;
			Ar << Size;
			Ar.Seek(Ar.Tell() + Size);
		}
		else if (Ar.IsSaving())
		{
			uint32 Len = 0;
			Ar << Len;
		}

		if(Ar.UE4Ver() < VER_UE4_REMOVE_LEVELBODYSETUP)
		{
			UBodySetup* DummySetup;
			Ar << DummySetup;
		}

		if( ( Ar.IsLoading() && !FPlatformProperties::SupportsTextureStreaming() ) ||
			( Ar.IsCooking() && !Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::TextureStreaming) ) )
		{
			TMap< UTexture2D*, bool > Dummy;
			Ar << Dummy;
		}
		else
		{
			Ar << ForceStreamTextures;
		}
	}

	// Mark archive and package as containing a map if we're serializing to disk.
	if( !HasAnyFlags( RF_ClassDefaultObject ) && Ar.IsPersistent() )
	{
		Ar.ThisContainsMap();
		GetOutermost()->ThisContainsMap();
	}

	// serialize the nav list
	Ar << NavListStart;
	Ar << NavListEnd;
	// and pylons
	if (Ar.UE4Ver() < VER_UE4_REMOVED_OLD_NAVMESH)
	{
		if (Ar.IsLoading())
		{
			// pylons were removed
			// so if reading old data, just read and discard these 2 pointers
			AActor* LegacyPylonListStart;
			AActor* LegacyPylonListEnd;
			Ar << LegacyPylonListStart;
			Ar << LegacyPylonListEnd;
		}
	}

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FPrecomputedLightVolume DummyVolume;
		Ar << DummyVolume;
	}
	else
	{
		Ar << *PrecomputedLightVolume;
	}

	Ar << PrecomputedVisibilityHandler;
	Ar << PrecomputedVolumeDistanceField;

	if (Ar.UE4Ver() >= VER_UE4_WORLD_LEVEL_INFO &&
		Ar.UE4Ver() < VER_UE4_WORLD_LEVEL_INFO_UPDATED)
	{
		FWorldTileInfo Info;
		Ar << Info;
	}
}


void ULevel::SortActorList()
{
	int32 StartIndex = 0;
	TArray<AActor*> NewActors;
	NewActors.Reserve(Actors.Num());

	// The world info and default brush have fixed actor indices.
	NewActors.Add(Actors[StartIndex++]);
	NewActors.Add(Actors[StartIndex++]);

	// Static not net relevant actors.
	for (int32 ActorIndex = StartIndex; ActorIndex < Actors.Num(); ActorIndex++)
	{
		AActor* Actor = Actors[ActorIndex];
		if (Actor != NULL && !Actor->IsPendingKill() && Actor->GetRemoteRole() == ROLE_None)
		{
			NewActors.Add(Actor);
		}
	}
	iFirstNetRelevantActor = NewActors.Num();

	// Static net relevant actors.
	for (int32 ActorIndex = StartIndex; ActorIndex < Actors.Num(); ActorIndex++)
	{
		AActor* Actor = Actors[ActorIndex];		
		if (Actor != NULL && !Actor->IsPendingKill() && Actor->GetRemoteRole() > ROLE_None)
		{
			NewActors.Add(Actor);
		}
	}

	// Replace with sorted list.
	Actors.AssignButKeepOwner(NewActors);

	// Don't use sorted optimization outside of gameplay so we can safely shuffle around actors e.g. in the Editor
	// without there being a chance to break code using dynamic/ net relevant actor iterators.
	if (!OwningWorld->IsGameWorld())
	{
		iFirstNetRelevantActor = 0;
	}

	// Add all network actors to the owning world
	if ( OwningWorld != NULL )
	{
		for ( int32 i = iFirstNetRelevantActor; i < Actors.Num(); i++ )
		{
			if ( Actors[ i ] != NULL )
			{
				OwningWorld->AddNetworkActor( Actors[ i ] );
			}
		}
	}
}


void ULevel::ValidateLightGUIDs()
{
	for( TObjectIterator<ULightComponent> It; It; ++It )
	{
		ULightComponent*	LightComponent	= *It;
		bool				IsInLevel		= LightComponent->IsIn( this );

		if( IsInLevel )
		{
			LightComponent->ValidateLightGUIDs();
		}
	}
}


void ULevel::PreSave()
{
	Super::PreSave();

#if WITH_EDITOR
	if( !IsTemplate() )
	{
		UPackage* Package = CastChecked<UPackage>(GetOutermost());

		ValidateLightGUIDs();

		// Clear out any crosslevel references
		for( int32 ActorIdx = 0; ActorIdx < Actors.Num(); ActorIdx++ )
		{
			AActor *Actor = Actors[ActorIdx];
			if( Actor != NULL )
			{
				Actor->ClearCrossLevelReferences();
			}
		}
	}
#endif // WITH_EDITOR
}


void ULevel::PostLoad()
{
	Super::PostLoad();

	// Ensure that the level is pointed to the owning world.  For streamed levels, this will be the world of the P map
	// they are streamed in to which we cached when the package loading was invoked
	OwningWorld = ULevel::StreamedLevelsOwningWorld.FindRef(GetOutermost()->GetFName());
	if (OwningWorld == NULL)
	{
		OwningWorld = CastChecked<UWorld>(GetOuter());
	}
	else
	{
		// This entry will not be used anymore, remove it
		ULevel::StreamedLevelsOwningWorld.Remove(GetOutermost()->GetFName());
	}

	UWorldComposition::OnLevelPostLoad(this);
		
#if WITH_EDITOR
	if(GetLinkerUE4Version() < VER_UE4_CLEAR_STANDALONE_FROM_LEVEL_SCRIPT_BLUEPRINTS)
	{
		if (LevelScriptBlueprint)
		{
			LevelScriptBlueprint->ConditionalPostLoad();
			LevelScriptBlueprint->ClearFlags(RF_Standalone);
		}
	}
#endif //WITH_EDITOR

	// in the Editor, sort Actor list immediately (at runtime we wait for the level to be added to the world so that it can be delayed in the level streaming case)
	if (GIsEditor)
	{
		SortActorList();
	}

	// Remove UTexture2D references that are NULL (missing texture).
	ForceStreamTextures.Remove( NULL );

	// Validate navigable geometry
	if (Model == NULL || Model->NumUniqueVertices == 0)
	{
		StaticNavigableGeometry.Empty();
	} 
}


void ULevel::ClearLevelComponents()
{
	bAreComponentsCurrentlyRegistered = false;

	// Remove the model components from the scene.
	for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
	{
		if(ModelComponents[ComponentIndex] && ModelComponents[ComponentIndex]->IsRegistered())
		{
			ModelComponents[ComponentIndex]->UnregisterComponent();
		}
	}

	TArray<UWorld*> Worlds;
	// Remove the actors' components from the scene and build a list of relevant worlds
	for( int32 ActorIndex=0; ActorIndex<Actors.Num(); ActorIndex++ )
	{
		AActor* Actor = Actors[ActorIndex];
		if( Actor )
		{
			Actor->UnregisterAllComponents();
			if( Actor->GetWorld() )
			{
				Worlds.AddUnique( Actor->GetWorld() );
			}
		}
	}

	// clear global motion blur state info in the relevant worlds
	for( int32 WorldIndex=0; WorldIndex<Worlds.Num(); WorldIndex++ )
	{
		if(Worlds[WorldIndex]->PersistentLevel == this && Worlds[WorldIndex]->Scene)
		{
			Worlds[WorldIndex]->Scene->SetClearMotionBlurInfoGameThread();
		}
	}
}

void ULevel::BeginDestroy()
{
	if ( GStreamingManager )
	{
		// At this time, referenced UTexture2Ds are still in memory.
		GStreamingManager->RemoveLevel( this );
	}

	Super::BeginDestroy();

	if (OwningWorld && IsPersistentLevel() && OwningWorld->Scene)
	{
		OwningWorld->Scene->SetPrecomputedVisibility(NULL);
		OwningWorld->Scene->SetPrecomputedVolumeDistanceField(NULL);

		RemoveFromSceneFence.BeginFence();
	}
}

bool ULevel::IsReadyForFinishDestroy()
{
	const bool bReady = Super::IsReadyForFinishDestroy();
	return bReady && RemoveFromSceneFence.IsFenceComplete();
}

void ULevel::FinishDestroy()
{
	delete PrecomputedLightVolume;
	PrecomputedLightVolume = NULL;

	Super::FinishDestroy();
}

/**
* A TMap key type used to sort BSP nodes by locality and zone.
*/
struct FModelComponentKey
{
	uint32	X;
	uint32	Y;
	uint32	Z;
	uint32	MaskedPolyFlags;

	friend bool operator==(const FModelComponentKey& A,const FModelComponentKey& B)
	{
		return	A.X == B.X 
			&&	A.Y == B.Y 
			&&	A.Z == B.Z 
			&&	A.MaskedPolyFlags == B.MaskedPolyFlags;
	}

	friend uint32 GetTypeHash(const FModelComponentKey& Key)
	{
		return FCrc::MemCrc_DEPRECATED(&Key,sizeof(Key));
	}
};


void ULevel::UpdateLevelComponents(bool bRerunConstructionScripts)
{
	// Update all components in one swoop.
	IncrementalUpdateComponents( 0, bRerunConstructionScripts );
}


void ULevel::IncrementalUpdateComponents(int32 NumActorsToUpdate, bool bRerunConstructionScripts)
{
	// A value of 0 means that we want to update all components.
	if( NumActorsToUpdate == 0 )
	{
		NumActorsToUpdate = MAX_int32;
	}
	// Only the game can use incremental update functionality.
	else
	{
		checkf(!GIsEditor && OwningWorld->IsGameWorld(),TEXT("Cannot call IncrementalUpdateComponents with non 0 argument in the Editor/ commandlets."));
	}

	// Do BSP on the first pass.
	if( CurrentActorIndexForUpdateComponents == 0 )
	{
		UpdateModelComponents();
	}

	for( int32 i=0; i < NumActorsToUpdate && CurrentActorIndexForUpdateComponents < Actors.Num(); i++ )
	{
		AActor* Actor = Actors[CurrentActorIndexForUpdateComponents++];
		if( Actor )
		{
#if PERF_TRACK_DETAILED_ASYNC_STATS
				double Start = FPlatformTime::Seconds();
#endif

				Actor->ReregisterAllComponents();

				// Rerun the construction script (if the actor is blueprint based and this is being run in the editor)
				// @TODO: Only do this if blueprint has changed!
				if (bRerunConstructionScripts && !IsTemplate() && !GIsUCCMakeStandaloneHeaderGenerator)
				{
					Actor->RerunConstructionScripts();
				}

#if PERF_TRACK_DETAILED_ASYNC_STATS
				// Add how long this took to class->time map
				double Time = FPlatformTime::Seconds() - Start;
				UClass* ActorClass = Actor->GetClass();
				FMapTimeEntry* CurrentEntry = UpdateComponentsTimePerActorClass.Find(ActorClass);
				// Is an existing entry - add to it
				if(CurrentEntry)
				{
					CurrentEntry->Time += Time;
					CurrentEntry->ObjCount += 1;
				}
				// Make a new entry for this class
				else
				{
					UpdateComponentsTimePerActorClass.Add(ActorClass, FMapTimeEntry(ActorClass, 1, Time));
				}
#endif
			}
		}

	// See whether we are done.
	if( CurrentActorIndexForUpdateComponents == Actors.Num() )
	{
		CurrentActorIndexForUpdateComponents	= 0;
		bAreComponentsCurrentlyRegistered		= true;
	}
	// Only the game can use incremental update functionality.
	else
	{
		// The editor is never allowed to incrementally updated components.  Make sure to pass in a value of zero for NumActorsToUpdate.
		check( OwningWorld->IsGameWorld() );
	}
}

#if WITH_EDITOR

void ULevel::CreateModelComponents()
{
	// Update the model vertices and edges.
	Model->UpdateVertices();

	Model->InvalidSurfaces = 0;

	// Clear the model index buffers.
	Model->MaterialIndexBuffers.Empty();

	TMap< FModelComponentKey, TArray<uint16> > ModelComponentMap;

	// Sort the nodes by zone, grid cell and masked poly flags.
	for(int32 NodeIndex = 0;NodeIndex < Model->Nodes.Num();NodeIndex++)
	{
		FBspNode& Node = Model->Nodes[NodeIndex];
		FBspSurf& Surf = Model->Surfs[Node.iSurf];

		if(Node.NumVertices > 0)
		{
			for(int32 BackFace = 0;BackFace < ((Surf.PolyFlags & PF_TwoSided) ? 2 : 1);BackFace++)
			{
				// Calculate the bounding box of this node.
				FBox NodeBounds(0);
				for(int32 VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
				{
					NodeBounds += Model->Points[Model->Verts[Node.iVertPool + VertexIndex].pVertex];
				}

				// Create a sort key for this node using the grid cell containing the center of the node's bounding box.
#define MODEL_GRID_SIZE_XY	2048.0f
#define MODEL_GRID_SIZE_Z	4096.0f
				FModelComponentKey Key;
				check( OwningWorld );
				if (OwningWorld->GetWorldSettings()->bMinimizeBSPSections)
				{
					Key.X				= 0;
					Key.Y				= 0;
					Key.Z				= 0;
				}
				else
				{
					Key.X				= FMath::Floor(NodeBounds.GetCenter().X / MODEL_GRID_SIZE_XY);
					Key.Y				= FMath::Floor(NodeBounds.GetCenter().Y / MODEL_GRID_SIZE_XY);
					Key.Z				= FMath::Floor(NodeBounds.GetCenter().Z / MODEL_GRID_SIZE_Z);
				}

				Key.MaskedPolyFlags = Surf.PolyFlags & PF_ModelComponentMask;

				// Find an existing node list for the grid cell.
				TArray<uint16>* ComponentNodes = ModelComponentMap.Find(Key);
				if(!ComponentNodes)
				{
					// This is the first node we found in this grid cell, create a new node list for the grid cell.
					ComponentNodes = &ModelComponentMap.Add(Key,TArray<uint16>());
				}

				// Add the node to the grid cell's node list.
				ComponentNodes->AddUnique(NodeIndex);
			}
		}
		else
		{
			// Put it in component 0 until a rebuild occurs.
			Node.ComponentIndex = 0;
		}
	}

	// Create a UModelComponent for each grid cell's node list.
	for(TMap< FModelComponentKey, TArray<uint16> >::TConstIterator It(ModelComponentMap);It;++It)
	{
		const FModelComponentKey&	Key		= It.Key();
		const TArray<uint16>&			Nodes	= It.Value();	

		for(int32 NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
		{
			Model->Nodes[Nodes[NodeIndex]].ComponentIndex = ModelComponents.Num();							
			Model->Nodes[Nodes[NodeIndex]].ComponentNodeIndex = NodeIndex;
		}

		UModelComponent* ModelComponent = new(this) UModelComponent(FPostConstructInitializeProperties(),Model,ModelComponents.Num(),Key.MaskedPolyFlags,Nodes);
		ModelComponents.Add(ModelComponent);

		for(int32 NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
		{
			Model->Nodes[Nodes[NodeIndex]].ComponentElementIndex = INDEX_NONE;

			const uint16								Node	 = Nodes[NodeIndex];
			const TIndirectArray<FModelElement>&	Elements = ModelComponent->GetElements();
			for( int32 ElementIndex=0; ElementIndex<Elements.Num(); ElementIndex++ )
			{
				if( Elements[ElementIndex].Nodes.Find( Node ) != INDEX_NONE )
				{
					Model->Nodes[Nodes[NodeIndex]].ComponentElementIndex = ElementIndex;
					break;
				}
			}
		}
	}

	// Clear old cached data in case we don't regenerate it below, e.g. after removing all BSP from a level.
	Model->NumIncompleteNodeGroups = 0;
	Model->CachedMappings.Empty();

	// Work only needed if we actually have BSP in the level.
	if( ModelComponents.Num() )
	{
		check( OwningWorld );
		// Build the static lighting vertices!
		/** The lights in the world which the system is building. */
		TArray<ULightComponentBase*> Lights;
		// Prepare lights for rebuild.
		for(TObjectIterator<ULightComponent> LightIt;LightIt;++LightIt)
		{
			ULightComponent* const Light = *LightIt;
			const bool bLightIsInWorld = Light->GetOwner() && OwningWorld->ContainsActor(Light->GetOwner());
			if (bLightIsInWorld && (Light->HasStaticLighting() || Light->HasStaticShadowing()))
			{
				// Make sure the light GUIDs and volumes are up-to-date.
				Light->ValidateLightGUIDs();

				// Add the light to the system's list of lights in the world.
				Lights.Add(Light);
			}
		}

		// For BSP, we aren't Component-centric, so we can't use the GetStaticLightingInfo 
		// function effectively. Instead, we look across all nodes in the Level's model and
		// generate NodeGroups - which are groups of nodes that are coplanar, adjacent, and 
		// have the same lightmap resolution (henceforth known as being "conodes"). Each 
		// NodeGroup will get a mapping created for it

		// create all NodeGroups
		Model->GroupAllNodes(this, Lights);

		// now we need to make the mappings/meshes
		for (TMap<int32, FNodeGroup*>::TIterator It(Model->NodeGroups); It; ++It)
		{
			FNodeGroup* NodeGroup = It.Value();

			if (NodeGroup->Nodes.Num())
			{
				// get one of the surfaces/components from the NodeGroup
				// @todo UE4: Remove need for GetSurfaceLightMapResolution to take a surfaceindex, or a ModelComponent :)
				UModelComponent* SomeModelComponent = ModelComponents[Model->Nodes[NodeGroup->Nodes[0]].ComponentIndex];
				int32 SurfaceIndex = Model->Nodes[NodeGroup->Nodes[0]].iSurf;

				// fill out the NodeGroup/mapping, as UModelComponent::GetStaticLightingInfo did
				SomeModelComponent->GetSurfaceLightMapResolution(SurfaceIndex, true, NodeGroup->SizeX, NodeGroup->SizeY, NodeGroup->WorldToMap, &NodeGroup->Nodes);
				NodeGroup->MapToWorld = NodeGroup->WorldToMap.Inverse();

				// Cache the surface's vertices and triangles.
				NodeGroup->BoundingBox.Init();

				for(int32 NodeIndex = 0;NodeIndex < NodeGroup->Nodes.Num();NodeIndex++)
				{
					const FBspNode& Node = Model->Nodes[NodeGroup->Nodes[NodeIndex]];
					const FBspSurf& NodeSurf = Model->Surfs[Node.iSurf];
					const FVector& TextureBase = Model->Points[NodeSurf.pBase];
					const FVector& TextureX = Model->Vectors[NodeSurf.vTextureU];
					const FVector& TextureY = Model->Vectors[NodeSurf.vTextureV];
					const int32 BaseVertexIndex = NodeGroup->Vertices.Num();
					// Compute the surface's tangent basis.
					FVector NodeTangentX = Model->Vectors[NodeSurf.vTextureU].SafeNormal();
					FVector NodeTangentY = Model->Vectors[NodeSurf.vTextureV].SafeNormal();
					FVector NodeTangentZ = Model->Vectors[NodeSurf.vNormal].SafeNormal();

					// Generate the node's vertices.
					for(uint32 VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
					{
						/*const*/ FVert& Vert = Model->Verts[Node.iVertPool + VertexIndex];
						const FVector& VertexWorldPosition = Model->Points[Vert.pVertex];

						FStaticLightingVertex* DestVertex = new(NodeGroup->Vertices) FStaticLightingVertex;
						DestVertex->WorldPosition = VertexWorldPosition;
						DestVertex->TextureCoordinates[0].X = ((VertexWorldPosition - TextureBase) | TextureX) / 128.0f;
						DestVertex->TextureCoordinates[0].Y = ((VertexWorldPosition - TextureBase) | TextureY) / 128.0f;
						DestVertex->TextureCoordinates[1].X = NodeGroup->WorldToMap.TransformPosition(VertexWorldPosition).X;
						DestVertex->TextureCoordinates[1].Y = NodeGroup->WorldToMap.TransformPosition(VertexWorldPosition).Y;
						DestVertex->WorldTangentX = NodeTangentX;
						DestVertex->WorldTangentY = NodeTangentY;
						DestVertex->WorldTangentZ = NodeTangentZ;

						// TEMP - Will be overridden when lighting is build!
						Vert.ShadowTexCoord = DestVertex->TextureCoordinates[1];

						// Include the vertex in the surface's bounding box.
						NodeGroup->BoundingBox += VertexWorldPosition;
					}

					// Generate the node's vertex indices.
					for(uint32 VertexIndex = 2;VertexIndex < Node.NumVertices;VertexIndex++)
					{
						NodeGroup->TriangleVertexIndices.Add(BaseVertexIndex + 0);
						NodeGroup->TriangleVertexIndices.Add(BaseVertexIndex + VertexIndex);
						NodeGroup->TriangleVertexIndices.Add(BaseVertexIndex + VertexIndex - 1);

						// track the source surface for each triangle
						NodeGroup->TriangleSurfaceMap.Add(Node.iSurf);
					}
				}
			}
		}
	}
	Model->UpdateVertices();

	for (int32 UpdateCompIdx = 0; UpdateCompIdx < ModelComponents.Num(); UpdateCompIdx++)
	{
		UModelComponent* ModelComp = ModelComponents[UpdateCompIdx];
		ModelComp->GenerateElements(true);
		ModelComp->InvalidateCollisionData();
	}
}
#endif

void ULevel::UpdateModelComponents()
{
	// Create/update the level's BSP model components.
	if(!ModelComponents.Num())
	{
#if WITH_EDITOR
		CreateModelComponents();
#endif // WITH_EDITOR
	}
	else
	{
		for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
		{
			if(ModelComponents[ComponentIndex] && ModelComponents[ComponentIndex]->IsRegistered())
			{
				ModelComponents[ComponentIndex]->UnregisterComponent();
			}
		}
	}

	if (ModelComponents.Num() > 0)
	{
		check( OwningWorld );
		// Update model components.
		for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
		{
			if(ModelComponents[ComponentIndex])
			{
				ModelComponents[ComponentIndex]->RegisterComponentWithWorld(OwningWorld);
			}
		}
	}

	// Initialize the model's index buffers.
	for(TMap<UMaterialInterface*,TScopedPointer<FRawIndexBuffer16or32> >::TIterator IndexBufferIt(Model->MaterialIndexBuffers);
		IndexBufferIt;
		++IndexBufferIt)
	{
		BeginInitResource(IndexBufferIt.Value());
	}

	// Can now release the model's vertex buffer, will have been used for collision
	if(!IsRunningCommandlet())
	{
		Model->ReleaseVertices();
	}

	Model->bInvalidForStaticLighting = true;
}

#if WITH_EDITOR
void ULevel::PreEditUndo()
{
	Super::PreEditUndo();

	// Release the model's resources.
	Model->BeginReleaseResources();
	Model->ReleaseResourcesFence.Wait();

	// Detach existing model components.  These are left in the array, so they are saved for undoing the undo.
	for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
	{
		if(ModelComponents[ComponentIndex])
		{
			ModelComponents[ComponentIndex]->UnregisterComponent();
		}
	}

	ReleaseRenderingResources();

	// Wait for the components to be detached.
	FlushRenderingCommands();

}


void ULevel::PostEditUndo()
{
	Super::PostEditUndo();

	Model->UpdateVertices();
	// Update model components that were detached earlier
	UpdateModelComponents();

	// If it's a streaming level and was not visible, don't init rendering resources
	if (OwningWorld)
	{
		bool bIsStreamingLevelVisible = false;
		if (OwningWorld->PersistentLevel == this)
		{
			bIsStreamingLevelVisible = FLevelUtils::IsLevelVisible(OwningWorld->PersistentLevel);
		}
		else
		{
			const int32 NumStreamingLevels = OwningWorld->StreamingLevels.Num();
			for (int i = 0; i < NumStreamingLevels; ++i)
			{
				const ULevelStreaming * StreamedLevel = OwningWorld->StreamingLevels[i];
				if (StreamedLevel && StreamedLevel->GetLoadedLevel() == this)
				{
					bIsStreamingLevelVisible = FLevelUtils::IsLevelVisible(StreamedLevel);
					break;
				}
			}
		}

		if (bIsStreamingLevelVisible)
		{
			InitializeRenderingResources();
		}
	}

	// Add the levelscriptactor back into the actors array
	if (LevelScriptActor)
	{
		// LSA is transient so won't track the actors list
		if (!Actors.Contains(LevelScriptActor))
		{
			Actors.Add(LevelScriptActor);
		}
	}

	if (LevelBoundsActor.IsValid())
	{
		LevelBoundsActor.Get()->OnLevelBoundsDirtied();
	}
}
#endif // WITH_EDITOR


void ULevel::InvalidateModelGeometry()
{
	// Save the level/model state for transactions.
	Model->Modify();
	Modify();

	// Begin releasing the model's resources.
	Model->BeginReleaseResources();

	// Remove existing model components.
	for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
	{
		if(ModelComponents[ComponentIndex])
		{
			ModelComponents[ComponentIndex]->Modify();
			ModelComponents[ComponentIndex]->UnregisterComponent();
		}
	}
	ModelComponents.Empty();
}


void ULevel::InvalidateModelSurface()
{
	Model->InvalidSurfaces = true;
}

void ULevel::CommitModelSurfaces()
{
	if(Model->InvalidSurfaces)
	{
		// Unregister model components
		for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
		{
			if(ModelComponents[ComponentIndex] && ModelComponents[ComponentIndex]->IsRegistered())
			{
				ModelComponents[ComponentIndex]->UnregisterComponent();
			}
		}

		// Begin releasing the model's resources.
		Model->BeginReleaseResources();

		// Wait for the model's resources to be released.
		FlushRenderingCommands();

		// Clear the model index buffers.
		Model->MaterialIndexBuffers.Empty();

		// Update the model vertices.
		Model->UpdateVertices();

		// Update the model components.
		for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
		{
			if(ModelComponents[ComponentIndex])
			{
				ModelComponents[ComponentIndex]->CommitSurfaces();
			}
		}
		Model->InvalidSurfaces = false;

		// Register model components before init'ing index buffer so collision has access to index buffer data
		// This matches the order of operation in ULevel::UpdateModelComponents
		if (ModelComponents.Num() > 0)
		{
			check( OwningWorld );
			// Update model components.
			for(int32 ComponentIndex = 0;ComponentIndex < ModelComponents.Num();ComponentIndex++)
			{
				if(ModelComponents[ComponentIndex])
				{
					ModelComponents[ComponentIndex]->RegisterComponentWithWorld(OwningWorld);
				}
			}
		}

		// Initialize the model's index buffers.
		for(TMap<UMaterialInterface*,TScopedPointer<FRawIndexBuffer16or32> >::TIterator IndexBufferIt(Model->MaterialIndexBuffers);
			IndexBufferIt;
			++IndexBufferIt)
		{
			BeginInitResource(IndexBufferIt.Value());
		}
	}
}


void ULevel::BuildStreamingData(UWorld* World, ULevel* TargetLevel/*=NULL*/, UTexture2D* UpdateSpecificTextureOnly/*=NULL*/)
{
#if WITH_EDITORONLY_DATA
	double StartTime = FPlatformTime::Seconds();

	bool bUseDynamicStreaming = false;
	GConfig->GetBool(TEXT("TextureStreaming"), TEXT("UseDynamicStreaming"), bUseDynamicStreaming, GEngineIni);

	TArray<ULevel* > LevelsToCheck;
	if ( TargetLevel )
	{
		LevelsToCheck.Add(TargetLevel);
	}
	else if ( World )
	{
		for ( int32 LevelIndex=0; LevelIndex < World->GetNumLevels(); LevelIndex++ )
		{
			ULevel* Level = World->GetLevel(LevelIndex);
			LevelsToCheck.Add(Level);
		}
	}
	else
	{
		for (TObjectIterator<ULevel> It; It; ++It)
		{
			ULevel* Level = *It;
			LevelsToCheck.Add(Level);
		}
	}

	for ( int32 LevelIndex=0; LevelIndex < LevelsToCheck.Num(); LevelIndex++ )
	{
		ULevel* Level = LevelsToCheck[LevelIndex];
		Level->BuildStreamingData( UpdateSpecificTextureOnly );
	}

	UE_LOG(LogLevel, Verbose, TEXT("ULevel::BuildStreamingData took %.3f seconds."), FPlatformTime::Seconds() - StartTime);
#else
	UE_LOG(LogLevel, Fatal,TEXT("ULevel::BuildStreamingData should not be called on a console"));
#endif
}

void ULevel::BuildStreamingData(UTexture2D* UpdateSpecificTextureOnly/*=NULL*/)
{
	bool bUseDynamicStreaming = false;
	GConfig->GetBool(TEXT("TextureStreaming"), TEXT("UseDynamicStreaming"), bUseDynamicStreaming, GEngineIni);

	if ( UpdateSpecificTextureOnly == NULL )
	{
		// Reset the streaming manager, when building data for a whole Level
		GStreamingManager->RemoveLevel( this );
		TextureToInstancesMap.Empty();
		DynamicTextureInstances.Empty();
		ForceStreamTextures.Empty();
		bTextureStreamingBuilt = false;
	}

	TArray<UObject *> ObjectsInOuter;
	GetObjectsWithOuter(this, ObjectsInOuter);

	for( int32 Index = 0; Index < ObjectsInOuter.Num(); Index++ )
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(ObjectsInOuter[Index]);
		if (Primitive)
		{
			const bool bIsClassDefaultObject = Primitive->IsTemplate(RF_ClassDefaultObject);
			if ( !bIsClassDefaultObject && Primitive->IsRegistered() )
			{
				const AActor* const Owner				= Primitive->GetOwner();
				const bool bIsStaticMeshComponent		= Primitive->IsA(UStaticMeshComponent::StaticClass());
				const bool bIsSkeletalMeshComponent		= Primitive->IsA(USkeletalMeshComponent::StaticClass());
				const bool bIsStatic					= Owner == NULL || Primitive->Mobility == EComponentMobility::Static;

				TArray<FStreamingTexturePrimitiveInfo> PrimitiveStreamingTextures;

				// Ask the primitive to enumerate the streaming textures it uses.
				Primitive->GetStreamingTextureInfo(PrimitiveStreamingTextures);

				for(int32 TextureIndex = 0;TextureIndex < PrimitiveStreamingTextures.Num();TextureIndex++)
				{
					const FStreamingTexturePrimitiveInfo& PrimitiveStreamingTexture = PrimitiveStreamingTextures[TextureIndex];
					UTexture2D* Texture2D = Cast<UTexture2D>(PrimitiveStreamingTexture.Texture);
					bool bCanBeStreamedByDistance = !FMath::IsNearlyZero(PrimitiveStreamingTexture.TexelFactor) && !FMath::IsNearlyZero(PrimitiveStreamingTexture.Bounds.W);

					// Only handle 2D textures that match the target texture.
					const bool bIsTargetTexture = (!UpdateSpecificTextureOnly || UpdateSpecificTextureOnly == Texture2D);
					bool bShouldHandleTexture = (Texture2D && bIsTargetTexture);

					// Check if this is a lightmap/shadowmap that shouldn't be streamed.
					if ( bShouldHandleTexture )
					{
						UShadowMapTexture2D* ShadowMap2D	= Cast<UShadowMapTexture2D>(Texture2D);
						ULightMapTexture2D* Lightmap2D		= Cast<ULightMapTexture2D>(Texture2D);
						if ( (Lightmap2D && (Lightmap2D->LightmapFlags & LMF_Streamed) == 0) ||
							(ShadowMap2D && (ShadowMap2D->ShadowmapFlags & SMF_Streamed) == 0) )
						{
							bShouldHandleTexture			= false;
						}
					}

					// Check if this is a duplicate texture
					if (bShouldHandleTexture)
					{
						for (int32 HandledTextureIndex = 0; HandledTextureIndex < TextureIndex; HandledTextureIndex++)
						{
							const FStreamingTexturePrimitiveInfo& HandledStreamingTexture = PrimitiveStreamingTextures[HandledTextureIndex];
							if ( PrimitiveStreamingTexture.Texture == HandledStreamingTexture.Texture &&
								FMath::IsNearlyEqual(PrimitiveStreamingTexture.TexelFactor, HandledStreamingTexture.TexelFactor) &&
								PrimitiveStreamingTexture.Bounds.Equals( HandledStreamingTexture.Bounds ) )
							{
								// It's a duplicate, don't handle this one.
								bShouldHandleTexture = false;
								break;
							}
						}
					}

					if(bShouldHandleTexture)
					{
						// Check if this is a world texture.
						const bool bIsWorldTexture			= 
							Texture2D->LODGroup == TEXTUREGROUP_World ||
							Texture2D->LODGroup == TEXTUREGROUP_WorldNormalMap ||
							Texture2D->LODGroup == TEXTUREGROUP_WorldSpecular;

						// Check if we should consider this a static mesh texture instance.
						bool bIsStaticMeshTextureInstance = bIsWorldTexture && !bIsSkeletalMeshComponent;

						// Treat non-static textures dynamically instead.
						if ( !bIsStatic && bUseDynamicStreaming )
						{
							bIsStaticMeshTextureInstance = false;
						}

						// Is the primitive set to force its textures to be resident?
						if ( Primitive->bForceMipStreaming )
						{
							// Add them to the ForceStreamTextures set.
							ForceStreamTextures.Add(Texture2D,true);
						}
						// Is this a static mesh texture instance?
						else if ( bIsStaticMeshTextureInstance && bCanBeStreamedByDistance )
						{
							// Texture instance information.
							FStreamableTextureInstance TextureInstance;
							TextureInstance.BoundingSphere	= PrimitiveStreamingTexture.Bounds;
							TextureInstance.TexelFactor		= PrimitiveStreamingTexture.TexelFactor;

							// See whether there already is an instance in the level.
							TArray<FStreamableTextureInstance>* TextureInstances = TextureToInstancesMap.Find( Texture2D );
							// We have existing instances.
							if( TextureInstances )
							{
								// Add to the array.
								TextureInstances->Add( TextureInstance );
							}
							// This is the first instance.
							else
							{
								// Create array with current instance as the only entry.
								TArray<FStreamableTextureInstance> NewTextureInstances;
								NewTextureInstances.Add( TextureInstance );
								// And set it.
								TextureToInstancesMap.Add( Texture2D, NewTextureInstances );
							}
						}
						// Is the texture used by a dynamic object that we can track at run-time.
						else if ( bUseDynamicStreaming && Owner && bCanBeStreamedByDistance )
						{
							// Texture instance information.
							FDynamicTextureInstance TextureInstance;
							TextureInstance.Texture = Texture2D;
							TextureInstance.BoundingSphere = PrimitiveStreamingTexture.Bounds;
							TextureInstance.TexelFactor	= PrimitiveStreamingTexture.TexelFactor;
							TextureInstance.OriginalRadius = PrimitiveStreamingTexture.Bounds.W;

							// See whether there already is an instance in the level.
							TArray<FDynamicTextureInstance>* TextureInstances = DynamicTextureInstances.Find( Primitive );
							// We have existing instances.
							if( TextureInstances )
							{
								// Add to the array.
								TextureInstances->Add( TextureInstance );
							}
							// This is the first instance.
							else
							{
								// Create array with current instance as the only entry.
								TArray<FDynamicTextureInstance> NewTextureInstances;
								NewTextureInstances.Add( TextureInstance );
								// And set it.
								DynamicTextureInstances.Add( Primitive, NewTextureInstances );
							}
						}
					}
				}
			}
		}
	}

	if ( UpdateSpecificTextureOnly == NULL )
	{
		// Normalize the texelfactor for lightmaps and shadowmaps
		NormalizeLightmapTexelFactor();

		bTextureStreamingBuilt = true;

		// Update the streaming manager.
		GStreamingManager->AddPreparedLevel( this );
	}
}


void ULevel::NormalizeLightmapTexelFactor()
{
	for ( TMap<UTexture2D*,TArray<FStreamableTextureInstance> >::TIterator It(TextureToInstancesMap); It; ++It )
	{
		UTexture2D* Texture2D = It.Key();
		if ( Texture2D->LODGroup == TEXTUREGROUP_Lightmap || Texture2D->LODGroup == TEXTUREGROUP_Shadowmap)
		{
			TArray<FStreamableTextureInstance>& TextureInstances = It.Value();

			// Clamp texelfactors to 20-80% range.
			// This is to prevent very low-res or high-res charts to dominate otherwise decent streaming.
			struct FCompareTexelFactor
			{
				FORCEINLINE bool operator()( const FStreamableTextureInstance& A, const FStreamableTextureInstance& B ) const
				{
					return A.TexelFactor < B.TexelFactor;
				}
			};
			TextureInstances.Sort( FCompareTexelFactor() );

			float MinTexelFactor = TextureInstances[ TextureInstances.Num() * 0.2f ].TexelFactor;
			float MaxTexelFactor = TextureInstances[ TextureInstances.Num() * 0.8f ].TexelFactor;
			for ( int32 InstanceIndex=0; InstanceIndex < TextureInstances.Num(); ++InstanceIndex )
			{
				FStreamableTextureInstance& Instance = TextureInstances[InstanceIndex];
				Instance.TexelFactor = FMath::Clamp( Instance.TexelFactor, MinTexelFactor, MaxTexelFactor );
			}
		}
	}
}

TArray<FStreamableTextureInstance>* ULevel::GetStreamableTextureInstances(UTexture2D*& TargetTexture)
{
	typedef TArray<FStreamableTextureInstance>	STIA_Type;
	for (TMap<UTexture2D*,STIA_Type>::TIterator It(TextureToInstancesMap); It; ++It)
	{
		TArray<FStreamableTextureInstance>& TSIA = It.Value();
		TargetTexture = It.Key();
		return &TSIA;
	}		

	return NULL;
}


ABrush* ULevel::GetBrush() const
{
	checkf( Actors.Num() >= 2, *GetPathName() );
	ABrush* DefaultBrush = Cast<ABrush>( Actors[1] );
	checkf( DefaultBrush != NULL, *GetPathName() );
	checkf( DefaultBrush->BrushComponent, *GetPathName() );
	checkf( DefaultBrush->Brush != NULL, *GetPathName() );
	return DefaultBrush;
}


AWorldSettings* ULevel::GetWorldSettings() const
{
	checkf( Actors.Num() >= 2, *GetPathName() );
	AWorldSettings* WorldSettings = Cast<AWorldSettings>( Actors[0] );
	checkf( WorldSettings != NULL, *GetPathName() );
	return WorldSettings;
}

ALevelScriptActor* ULevel::GetLevelScriptActor() const
{
	return LevelScriptActor;
}


void ULevel::InitializeActors()
{
	check( OwningWorld );
	bool			bIsServer				= OwningWorld->IsServer();
	APhysicsVolume*	DefaultPhysicsVolume	= OwningWorld->GetDefaultPhysicsVolume();

	// Kill non relevant client actors, initialize render time, set initial physic volume, initialize script execution and rigid body physics.
	for( int32 ActorIndex=0; ActorIndex<Actors.Num(); ActorIndex++ )
	{
		AActor* Actor = Actors[ActorIndex];
		if( Actor )
		{
			// Kill off actors that aren't interesting to the client.
			if( !Actor->bActorInitialized && !Actor->bActorSeamlessTraveled )
			{
				// Add to startup list
				if (Actor->bNetLoadOnClient)
				{
					Actor->bNetStartup = true;
				}

				if (!bIsServer)
				{
					if (!Actor->bNetLoadOnClient)
					{
						Actor->Destroy();
					}
					else
					{
						// Exchange the roles if:
						//	-We are a client
						//  -This is bNetLoadOnClient=true
						//  -RemoteRole != ROLE_None
						Actor->ExchangeNetRoles(true);
					}
				}				
			}

			Actor->bActorSeamlessTraveled = false;
		}
	}
}

void ULevel::InitializeRenderingResources()
{
	// OwningWorld can be NULL when InitializeRenderingResources is called during undo, where a transient ULevel is created to allow undoing level move operations
	// At the point at which Pre/PostEditChange is called on that transient ULevel, it is not part of any world and therefore should not have its rendering resources initialized
	if (OwningWorld)
	{
		PrecomputedLightVolume->AddToScene(OwningWorld->Scene);

		if (OwningWorld->Scene)
		{
			OwningWorld->Scene->OnLevelAddedToWorld(GetOutermost()->GetFName());
		}
	}
}

void ULevel::ReleaseRenderingResources()
{
	if (OwningWorld && PrecomputedLightVolume)
	{
		PrecomputedLightVolume->RemoveFromScene(OwningWorld->Scene);
	}
}

void ULevel::RouteActorInitialize()
{
	// Send PreInitializeComponents and collect volumes.
	for( int32 ActorIndex=0; ActorIndex<Actors.Num(); ActorIndex++ )
	{
		AActor* const Actor = Actors[ActorIndex];
		if( Actor )
		{
			if( !Actor->bActorInitialized && Actor->bWantsInitialize )
			{
				Actor->PreInitializeComponents();
			}
		}
	}

	const bool bCallBeginPlay = OwningWorld->bMatchStarted;

	// Send InitializeComponents on components and PostInitializeComponents.
	for( int32 ActorIndex=0; ActorIndex<Actors.Num(); ActorIndex++ )
	{
		AActor* Actor = Actors[ActorIndex];
		if( Actor )
		{
			if( !Actor->bActorInitialized )
			{
				// Call BeginPlay on Components.
				Actor->InitializeComponents();

				if(Actor->bWantsInitialize)
				{
					Actor->PostInitializeComponents(); // should set Actor->bActorInitialized = true
					if (!Actor->bActorInitialized && !Actor->IsPendingKill())
					{
						UE_LOG(LogActor, Fatal, TEXT("%s failed to route PostInitializeComponents.  Please call Super::PostInitializeComponents() in your <className>::PostInitializeComponents() function. "), *Actor->GetFullName() );
					}
				}
				else
				{
					Actor->bActorInitialized = true;
				}

				if (bCallBeginPlay)
				{
					Actor->BeginPlay();
				}
			}

			// Components are all set up, init touching state.
			// Note: Not doing notifies here since loading or streaming in isn't actually conceptually beginning a touch.
			//	     Rather, it was always touching and the mechanics of loading is just an implementation detail.
			Actor->UpdateOverlaps(false);
		}
	}
}

bool ULevel::HasAnyActorsOfType(UClass *SearchType)
{
	// just search the actors array
	for (int32 Idx = 0; Idx < Actors.Num(); Idx++)
	{
		AActor *Actor = Actors[Idx];
		// if valid, not pending kill, and
		// of the correct type
		if (Actor != NULL &&
			!Actor->IsPendingKill() &&
			Actor->IsA(SearchType))
		{
			return true;
		}
	}
	return false;
}

#if WITH_EDITOR

ULevelScriptBlueprint* ULevel::GetLevelScriptBlueprint(bool bDontCreate)
{
	const FString LevelScriptName = ULevelScriptBlueprint::CreateLevelScriptNameFromLevel(this);
	if( !LevelScriptBlueprint && !bDontCreate)
	{
		// If no blueprint is found, create one. 
		LevelScriptBlueprint = Cast<ULevelScriptBlueprint>(FKismetEditorUtilities::CreateBlueprint(GEngine->LevelScriptActorClass, this, FName(*LevelScriptName), BPTYPE_LevelScript, ULevelScriptBlueprint::StaticClass()));

		// LevelScript blueprints should not be standalone
		LevelScriptBlueprint->ClearFlags(RF_Standalone);
		ULevel::LevelDirtiedEvent.Broadcast();
	}

	// Ensure that friendly name is always up-to-date
	if (LevelScriptBlueprint)
	{
		LevelScriptBlueprint->FriendlyName = LevelScriptName;
	}

	return LevelScriptBlueprint;
}

void ULevel::OnLevelScriptBlueprintChanged(ULevelScriptBlueprint* InBlueprint)
{
	if( !InBlueprint->bIsRegeneratingOnLoad )
	{
		// Make sure this is OUR level scripting blueprint
		check(InBlueprint == LevelScriptBlueprint);
		UClass* SpawnClass = (LevelScriptBlueprint->GeneratedClass) ? LevelScriptBlueprint->GeneratedClass : LevelScriptBlueprint->SkeletonGeneratedClass;

		// Get rid of the old LevelScriptActor
		if( LevelScriptActor )
		{
			LevelScriptActor->Destroy();
			LevelScriptActor = NULL;
		}

		check( OwningWorld );
		// Create the new one
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.OverrideLevel = this;
		LevelScriptActor = OwningWorld->SpawnActor<ALevelScriptActor>( SpawnClass, SpawnInfo );
		LevelScriptActor->ClearFlags(RF_Transactional);
		check(LevelScriptActor->GetOuter() == this);

		if( LevelScriptActor )
		{
			// Finally, fixup all the bound events to point to their new LSA
			FBlueprintEditorUtils::FixLevelScriptActorBindings(LevelScriptActor, InBlueprint);
		}		
	}
}	

#endif	//WITH_EDITOR


#if WITH_EDITOR
void ULevel::AddMovieSceneBindings( class UMovieSceneBindings* MovieSceneBindings )
{
	// @todo sequencer major: We need to clean up stale bindings objects that no PlayMovieScene node references anymore (LevelScriptBlueprint compile time?)
	if( ensure( MovieSceneBindings != NULL ) )
	{
		Modify();

		// @todo sequencer UObjects: Dangerous cast here to work around MovieSceneCore not being a dependency module of engine
		UObject* ObjectToAdd = (UObject*)MovieSceneBindings;
		MovieSceneBindingsArray.AddUnique( ObjectToAdd );
	}
}

void ULevel::ClearMovieSceneBindings()
{
	Modify();

	MovieSceneBindingsArray.Reset();
}
#endif

void ULevel::AddActiveRuntimeMovieScenePlayer( UObject* RuntimeMovieScenePlayer )
{
	ActiveRuntimeMovieScenePlayers.Add( RuntimeMovieScenePlayer );
}


void ULevel::TickRuntimeMovieScenePlayers( const float DeltaSeconds )
{
	for( int32 CurPlayerIndex = 0; CurPlayerIndex < ActiveRuntimeMovieScenePlayers.Num(); ++CurPlayerIndex )
	{
		// Should never have any active RuntimeMovieScenePlayers on an editor world!
		check( OwningWorld->IsGameWorld() );

		IRuntimeMovieScenePlayerInterface* RuntimeMovieScenePlayer =
			InterfaceCast< IRuntimeMovieScenePlayerInterface >( ActiveRuntimeMovieScenePlayers[ CurPlayerIndex ] );
		check( RuntimeMovieScenePlayer != NULL );

		// @todo sequencer runtime: Support expiring instances of RuntimeMovieScenePlayers that have finished playing
		// @todo sequencer runtime: Support destroying spawned actors for a completed moviescene
		RuntimeMovieScenePlayer->Tick( DeltaSeconds );
	}
}


bool ULevel::IsPersistentLevel() const
{
	bool bIsPersistent = false;
	if( OwningWorld )
	{
		bIsPersistent = (this == OwningWorld->PersistentLevel);
	}
	return bIsPersistent;
}

bool ULevel::IsCurrentLevel() const
{
	bool bIsCurrent = false;
	if( OwningWorld )
	{
		bIsCurrent = (this == OwningWorld->GetCurrentLevel());
	}
	return bIsCurrent;
}

void ULevel::ApplyWorldOffset(const FVector& InWorldOffset, bool bWorldShift)
{
	// Update texture streaming data to account for the move
	for (TMap< UTexture2D*, TArray<FStreamableTextureInstance> >::TIterator It(TextureToInstancesMap); It; ++It)
	{
		TArray<FStreamableTextureInstance>& TextureInfo = It.Value();
		for (int32 i = 0; i < TextureInfo.Num(); i++)
		{
			TextureInfo[i].BoundingSphere.Center+= InWorldOffset;
		}
	}
	
	// Move precomputed light samples
	if (PrecomputedLightVolume)
	{
		PrecomputedLightVolume->ApplyWorldOffset(InWorldOffset, bWorldShift);
	}

	// Iterate over all actors in the level and move them
	for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ActorIndex++)
	{
		AActor* Actor = Actors[ActorIndex];
		if (Actor)
		{
			FVector Offset = (bWorldShift && Actor->bIgnoresOriginShifting) ? FVector::ZeroVector : InWorldOffset;
						
			if (!Actor->IsA(ANavigationData::StaticClass())) // Navigation data will be moved in NavigationSystem
			{
				Actor->ApplyWorldOffset(Offset, bWorldShift);
			}
		}
	}
	
	// Move model geometry
	for (int32 CompIdx = 0; CompIdx < ModelComponents.Num(); ++CompIdx)
	{
		ModelComponents[CompIdx]->ApplyWorldOffset(InWorldOffset, bWorldShift);
	}
}

void ULevel::RegisterActorForAutoReceiveInput(AActor* Actor, const int32 PlayerIndex)
{
	PendingAutoReceiveInputActors.Add(FPendingAutoReceiveInputActor(Actor, PlayerIndex));
};

void ULevel::PushPendingAutoReceiveInput(APlayerController* InPlayerController)
{
	check( InPlayerController );
	int32 PlayerIndex = -1;
	int32 Index = 0;
	for( FConstPlayerControllerIterator Iterator = InPlayerController->GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;
		if (InPlayerController == PlayerController)
		{
			PlayerIndex = Index;
			break;
		}
		Index++;
	}

	if (PlayerIndex >= 0)
	{
		TArray<AActor*> ActorsToAdd;
		for (int32 PendingIndex = PendingAutoReceiveInputActors.Num() - 1; PendingIndex >= 0; --PendingIndex)
		{
			FPendingAutoReceiveInputActor& PendingActor = PendingAutoReceiveInputActors[PendingIndex];
			if (PendingActor.PlayerIndex == PlayerIndex)
			{
				if (PendingActor.Actor.IsValid())
				{
					ActorsToAdd.Add(PendingActor.Actor.Get());
				}
				PendingAutoReceiveInputActors.RemoveAtSwap(PendingIndex);
			}
		}
		for (int32 ToAddIndex = ActorsToAdd.Num() - 1; ToAddIndex >= 0; --ToAddIndex)
		{
			APawn* PawnToPossess = Cast<APawn>(ActorsToAdd[ToAddIndex]);
			if (PawnToPossess)
			{
				InPlayerController->Possess(PawnToPossess);
			}
			else
			{
				ActorsToAdd[ToAddIndex]->EnableInput(InPlayerController);
			}
		}
	}
}

void ULevel::IncStreamingLevelRefs()
{
	if (NumStreamingLevelRefs == 0)
	{
		// Remove from pending unload list if any
		FLevelStreamingGCHelper::CancelUnloadRequest(this);
	}
	NumStreamingLevelRefs++;
}
	
void ULevel::DecStreamingLevelRefs()
{
	NumStreamingLevelRefs--;
	
	// Add level to unload list in case:
	// 1. there are no any streaming objects referencing this level
	// 2. level is not in world visible level list
	// 3. level is not in the process to became part of world visible levels list
	if (NumStreamingLevelRefs <= 0 && 
		bIsVisible == false &&
		HasVisibilityRequestPending() == false)
	{
		FLevelStreamingGCHelper::RequestUnload(this);
	}
}

int32 ULevel::GetStreamingLevelRefs() const
{
	return NumStreamingLevelRefs;
}


void ULevel::AddAssetUserData(UAssetUserData* InUserData)
{
	if(InUserData != NULL)
	{
		UAssetUserData* ExistingData = GetAssetUserDataOfClass(InUserData->GetClass());
		if(ExistingData != NULL)
		{
			AssetUserData.Remove(ExistingData);
		}
		AssetUserData.Add(InUserData);
	}
}

UAssetUserData* ULevel::GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass)
{
	for(int32 DataIdx=0; DataIdx<AssetUserData.Num(); DataIdx++)
	{
		UAssetUserData* Datum = AssetUserData[DataIdx];
		if(Datum != NULL && Datum->IsA(InUserDataClass))
		{
			return Datum;
		}
	}
	return NULL;
}

void ULevel::RemoveUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass)
{
	for(int32 DataIdx=0; DataIdx<AssetUserData.Num(); DataIdx++)
	{
		UAssetUserData* Datum = AssetUserData[DataIdx];
		if(Datum != NULL && Datum->IsA(InUserDataClass))
		{
			AssetUserData.RemoveAt(DataIdx);
			return;
		}
	}
}

/*-----------------------------------------------------------------------------
ULineBatchComponent implementation.
-----------------------------------------------------------------------------*/

/** Represents a LineBatchComponent to the scene manager. */
class FLineBatcherSceneProxy : public FPrimitiveSceneProxy
{
public:
	FLineBatcherSceneProxy(const ULineBatchComponent* InComponent) :
	  FPrimitiveSceneProxy(InComponent), Lines(InComponent->BatchedLines), 
		  Points(InComponent->BatchedPoints), Meshes(InComponent->BatchedMeshes)
	  {
		  bWillEverBeLit = false;
		  ViewRelevance.bDrawRelevance = true;
		  ViewRelevance.bDynamicRelevance = true;
		  ViewRelevance.bNormalTranslucencyRelevance = true;
	  }

	  /** 
	  * Draw the scene proxy as a dynamic element
	  *
	  * @param	PDI - draw interface to render to
	  * @param	View - current view
	  */
	  virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) OVERRIDE
	  {
		  QUICK_SCOPE_CYCLE_COUNTER( STAT_LineBatcherSceneProxy_DrawDynamicElements );

		  for (int32 i = 0; i < Lines.Num(); i++)
		  {
			  PDI->DrawLine(Lines[i].Start, Lines[i].End, Lines[i].Color, Lines[i].DepthPriority, Lines[i].Thickness);
		  }

		  for (int32 i = 0; i < Points.Num(); i++)
		  {
			  PDI->DrawPoint(Points[i].Position, Points[i].Color, Points[i].PointSize, Points[i].DepthPriority);
		  }

		  for (int32 i = 0; i < Meshes.Num(); i++)
		  {
			  static FVector const PosX(1.f,0,0);
			  static FVector const PosY(0,1.f,0);
			  static FVector const PosZ(0,0,1.f);

			  FBatchedMesh const& M = Meshes[i];

			  // this seems far from optimal in terms of perf, but it's for debugging
			  FDynamicMeshBuilder MeshBuilder;

			  // set up geometry
			  for (int32 VertIdx=0; VertIdx < M.MeshVerts.Num(); ++VertIdx)
			  {
				  MeshBuilder.AddVertex( M.MeshVerts[VertIdx], FVector2D::ZeroVector, PosX, PosY, PosZ, FColor::White );
			  }
			  //MeshBuilder.AddTriangles(M.MeshIndices);
			  for (int32 Idx=0; Idx < M.MeshIndices.Num(); Idx+=3)
			  {
				  MeshBuilder.AddTriangle( M.MeshIndices[Idx], M.MeshIndices[Idx+1], M.MeshIndices[Idx+2] );
			  }

			  FMaterialRenderProxy* const MaterialRenderProxy = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false), M.Color);
			  MeshBuilder.Draw(PDI, FMatrix::Identity, MaterialRenderProxy, M.DepthPriority);
		  }
	  }

	  /**
	  *  Returns a struct that describes to the renderer when to draw this proxy.
	  *	@param		Scene view to use to determine our relevence.
	  *  @return		View relevance struct
	  */
	  virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	  {
		  return ViewRelevance;
	  }
	  virtual uint32 GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	  uint32 GetAllocatedSize( void ) const 
	  { 
		  return( FPrimitiveSceneProxy::GetAllocatedSize() + Lines.GetAllocatedSize() + Points.GetAllocatedSize() + Meshes.GetAllocatedSize() ); 
	  }

private:
	TArray<FBatchedLine> Lines;
	TArray<FBatchedPoint> Points;
	TArray<FBatchedMesh> Meshes;
	FPrimitiveViewRelevance ViewRelevance;
};

ULineBatchComponent::ULineBatchComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;
	bTickInEditor = true;
	PrimaryComponentTick.bCanEverTick = true;
	BodyInstance.bEnableCollision_DEPRECATED = false;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bUseEditorCompositing = true;
	bGenerateOverlapEvents = false;
}

void ULineBatchComponent::DrawLine(const FVector& Start,const FVector& End,const FLinearColor& Color,uint8 DepthPriority,const float Thickness, const float LifeTime)
{
	new(BatchedLines) FBatchedLine(Start,End,Color,LifeTime,Thickness,DepthPriority);
	// LineBatcher and PersistentLineBatcher components will be updated at the end of UWorld::Tick
	MarkRenderStateDirty();
}

void ULineBatchComponent::DrawLines(const TArray<FBatchedLine>& InLines)
{
	BatchedLines.Append(InLines);
	// LineBatcher and PersistentLineBatcher components will be updated at the end of UWorld::Tick
	MarkRenderStateDirty();
}

void ULineBatchComponent::DrawPoint(
	const FVector& Position,
	const FLinearColor& Color,
	float PointSize,
	uint8 DepthPriority,
	float LifeTime
	)
{
	new(BatchedPoints) FBatchedPoint(Position, Color, PointSize, LifeTime, DepthPriority);
	// LineBatcher and PersistentLineBatcher components will be updated at the end of UWorld::Tick
	MarkRenderStateDirty();
}

void ULineBatchComponent::DrawBox(const FBox& Box, const FMatrix& TM, const FColor& Color, uint8 DepthPriorityGroup)
{
	FVector	B[2], P, Q;
	int32 ai, aj;
	B[0] = Box.Min;
	B[1] = Box.Max;

	for( ai=0; ai<2; ai++ ) for( aj=0; aj<2; aj++ )
	{
		P.X=B[ai].X; Q.X=B[ai].X;
		P.Y=B[aj].Y; Q.Y=B[aj].Y;
		P.Z=B[0].Z; Q.Z=B[1].Z;
		new(BatchedLines) FBatchedLine(TM.TransformPosition(P), TM.TransformPosition(Q), Color, DefaultLifeTime, 0.0f, DepthPriorityGroup);

		P.Y=B[ai].Y; Q.Y=B[ai].Y;
		P.Z=B[aj].Z; Q.Z=B[aj].Z;
		P.X=B[0].X; Q.X=B[1].X;
		new(BatchedLines) FBatchedLine(TM.TransformPosition(P), TM.TransformPosition(Q), Color, DefaultLifeTime, 0.0f, DepthPriorityGroup);

		P.Z=B[ai].Z; Q.Z=B[ai].Z;
		P.X=B[aj].X; Q.X=B[aj].X;
		P.Y=B[0].Y; Q.Y=B[1].Y;
		new(BatchedLines) FBatchedLine(TM.TransformPosition(P), TM.TransformPosition(Q), Color, DefaultLifeTime, 0.0f, DepthPriorityGroup);
	}
	// LineBatcher and PersistentLineBatcher components will be updated at the end of UWorld::Tick
	MarkRenderStateDirty();
}

void ULineBatchComponent::DrawSolidBox(const FBox& Box, const FTransform& Xform, const FColor& Color, uint8 DepthPriority, float LifeTime)
{
	int32 const NewMeshIdx = BatchedMeshes.Add(FBatchedMesh());
	FBatchedMesh& BM = BatchedMeshes[NewMeshIdx];

	BM.Color = Color;
	BM.DepthPriority = DepthPriority;
	BM.RemainingLifeTime = LifeTime;

	BM.MeshVerts.AddUninitialized(8);
	BM.MeshVerts[0] = Xform.TransformPosition( FVector(Box.Min.X, Box.Min.Y, Box.Max.Z) );
	BM.MeshVerts[1] = Xform.TransformPosition( FVector(Box.Max.X, Box.Min.Y, Box.Max.Z) );
	BM.MeshVerts[2] = Xform.TransformPosition( FVector(Box.Min.X, Box.Min.Y, Box.Min.Z) );
	BM.MeshVerts[3] = Xform.TransformPosition( FVector(Box.Max.X, Box.Min.Y, Box.Min.Z) );
	BM.MeshVerts[4] = Xform.TransformPosition( FVector(Box.Min.X, Box.Max.Y, Box.Max.Z) );
	BM.MeshVerts[5] = Xform.TransformPosition( FVector(Box.Max.X, Box.Max.Y, Box.Max.Z) );
	BM.MeshVerts[6] = Xform.TransformPosition( FVector(Box.Min.X, Box.Max.Y, Box.Min.Z) );
	BM.MeshVerts[7] = Xform.TransformPosition( FVector(Box.Max.X, Box.Max.Y, Box.Min.Z) );

	// clockwise
	BM.MeshIndices.AddUninitialized(36);
	int32 const Indices[36] = {	3,2,0,
		3,0,1,
		7,3,1,
		7,1,5,
		6,7,5,
		6,5,4,
		2,6,4,
		2,4,0,
		1,0,4,
		1,4,5,
		7,6,2,
		7,2,3	};

	for (int32 Idx=0; Idx<36; ++Idx)
	{
		BM.MeshIndices[Idx] = Indices[Idx];
	}

	MarkRenderStateDirty();
}

void ULineBatchComponent::DrawMesh(TArray<FVector> const& Verts, TArray<int32> const& Indices, FColor const& Color, uint8 DepthPriority, float LifeTime)
{
	// modifying array element directly to avoid copying arrays
	int32 const NewMeshIdx = BatchedMeshes.Add(FBatchedMesh());
	FBatchedMesh& BM = BatchedMeshes[NewMeshIdx];

	BM.MeshIndices = Indices;
	BM.MeshVerts = Verts;
	BM.Color = Color;
	BM.DepthPriority = DepthPriority;
	BM.RemainingLifeTime = LifeTime;

	MarkRenderStateDirty();
}

void ULineBatchComponent::DrawDirectionalArrow(const FMatrix& ArrowToWorld,FColor InColor,float Length,float ArrowSize,uint8 DepthPriority)
{
	const FVector Tip = ArrowToWorld.TransformPosition(FVector(Length,0,0));
	new(BatchedLines) FBatchedLine(Tip,ArrowToWorld.TransformPosition(FVector::ZeroVector),InColor,DefaultLifeTime,0.0f,DepthPriority);
	new(BatchedLines) FBatchedLine(Tip,ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,+ArrowSize,+ArrowSize)),InColor,DefaultLifeTime,0.0f,DepthPriority);
	new(BatchedLines) FBatchedLine(Tip,ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,+ArrowSize,-ArrowSize)),InColor,DefaultLifeTime,0.0f,DepthPriority);
	new(BatchedLines) FBatchedLine(Tip,ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,-ArrowSize,+ArrowSize)),InColor,DefaultLifeTime,0.0f,DepthPriority);
	new(BatchedLines) FBatchedLine(Tip,ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,-ArrowSize,-ArrowSize)),InColor,DefaultLifeTime,0.0f,DepthPriority);
	MarkRenderStateDirty();
}

/** Draw a circle */
void ULineBatchComponent::DrawCircle(const FVector& Base,const FVector& X,const FVector& Y,FColor Color,float Radius,int32 NumSides,uint8 DepthPriority)
{
	const float	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;

	for(int32 SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		new(BatchedLines) FBatchedLine(LastVertex,Vertex,Color,DefaultLifeTime,0.0f,DepthPriority);
		LastVertex = Vertex;
	}

	MarkRenderStateDirty();
}

void ULineBatchComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	bool bDirty = false;
	// Update the life time of batched lines, removing the lines which have expired.
	for(int32 LineIndex=0; LineIndex < BatchedLines.Num(); LineIndex++)
	{
		FBatchedLine& Line = BatchedLines[LineIndex];
		if (Line.RemainingLifeTime > 0.0f)
		{
			Line.RemainingLifeTime -= DeltaTime;
			if(Line.RemainingLifeTime <= 0.0f)
			{
				// The line has expired, remove it.
				BatchedLines.RemoveAt(LineIndex--);
				bDirty = true;
			}
		}
	}

	// Update the life time of batched points, removing the points which have expired.
	for(int32 PtIndex=0; PtIndex < BatchedPoints.Num(); PtIndex++)
	{
		FBatchedPoint& Pt = BatchedPoints[PtIndex];
		if (Pt.RemainingLifeTime > 0.0f)
		{
			Pt.RemainingLifeTime -= DeltaTime;
			if(Pt.RemainingLifeTime <= 0.0f)
			{
				// The point has expired, remove it.
				BatchedPoints.RemoveAt(PtIndex--);
				bDirty = true;
			}
		}
	}

	// Update the life time of batched meshes, removing the meshes which have expired.
	for(int32 MeshIndex=0; MeshIndex < BatchedMeshes.Num(); MeshIndex++)
	{
		FBatchedMesh& Mesh = BatchedMeshes[MeshIndex];
		if (Mesh.RemainingLifeTime > 0.0f)
		{
			Mesh.RemainingLifeTime -= DeltaTime;
			if(Mesh.RemainingLifeTime <= 0.0f)
			{
				// The mesh has expired, remove it.
				BatchedMeshes.RemoveAt(MeshIndex--);
				bDirty = true;
			}
		}
	}

	if(bDirty)
	{
		MarkRenderStateDirty();
	}
}

/**
* Creates a new scene proxy for the line batcher component.
* @return	Pointer to the FLineBatcherSceneProxy
*/
FPrimitiveSceneProxy* ULineBatchComponent::CreateSceneProxy()
{
	return new FLineBatcherSceneProxy(this);
}

FBoxSphereBounds ULineBatchComponent::CalcBounds( const FTransform & LocalToWorld ) const 
{
	const FVector BoxExtent(HALF_WORLD_MAX);
	return FBoxSphereBounds(FVector::ZeroVector, BoxExtent, BoxExtent.Size());
}

void ULineBatchComponent::Flush()
{
	if (BatchedLines.Num() > 0 || BatchedPoints.Num() > 0 || BatchedMeshes.Num() > 0)
	{
	BatchedLines.Empty();
	BatchedPoints.Empty();
	BatchedMeshes.Empty();
	MarkRenderStateDirty();
}
}

