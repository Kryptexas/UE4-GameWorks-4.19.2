// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// AIController, the base class of AI.
//
// Controllers are non-physical actors that can be attached to a pawn to control
// its actions.  AIControllers implement the artificial intelligence for the pawns they control.
//
//=============================================================================

#pragma once
#include "AI/AITypes.h"
#include "AI/Navigation/NavigationTypes.h"
#include "AI/Navigation/NavigationSystem.h"
#include "AI/Navigation/PathFollowingComponent.h"
#include "AIController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FAIMoveCompletedSignature, int32, RequestID, EPathFollowingResult::Type, Result );

namespace FAISystem
{
	static const FVector InvalidLocation(FLT_MAX);

	FORCEINLINE bool IsValidLocation(const FVector& TestLocation)
	{
		return TestLocation != InvalidLocation;
	}
}

namespace EAIRequestPriority
{
	enum Type
	{
		SoftScript, // actions requested by Level Designers by placing AI-hinting elements on the map
		Logic,	// actions AI wants to do due to its internal logic				
		HardScript, // actions LDs really want AI to perform
		Reaction,	// actions being result of game-world mechanics, like hit reactions, death, falling, etc. In general things not depending on what AI's thinking
		Ultimate,	// ultimate priority, to be used with caution, makes AI perform given action regardless of anything else (for example disabled reactions)
	};

	static const int32 Lowest = Logic;
	static const int32 MAX = Ultimate + 1;
}

namespace EAIFocusPriority
{
	typedef uint8 Type;

	const Type Default = 0;
	const Type Move = 1;
	const Type Gameplay = 2;

	const Type LastFocusPriority = Gameplay;
}

struct FFocusKnowledge
{
	struct FFocusItem
	{
		TWeakObjectPtr<class AActor> Actor;
		struct FBasedPosition Position;    
	};

	FFocusKnowledge() 
	{
		Priorities.Reserve(6);
	}
	TArray<struct FFocusItem> Priorities;
};

UCLASS(BlueprintType, Blueprintable, dependson=(UPathFollowingComponent))
class ENGINE_API AAIController : public AController
{
	GENERATED_UCLASS_BODY()

protected:
	FFocusKnowledge	FocusInformation;

public:
	/** used for alternating LineOfSight traces */
	UPROPERTY()
	mutable uint32 bLOSflag:1;

	/** Skip extra line of sight traces to extremities of target being checked. */
	UPROPERTY()
	uint32 bSkipExtraLOSChecks:1;

	/** Is strafing allowed during movement? */
	UPROPERTY()
	uint32 bAllowStrafe:1;	

	/** Specifies if this AI wants its own PlayerState. */
	UPROPERTY()
	uint32 bWantsPlayerState:1;

	/** Component used for pathfinding and querying environment's navigation. */
	UPROPERTY()
	TSubobjectPtr<class UNavigationComponent> NavComponent;

	/** Component used for moving along a path. */
	UPROPERTY()
	TSubobjectPtr<class UPathFollowingComponent> PathFollowingComponent;

	/** Component responsible for behaviors. */
	UPROPERTY()
	class UBrainComponent* BrainComponent;

	/** Makes AI go toward specified Goal actor (destination will be continuously updated)
	 *  @param AcceptanceRadius - finish move if pawn gets close enough
	 *  @param bStopOnOverlap - add pawn's radius to AcceptanceRadius
	 *  @param bUsePathfinding - use navigation data to calculate path (otherwise it will go in straight line)
	 *  @param bCanStrafe - set focus related flag: bAllowStrafe
	 *	@note AcceptanceRadius has default value or -1 due to Header Parser not being able to recognize UPathFollowingComponent::DefaultAcceptanceRadius
	 */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	EPathFollowingRequestResult::Type MoveToActor(class AActor* Goal, float AcceptanceRadius = -1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bCanStrafe = true, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);
	// @todo: above should be: 
	// EPathFollowingRequestResult::Type MoveToActor(class AActor* Goal, float AcceptanceRadius = /*UPathFollowingComponent::DefaultAcceptanceRadius==*/-1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bCanStrafe = true);
	// but parser doesn't like this (it's a bug, when fixed this will be changed)

