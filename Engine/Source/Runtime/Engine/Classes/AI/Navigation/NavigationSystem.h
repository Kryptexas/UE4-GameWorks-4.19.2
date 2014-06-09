// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AI/Navigation/NavFilters/NavigationQueryFilter.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenericOctreePublic.h"
#include "AI/Navigation/NavigationData.h"
#include "NavigationSystem.generated.h"

#define NAVSYS_DEBUG (0 && UE_BUILD_DEBUG)

// if we'll be rebuilding navigation at runtime
#define WITH_RUNTIME_NAVIGATION_BUILDING (1 && WITH_NAVIGATION_GENERATOR)

#define NAVOCTREE_CONTAINS_COLLISION_DATA (1 && WITH_RECAST)

#define NAV_USE_MAIN_NAVIGATION_DATA NULL

DECLARE_LOG_CATEGORY_EXTERN(LogNavigation, Warning, All);

/** delegate to let interested parties know that new nav area class has been registered */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNavAreaChanged, const class UClass* /*AreaClass*/);

/** Delegate to let interested parties know that Nav Data has been registered */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavDataRegistered, class ANavigationData*, NavData);

namespace NavigationDebugDrawing
{
	extern const ENGINE_API float PathLineThickness;
	extern const ENGINE_API FVector PathOffset;
	extern const ENGINE_API FVector PathNodeBoxExtent;
}

namespace FNavigationSystem
{
	enum EMode
	{
		InvalidMode = -1,
		GameMode,
		EditorMode,
		SimulationMode,
		PIEMode,
	};
}

namespace FNavigationSystem
{
	/** 
	 * Used to construct an ANavigationData instance for specified navigation data agent 
	 */
	typedef class ANavigationData* (*FNavigationDataInstanceCreator)(class UWorld*, const FNavDataConfig&);
}

struct FNavigationSystemExec: public FSelfRegisteringExec
{
	FNavigationSystemExec()
	{
	}

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;
	// End FExec Interface
};

class ENGINE_API FNavigationLockContext
{
public:
	FNavigationLockContext() : MyWorld(NULL), bSingleWorld(false) { LockUpdates(); }
	FNavigationLockContext(UWorld* InWorld) : MyWorld(InWorld), bSingleWorld(true) { LockUpdates(); }
	~FNavigationLockContext() { UnlockUpdates(); }

private:
	UWorld* MyWorld;
	bool bSingleWorld;

	void LockUpdates();
	void UnlockUpdates();
};

UCLASS(Within=World, config=Engine, defaultconfig)
class ENGINE_API UNavigationSystem : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	virtual ~UNavigationSystem();

	UPROPERTY()
	ANavigationData* MainNavData;

	/** Should navigation system spawn default Navigation Data when there's none and there are navigation bounds present? */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bAutoCreateNavigationData:1;

	/** Should navigation system rebuild navigation data at game/pie runtime? 
	 *	It also includes spawning navigation data instances if there are none present. */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bBuildNavigationAtRuntime:1;

	/** if set to true will result navigation system not rebuild navigation until 
	 *	a call to ReleaseInitialBuildingLock() is called. Does not influence 
	 *	editor-time generation (i.e. does influence PIE and Game).
	 *	Defaults to false.*/
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bInitialBuildingLocked:1;

	/** If set to true (default) navigation will be generated only within special navigation 
	 *	bounds volumes (like ANavMeshBoundsVolume). Set to false means navigation should be generated
	 *	everywhere.
	 */
	// @todo removing it from edition since it's currently broken and I'm not sure we want that at all
	// since I'm not sure we can make it efficient in a generic case
	//UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bWholeWorldNavigable:1;

	/** If set to true (default) generation seeds will include locations of all player controlled pawns */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bAddPlayersToGenerationSeeds:1;

	/** false by default, if set to true will result in not caring about nav agent height 
	 *	when trying to match navigation data to passed in nav agent */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bSkipAgentHeightCheckWhenPickingNavData:1;

	UPROPERTY(config, EditAnywhere, Category=Agents)
	TArray<FNavDataConfig> SupportedAgents;
	
	/** update frequency for dirty areas on navmesh */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	float DirtyAreasUpdateFreq;

	UPROPERTY()
	TArray<class ANavigationData*> NavDataSet;

	UPROPERTY(transient)
	TArray<class ANavigationData*> NavDataRegistrationQueue;

	TSet<FNavigationDirtyElement> PendingOctreeUpdates;

 	UPROPERTY(/*BlueprintAssignable, */Transient)
	FOnNavDataRegistered OnNavDataRegisteredEvent;

