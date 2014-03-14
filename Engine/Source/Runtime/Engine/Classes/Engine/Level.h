// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// The level object.  Contains the level's actor list, Bsp information, and brush list.
//

#pragma once
#include "Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h"

#include "Level.generated.h"

/**
 * Structure containing all information needed for determining the screen space
 * size of an object/ texture instance.
 */
USTRUCT()
struct ENGINE_API FStreamableTextureInstance
{
	GENERATED_USTRUCT_BODY()

	/** Bounding sphere/ box of object */
	FSphere BoundingSphere;
	/** Object (and bounding sphere) specific texel scale factor  */
	float	TexelFactor;

	/**
	 * FStreamableTextureInstance serialize operator.
	 *
	 * @param	Ar					Archive to to serialize object to/ from
	 * @param	TextureInstance		Object to serialize
	 * @return	Returns the archive passed in
	 */
	friend FArchive& operator<<( FArchive& Ar, FStreamableTextureInstance& TextureInstance );
};

/**
 * Serialized ULevel information about dynamic texture instances
 */
USTRUCT()
struct ENGINE_API FDynamicTextureInstance : public FStreamableTextureInstance
{
	GENERATED_USTRUCT_BODY()

	/** Texture that is used by a dynamic UPrimitiveComponent. */
	UPROPERTY()
	UTexture2D*					Texture;

	/** Whether the primitive that uses this texture is attached to the scene or not. */
	UPROPERTY()
	bool						bAttached;
	
	/** Original bounding sphere radius, at the time the TexelFactor was calculated originally. */
	UPROPERTY()
	float						OriginalRadius;

	/**
	 * FDynamicTextureInstance serialize operator.
	 *
	 * @param	Ar					Archive to to serialize object to/ from
	 * @param	TextureInstance		Object to serialize
	 * @return	Returns the archive passed in
	 */
	friend FArchive& operator<<( FArchive& Ar, FDynamicTextureInstance& TextureInstance );
};


/** 
 *	Used for storing cached data for simplified static mesh collision at a particular scaling.
 *	One entry, relating to a particular static mesh cached at a particular scaling, giving an index into the CachedPhysSMDataStore. 
 */
USTRUCT()
struct ENGINE_API FCachedPhysSMData
{
	GENERATED_USTRUCT_BODY()
	/** Scale of mesh that the data was cached for. */
	FVector				Scale3D;

	/** Index into CachedPhysSMDataStore that this cached data is stored at. */
	int32					CachedDataIndex;

	/** Serializer function. */
	friend FArchive& operator<<( FArchive& Ar, FCachedPhysSMData& D )
	{
		Ar << D.Scale3D << D.CachedDataIndex;
		return Ar;
	}
};

/** 
 *	Used for storing cached data for per-tri static mesh collision at a particular scaling. 
 */
USTRUCT()
struct ENGINE_API FCachedPerTriPhysSMData
{
	GENERATED_USTRUCT_BODY()
	/** Scale of mesh that the data was cached for. */
	FVector			Scale3D;

	/** Index into array Cached data for this mesh at this scale. */
	int32				CachedDataIndex;

	/** Serializer function. */
	friend FArchive& operator<<( FArchive& Ar, FCachedPerTriPhysSMData& D )
	{
		Ar << D.Scale3D << D.CachedDataIndex;
		return Ar;
	}
};

struct FPendingAutoReceiveInputActor
{
	TWeakObjectPtr<AActor> Actor;
	int32 PlayerIndex;

	FPendingAutoReceiveInputActor(AActor* InActor, const int32 InPlayerIndex)
		: Actor(InActor)
		, PlayerIndex(InPlayerIndex)
	{
	}
};

/** A precomputed visibility cell, whose data is stored in FCompressedVisibilityChunk. */
class FPrecomputedVisibilityCell
{
public:

	/** World space min of the cell. */
	FVector Min;

	/** Index into FPrecomputedVisibilityBucket::CellDataChunks of this cell's data. */
	uint16 ChunkIndex;

	/** Index into the decompressed chunk data of this cell's visibility data. */
	uint16 DataOffset;

	friend FArchive& operator<<( FArchive& Ar, FPrecomputedVisibilityCell& D )
	{
		Ar << D.Min << D.ChunkIndex << D.DataOffset;
		return Ar;
	}
};