	/** Makes AI go toward specified Dest location
	 *  @param AcceptanceRadius - finish move if pawn gets close enough
	 *  @param bStopOnOverlap - add pawn's radius to AcceptanceRadius
	 *  @param bUsePathfinding - use navigation data to calculate path (otherwise it will go in straight line)
	 *  @param bCanStrafe - set focus related flag: bAllowStrafe
	 *	@note AcceptanceRadius has default value or -1 due to Header Parser not being able to recognize UPathFollowingComponent::DefaultAcceptanceRadius
	 */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	EPathFollowingRequestResult::Type MoveToLocation(const FVector& Dest, float AcceptanceRadius = -1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bProjectDestinationToNavigation = false, bool bCanStrafe = true, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);
	// @todo: above should be: 
	// EPathFollowingRequestResult::Type MoveToLocation(const FVector& Dest, float AcceptanceRadius = /*UPathFollowingComponent::DefaultAcceptanceRadius==*/-1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bCanStrafe = true);
	// but parser doesn't like this (it's a bug, when fixed this will be changed)

	/** Handle move requests
	 *  @param Path - path to follow (can use incomplete)
	 *  @param Goal - goal actor if following actor
	 *  @param AcceptanceRadius - finish move if pawn gets close enough
	 *  @param CustomData - game specific data, that will be passed to pawn's movement component
	 *  @return RequestID, or 0 when failed
	 */
	virtual uint32 RequestMove(FNavPathSharedPtr Path, class AActor* Goal = NULL, float AcceptanceRadius = UPathFollowingComponent::DefaultAcceptanceRadius, bool bStopOnOverlap = true, FCustomMoveSharedPtr CustomData = NULL);

	/** Aborts the move AI is currently performing */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	virtual void StopMovement();

	/** Called on completing current movement request */
	virtual void OnMoveCompleted(uint32 RequestID, EPathFollowingResult::Type Result);

	/** Returns the Move Request ID for the current move */
	FORCEINLINE uint32 GetCurrentMoveRequestID() const { return PathFollowingComponent.IsValid() ? PathFollowingComponent->GetCurrentRequestId() : INVALID_MOVEREQUESTID; }

	/** Blueprint notification that we've completed the current movement request */
	UPROPERTY(BlueprintAssignable, meta=(FriendlyName="MoveCompleted"))
	FAIMoveCompletedSignature ReceiveMoveCompleted;