private:
	TWeakObjectPtr<class UCrowdManager> CrowdManager;

	// required navigation data 
	UPROPERTY(config)
	TArray<FStringClassReference> RequiredNavigationDataClassNames;

	/** set to true when navigation processing was blocked due to missing nav bounds */
	uint32 bNavDataRemovedDueToMissingNavBounds:1;

public:
	//----------------------------------------------------------------------//
	// Kismet functions
	//----------------------------------------------------------------------//
	/** Project a point onto the NavigationData */
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static FVector ProjectPointToNavigation(UObject* WorldContext, const FVector& Point, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);
	
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static FVector GetRandomPoint(UObject* WorldContext, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static FVector GetRandomPointInRadius(UObject* WorldContext, const FVector& Origin, float Radius, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	/** Potentially expensive. Use with caution. Consider using UPathFollowingComponent::GetRemainingPathCost instead */
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static ENavigationQueryResult::Type GetPathCost(UObject* WorldContext, const FVector& PathStart, const FVector& PathEnd, float& PathCost, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	/** Potentially expensive. Use with caution */
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static ENavigationQueryResult::Type GetPathLength(UObject* WorldContext, const FVector& PathStart, const FVector& PathEnd, float& PathLength, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static bool IsNavigationBeingBuilt(UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	static void SimpleMoveToActor(class AController* Controller, const AActor* Goal);

	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	static void SimpleMoveToLocation(class AController* Controller, const FVector& Goal);

	/** Performs navigation raycast on NavigationData appropriate for given Querier.
	 *	@param Querier if not passed default navigation data will be used
	 *	@param HitLocation if line was obstructed this will be set to hit location. Otherwise it contains SegmentEnd
	 *	@return true if line from RayStart to RayEnd was obstructed. Also, true when no navigation data present */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ))
	static bool NavigationRaycast(UObject* WorldContext, const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL, class AController* Querier = NULL);

	/** delegate type for events that dirty the navigation data ( Params: const FBox& DirtyBounds ) */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnNavigationDirty, const FBox&);
	/** called after navigation influencing event takes place*/
	static FOnNavigationDirty NavigationDirtyEvent;

	enum ERegistrationResult
	{
		RegistrationError,
		RegistrationFailed_DataPendingKill,			// means navigation data being registered is marked as pending kill
		RegistrationFailed_AgentAlreadySupported,	// this means that navigation agent supported by given nav data is already handled by some other, previously registered instance
		RegistrationFailed_AgentNotValid,			// given instance contains navmesh that doesn't support any of expected agent types, or instance doesn't specify any agent
		RegistrationSuccessful,
	};

	enum EOctreeUpdateMode
	{
		OctreeUpdate_Default = 0,						// regular update, mark dirty areas depending on exported content
		OctreeUpdate_Geometry = 1,						// full update, mark dirty areas for geometry rebuild
		OctreeUpdate_Modifiers = 2,						// quick update, mark dirty areas for modifier rebuild
		OctreeUpdate_Refresh = 4,						// update is used for refresh, don't invalidate pending queue
	};

	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject Interface

	virtual void Tick(float DeltaSeconds);

	UWorld* GetWorld() const { return GetOuterUWorld(); }

	class UCrowdManager* GetCrowdManager() const { return CrowdManager.Get(); }

	/** spawn new crowd manager */
	virtual void CreateCrowdManager();

	//----------------------------------------------------------------------//
	// Public querying interface                                                                
	//----------------------------------------------------------------------//
	/** 
	 *	Synchronously looks for a path from @fLocation to @EndLocation for agent with properties @AgentProperties. NavData actor appropriate for specified 
	 *	FNavAgentProperties will be found automatically
	 *	@param ResultPath results are put here
	 *	@param NavData optional navigation data that will be used instead of the one that would be deducted from AgentProperties
	 *  @param Mode switch between normal and hierarchical path finding algorithms
	 */
	FPathFindingResult FindPathSync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular);

	/** 
	 *	Does a simple path finding from @StartLocation to @EndLocation on specified NavData. If none passed MainNavData will be used
	 *	Result gets placed in ResultPath
	 *	@param NavData optional navigation data that will be used instead main navigation data
	 *  @param Mode switch between normal and hierarchical path finding algorithms
	 */
	FPathFindingResult FindPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular);

	/** 
	 *	Asynchronously looks for a path from @StartLocation to @EndLocation for agent with properties @AgentProperties. NavData actor appropriate for specified 
	 *	FNavAgentProperties will be found automatically
	 *	@param ResultDelegate delegate that will be called once query has been processed and finished. Will be called even if query fails - in such case see comments for delegate's params
	 *	@param NavData optional navigation data that will be used instead of the one that would be deducted from AgentProperties
	 *	@param PathToFill if points to an actual navigation path instance than this instance will be filled with resulting path. Otherwise a new instance will be created and 
	 *		used in call to ResultDelegate
	 *  @param Mode switch between normal and hierarchical path finding algorithms
	 *	@return request ID
	 */
	uint32 FindPathAsync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, const FNavPathQueryDelegate& ResultDelegate, EPathFindingMode::Type Mode = EPathFindingMode::Regular);

	/** Removes query indicated by given ID from queue of path finding requests to process. */
	void AbortAsyncFindPathRequest(uint32 AsynPathQueryID);
	
	/** 
	 *	Synchronously check if path between two points exists
	 *  Does not return path object, but will run faster (especially in hierarchical mode)
	 *  @param Mode switch between normal and hierarchical path finding algorithms. @note Hierarchical mode ignores QueryFilter
	 *	@return true if path exists
	 */
	bool TestPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular) const;

	/** Finds random point in navigable space
	 *	@param ResultLocation Found point is put here
	 *	@param NavData If NavData == NULL then MainNavData is used.
	 *	@return true if any location found, false otherwise */
	bool GetRandomPoint(FNavLocation& ResultLocation, class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/** Finds random point in navigable space restricted to Radius around Origin
	 *	@param ResultLocation Found point is put here
	 *	@param NavData If NavData == NULL then MainNavData is used.
	 *	@return true if any location found, false otherwise */
	bool GetRandomPointInRadius(const FVector& Origin, float Radius, FNavLocation& ResultLocation, class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** Calculates a path from PathStart to PathEnd and retrieves its cost. 
	 *	@NOTE potentially expensive, so use it with caution */
	ENavigationQueryResult::Type GetPathCost(const FVector& PathStart, const FVector& PathEnd, float& PathCost, const class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** Calculates a path from PathStart to PathEnd and retrieves its overestimated length.
	 *	@NOTE potentially expensive, so use it with caution */
	ENavigationQueryResult::Type GetPathLength(const FVector& PathStart, const FVector& PathEnd, float& PathLength, const ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** Calculates a path from PathStart to PathEnd and retrieves its overestimated length and cost.
	 *	@NOTE potentially expensive, so use it with caution */
	ENavigationQueryResult::Type GetPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& PathLength, float& PathCost, const ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	// @todo document
	bool ProjectPointToNavigation(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent = INVALID_NAVEXTENT, const FNavAgentProperties* AgentProperties = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL)
	{
		return ProjectPointToNavigation(Point, OutLocation, Extent, AgentProperties != NULL ? GetNavDataForProps(*AgentProperties) : GetMainNavData(FNavigationSystem::DontCreate), QueryFilter);
	}

	// @todo document
	bool ProjectPointToNavigation(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent = INVALID_NAVEXTENT, const class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** 
	 * Looks for NavData generated for specified movement properties and returns it. NULL if not found;
	 */
	class ANavigationData* GetNavDataForProps(const FNavAgentProperties& AgentProperties);

	/** 
	 * Looks for NavData generated for specified movement properties and returns it. NULL if not found; Const version.
	 */
	const class ANavigationData* GetNavDataForProps(const FNavAgentProperties& AgentProperties) const;

	/** Returns the world nav mesh object.  Creates one if it doesn't exist. */
	class ANavigationData* GetMainNavData(FNavigationSystem::ECreateIfEmpty CreateNewIfNoneFound);
	/** Returns the world nav mesh object.  Creates one if it doesn't exist. */
	const class ANavigationData* GetMainNavData() const { return MainNavData; }

	TSharedPtr<FNavigationQueryFilter> CreateDefaultQueryFilterCopy() const;

	/** Super-hacky safety feature for threaded navmesh building. Will be gone once figure out why keeping TSharedPointer to Navigation Generator doesn't 
	 *	guarantee its existence */
	bool ShouldGeneratorRun(const class FNavDataGenerator* Generator) const;

	bool IsNavigationBuilt(const class AWorldSettings* Settings) const;

	bool IsThereAnywhereToBuildNavigation() const;

	bool ShouldGenerateNavigationEverywhere() const { return bWholeWorldNavigable; }

	FBox GetWorldBounds() const;
	
	FBox GetLevelBounds(ULevel* InLevel) const;

	/** @return default walkable area class */
	FORCEINLINE static TSubclassOf<class UNavArea> GetDefaultWalkableArea() { return DefaultWalkableArea; }

	/** @return default obstacle area class */
	FORCEINLINE static TSubclassOf<class UNavArea> GetDefaultObstacleArea() { return DefaultObstacleArea; }

	void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift);

	/** checks if navigation/navmesh is dirty and needs to be rebuilt */
	bool IsNavigationDirty();

	static bool DoesPathIntersectBox(const FNavigationPath* Path, const FBox& Box);

	//----------------------------------------------------------------------//
	// Bookkeeping 
	//----------------------------------------------------------------------//
	
	// @todo document
	void UnregisterNavData(class ANavigationData* NavData);

	/** adds NavData to registration candidates queue - NavDataRegistrationQueue */
	void RequestRegistration(class ANavigationData* NavData, bool bTriggerRegistrationProcessing = true);

	/** Processes registration of candidates queues via RequestRegistration and stored in NavDataRegistrationQueue */
	void ProcessRegistrationCandidates();

	/** registers NavArea classes awaiting registration in PendingNavAreaRegistration */
	void ProcessNavAreaPendingRegistration();
	
	/** @return pointer to ANavigationData instance of given ID, or NULL if it was not found. Note it looks only through registered navigation data */
	class ANavigationData* GetNavDataWithID(const uint16 NavDataID) const;

	void ReleaseInitialBuildingLock();

	//----------------------------------------------------------------------//
	// navigation octree related functions
	//----------------------------------------------------------------------//
	FSetElementId RegisterNavigationRelevantActor(class AActor* Actor, int32 UpdateFlags = OctreeUpdate_Default);
	void UnregisterNavigationRelevantActor(class AActor* Actor, int32 UpdateFlags = OctreeUpdate_Default);

#if WITH_NAVIGATION_GENERATOR
	void AddDirtyArea(const FBox& NewArea, int32 Flags);
	void AddDirtyAreas(const TArray<FBox>& NewAreas, int32 Flags);
#endif // WITH_NAVIGATION_GENERATOR

	const class FNavigationOctree* GetNavOctree() const { return NavOctree; }

	void UpdateNavOctree(class AActor* Actor, int32 UpdateFlags = OctreeUpdate_Default);
	void UpdateNavOctree(class UActorComponent* ActorComp, int32 UpdateFlags = OctreeUpdate_Default);
	void UpdateNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds);

	/** called to gather navigation relevant actors that have been created while
	 *	Navigation System was not present */
	void PopulateNavOctree();

	FORCEINLINE void SetObjectsNavOctreeId(UObject* Object, FOctreeElementId Id) { ObjectToOctreeId.Add(Object, Id); }
	FORCEINLINE const FOctreeElementId* GetObjectsNavOctreeId(const UObject* Object) const { return ObjectToOctreeId.Find(Object); }
	FORCEINLINE	void RemoveObjectsNavOctreeId(UObject* Object) { ObjectToOctreeId.Remove(Object); }

	/** find all elements in navigation octree within given box (intersection) */
	void FindElementsInNavOctree(const FBox& QueryBox, const struct FNavigationOctreeFilter& Filter, TArray<struct FNavigationOctreeElement>& Elements);

	//----------------------------------------------------------------------//
	// Smart links
	//----------------------------------------------------------------------//
	void RegisterSmartLink(class USmartNavLinkComponent* LinkComp);
	void UnregisterSmartLink(class USmartNavLinkComponent* LinkComp);
	
	/** find first available smart link ID */
	uint32 FindFreeSmartLinkId() const;

	/** find smart link by link ID */
	class USmartNavLinkComponent* GetSmartLink(uint32 SmartLinkId) const;

	/** updates smart link for all active navigation data instances */
	void UpdateSmartLink(class USmartNavLinkComponent* LinkComp);

	//----------------------------------------------------------------------//
	// Areas
	//----------------------------------------------------------------------//
	static void RequestAreaRegistering(UClass* NavAreaClass);
	static void RequestAreaUnregistering(UClass* NavAreaClass);

	/** find index in SupportedAgents array for given navigation data */
	int32 GetSupportedAgentIndex(const ANavigationData* NavData) const;

	/** find index in SupportedAgents array for agent type */
	int32 GetSupportedAgentIndex(const FNavAgentProperties& NavAgent) const;

	//----------------------------------------------------------------------//
	// Filters
	//----------------------------------------------------------------------//
	
	/** prepare descriptions of navigation flags in UNavigationQueryFilter class: using enum */
	void DescribeFilterFlags(class UEnum* FlagsEnum) const;

	/** prepare descriptions of navigation flags in UNavigationQueryFilter class: using array */
	void DescribeFilterFlags(const TArray<FString>& FlagsDesc) const;

	/** removes cached filters from currently registered navigation data */
	void ResetCachedFilter(TSubclassOf<UNavigationQueryFilter> FilterClass);

	//----------------------------------------------------------------------//
	// building
	//----------------------------------------------------------------------//
#if WITH_NAVIGATION_GENERATOR
	/** Triggers navigation building on all eligible navigation data. */
	virtual void Build();

	// @todo document
	void OnUpdateStreamingStarted();
	// @todo document
	void OnUpdateStreamingFinished();
	// @todo document
	void OnPIEStart();
	// @todo document
	void OnPIEEnd();
	
	// @todo document
	FORCEINLINE bool IsNavigationBuildingLocked() const { return bNavigationBuildingLocked || bInitialBuildingLockActive; }

	// @todo document
	void OnNavigationBoundsUpdated(class AVolume* NavVolume);

	/** Used to display "navigation building in progress" notify */
	bool IsNavigationBuildInProgress(bool bCheckDirtyToo = true);
#endif // WITH_NAVIGATION_GENERATOR

	/** Sets up SuportedAgents and NavigationDataCreators. Override it to add additional setup, but make sure to call Super implementation */
	virtual void DoInitialSetup();

	/** Called upon UWorld destruction to release what needs to be released */
	void CleanUp();

	/** 
	 *	Called when owner-UWorld initializes actors
	 */
	virtual void OnInitializeActors();

	/** */
	virtual void OnWorldInitDone(FNavigationSystem::EMode Mode);

#if WITH_EDITOR
	/** allow editor to toggle whether seamless navigation building is enabled */
	static void SetNavigationAutoUpdateEnabled(bool bNewEnable, UNavigationSystem* InNavigationsystem);

	/** check whether seamless navigation building is enabled*/
	FORCEINLINE static bool GetIsNavigationAutoUpdateEnabled() { return bNavigationAutoUpdateEnabled; }

	bool AreFakeComponentChangesBeingApplied() { return bFakeComponentChangesBeingApplied; }
	void BeginFakeComponentChanges() { bFakeComponentChangesBeingApplied = true; }
	void EndFakeComponentChanges() { bFakeComponentChangesBeingApplied = false; }

	void UpdateLevelCollision(ULevel* InLevel);

	virtual void OnEditorModeChanged(class FEdMode* Mode, bool IsEntering);
#endif // WITH_EDITOR

	/** register actor important for generation (navigation data will be build around them first) */
	void RegisterGenerationSeed(AActor* SeedActor);
	void UnregisterGenerationSeed(AActor* SeedActor);
	void GetGenerationSeeds(TArray<FVector>& SeedLocations) const;

	static UNavigationSystem* CreateNavigationSystem(class UWorld* WorldOwner);

	static UNavigationSystem* GetCurrent(class UWorld* World);
	static UNavigationSystem* GetCurrent(class UObject* WorldContextObject);

	/** try to create and setup navigation system */
	static void InitializeForWorld(class UWorld* World, FNavigationSystem::EMode Mode);

	// Fetch the array of all nav-agent properties.
	void GetNavAgentPropertiesArray(TArray<FNavAgentProperties>& OutNavAgentProperties) const;

	static FORCEINLINE bool ShouldUpdateNavOctreeOnPrimitiveComponentChange()
	{
		return bUpdateNavOctreeOnPrimitiveComponentChange;
	}

	/** 
	 * Exec command handlers
	 */
	bool HandleCycleNavDrawnCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleCountNavMemCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	
	//----------------------------------------------------------------------//
	// debug
	//----------------------------------------------------------------------//
	void CycleNavigationDataDrawn();

protected:
#if WITH_EDITOR
	bool bFakeComponentChangesBeingApplied;
#endif

	FNavigationSystem::EMode OperationMode;

	class FNavigationOctree* NavOctree;

	TArray<FAsyncPathFindingQuery> AsyncPathFindingQueries;

	FCriticalSection NavDataRegistration;

	TMap<FNavAgentProperties, ANavigationData*> AgentToNavDataMap;
	
	TMap<const UObject*, FOctreeElementId> ObjectToOctreeId;

	/** Map of all custom navigation links, that are relevant for path following */
	TMap<uint32, class USmartNavLinkComponent*> SmartLinksMap;

	/** List of actors relevant to generation of navigation data */
	TArray<TWeakObjectPtr<AActor> > GenerationSeeds;

#if WITH_NAVIGATION_GENERATOR
	/** stores areas marked as dirty throughout the frame, processes them 
	 *	once a frame in Tick function */
	TArray<FNavigationDirtyArea> DirtyAreas;
#endif // WITH_NAVIGATION_GENERATOR	

	// async queries
	FCriticalSection NavDataRegistrationSection;
	
	uint32 bNavigationBuildingLocked:1;
	uint32 bInitialBuildingLockActive:1;
	uint32 bInitialSetupHasBeenPerformed:1;

	/** cached navigable world bounding box*/
	mutable FBox NavigableWorldBounds;

	/** indicates which of multiple navigation data instances to draw*/
	int32 CurrentlyDrawnNavDataIndex;

	/** temporary cumulative time to calculate when we need to update dirty areas */
	float DirtyAreasUpdateTime;

#if !UE_BUILD_SHIPPING
	/** self-registering exec command to handle nav sys console commands */
	static FNavigationSystemExec ExecHandler;
#endif // !UE_BUILD_SHIPPING

	/** collection of delegates to create all available navigation data types */
	static TArray<TSubclassOf<class ANavigationData> > NavDataClasses;
	
	/** whether seamless navigation building is enabled */
	static bool bNavigationAutoUpdateEnabled;

	static bool bUpdateNavOctreeOnPrimitiveComponentChange;

	static TArray<UClass*> PendingNavAreaRegistration;
	static TArray<const UClass*> NavAreaClasses;
	static TSubclassOf<class UNavArea> DefaultWalkableArea;
	static TSubclassOf<class UNavArea> DefaultObstacleArea;

	static FOnNavAreaChanged NavAreaAddedObservers;
	static FOnNavAreaChanged NavAreaRemovedObservers;

	/** delegate handler for PostLoadMap event */
	void OnPostLoadMap();
	/** delegate handler for ActorMoved events */
	void OnActorMoved(AActor* Actor);
	/** delegate handler called when navigation is dirtied*/
	void OnNavigationDirtied(const FBox& Bounds);
	
	/** Registers given navigation data with this Navigation System.
	 *	@return RegistrationSuccessful if registration was successful, other results mean it failed
	 *	@see ERegistrationResult
	 */
	ERegistrationResult RegisterNavData(class ANavigationData* NavData);

	/** tries to register navigation area */
	void RegisterNavAreaClass(UClass* NavAreaClass);
	
 	FSetElementId RegisterNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds);
	void UnregisterNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds);
	
	/** read element data from navigation octree */
	bool GetNavOctreeElementData(class UObject* NodeOwner, int32& DirtyFlags, FBox& DirtyBounds);

	/** Adds given element to NavOctree. No check for owner's validity are performed, 
	 *	nor its presence in NavOctree - function assumes callee responsibility 
	 *	in this regard **/
	void AddElementToNavOctree(const FNavigationDirtyElement& DirtyElement);

	void AddActorElementToNavOctree(class AActor* Actor, const FNavigationDirtyElement& DirtyElement);
	void AddComponentElementToNavOctree(class UActorComponent* ActorComp, const FNavigationDirtyElement& DirtyElement, const FBox& Bounds);

	void SetCrowdManager(class UCrowdManager* NewCrowdManager); 

#if WITH_NAVIGATION_GENERATOR

	/** Add BSP collision data to navigation octree */
	void AddLevelCollisionToOctree(ULevel* Level);
	
	/** Remove BSP collision data from navigation octree */
	void RemoveLevelCollisionFromOctree(ULevel* Level);

	/** registers or unregisters all generators from rebuilding callbacks */
	void EnableAllGenerators(bool bEnable, bool bForce = false);

private:
	// @todo document
	void NavigationBuildingLock();
	// @todo document
	void NavigationBuildingUnlock(bool bForce = false);

	void SpawnMissingNavigationData();

	/** constructs a navigation data instance of specified NavDataClass, in passed World
	 *	for supplied NavConfig */
	virtual ANavigationData* CreateNavigationDataInstance(TSubclassOf<class ANavigationData> NavDataClass, UWorld* World, const FNavDataConfig& NavConfig);

	/** Triggers navigation building on all eligible navigation data. */
	void RebuildAll();
		 
	/** Handler for FWorldDelegates::LevelAddedToWorld event */
	 void OnLevelAddedToWorld(ULevel* InLevel, UWorld* InWorld);
	 
	/** Handler for FWorldDelegates::LevelRemovedFromWorld event */
	void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);
#endif // WITH_NAVIGATION_GENERATOR

	/** Adds given request to requests queue. Note it's to be called only on game thread only */
	void AddAsyncQuery(const FAsyncPathFindingQuery& Query);
		 
	/** spawns a non-game-thread task to process requests given in PathFindingQueries.
	 *	In the process PathFindingQueries gets copied. */
	void TriggerAsyncQueries(TArray<FAsyncPathFindingQuery>& PathFindingQueries);

	/** Processes pathfinding requests given in PathFindingQueries.*/
	void PerformAsyncQueries(TArray<FAsyncPathFindingQuery> PathFindingQueries);
};

