// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationComponent.generated.h"

UCLASS(HeaderGroup=Component, dependson=(UNavAgentInterface, UNavPathObserverInterface, UNavigationSystem), config=Engine)
class ENGINE_API UNavigationComponent : public UActorComponent, public INavigationPathGenerator
{
	GENERATED_UCLASS_BODY()

	static const bool QuerySuccess = true;
	static const bool QueryFailure = false;

	/** Actor this instance is observing. Can be NULL */
	UPROPERTY()
	const class AActor* GoalActor;

	// Begin UActorComponent Interface
	virtual void InitializeComponent() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	virtual void SetComponentTickEnabledAsync(bool bEnabled) OVERRIDE;
	// End UActorComponent Interface

	// Begin INavigationPathGenerator Interface
	virtual FNavPathSharedPtr GetGeneratedPath(class INavAgentInterface* Agent) OVERRIDE { return Path; }
	// End INavigationPathGenerator Interface

	/** updates cached pointers to relevant owner's components */
	virtual void UpdateCachedComponents();

	/** called to notify navigation component to update various cached data related to navigation agent it's driving */
	virtual void OnNavAgentChanged();

	//----------------------------------------------------------------------//
	// querying                                                                
	//----------------------------------------------------------------------//
	
	/** Retrieves ANavigationData instance being a context to this NavigationComponent. */
	FORCEINLINE const class ANavigationData* GetNavData() const { return MyNavData ? MyNavData : PickNavData(); }
	
	/** Retrieves ANavigationData instance being a context to this NavigationComponent. */
	FORCEINLINE class ANavigationData* GetNavData() { return MyNavData ? MyNavData : PickNavData(); }

	/** Returns a random location on the navmesh. */
	FNavLocation GetRandomPointOnNavMesh() const;

	/** Projects given Location to navigation referenced by this NavigationComponent,
	 *	using any additional internal information available. */
	FNavLocation ProjectPointToNavigation(const FVector& Location) const;

	/** Projects given Location to navigation referenced by this NavigationComponent,
	 *	using any additional internal information available. */
	bool ProjectPointToNavigation(const FVector& Location, FNavLocation& ProjectedLocation) const;
	bool ProjectPointToNavigation(const FVector& Location, FVector& ProjectedLocation) const;
	
	/** Requests generating simple path in straight line to an Actor,
	 *  and continuous update when goal actor moves more than RepathDistance from path end point.
	 *	It doesn't check bDoAsyncPathfinding flag, and path will be always available after function call */
	bool FindSimplePathToActor(const AActor* NewGoalActor, float RepathDistance = 100.0f);

	/** Requests generating path to an Actor, and continuous update of path 
	 *	if GoalActor moves more than RepathDistance from path end point.
	 *	If bDoAsyncPathfinding == true then MyNavAgent->OnPathUpdated will be 
	 *	called when path finishes generating */
	bool FindPathToActor(const AActor* NewGoalActor, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL, float RepathDistance = 100.f);

	/** requests generating simple path in straight line to specified DestLocation. Aborts previous pathfinding requests. 
	 *	It doesn't check bDoAsyncPathfinding flag, and path will be always available after function call */
	bool FindSimplePathToLocation(const FVector& DestLocation);