/** A chunk of compressed visibility data from multiple FPrecomputedVisibilityCell's. */
class FCompressedVisibilityChunk
{
public:
	/** Whether the chunk is compressed. */
	bool bCompressed;

	/** Size of the uncompressed chunk. */
	int32 UncompressedSize;

	/** Compressed visibility data if bCompressed is true. */
	TArray<uint8> Data;

	friend FArchive& operator<<( FArchive& Ar, FCompressedVisibilityChunk& D )
	{
		Ar << D.bCompressed << D.UncompressedSize << D.Data;
		return Ar;
	}
};

/** A bucket of visibility cells that have the same spatial hash. */
class FPrecomputedVisibilityBucket
{
public:
	/** Size in bytes of the data of each cell. */
	int32 CellDataSize;

	/** Cells in this bucket. */
	TArray<FPrecomputedVisibilityCell> Cells;

	/** Data chunks corresponding to Cells. */
	TArray<FCompressedVisibilityChunk> CellDataChunks;

	friend FArchive& operator<<( FArchive& Ar, FPrecomputedVisibilityBucket& D )
	{
		Ar << D.CellDataSize << D.Cells << D.CellDataChunks;
		return Ar;
	}
};

/** Handles operations on precomputed visibility data for a level. */
class FPrecomputedVisibilityHandler
{
public:

	FPrecomputedVisibilityHandler() :
		Id(NextId)
	{
		NextId++;
	}
	
	~FPrecomputedVisibilityHandler() 
	{ 
		UpdateVisibilityStats(false);
	}

	/** Updates visibility stats. */
	ENGINE_API void UpdateVisibilityStats(bool bAllocating) const;

	/** Sets this visibility handler to be actively used by the rendering scene. */
	ENGINE_API void UpdateScene(FSceneInterface* Scene) const;

	/** Invalidates the level's precomputed visibility and frees any memory used by the handler. */
	ENGINE_API void Invalidate(FSceneInterface* Scene);

	/** @return the Id */
	int32 GetId() const { return Id; }

	friend FArchive& operator<<( FArchive& Ar, FPrecomputedVisibilityHandler& D );
	
private:

	/** World space origin of the cell grid. */
	FVector2D PrecomputedVisibilityCellBucketOriginXY;

	/** World space size of every cell in x and y. */
	float PrecomputedVisibilityCellSizeXY;

	/** World space height of every cell. */
	float PrecomputedVisibilityCellSizeZ;

	/** Number of cells in each bucket in x and y. */
	int32	PrecomputedVisibilityCellBucketSizeXY;

	/** Number of buckets in x and y. */
	int32	PrecomputedVisibilityNumCellBuckets;

	static int32 NextId;

	/** Id used by the renderer to know when cached visibility data is valid. */
	int32 Id;

	/** Visibility bucket data. */
	TArray<FPrecomputedVisibilityBucket> PrecomputedVisibilityCellBuckets;

	friend class FLightmassProcessor;
	friend class FSceneViewState;
};

/** Volume distance field generated by Lightmass, used by image based reflections for shadowing. */
class FPrecomputedVolumeDistanceField
{
public:

	/** Sets this volume distance field to be actively used by the rendering scene. */
	ENGINE_API void UpdateScene(FSceneInterface* Scene) const;

	/** Invalidates the level's volume distance field and frees any memory used by it. */
	ENGINE_API void Invalidate(FSceneInterface* Scene);

	friend FArchive& operator<<( FArchive& Ar, FPrecomputedVolumeDistanceField& D );

private:
	/** Largest world space distance stored in the volume. */
	float VolumeMaxDistance;
	/** World space bounding box of the volume. */
	FBox VolumeBox;
	/** Volume dimension X. */
	int32 VolumeSizeX;
	/** Volume dimension Y. */
	int32 VolumeSizeY;
	/** Volume dimension Z. */
	int32 VolumeSizeZ;
	/** Distance field data. */
	TArray<FColor> Data;

	friend class FScene;
	friend class FLightmassProcessor;
};

UCLASS(customConstructor,MinimalAPI)
class ULevel : public ULevelBase, public IInterface_AssetUserData
{
	GENERATED_UCLASS_BODY()

	/** Set before calling LoadPackage for a streaming level to ensure that OwningWorld is correct on the Level */
	ENGINE_API static TMap<FName, UWorld*> StreamedLevelsOwningWorld;
		
