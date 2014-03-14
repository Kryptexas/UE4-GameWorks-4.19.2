// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AI/AITypes.h"
#include "NavigationTypes.h"
#include "Components/ActorComponent.h"
#include "AI/AIResourceInterface.h"
#include "PathFollowingComponent.generated.h"

UENUM(BlueprintType)
namespace EPathFollowingStatus
{
	enum Type
	{
		Idle,				// no requests
		Waiting,			// request with incomplete path, will start after UpdateMove()
		Paused,				// request paused, will continue after ResumeMove()
		Moving,				// following path
	};
}

UENUM(BlueprintType)
namespace EPathFollowingResult
{
	enum Type
	{
		Success,			// reached destination
		Blocked,			// movement was blocked
		OffPath,			// agent is not on path
		Aborted,			// aborted and stopped (failure)
		Skipped,			// aborted and replaced with new request
		Invalid,			// request was invalid
	};
}

// left for now, will be removed soon! please use EPathFollowingStatus instead
UENUM(BlueprintType)
namespace EPathFollowingAction
{
	enum Type
	{
		Error,
		NoMove,
		DirectMove,
		PartialPath,
		PathToGoal,
	};
}

UENUM(BlueprintType)
namespace EPathFollowingRequestResult
{
	enum Type
	{
		Failed,
		AlreadyAtGoal,
		RequestSuccessful
	};
}

UCLASS(HeaderGroup=Component, config=Engine)
class ENGINE_API UPathFollowingComponent : public UActorComponent, public IAIResourceInterface
{
	GENERATED_UCLASS_BODY()

	DECLARE_DELEGATE_TwoParams(FPostProcessMoveSignature, UPathFollowingComponent* /*comp*/, FVector& /*velocity*/);
	DECLARE_DELEGATE_OneParam(FRequestCompletedSignature, EPathFollowingResult::Type /*Result*/);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FMoveCompletedSignature, uint32 /*RequestID*/, EPathFollowingResult::Type /*Result*/);

	/** delegate for modifying path following velocity */
	FPostProcessMoveSignature PostProcessMove;

	/** delegate for move completion notify */
	FMoveCompletedSignature OnMoveFinished;

	// Begin UActorComponent Interface
	virtual void InitializeComponent() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	// End UActorComponent Interface

	/** updates cached pointers to relevant owner's components */
	virtual void UpdateCachedComponents();

	/** start movement along path
	 *  @returns request ID or 0 when failed */
	virtual uint32 RequestMove(FNavPathSharedPtr Path, FRequestCompletedSignature OnComplete, const AActor* DestinationActor = NULL, float AcceptanceRadius = UPathFollowingComponent::DefaultAcceptanceRadius, bool bStopOnOverlap = true, FCustomMoveSharedPtr GameData = NULL);

	/** start movement along path
	 *  @returns request ID or 0 when failed */
	FORCEINLINE uint32 RequestMove(FNavPathSharedPtr InPath, const AActor* InDestinationActor = NULL, float InAcceptanceRadius = UPathFollowingComponent::DefaultAcceptanceRadius, bool InStopOnOverlap = true, FCustomMoveSharedPtr InGameData = NULL)
	{
		return RequestMove(InPath, UnboundRequestDelegate, InDestinationActor, InAcceptanceRadius, InStopOnOverlap, InGameData);
	}

	/** update path for specified request
	 *  @param RequestID - request to update */
	virtual bool UpdateMove(FNavPathSharedPtr Path, uint32 RequestID = UPathFollowingComponent::CurrentMoveRequestMetaId);

	/** aborts following path
	 *  @param RequestID - request to abort, 0 = current
	 *  @param bResetVelocity - try to stop movement component
	 *  @param bSilent - finish with Skipped result instead of Aborted */
	virtual void AbortMove(const FString& Reason, uint32 RequestID = UPathFollowingComponent::CurrentMoveRequestMetaId, bool bResetVelocity = true, bool bSilent = false);

	/** pause path following
	*  @param RequestID - request to pause, 0 = current */
	virtual void PauseMove(uint32 RequestID = UPathFollowingComponent::CurrentMoveRequestMetaId, bool bResetVelocity = true);

	/** resume path following
	*  @param RequestID - request to resume, 0 = current */
	virtual void ResumeMove(uint32 RequestID = UPathFollowingComponent::CurrentMoveRequestMetaId);

	/** notify about finished movement */
	virtual void OnPathFinished(EPathFollowingResult::Type Result);

	/** notify about finishing move along current path segment */
	virtual void OnSegmentFinished();

	/** set associated movement component */
	virtual void SetMovementComponent(class UNavMovementComponent* MoveComp);

	/** get current focal point of movement */
	virtual FVector GetMoveFocus(bool bAllowStrafe) const;

	/** simple test for stationary agent (used as early finish condition), check if reached given point
	 *  @param TestPoint - point to test
	 *  @param AcceptanceRadius - allowed 2D distance
	 *  @param bExactSpot - false: increase AcceptanceRadius with agent's radius
	 */
	bool HasReached(const FVector& TestPoint, float AcceptanceRadius = UPathFollowingComponent::DefaultAcceptanceRadius, bool bExactSpot = false) const;

	/** simple test for stationary agent (used as early finish condition), check if reached given goal
	 *  @param TestGoal - actor to test
	 *  @param AcceptanceRadius - allowed 2D distance
	 *  @param bExactSpot - false: increase AcceptanceRadius with agent's radius
	 */
	bool HasReached(const AActor* TestGoal, float AcceptanceRadius = UPathFollowingComponent::DefaultAcceptanceRadius, bool bExactSpot = false) const;

	/** update state of block detection */
	void SetBlockDetectionState(bool bEnable);

	/** @returns state of block detection */
	bool IsBlockDetectionActive() const { return bUseBlockDetection; }

	/** set block detection params */
	void SetBlockDetection(float DistanceThreshold, float Interval, int32 NumSamples);

	/** set threshold for precise reach tests in intermediate goals (minimal test radius)  */
	void SetPreciseReachThreshold(float AgentRadiusMultiplier, float AgentHeightMultiplier);

	/** set status of last requested move */
	void SetLastMoveAtGoal(bool bFinishedAtGoal);

	/** @returns estimated cost of unprocessed path segments
	 *	@NOTE 0 means, that component is following final path segment or doesn't move */
	float GetRemainingPathCost() const;
	
	/** Returns current location on navigation data */
	FNavLocation GetCurrentNavLocation() const;

	FORCEINLINE EPathFollowingStatus::Type GetStatus() const { return Status; }
	FORCEINLINE float GetAcceptanceRadius() const { return AcceptanceRadius; }
	FORCEINLINE AActor* GetMoveGoal() const { return DestinationActor.Get(); }
	FORCEINLINE bool HasPartialPath() const { return Path.IsValid() && Path->IsPartial(); }
	FORCEINLINE bool DidMoveReachGoal() const { return bLastMoveReachedGoal && (Status == EPathFollowingStatus::Idle); }

	FORCEINLINE uint32 GetCurrentRequestId() const { return CurrentRequestId; }
	FORCEINLINE uint32 GetNextPathIndex() const { return MoveSegmentStartIndex + 1; }
	FORCEINLINE FVector GetCurrentTargetLocation() const { return *CurrentDestination; }
	FORCEINLINE FVector GetCurrentDirection() const { return MoveSegmentDirection; }
	FORCEINLINE FBasedPosition GetCurrentTargetLocationBased() const { return CurrentDestination; }

	/** will be deprecated soon, please use AIController.GetMoveStatus instead! */
	UFUNCTION(BlueprintCallable, Category="AI|Components|PathFollowing")
	EPathFollowingAction::Type GetPathActionType() const;

	/** will be deprecated soon, please use AIController.GetImmediateMoveDestination instead! */
	UFUNCTION(BlueprintCallable, Category="AI|Components|PathFollowing")
	FVector GetPathDestination() const;

	/** readable name of current status */
	FString GetStatusDesc() const;
	/** readable name of result enum */
	FString GetResultDesc(EPathFollowingResult::Type Result) const;

	virtual void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos) const;
