// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationData.generated.h"

USTRUCT()
struct FSupportedAreaData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString AreaClassName;

	UPROPERTY()
	int32 AreaID;

	UPROPERTY(transient)
	const UClass* AreaClass;
};

/** 
 *	Represents abstract Navigation Data (sub-classed as NavMesh, NavGraph, etc)
 *	Used as a common interface for all navigation types handled by NavigationSystem
 */
UCLASS(config=Engine, dependson=UNavigationSystem, MinimalAPI, NotBlueprintable, abstract)
class ANavigationData : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UPrimitiveComponent* RenderingComp;

	UPROPERTY()
	FNavDataConfig NavDataConfig;

	/** if set to true then this navigation data will be drawing itself when requested as part of "show navigation" */
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bEnableDrawing:1;

	/** navigation data will be drawn only if it's for default agent (SupportedAgents[0]) */
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bShowOnlyDefaultAgent:1;

	//----------------------------------------------------------------------//
	// game-time config
	//----------------------------------------------------------------------//

	/** Size of the tallest agent that will path with this navmesh. */
	UPROPERTY(EditAnywhere, Category=Runtime, config)
	uint32 bRebuildAtRuntime:1;

	//----------------------------------------------------------------------//
	// Life cycle                                                                
	//----------------------------------------------------------------------//
	/** Dtor */
	virtual ~ANavigationData();

	// Begin UObject/AActor Interface
	virtual void PostInitProperties() OVERRIDE;
	virtual void PostInitializeComponents() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditUndo() OVERRIDE;
#endif // WITH_EDITOR
	virtual void Destroyed() OVERRIDE;
	// End UObject Interface
		
	virtual void CleanUp();

	virtual void CleanUpAndMarkPendingKill();

	FORCEINLINE bool IsRegistered() const { return bRegistered; }
	void OnRegistered();
	
	FORCEINLINE uint16 GetNavDataUniqueID() const { return NavDataUniqueID; }

	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) OVERRIDE;

	virtual void RerunConstructionScripts() OVERRIDE;

	virtual bool NeedsRebuild() { return false; }

	//----------------------------------------------------------------------//
	// Generation & data access                                                      
	//----------------------------------------------------------------------//
public:
	const FNavDataConfig& GetConfig() const { return NavDataConfig; }
	virtual void SetConfig(const FNavDataConfig& Src) { NavDataConfig = Src; }

	void SetSupportsDefaultAgent(bool bIsDefault) { bSupportsDefaultAgent = bIsDefault; } 
	bool IsSupportingDefaultAgent() const { return bSupportsDefaultAgent; }

protected:
	virtual void FillConfig(FNavDataConfig& Dest) { Dest = NavDataConfig; }

public:
#if WITH_NAVIGATION_GENERATOR 
	/** Retrieves navmesh's generator, creating one if not already present */
	class FNavDataGenerator* GetGenerator(NavigationSystem::ECreateIfEmpty CreateIfNone);

	const class FNavDataGenerator* GetGenerator() const { return NavDataGenerator.Get(); }
#endif // WITH_NAVIGATION_GENERATOR
	/** releases navigation generator if any has been created */
protected:
	virtual void DestroyGenerator();

	/** register self with navigation system as new NavAreaDefinition(s) observer */
	void RegisterAsNavAreaClassObserver();

public:
	/** 
	 *	Created an instance of navigation path of specified type.
	 *	PathType needs to derive from FNavigationPath 
	 */
	template<typename PathType> 
	FNavPathSharedPtr CreatePathInstance() const
	{
		FNavPathSharedPtr SharedPath = MakeShareable(new PathType());
		SharedPath->SetOwner(this);

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &ANavigationData::RegisterActivePath, SharedPath)
			, TEXT("Adding a path to ActivePaths")
			, NULL
			, ENamedThreads::GameThread
			);

		return SharedPath;
	}

	/** removes from ActivePaths all paths that no longer have shared references (and are invalid in fact) */
	void PurgeUnusedPaths();

protected:
	void RegisterActivePath(FNavPathSharedPtr SharedPath)
	{
		check(IsInGameThread());
		ActivePaths.Add(SharedPath);
	}

