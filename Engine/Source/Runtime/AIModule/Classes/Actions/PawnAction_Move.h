// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PawnAction_Move.generated.h"

namespace EPawnActionMoveMode
{
	enum Type
	{
		UsePathfinding,
		StraightLine,
	};
}

UCLASS()
class AIMODULE_API UPawnAction_Move : public UPawnAction
{
	GENERATED_UCLASS_BODY()
protected:
	UPROPERTY(Category = PawnAction, EditAnywhere, BlueprintReadWrite)
	class AActor* GoalActor;

	UPROPERTY(Category = PawnAction, EditAnywhere, BlueprintReadWrite)
	FVector GoalLocation;

	UPROPERTY(Category = PawnAction, EditAnywhere, meta = (ClampMin = "0.01"), BlueprintReadWrite)
	float AcceptableRadius;

	/** "None" will result in default filter being used */
	UPROPERTY(Category = PawnAction, EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UNavigationQueryFilter> FilterClass;

	UPROPERTY(Category = PawnAction, EditAnywhere, BlueprintReadWrite)
	uint32 bAllowStrafe : 1;
	
	/** if set to true (default) will make action succeed when the pawn's collision component overlaps with goal's collision component */
	UPROPERTY()
	uint32 bFinishOnOverlap : 1;

	/** if set, movement will use path finding */
	UPROPERTY()
	uint32 bUsePathfinding : 1;

	/** if set, GoalLocation will be projected on navigation before using  */
	UPROPERTY()
	uint32 bProjectGoalToNavigation : 1;

	/** if set, path to GoalActor will be updated with goal's movement */
	UPROPERTY()
	uint32 bUpdatePathToGoal : 1;

	/** if set, other actions with the same priority will be aborted when path is changed */
	UPROPERTY()
	uint32 bAbortChildActionOnPathChange : 1;

public:
	virtual void BeginDestroy() override;

	static UPawnAction_Move* CreateAction(UWorld& World, class AActor* GoalActor, EPawnActionMoveMode::Type Mode);
	static UPawnAction_Move* CreateAction(UWorld& World, const FVector& GoalLocation, EPawnActionMoveMode::Type Mode);

	static bool CheckAlreadyAtGoal(class AAIController* Controller, const FVector& TestLocation, float Radius);
	static bool CheckAlreadyAtGoal(class AAIController* Controller, const AActor* TestGoal, float Radius);

	virtual void HandleAIMessage(UBrainComponent*, const struct FAIMessage&) override;

	void SetPath(FNavPathSharedRef InPath);
	void OnPathUpdated(FNavigationPath* UpdatedPath, ENavPathEvent::Type Event);

	void SetAcceptableRadius(float NewAcceptableRadius) { AcceptableRadius = NewAcceptableRadius; }
	void SetFinishOnOverlap(bool bNewFinishOnOverlap) { bFinishOnOverlap = bNewFinishOnOverlap; }
	void EnableStrafing(bool bNewStrafing) { bAllowStrafe = bNewStrafing; }
	void EnablePathUpdateOnMoveGoalLocationChange(bool bEnable) { bUpdatePathToGoal = bEnable; }
	void EnableGoalLocationProjectionToNavigation(bool bEnable) { bProjectGoalToNavigation = bEnable; }
	void EnableChildAbortionOnPathUpdate(bool bEnable) { bAbortChildActionOnPathChange = bEnable; }
	void SetFilterClass(TSubclassOf<class UNavigationQueryFilter> NewFilterClass) { FilterClass = NewFilterClass; }

protected:
	/** currently followed path */
	FNavPathSharedPtr Path;

	FNavigationPath::FPathObserverDelegate::FDelegate PathObserver;
	
	void ClearPath();
	virtual bool Start() override;
	virtual bool Pause(const UPawnAction* PausedBy) override;
	virtual bool Resume() override;
	virtual void OnFinished(EPawnActionResult::Type WithResult) override;
	virtual EPawnActionAbortState::Type PerformAbort(EAIForceParam::Type ShouldForce) override;

	virtual EPathFollowingRequestResult::Type RequestMove(AAIController* Controller);
	
	bool PerformMoveAction();
	void DeferredPerformMoveAction();

	void TryToRepath();
	void ClearPendingRepath();
};