	/** 
	 * The World that has this level in its Levels array. 
	 * This is not the same as GetOuter(), because GetOuter() for a streaming level is a vestigial world that is not used. 
	 * It should not be accessed during BeginDestroy(), just like any other UObject references, since GC may occur in any order.
	 */
	UPROPERTY(transient)
	UWorld* OwningWorld;

	/** BSP UModel. */
	UPROPERTY()
	class UModel* Model;

	/** BSP Model components used for rendering. */
	UPROPERTY()
	TArray<class UModelComponent*> ModelComponents;

#if WITH_EDITORONLY_DATA
	/** Reference to the blueprint for level scripting */
	UPROPERTY()
	class ULevelScriptBlueprint* LevelScriptBlueprint;
#endif //WITH_EDITORONLY_DATA

	/** The level scripting actor, created by instantiating the class from LevelScriptBlueprint.  This handles all level scripting */
	UPROPERTY(NonTransactional)
	class ALevelScriptActor* LevelScriptActor;

	/**
	 * Start and end of the navigation list for this level, used for quickly fixing up
	 * when streaming this level in/out. @TODO DEPRECATED - DELETE
	 */
	UPROPERTY()
	class ANavigationObjectBase *NavListStart;
	UPROPERTY()
	class ANavigationObjectBase	*NavListEnd;

	/** Total number of KB used for lightmap textures in the level. */
	UPROPERTY(VisibleAnywhere, Category=Level)
	float LightmapTotalSize;
	/** Total number of KB used for shadowmap textures in the level. */
	UPROPERTY(VisibleAnywhere, Category=Level)
	float ShadowmapTotalSize;

	/** threes of triangle vertices - AABB filtering friendly. Stored if there's a runtime need to rebuild navigation that accepts BSPs 
	 *	as well - it's a lot easier this way than retrieve this data at runtime */
	UPROPERTY()
	TArray<FVector> StaticNavigableGeometry;

	bool bTextureStreamingBuilt;

	/** Static information used by texture streaming code, generated during PreSave									*/
	TMap<UTexture2D*,TArray<FStreamableTextureInstance> >	TextureToInstancesMap;

	/** Information about textures on dynamic primitives. Used by texture streaming code, generated during PreSave.		*/
	TMap<TWeakObjectPtr<UPrimitiveComponent>,TArray<FDynamicTextureInstance> >	DynamicTextureInstances;

	/** Set of textures used by PrimitiveComponents that have bForceMipStreaming checked. */
	TMap<UTexture2D*,bool>									ForceStreamTextures;

	/** Index into Actors array pointing to first net relevant actor. Used as an optimization for FActorIterator	*/
	int32											iFirstNetRelevantActor;

	/** Data structures for holding the tick functions **/
	class FTickTaskLevel*		TickTaskLevel;

	/** 
	* The precomputed light information for this level.  
	* The extra level of indirection is to allow forward declaring FPrecomputedLightVolume.
	*/
	class FPrecomputedLightVolume*				PrecomputedLightVolume;

	/** Contains precomputed visibility data for this level. */
	FPrecomputedVisibilityHandler				PrecomputedVisibilityHandler;

	/** Precomputed volume distance field for this level. */
	FPrecomputedVolumeDistanceField				PrecomputedVolumeDistanceField;

	/** Fence used to track when the rendering thread has finished referencing this ULevel's resources. */
	FRenderCommandFence							RemoveFromSceneFence;

	/** Whether components are currently registered or not. */
	uint32										bAreComponentsCurrentlyRegistered:1;

	/** Whether the geometry needs to be rebuilt for correct lighting */
	uint32										bGeometryDirtyForLighting:1;

	/** Whether the level is currently visible/ associated with the world */
	UPROPERTY(transient)
	uint32										bIsVisible:1;
	
	/** Whether this level is locked; that is, its actors are read-only 
	 *	Used by WorldBrowser to lock a level when corresponding ULevelStreaming does not exist
	 */
	UPROPERTY()
	uint32 										bLocked:1;
	
	/** The below variables are used temporarily while making a level visible.				*/

