// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AITypes.h"
#include "AI/Navigation/NavigationTypes.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Controller.h"
#include "Actions/PawnActionsComponent.h"
#include "Perception/AIPerceptionListenerInterface.h"
#include "AIController.generated.h"

class APawn;
class UNavigationComponent;
class UPathFollowingComponent;
class UBrainComponent;
class UAIPerceptionComponent;
struct FBasedPosition;
class UPawnAction;
class UPawnActionsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAIMoveCompletedSignature, FAIRequestID, RequestID, EPathFollowingResult::Type, Result);

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
		TWeakObjectPtr<AActor> Actor;
		FBasedPosition Position;    

		FVector GetLocation() const 
		{
			const AActor* FocusActor = Actor.Get();
			return FocusActor ? FocusActor->GetActorLocation() : *Position;
		}
	};

	FFocusKnowledge() 
	{
		Priorities.Reserve(6);
	}
	TArray<struct FFocusItem> Priorities;
};

//=============================================================================
/**
 * AIController is the base class of controllers for AI-controlled Pawns.
 * 
 * Controllers are non-physical actors that can be attached to a pawn to control its actions.
 * AIControllers manage the artificial intelligence for the pawns they control.
 * In networked games, they only exist on the server.
 *
 * @see https://docs.unrealengine.com/latest/INT/Gameplay/Framework/Controller/
 */

UCLASS(BlueprintType, Blueprintable)
class AIMODULE_API AAIController : public AController, public IAIPerceptionListenerInterface
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

private_subobject:
	/** Component used for pathfinding and querying environment's navigation. */
	DEPRECATED_FORGAME(4.6, "NavComponent should not be accessed directly, please use GetNavComponent() function instead. NavComponent will soon be private and your code will not compile.")
	UPROPERTY()
	UNavigationComponent* NavComponent;

	/** Component used for moving along a path. */
	DEPRECATED_FORGAME(4.6, "PathFollowingComponent should not be accessed directly, please use GetPathFollowingComponent() function instead. PathFollowingComponent will soon be private and your code will not compile.")
	UPROPERTY()
	UPathFollowingComponent* PathFollowingComponent;

public:

	/** Component responsible for behaviors. */
	UPROPERTY(BlueprintReadWrite, Category = AI)
	UBrainComponent* BrainComponent;

	UPROPERTY(BlueprintReadOnly, Category = AI)
	UAIPerceptionComponent* PerceptionComponent;

private_subobject:
	DEPRECATED_FORGAME(4.6, "ActionsComp should not be accessed directly, please use GetActionsComp() function instead. ActionsComp will soon be private and your code will not compile.")
	UPROPERTY(BlueprintReadOnly, Category = AI, meta = (AllowPrivateAccess = "true"))
	UPawnActionsComponent* ActionsComp;