	/** requests generation of a path to a specified pawn. Aborts previous pathfinding requests. 
	 *	If bDoAsyncPathfinding == true then MyNavAgent->OnPathUpdated will be 
	 *	called when path finishes generating */
	bool FindPathToLocation(const FVector& DestLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/**
	 *	Freezes NavComponent's actions like tracing goal actor. If following 
	 *	pathfinding request will have the same destination then stored
	 *	path will be reused (if still valid)
	 */
	void Pause();

	/** Notifies navigation system given pathfinding request is no longer valid.
	 *	Even if path ends up being generated (due to the async nature of its
	 *	generation) result will be discarded. 
	 *	@NOTE Function just aborts currently requested path. If NavigationComponent 
	 *	instance observes and actor it will generate a new one. */
	void AbortAsyncFindPathRequest();

	virtual void SetPath(const FNavPathSharedPtr& NewPath);
	
	/** @todo: document */
	bool GetPathPoint(uint32 PathVertIdx, FNavPathPoint& PathPoint) const;

	FORCEINLINE const FNavPathSharedPtr& GetPath() const { return Path; }

	/** @todo: document */
	bool HasValidPath() const;

	FORCEINLINE bool IsPathPartial() const { return bIsPathPartial != 0; }

	/** @todo: document */
	FORCEINLINE bool IsFollowing() const { return GoalActor != NULL; }

	FORCEINLINE bool DoesAsyncPathfinding() const { return bDoAsyncPathfinding; }

	FORCEINLINE bool IsPaused() const { return bIsPaused; }

	/** Return true if path should be updated when nearby smart link change its state */
	FORCEINLINE bool WantsSmartLinkUpdates() const { return bUpdateForSmartLinks; }

	/** Returns current move goal for actor, using data from GetMoveGoalOffset() */
	FORCEINLINE FVector GetCurrentMoveGoal(const class AActor* InGoalActor, class AActor* MovingActor) const
	{
		const INavAgentInterface* NavAgent = InterfaceCast<const INavAgentInterface>(InGoalActor);
		if (NavAgent)
		{
			const FVector Offset = NavAgent->GetMoveGoalOffset(MovingActor);
			const FVector MoveGoal = FRotationTranslationMatrix(InGoalActor->GetActorRotation(), NavAgent->GetNavAgentLocation()).TransformPosition(Offset);
			return MoveGoal;
		}
		else
		{
			return InGoalActor->GetActorLocation();
		}
	}

	//----------------------------------------------------------------------//
	// misc
	//----------------------------------------------------------------------//

	/** Aborts actor following process triggered with FindPathToActor */
	void ResetTransientData();

	/** Notify path following about changes to update current movement */
	void SetPathFollowingUpdates(bool bEnabled);

	/** Find new path when it gets invalidated (e.g. runtime navmesh rebuild) */
	void SetRepathWhenInvalid(bool bEnabled);

	UFUNCTION()
	virtual void OnNavDataRegistered(class ANavigationData* NavData);

	/** 
	 *	Called when given path becomes invalid (via @see PathObserverDelegate)
	 *	NOTE: InvalidatedPath doesn't have to be instance's current Path
	 */
	void OnPathInvalid(FNavigationPath* InvalidatedPath);

	/** Called when nearby smart link changes its state */
	virtual void OnSmartLinkBroadcast(class USmartNavLinkComponent* NearbyLink);

	/** set whether this component should receive update broadcasts from nearby smart links or not */
	void SetReceiveSmartLinkUpdates(bool bEnabled);

	//----------------------------------------------------------------------//
	// debug
	//----------------------------------------------------------------------//

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

	/** quick and dirty debug render. If Canvas not NULL additional data (if present) will be drawn */
	void DrawDebugCurrentPath(class UCanvas* Canvas = NULL, FColor PathColor = FColor::White, const uint32 NextPathPointIndex = 0);


protected:

	/** associated path following component */
	UPROPERTY(transient)
	class UPathFollowingComponent* PathFollowComp;

	/** Shared path - there can be multiple users of one path instance. No not modify it, unless you know what you're doing */
	FNavPathSharedPtr Path;

	/** caches information whether contained path is partial (i.e. destination was not reachable so path leads to best guess location */
	uint32 bIsPathPartial : 1;

	/** whether component wants to be ticked */
	uint32 bShoudTick : 1;

	/** NavigationComponent can be "Paused" to not update it's path or observe it in any way
	 *	but not release generated path neither. Subsequent calls to FindPathToActor or 
	 *	FindPathToLocation will result in checking if new destination is the same as
	 *	the old one, and if so internal ResumePath will be called 
	 *	@NOTE: contained path invalidation while component is paused will result 
	 *		in clearing pause along with other transient data */
	uint32 bIsPaused : 1;

	/** repath to goal actor using simple paths (without navigation data) */
	uint32 bUseSimplePath : 1;

	/** if set, path following component will be notified about path changes */
	uint32 bNotifyPathFollowing : 1;

	/** if set, invalid paths will trigger repath */
	uint32 bRepathWhenInvalid : 1;

	/** if set, path will be updated if agent receives smart link broadcast */
	uint32 bUpdateForSmartLinks : 1;

	UPROPERTY(config)
	uint32 bDoAsyncPathfinding : 1;

	/** use HPA* for pathfind requests (faster, but less accurate) */
	UPROPERTY(config)
	uint32 bDoHierarchicalPathfinding : 1;

	/** if A* search fails on running out of nodes, try searching again but switch start with goal
	 *  (uses backtracking feature to handle one way offmesh links properly) */
	UPROPERTY(config)
	uint32 bSearchFromGoalWhenOutOfNodes : 1;

	mutable INavAgentInterface* MyNavAgent;
	mutable INavPathObserverInterface* MyPathObserver;

	FVector CurrentGoal;

	TSharedPtr<const FNavigationQueryFilter> StoredQueryFilter;

	/** contains location of GoalActor from the moment path has been generated. Used for 
	 *	generated path invalidation when if goal actor moved away from original location */
	FVector OriginalGoalActorLocation;

	int32 AsynPathQueryID;
	
	/** if GoalActor moves move than this away from last point of Path Navigation Component will re-run path finding */
	float RepathDistanceSq;

	FNavigationPath::FPathObserverDelegate PathObserverDelegate;

	struct FDeferredRepath
	{
		TWeakObjectPtr<AActor> GoalActor;
		FVector GoalLocation;
		bool bSimplePath;
	};
	FDeferredRepath RepathData;

	/** asks NavigationSystem for NavigationData appropriate for self and caches it in MyNavData */
	class ANavigationData* PickNavData() const;

	/** Generates simple path in straight line without using navigation data. */
	bool GenerateSimplePath(const FVector& GoalLocation);

	/** Generates a path between component owner's location and GoalLocation and stores 
	 *	it locally in PathPolys.  Path is available upon return from this function. */
	bool GeneratePathTo(const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/** Generates a path between the given points and stores it locally in PathPolys.  
	 *	Path is available upon return from this function. */
	bool GeneratePath(const FVector& FromLocation, const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/** Asynchronous version of GeneratePathTo().  Path isn't available until callback is received. */
	bool AsyncGeneratePathTo(const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/** Asynchronous version of GeneratePath().  Path isn't available until callback is received. */
	bool AsyncGeneratePath(const FVector& FromLocation, const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	bool ResumePath();

	void DeferredResumePath();
	
	bool RePathTo(const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> StoredQueryFilter = NULL);

	/** send notifies about updated path */
	void NotifyPathUpdate();

	/** find new path to GoalActor or CurrentGoal  */
	void DeferredRepathToGoal();

private:

	/** cached pointer to ANavMesh this navigation component's using. Use Reset to force recaching. 
	 *	@note read it via GetNavData that will update it if null. */
	UPROPERTY()
	mutable ANavigationData* MyNavData;

	void AsyncGeneratePath_OnCompleteCallback(uint32 QueryID, ENavigationQueryResult::Type Result, FNavPathSharedPtr ResultPath);	
};