	/** Whether we already moved actors.													*/
	uint32										bAlreadyMovedActors:1;
	/** Whether we already shift actors positions according to world composition.			*/
	uint32										bAlreadyShiftedActors:1;
	/** Whether we already updated components.												*/
	uint32										bAlreadyUpdatedComponents:1;
	/** Whether we already associated streamable resources.									*/
	uint32										bAlreadyAssociatedStreamableResources:1;
	/** Whether we already initialized actors.												*/
	uint32										bAlreadyInitializedActors:1;
	/** Whether we already routed initialize on actors.										*/
	uint32										bAlreadyRoutedActorInitialize:1;
	/** Whether we already sorted the actor list.											*/
	uint32										bAlreadySortedActorList:1;
	/** Whether one or more streaming levels requested to show this level					*/
	uint32										bHasShowRequest:1;
	/** Whether one or more streaming levels requested to hide this level					*/
	uint32										bHasHideRequest:1;
	/** Whether this level is in the process of being associated with its world				*/
	uint32										bIsAssociatingLevel:1;
	/** Whether this level should be fully added to the world before rendering his components	*/
	uint32										bRequireFullVisibilityToRender:1;
		
	/** Current index into actors array for updating components.							*/
	int32										CurrentActorIndexForUpdateComponents;

	/** Whether the level is currently pending being made visible.							*/
	bool HasVisibilityRequestPending() const
	{
		return (OwningWorld && this == OwningWorld->CurrentLevelPendingVisibility);
	}

#if PERF_TRACK_DETAILED_ASYNC_STATS
	/** Mapping of how long each actor class takes to have UpdateComponents called on it */
	TMap<const UClass*,struct FMapTimeEntry>		UpdateComponentsTimePerActorClass;
#endif // PERF_TRACK_DETAILED_ASYNC_STATS

	/** Actor which defines level logical bounding box				*/
	TWeakObjectPtr<ALevelBounds>				LevelBoundsActor;

	/** Called when Level bounds actor has been updated */
	DECLARE_EVENT( ULevel, FLevelBoundsActorUpdatedEvent );
	FLevelBoundsActorUpdatedEvent& LevelBoundsActorUpdated() { return LevelBoundsActorUpdatedEvent; }
	/**	Broadcasts that Level bounds actor has been updated */ 
	void BroadcastLevelBoundsActorUpdated() { LevelBoundsActorUpdatedEvent.Broadcast(); }
private:
	FLevelBoundsActorUpdatedEvent LevelBoundsActorUpdatedEvent; 

protected:

	/** Array of all MovieSceneBindings that are used in this level.  These store the relationship between
	    a MovieScene asset and possessed actors in this level. */
	UPROPERTY()
	TArray< class UObject* > MovieSceneBindingsArray;

	/** List of RuntimeMovieScenePlayers that are currently active in this level.  We'll keep references to these to keep
	    them around until they're no longer needed.  Also, we'll tick them every frame! */
	// @todo sequencer uobjects: Ideally this is using URuntimeMovieScenePlayer* and not UObject*, but there are DLL/interface issues with that
	UPROPERTY( transient )
	TArray< UObject* > ActiveRuntimeMovieScenePlayers;

	/** Array of user data stored with the asset */
	UPROPERTY()
	TArray<UAssetUserData*> AssetUserData;


private:

	TArray<FPendingAutoReceiveInputActor> PendingAutoReceiveInputActors;

	/** Number of streaming levels referring this level	*/
	int32 NumStreamingLevelRefs;

public:
	/** Called when a level package has been dirtied. */
	ENGINE_API static FSimpleMulticastDelegate LevelDirtiedEvent;

	// Constructor.
	ENGINE_API ULevel(const class FPostConstructInitializeProperties& PCIP, const FURL& InURL );
	ULevel(const class FPostConstructInitializeProperties& PCIP );
	~ULevel();

	// Begin UObject interface.
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	virtual void FinishDestroy() OVERRIDE;

#if	WITH_EDITOR
	virtual void PreEditUndo() OVERRIDE;
	virtual void PostEditUndo() OVERRIDE;	
#endif // WITH_EDITOR
	virtual void PostLoad() OVERRIDE;
	virtual void PreSave() OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface.

	/**
	 * Adds a newly-created RuntimeMovieScenePlayer to this level.  The level assumes ownership of this object.
	 *
	 * @param	RuntimeMovieScenePlayer	The RuntimeMovieScenePlayer instance to add
	 */
	ENGINE_API void AddActiveRuntimeMovieScenePlayer( class UObject* RuntimeMovieScenePlayer );