public:
	/** Event called when PossessedPawn is possesed by this controller. */
	UFUNCTION(BlueprintImplementableEvent, Category = "AI")
	void OnPossess(APawn* PossessedPawn);

	/** Makes AI go toward specified Goal actor (destination will be continuously updated)
	 *  @param AcceptanceRadius - finish move if pawn gets close enough
	 *  @param bStopOnOverlap - add pawn's radius to AcceptanceRadius
	 *  @param bUsePathfinding - use navigation data to calculate path (otherwise it will go in straight line)
	 *  @param bCanStrafe - set focus related flag: bAllowStrafe
	 *	@note AcceptanceRadius has default value or -1 due to Header Parser not being able to recognize UPathFollowingComponent::DefaultAcceptanceRadius
	 */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	EPathFollowingRequestResult::Type MoveToActor(AActor* Goal, float AcceptanceRadius = -1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bCanStrafe = true, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);
	// @todo: above should be: 
	// EPathFollowingRequestResult::Type MoveToActor(AActor* Goal, float AcceptanceRadius = /*UPathFollowingComponent::DefaultAcceptanceRadius==*/-1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bCanStrafe = true);
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
	virtual FAIRequestID RequestMove(FNavPathSharedPtr Path, AActor* Goal = NULL, float AcceptanceRadius = UPathFollowingComponent::DefaultAcceptanceRadius, bool bStopOnOverlap = true, FCustomMoveSharedPtr CustomData = NULL);

	/** if AI is currently moving due to request given by RequestToPause, then the move will be paused */
	bool PauseMove(FAIRequestID RequestToPause);

	/** resumes last AI-performed, paused request provided it's ID was equivalent to RequestToResume */
	bool ResumeMove(FAIRequestID RequestToResume);

	/** Aborts the move the controller is currently performing */
	virtual void StopMovement() override;

	/** Called on completing current movement request */
	virtual void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	/** Returns the Move Request ID for the current move */
	FORCEINLINE FAIRequestID GetCurrentMoveRequestID() const { return GetPathFollowingComponent() ? GetPathFollowingComponent()->GetCurrentRequestId() : FAIRequestID::InvalidRequest; }

	/** Blueprint notification that we've completed the current movement request */
	UPROPERTY(BlueprintAssignable, meta=(DisplayName="MoveCompleted"))
	FAIMoveCompletedSignature ReceiveMoveCompleted;

	/** @returns path to actor Goal (can be incomplete if async pathfinding is used) */
	FNavPathSharedPtr FindPath(AActor* Goal, bool bUsePathfinding = true, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

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
	virtual void InitNavigationControl(UNavigationComponent*& PathFindingComp, UPathFollowingComponent*& PathFollowingComp) override;

	/** Starts executing behavior tree. */
	UFUNCTION(BlueprintCallable, Category="AI")
	virtual bool RunBehaviorTree(class UBehaviorTree* BTAsset);

	/** makes AI use specified BB asset */
	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual bool UseBlackboard(class UBlackboardData* BlackboardAsset);

	/** Retrieve the final position that controller should be looking at. */
	UFUNCTION(BlueprintCallable, Category="AI")
	virtual FVector GetFocalPoint() const;

	FVector GetFocalPoint(EAIFocusPriority::Type Priority) const;
	FORCEINLINE FFocusKnowledge::FFocusItem GetFocusItem(EAIFocusPriority::Type Priority) const { return FocusInformation.Priorities.IsValidIndex(Priority) ? FocusInformation.Priorities[Priority] : FFocusKnowledge::FFocusItem(); }
	
	/** Set FocalPoint as absolute position or offset from base. */
	UFUNCTION(BlueprintCallable, Category="AI", meta=(FriendlyName="SetFocalPoint"))
	void K2_SetFocalPoint(FVector FP, bool bOffsetFromBase = false);

	/** Set Focus for actor, will set FocalPoint as a result. */
	UFUNCTION(BlueprintCallable, Category="AI", meta=(FriendlyName="SetFocus"))
	void K2_SetFocus(AActor* NewFocus);

	/** Get the focused actor. */
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
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void PostRegisterAllComponents() override;
	// End AActor Interface

	// Begin AController Interface
	virtual void Possess(class APawn* InPawn) override;
	virtual void UnPossess() override;
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(struct FVisualLogEntry* Snapshot) const override;
#endif

	virtual void Reset() override;
	virtual void GetPlayerViewPoint(FVector& out_Location, FRotator& out_Rotation) const override;

	/**
	 * Checks line to center and top of other actor
	 * @param Other is the actor whose visibility is being checked.
	 * @param ViewPoint is eye position visibility is being checked from.  If vect(0,0,0) passed in, uses current viewtarget's eye position.
	 * @param bAlternateChecks used only in AIController implementation
	 * @return true if controller's pawn can see Other actor.
	 */
	virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const override;
	// End AController Interface

	/** Notifies AIController of changes in given actors' perception */
	virtual void ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	/** Update direction AI is looking based on FocalPoint */
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true);

	/** Set FocalPoint for given priority as absolute position or offset from base. */
	virtual void SetFocalPoint(FVector FP, bool bOffsetFromBase=false, uint8 InPriority=EAIFocusPriority::Gameplay);

	/* Set Focus actor for given priority, will set FocalPoint as a result. */
	virtual void SetFocus(AActor* NewFocus, EAIFocusPriority::Type InPriority = EAIFocusPriority::Gameplay);

	/** Clears Focus for given priority, will also clear FocalPoint as a result
	 *	@param InPriority focus priority to clear. If you don't know what to use you probably mean EAIFocusPriority::Gameplay*/
	virtual void ClearFocus(EAIFocusPriority::Type InPriority);

	//----------------------------------------------------------------------//
	// IAIPerceptionListenerInterface
	//----------------------------------------------------------------------//
	virtual class UAIPerceptionComponent* GetPerceptionComponent() override { return PerceptionComponent; }

	//----------------------------------------------------------------------//
	// Actions
	//----------------------------------------------------------------------//
	bool PerformAction(UPawnAction& Action, EAIRequestPriority::Type Priority, UObject* const Instigator = NULL);

	//----------------------------------------------------------------------//
	// debug/dev-time 
	//----------------------------------------------------------------------//
	virtual FString GetDebugIcon() const;
	
	// Cheat/debugging functions
	static void ToggleAIIgnorePlayers() { bAIIgnorePlayers = !bAIIgnorePlayers; }
	static bool AreAIIgnoringPlayers() { return bAIIgnorePlayers; }

	/** If true, AI controllers will ignore players. */
	static bool bAIIgnorePlayers;

public:
	/** Returns NavComponent subobject **/
	UNavigationComponent* GetNavComponent() const;
	/** Returns PathFollowingComponent subobject **/
	UPathFollowingComponent* GetPathFollowingComponent() const;
	/** Returns ActionsComp subobject **/
	UPawnActionsComponent* GetActionsComp() const;
};