	/** @returns path to actor Goal (can be incomplete if async pathfinding is used) */
	FNavPathSharedPtr FindPath(class AActor* Goal, bool bUsePathfinding = true, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/** @returns path to point Dest (can be incomplete if async pathfinding is used) */
	FNavPathSharedPtr FindPath(const FVector& Dest, bool bUsePathfinding = true, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/** Returns status of path following */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	EPathFollowingStatus::Type GetMoveStatus() const;

	/** Returns true if the current PathFollowingComponent's path is partial (does not reach desired destination). */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	bool HasPartialPath() const;
	
	/** Returns position of current path segment's end. */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	FVector GetImmediateMoveDestination() const;

	/** Updates state of movement block detection. */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	void SetMoveBlockDetection(bool bEnable);

	/** Prepares path finding and path following components. */
	virtual void InitNavigationControl(UNavigationComponent*& PathFindingComp, UPathFollowingComponent*& PathFollowingComp) OVERRIDE;

	/** Starts executing behavior tree. */
	UFUNCTION(BlueprintCallable, Category="AI")
	virtual bool RunBehaviorTree(class UBehaviorTree* BTAsset);

	/** Retrieve the final position that controller should be looking at. */
	UFUNCTION(BlueprintCallable, Category="AI")
	virtual FVector GetFocalPoint();

	FORCEINLINE FVector GetFocalPoint(EAIFocusPriority::Type Priority) const {  return FocusInformation.Priorities.IsValidIndex(Priority) ? *FocusInformation.Priorities[Priority].Position : FAISystem::InvalidLocation; }

	/** Set FocalPoint as absolute position or offset from base. */
	UFUNCTION(BlueprintCallable, Category="AI", meta=(FriendlyName="SetFocalPoint"))
	void K2_SetFocalPoint(FVector FP, bool bOffsetFromBase = false);

	/* Set Focus for actor, will set FocalPoint as a result. */
	UFUNCTION(BlueprintCallable, Category="AI", meta=(FriendlyName="SetFocus"))
	void K2_SetFocus(AActor* NewFocus);

	UFUNCTION(BlueprintCallable, Category="AI")
	AActor* GetFocusActor() const;

	FORCEINLINE AActor* GetFocusActor(EAIFocusPriority::Type Priority) const {  return FocusInformation.Priorities.IsValidIndex(Priority) ? FocusInformation.Priorities[Priority].Actor.Get() : NULL; }

	/** Clears Focus, will also clear FocalPoint as a result */
	UFUNCTION(BlueprintCallable, Category="AI", meta=(FriendlyName="ClearFocus"))
	void K2_ClearFocus();


	/** 
	 * Computes a launch velocity vector to toss a projectile and hit the given destination.
	 * Performance note: Potentially expensive. Nonzero CollisionRadius and bOnlyTraceUp=false are the more expensive options.
	 * 
	 * @param OutTossVelocity - out param stuffed with the computed velocity to use
	 * @param Start - desired start point of arc
	 * @param End - desired end point of arc
	 * @param TossSpeed - Initial speed of the theoretical projectile. Assumed to only change due to gravity for the entire lifetime of the projectile
	 * @param CollisionSize (optional) - is the size of bounding box of the tossed actor (defaults to (0,0,0)
	 * @param bOnlyTraceUp  (optional) - when true collision checks verifying the arc will only be done along the upward portion of the arc
	 * @return - true if a valid arc was computed, false if no valid solution could be found
	 */
	bool SuggestTossVelocity(FVector& OutTossVelocity, FVector Start, FVector End, float TossSpeed, bool bPreferHighArc, float CollisionRadius=0, bool bOnlyTraceUp=false);

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) OVERRIDE;
	virtual void PostInitializeComponents() OVERRIDE;
	// End AActor Interface

	// Begin AController Interface
	virtual void Possess(class APawn* InPawn) OVERRIDE;
	virtual void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos) OVERRIDE;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(struct FVisLogEntry* Snapshot) const OVERRIDE;
#endif

	virtual void Reset() OVERRIDE;
	virtual void GetPlayerViewPoint(FVector& out_Location, FRotator& out_Rotation) const OVERRIDE;

	/**
	 * Checks line to center and top of other actor
	 * @param Other is the actor whose visibility is being checked.
	 * @param ViewPoint is eye position visibility is being checked from.  If vect(0,0,0) passed in, uses current viewtarget's eye position.
	 * @param bAlternateChecks used only in AIController implementation
	 * @return true if controller's pawn can see Other actor.
	 */
	virtual bool LineOfSightTo(const class AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) OVERRIDE;
	// End AController Interface

	/** Update direction AI is looking based on FocalPoint */
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true);

public:
	/** Set FocalPoint for given priority as absolute position or offset from base. */
	virtual void SetFocalPoint(FVector FP, bool bOffsetFromBase=false, uint8 InPriority=EAIFocusPriority::Gameplay);

	/* Set Focus actor for given priority, will set FocalPoint as a result. */
	virtual void SetFocus(AActor* NewFocus, uint8 InPriority=EAIFocusPriority::Gameplay);

	/** Clears Focus for given priority, will also clear FocalPoint as a result */
	virtual void ClearFocus(uint8 InPriority=EAIFocusPriority::Gameplay);

	virtual FString GetDebugIcon() const {return FString();}

	// Cheat/debugging functions
	static void ToggleAIIgnorePlayers() { bAIIgnorePlayers = !bAIIgnorePlayers; }
	static bool AreAIIgnoringPlayers() { return bAIIgnorePlayers; }

	/** If true, AI controllers will ignore players. */
	static bool bAIIgnorePlayers;
};