	/**
	 * Called by the engine every frame to tick all actively-playing RuntimeMovieScenePlayers
	 *
	 * @param	DeltaSeconds	Time elapsed since last tick
	 */
	void TickRuntimeMovieScenePlayers( const float DeltaSeconds );

	/**
	 * Clears all components of actors associated with this level (aka in Actors array) and 
	 * also the BSP model components.
	 */
	ENGINE_API void ClearLevelComponents();

	/**
	 * Updates all components of actors associated with this level (aka in Actors array) and 
	 * creates the BSP model components.
	 * @param bRerunConstructionScripts	If we want to rerun construction scripts on actors in level
	 */
	ENGINE_API void UpdateLevelComponents(bool bRerunConstructionScripts);

	/**
	 * Incrementally updates all components of actors associated with this level.
	 *
	 * @param NumActorsToUpdate		Number of actors to update in this run, 0 for all
	 * @param bRerunConstructionScripts	If we want to rerun construction scripts on actors in level
	 */
	void IncrementalUpdateComponents( int32 NumActorsToUpdate, bool bRerunConstructionScripts );

	/**
	 * Invalidates the cached data used to render the level's UModel.
	 */
	void InvalidateModelGeometry();

#if WITH_EDITOR
	/** Called to creete ModelComponents for BSP rendering */
	void CreateModelComponents();
#endif // WITH_EDITOR

	/**
	 * Updates the model components associated with this level
	 */
	ENGINE_API void UpdateModelComponents();

	/**
	 * Commits changes made to the UModel's surfaces.
	 */
	void CommitModelSurfaces();

	/**
	 * Discards the cached data used to render the level's UModel.  Assumes that the
	 * faces and vertex positions haven't changed, only the applied materials.
	 */
	void InvalidateModelSurface();

	/**
	 * Makes sure that all light components have valid GUIDs associated.
	 */
	void ValidateLightGUIDs();

	/**
	 * Sorts the actor list by net relevancy and static behaviour. First all not net relevant static
	 * actors, then all net relevant static actors and then the rest. This is done to allow the dynamic
	 * and net relevant actor iterators to skip large amounts of actors.
	 */
	ENGINE_API void SortActorList();

	/**
	 * Initializes all actors after loading completed.
	 */
	void InitializeActors();

	/** Initializes rendering resources for this level. */
	void InitializeRenderingResources();

	/** Releases rendering resources for this level. */
	ENGINE_API void ReleaseRenderingResources();

	/**
	 * Routes pre and post initialize to actors and also sets volumes.
	 *
	 * @todo seamless worlds: this doesn't correctly handle volumes in the multi- level case
	 */
	void RouteActorInitialize();



	/**
	 * Rebuilds static streaming data for all levels in the specified UWorld.
	 *
	 * @param World				Which world to rebuild streaming data for. If NULL, all worlds will be processed.
	 * @param TargetLevel		[opt] Specifies a single level to process. If NULL, all levels will be processed.
	 * @param TargetTexture		[opt] Specifies a single texture to process. If NULL, all textures will be processed.
	 */
	ENGINE_API static void BuildStreamingData(UWorld* World, ULevel* TargetLevel=NULL, UTexture2D* TargetTexture=NULL);

	/**
	 * Rebuilds static streaming data for this level.
	 *
	 * @param TargetTexture			[opt] Specifies a single texture to process. If NULL, all textures will be processed.
	 */
	void BuildStreamingData(UTexture2D* TargetTexture=NULL);

	/**
	 * Clamp lightmap and shadowmap texelfactors to 20-80% range.
	 * This is to prevent very low-res or high-res charts to dominate otherwise decent streaming.
	 */
	void NormalizeLightmapTexelFactor();

	/** Retrieves the array of streamable texture isntances. */
	ENGINE_API TArray<FStreamableTextureInstance>* GetStreamableTextureInstances(UTexture2D*& TargetTexture);

	/**
	 * Returns the default brush for this level.
	 *
	 * @return		The default brush for this level.
	 */
	ENGINE_API class ABrush* GetBrush() const;

	/**
	 * Returns the world info for this level.
	 *
	 * @return		The AWorldSettings for this level.
	 */
	ENGINE_API 
	class AWorldSettings* GetWorldSettings() const;