public:
	/** Returns bounding box for the navmesh. */
	virtual FBox GetBounds() const PURE_VIRTUAL(ANavigationData::GetBounds,return FBox(););
	
	//----------------------------------------------------------------------//
	// Debug                                                                
	//----------------------------------------------------------------------//
	ENGINE_API void DrawDebugPath(FNavigationPath* Path, FColor PathColor = FColor::White, class UCanvas* Canvas = NULL, bool bPersistent = true, const uint32 NextPathPointIndex = 0) const;

	/** @return Total mem counted, including super calls. */
	virtual ENGINE_API uint32 LogMemUsed() const;

	//----------------------------------------------------------------------//
	// Batch processing (important with async rebuilding)
	//----------------------------------------------------------------------//

	/** Starts batch processing and locks access to navigation data from other threads */
	virtual void BeginBatchQuery() const {}

	/** Finishes batch processing and release locks */
	virtual void FinishBatchQuery() const {};

	//----------------------------------------------------------------------//
	// Querying                                                                
	//----------------------------------------------------------------------//
	FORCEINLINE TSharedPtr<const FNavigationQueryFilter> GetDefaultQueryFilter() const { return DefaultQueryFilter; }
	FORCEINLINE const INavigationQueryFilterInterface* GetDefaultQueryFilterImpl() const { return DefaultQueryFilter->GetImplementation(); }	
	FORCEINLINE FVector GetDefaultQueryExtent() const { return NavDataConfig.DefaultQueryExtent; }

	/** 
	 *	Synchronously looks for a path from @StartLocation to @EndLocation for agent with properties @AgentProperties. NavMesh actor appropriate for specified 
	 *	FNavAgentProperties will be found automatically
	 *	@param ResultPath results are put here
	 *	@return true if path has been found, false otherwise
	 *
	 *	@note don't make this function virtual! Look at implementation details and its comments for more info.
	 */
	FORCEINLINE FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query) const
	{
		check(FindPathImplementation);
		// this awkward implementation avoids virtual call overhead - it's possible this function will be called a lot
		return (*FindPathImplementation)(AgentProperties, Query);
	}

	/** 
	 *	Synchronously looks for a path from @StartLocation to @EndLocation for agent with properties @AgentProperties. NavMesh actor appropriate for specified 
	 *	FNavAgentProperties will be found automatically
	 *	@param ResultPath results are put here
	 *	@return true if path has been found, false otherwise
	 *
	 *	@note don't make this function virtual! Look at implementation details and its comments for more info.
	 */
	FORCEINLINE FPathFindingResult FindHierarchicalPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query) const
	{
		check(FindHierarchicalPathImplementation);
		// this awkward implementation avoids virtual call overhead - it's possible this function will be called a lot
		return (*FindHierarchicalPathImplementation)(AgentProperties, Query);
	}

	/** 
	 *	Synchronously checks if path between two points exists
	 *	FNavAgentProperties will be found automatically
	 *	@return true if path has been found, false otherwise
	 *
	 *	@note don't make this function virtual! Look at implementation details and its comments for more info.
	 */
	FORCEINLINE bool TestPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query) const
	{
		check(TestPathImplementation);
		// this awkward implementation avoids virtual call overhead - it's possible this function will be called a lot
		return (*TestPathImplementation)(AgentProperties, Query);
	}

	/** 
	 *	Synchronously checks if path between two points exists using hierarchical graph
	 *	FNavAgentProperties will be found automatically
	 *	@return true if path has been found, false otherwise
	 *
	 *	@note don't make this function virtual! Look at implementation details and its comments for more info.
	 */
	FORCEINLINE bool TestHierarchicalPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query) const
	{
		check(TestHierarchicalPathImplementation);
		// this awkward implementation avoids virtual call overhead - it's possible this function will be called a lot
		return (*TestHierarchicalPathImplementation)(AgentProperties, Query);
	}

	/** 
	 *	Synchronously makes a raycast on navigation data using QueryFilter
	 *	@param HitLocation if line was obstructed this will be set to hit location. Otherwise it contains SegmentEnd
	 *	@return true if line from RayStart to RayEnd was obstructed
	 *
	 *	@note don't make this function virtual! Look at implementation details and its comments for more info.
	 */
	FORCEINLINE bool Raycast(const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const
	{
		check(RaycastImplementation);
		// this awkward implementation avoids virtual call overhead - it's possible this function will be called a lot
		return (*RaycastImplementation)(this, RayStart, RayEnd, HitLocation, QueryFilter);
	}

	virtual FNavLocation ENGINE_API GetRandomPoint(TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const PURE_VIRTUAL(ANavigationData::GetRandomPoint,return FNavLocation(););

	virtual bool ENGINE_API GetRandomPointInRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const PURE_VIRTUAL(ANavigationData::GetRandomPointInRadius,return false;);

	/**	Tries to project given Point to this navigation type, within given Extent.
	 *	@param OutLocation if successful this variable will be filed with result
	 *	@return true if successful, false otherwise
	 */
	virtual bool ENGINE_API ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const PURE_VIRTUAL(ANavigationData::ProjectPoint,return false;);

	/** Calculates path from PathStart to PathEnd and retrieves its cost. 
	 *	@NOTE this function does not generate string pulled path so the result is an (over-estimated) approximation
	 *	@NOTE potentially expensive, so use it with caution */
	virtual ENavigationQueryResult::Type CalcPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const PURE_VIRTUAL(ANavigationData::CalcPathCost,return ENavigationQueryResult::Invalid;);

	/** Calculates path from PathStart to PathEnd and retrieves its length.
	 *	@NOTE this function does not generate string pulled path so the result is an (over-estimated) approximation
	 *	@NOTE potentially expensive, so use it with caution */
	virtual ENavigationQueryResult::Type CalcPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const PURE_VIRTUAL(ANavigationData::CalcPathLength,return ENavigationQueryResult::Invalid;);

	/** Calculates path from PathStart to PathEnd and retrieves its length.
	 *	@NOTE this function does not generate string pulled path so the result is an (over-estimated) approximation
	 *	@NOTE potentially expensive, so use it with caution */
	virtual ENavigationQueryResult::Type CalcPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const PURE_VIRTUAL(ANavigationData::CalcPathLength,return ENavigationQueryResult::Invalid;);

	//----------------------------------------------------------------------//
	// Areas
	//----------------------------------------------------------------------//

	/** new area was registered in navigation system */
	virtual void OnNavAreaAdded(const UClass* NavAreaClass, int32 AgentIndex);
	void OnNavAreaAddedNotify(const UClass* NavAreaClass);
	
	/** area was removed from navigation system */
	virtual void OnNavAreaRemoved(const UClass* NavAreaClass);
	void OnNavAreaRemovedNotify(const UClass* NavAreaClass);
	
	/** add all existing areas */
	void ProcessNavAreas(const TArray<const UClass*>& AreaClasses, int32 AgentIndex);

	/** get class associated with AreaID */
	ENGINE_API const UClass* GetAreaClass(int32 AreaID) const;
	
	/** check if AreaID was assigned to class (class itself may not be loaded yet!) */
	bool IsAreaAssigned(int32 AreaID) const;

	/** get ID assigned to AreaClas or -1 when not assigned */
	ENGINE_API int32 GetAreaID(const UClass* AreaClass) const;

	/** get max areas supported by this navigation data */
	virtual int32 GetMaxSupportedAreas() const { return MAX_int32; }

	/** read all supported areas */
	void GetSupportedAreas(TArray<FSupportedAreaData>& Areas) const { Areas = SupportedAreas; }

	//----------------------------------------------------------------------//
	// Smart links
	//----------------------------------------------------------------------//

	virtual void UpdateSmartLink(class USmartNavLinkComponent* LinkComp);

	//----------------------------------------------------------------------//
	// all the rest                                                                
	//----------------------------------------------------------------------//
	virtual UPrimitiveComponent* ConstructRenderingComponent() { return NULL; }

protected:
	void InstantiateAndRegisterRenderingComponent();

	/** get ID to assign for newly added area */
	virtual int32 GetNewAreaID(const UClass* AreaClass) const;
	
#if WITH_NAVIGATION_GENERATOR
	virtual class FNavDataGenerator* ConstructGenerator(const FNavAgentProperties& AgentProps) PURE_VIRTUAL(ANavigationData::ConstructGenerator, return NULL; );
#endif // WITH_NAVIGATION_GENERATOR

protected:
	/** Navigation data versioning. */
	uint32 DataVersion;

	typedef FPathFindingResult (*FFindPathPtr)(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
	FFindPathPtr FindPathImplementation;
	FFindPathPtr FindHierarchicalPathImplementation; 
	
	typedef bool (*FTestPathPtr)(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
	FTestPathPtr TestPathImplementation;
	FTestPathPtr TestHierarchicalPathImplementation; 

	typedef bool (*FNavRaycastPtr)(const ANavigationData* NavDataInstance, const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter);
	FNavRaycastPtr RaycastImplementation; 

public:
	/** common shared pointer type for all generators */
	typedef TSharedPtr<FNavDataGenerator, ESPMode::ThreadSafe> FNavDataGeneratorSharedPtr;

	/** common weak pointer type for all generators */
	typedef TWeakPtr<FNavDataGenerator, ESPMode::ThreadSafe> FNavDataGeneratorWeakPtr;

protected:
#if WITH_NAVIGATION_GENERATOR
	FNavDataGeneratorSharedPtr NavDataGenerator;
#endif // WITH_NAVIGATION_GENERATOR

	/** 
	 *	Container for all path objects generated with this Navigation Data instance. 
	 *	Is meant to be added to only on GameThread, and in fact should user should never 
	 *	add items to it manually, @see CreatePathInstance
	 */
	TArray<FNavPathWeakPtr> ActivePaths;

	/** Query filter used when no other has been passed to relevant functions */
	TSharedPtr<FNavigationQueryFilter> DefaultQueryFilter;

	/** serialized area class - ID mapping */
	UPROPERTY()
	TArray<FSupportedAreaData> SupportedAreas;

	/** whether this instance is registered with Navigation System*/
	uint32 bRegistered : 1;

	/** was it generated for default agent (SupportedAgents[0]) */
	uint32 bSupportsDefaultAgent:1;

private:
	uint16 NavDataUniqueID;

	static uint16 GetNextUniqueID();
};