#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

	/** called when moving agent collides with another actor */
	UFUNCTION()
	virtual void OnActorBump(class AActor* SelfActor, class AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	/** Called when movement is blocked by a collision with another actor.  */
	virtual void OnMoveBlockedBy(const FHitResult& BlockingImpact) {}

	// IAIResourceInterface begin
	virtual void LockResource(EAILockSource::Type LockSource) OVERRIDE;
	virtual void ClearResourceLock(EAILockSource::Type LockSource) OVERRIDE;
	virtual void ForceUnlockResource() OVERRIDE;
	virtual bool IsResourceLocked() const OVERRIDE;
	// IAIResourceInterface end

protected:

	/** associated movement component */
	UPROPERTY(transient)
	class UNavMovementComponent* MovementComp;

	/** associated navigation component */
	UPROPERTY(transient)
	class UNavigationComponent* NavComp;

	/** smart link being currently traversed */
	UPROPERTY(transient)
	class USmartNavLinkComponent* CurrentSmartLink;

	/** navigation data for agent described in movement component */
	UPROPERTY(transient)
	ANavigationData* MyNavData;

	/** current status */
	TEnumAsByte<EPathFollowingStatus::Type> Status;

	/** requested path */
	FNavPathSharedPtr Path;

	/** value based on navigation agent's properties that's used for AcceptanceRadius when DefaultAcceptanceRadius is requested */
	float MyDefaultAcceptanceRadius;

	/** min distance to destination to consider request successful */
	float AcceptanceRadius;

	/** part of agent radius used as min acceptance radius */
	float MinAgentRadiusPct;

	/** part of agent height used as min acceptable height difference */
	float MinAgentHeightPct;

	/** game specific data */
	FCustomMoveSharedPtr GameData;

	/** current request observer */
	FRequestCompletedSignature OnRequestFinished;

	/** destination actor. Use SetDestinationActor to set this */
	TWeakObjectPtr<AActor> DestinationActor;

	/** cached DestinationActor cast to INavAgentInterface. Use SetDestinationActor to set this */
	const INavAgentInterface* DestinationAgent;

	/** destination for current path segment */
	FBasedPosition CurrentDestination;

	/** relative offset from goal actor's location to end of path */
	FVector MoveOffset;

	/** agent location when movement was paused */
	FVector LocationWhenPaused;

	/** increase acceptance radius with agent's radius */
	uint32 bStopOnOverlap : 1;

	/** if set, movement block detection will be used */
	uint32 bUseBlockDetection : 1;

	/** set when agent collides with goal actor */
	uint32 bCollidedWithGoal : 1;

	/** set when last move request was finished at goal */
	uint32 bLastMoveReachedGoal : 1;

	/** detect blocked movement when distance between center of location samples and furthest one (centroid radius) is below threshold */
	float BlockDetectionDistance;

	/** interval for collecting location samples */
	float BlockDetectionInterval;

	/** number of samples required for block detection */
	int32 BlockDetectionSampleCount;

	/** reset path following data */
	virtual void Reset();

	/** should verify if agent if still on path ater movement has been resumed? */
	virtual bool ShouldCheckPathOnResume() const;

	/** sets variables related to current move segment */
	virtual void SetMoveSegment(uint32 SegmentStartIndex);
	
	/** follow current path segment */
	virtual void FollowPathSegment(float DeltaTime);

	/** check state of path following, update move segment if needed */
	virtual void UpdatePathSegment();

	/** update blocked movement detection, @returns true if new sample was added */
	bool UpdateBlockDetection();

	/** check if move is completed */
	bool HasReachedDestination(const FVector& CurrentLocation) const;

	/** check if segment is completed */
	bool HasReachedCurrentTarget(const FVector& CurrentLocation) const;

	/** check if moving agent has reached goal defined by cylinder */
	bool HasReachedInternal(const FVector& GoalLocation, float GoalRadius, float GoalHalfHeight, const FVector& AgentLocation, float RadiusThreshold, bool bUseAgentRadius) const;

	/** check if agent is on path */
	bool IsOnPath() const;

	/** check if movement is blocked */
	bool IsBlocked() const;

	/** Checks if this PathFollowingComponent is already on path, and
	 *	if so determines index of next path point
	 *	@return what PathFollowingComponent thinks should be next path point. INDEX_NONE if given path is invalid
	 *	@note this function does not set MoveSegmentEndIndex */
	int32 DetermineStartingPathPoint(const FNavigationPath* ConsideredPath) const;

	/** switch to next segment on path */
	FORCEINLINE void SetNextMoveSegment() { SetMoveSegment(GetNextPathIndex()); }

	FORCEINLINE static uint32 GetNextRequestId() { return NextRequestId++; }
	FORCEINLINE void StoreRequestId() { CurrentRequestId = UPathFollowingComponent::GetNextRequestId(); }

	void SetDestinationActor(const AActor* InDestinationActor);

	/** check if movement component is valid or tries to grab one from owner 
	 *	@param bForce results in looking for owner's movement component even if pointer to one is already cached */
	bool UpdateMovementComponent(bool bForce = false);

	/** clears Block Detection stored data effectively resetting the mechanism */
	void ResetBlockDetectionData();

	/** force creating new location sample for block detection */
	void ForceBlockDetectionUpdate();

	/** set move focus in AI owner */
	void UpdateMoveFocus();

private:

	/** index of path point being current move target */
	uint32 MoveSegmentStartIndex;

	/** reference of node at segment start */
	NavNodeRef MoveSegmentStartRef;

	/** reference of node at segment end */
	NavNodeRef MoveSegmentEndRef;

	/** direction of current move segment */
	FVector MoveSegmentDirection;

	/** timestamp of last location sample */
	float LastSampleTime;

	/** index of next location sample in array */
	int32 NextSampleIdx;

	/** location samples for stuck detection */
	TArray<FBasedPosition> LocationSamples;

	/** used for debugging purposes to be able to identify which logged information
	 *	results from which request, if there was multiple ones during one frame */
	static uint32 NextRequestId;
	uint32 CurrentRequestId;

	/** Current location on navigation data.  Lazy-updated, so read this via GetCurrentNavLocation(). 
	 *	Since it makes conceptual sense for GetCurrentNavLocation() to be const but we may 
	 *	need to update the cached value, CurrentNavLocation is mutable. */
	mutable FNavLocation CurrentNavLocation;

	/** used to keep track of which subsystem requested this AI resource be locked */
	FAIResourceLock ResourceLock;

	/** empty delegate for RequestMove */
	static FRequestCompletedSignature UnboundRequestDelegate;

public:
	/** special float constant to symbolize "use default value". This does not contain 
	 *	value to be used, it's used to detect the fact that it's requested, and 
	 *	appropriate value from querier/doer will be pulled */
	static const float DefaultAcceptanceRadius;

	/** value to be passed as move request ID when such ID is expected and user means "current move" */
	static const uint32 CurrentMoveRequestMetaId;
};