	/**
	 * Returns the level scripting actor associated with this level
	 * @return	a pointer to the level scripting actor for this level (may be NULL)
	 */
	ENGINE_API class ALevelScriptActor* GetLevelScriptActor() const;

	/**
	 * Utility searches this level's actor list for any actors of the specified type.
	 */
	bool HasAnyActorsOfType(UClass *SearchType);

	/**
	 * Resets the level nav list.
	 */
	ENGINE_API void ResetNavList();

#if WITH_EDITOR
	/**
	 *	Grabs a reference to the level scripting blueprint for this level.  If none exists, it creates a new blueprint
	 *
	 * @param	bDontCreate		If true, if no level scripting blueprint is found, none will be created
	 */
	ENGINE_API class ULevelScriptBlueprint* GetLevelScriptBlueprint(bool bDontCreate=false);

	/**
	 *  Called when the level script blueprint has been successfully changed and compiled.  Handles creating an instance of the blueprint class in LevelScriptActor
	 */
	ENGINE_API void OnLevelScriptBlueprintChanged(class ULevelScriptBlueprint* InBlueprint);
#endif

	/** @todo document */
	ENGINE_API TArray<FVector> const* GetStaticNavigableGeometry() const { return &StaticNavigableGeometry;}

	/* 
	* Is this the persistent level 
	*/
	ENGINE_API bool IsPersistentLevel() const;

	/* 
	* Is this the current level in the world it is owned by
	*/
	ENGINE_API bool IsCurrentLevel() const;
	
	/* 
	 * Shift level actors by specified offset
	 * The offset vector will get subtracted from all actors positions and corresponding data structures
	 *
	 * @param InWorldOffset	 Vector to shift all actors by
	 * @param bWorldShift	 Whether this call is part of whole world shifting
	 */
	ENGINE_API void ApplyWorldOffset(const FVector& InWorldOffset, bool bWorldShift);

	/** Register an actor that should be added to a player's input stack when they are created */
	void RegisterActorForAutoReceiveInput(AActor* Actor, const int32 PlayerIndex);

	/** Push any pending auto receive input actor's input components on to the player controller's input stack */
	void PushPendingAutoReceiveInput(APlayerController* PC);

	/** Increments number of steaming objects referring this level */
	ENGINE_API void IncStreamingLevelRefs();
	
	/** Decrements number of steaming objects referring this level */
	ENGINE_API void DecStreamingLevelRefs();

	/** Returns number of steaming objects referring this level */
	ENGINE_API int32 GetStreamingLevelRefs() const;
	
	// Begin IInterface_AssetUserData Interface
	virtual void AddAssetUserData(UAssetUserData* InUserData) OVERRIDE;
	virtual void RemoveUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass) OVERRIDE;
	virtual UAssetUserData* GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass) OVERRIDE;
	// End IInterface_AssetUserData Interface

#if WITH_EDITOR
	/** meant to be called only from editor, calculating and storing static geometry to be used with off-line and/or on-line navigation building */
	ENGINE_API void RebuildStaticNavigableGeometry();


	/**
	 * Adds a new MovieSceneBindings object to the level script actor.  This actor takes full ownership of
	 * the MovieSceneBindings
	 *
	 * @param	MovieSceneBindings	The MovieSceneBindings object to take ownership of
	 */
	virtual void AddMovieSceneBindings( class UMovieSceneBindings* MovieSceneBindings );

	/**
	 * Clears all existing MovieSceneBindings off of the level.  Only meant to be called by the level script compiler.
	 */
	virtual void ClearMovieSceneBindings();


#endif

};


/**
 * Macro for wrapping Delegates in TScopedCallback
 */
 #define DECLARE_SCOPED_DELEGATE( CallbackName, TriggerFunc )						\
	class ENGINE_API FScoped##CallbackName##Impl										\
	{																				\
	public:																			\
		static void FireCallback() { TriggerFunc; }									\
	};																				\
																					\
	typedef TScopedCallback<FScoped##CallbackName##Impl> FScoped##CallbackName;

DECLARE_SCOPED_DELEGATE( LevelDirtied, ULevel::LevelDirtiedEvent.Broadcast() );

#undef DECLARE_SCOPED_DELEGATE
