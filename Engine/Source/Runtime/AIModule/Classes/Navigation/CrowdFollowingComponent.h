// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Navigation/CrowdAgentInterface.h"
#include "Navigation/PathFollowingComponent.h"
#include "CrowdFollowingComponent.generated.h"

namespace ECrowdAvoidanceQuality
{
	enum Type
	{
		Low,
		Medium,
		Good,
		High,
	};
}

UCLASS()
class AIMODULE_API UCrowdFollowingComponent : public UPathFollowingComponent, public ICrowdAgentInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FVector CrowdAgentMoveDirection;

	// ICrowdAgentInterface BEGIN
	virtual FVector GetCrowdAgentLocation() const override;
	virtual FVector GetCrowdAgentVelocity() const override;
	virtual void GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const override;
	virtual float GetCrowdAgentMaxSpeed() const override;
	// ICrowdAgentInterface END

	// PathFollowingComponent BEGIN
	virtual void Initialize() override;
	virtual void Cleanup() override;
	virtual void AbortMove(const FString& Reason, FAIRequestID RequestID = FAIRequestID::CurrentRequest, bool bResetVelocity = true, bool bSilent = false) override;
	virtual void PauseMove(FAIRequestID RequestID = FAIRequestID::CurrentRequest, bool bResetVelocity = true) override;
	virtual void ResumeMove(FAIRequestID RequestID = FAIRequestID::CurrentRequest) override;
	virtual FVector GetMoveFocus(bool bAllowStrafe) const override;
	virtual void OnLanded() override;
	virtual void OnPathFinished(EPathFollowingResult::Type Result) override;
	virtual void OnPathUpdated() override;
	// PathFollowingComponent END

	/** update params in crowd manager */
	void UpdateCrowdAgentParams() const;

	/** pass agent velocity to movement component */
	virtual void ApplyCrowdAgentVelocity(const FVector& NewVelocity, const FVector& DestPathCorner, bool bTraversingLink);

	/** master switch for crowd steering & avoidance */
	void SuspendCrowdSteering(bool bSuspend);

	/** switch between crowd simulation and parent implementation (following path segments) */
	virtual void SetCrowdSimulation(bool bEnable);

	/** called when agent moved to next nav node (poly) */
	virtual void OnNavNodeChanged(NavNodeRef NewPolyRef, NavNodeRef PrevPolyRef, int32 CorridorSize);

	void SetCrowdAnticipateTurns(bool bEnable, bool bUpdateAgent = true);
	void SetCrowdObstacleAvoidance(bool bEnable, bool bUpdateAgent = true);
	void SetCrowdSeparation(bool bEnable, bool bUpdateAgent = true);
	void SetCrowdOptimizeVisibility(bool bEnable, bool bUpdateAgent = true);
	void SetCrowdOptimizeTopology(bool bEnable, bool bUpdateAgent = true);
	void SetCrowdSeparationWeight(float Weight, bool bUpdateAgent = true);
	void SetCrowdCollisionQueryRange(float Range, bool bUpdateAgent = true);
	void SetCrowdPathOptimizationRange(float Range, bool bUpdateAgent = true);
	void SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Type Quality, bool bUpdateAgent = true);

	FORCEINLINE bool IsCrowdSimulationEnabled() const { return bEnableCrowdSimulation; }
	FORCEINLINE bool IsCrowdAnticipateTurnsEnabled() const { return bEnableAnticipateTurns && !bSuspendCrowdSimulation; }
	FORCEINLINE bool IsCrowdObstacleAvoidanceEnabled() const { return bEnableObstacleAvoidance && !bSuspendCrowdSimulation; }
	FORCEINLINE bool IsCrowdSeparationEnabled() const { return bEnableSeparation && !bSuspendCrowdSimulation; }
	FORCEINLINE bool IsCrowdOptimizeVisibilityEnabled() const { return bEnableOptimizeVisibility; /** don't check suspend here! */ }
	FORCEINLINE bool IsCrowdOptimizeTopologyEnabled() const { return bEnableOptimizeTopology && !bSuspendCrowdSimulation; }
	FORCEINLINE bool IsCrowdPathOffsetEnabled() const { return bEnablePathOffset; }
	FORCEINLINE float GetCrowdSeparationWeight() const { return SeparationWeight; }
	FORCEINLINE float GetCrowdCollisionQueryRange() const { return CollisionQueryRange; }
	FORCEINLINE float GetCrowdPathOptimizationRange() const { return PathOptimizationRange; }
	FORCEINLINE ECrowdAvoidanceQuality::Type GetCrowdAvoidanceQuality() const { return AvoidanceQuality; }

	virtual void GetDebugStringTokens(TArray<FString>& Tokens, TArray<EPathFollowingDebugTokens::Type>& Flags) const;
#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:

	UPROPERTY(transient)
	class UCharacterMovementComponent* CharacterMovement;

	/** if set, velocity will be updated even if agent is falling */
	uint32 bAffectFallingVelocity : 1;

	/** if set, move focus will match velocity direction */
	uint32 bRotateToVelocity : 1;

	/** if set, move velocity will be updated in every tick */
	uint32 bUpdateDirectMoveVelocity : 1;

	/** if set, agent will be simulated by crowd, otherwise it will act only as an obstacle */
	uint32 bEnableCrowdSimulation : 1;

	/** if set, avoidance and steering will be suspended (used for direct move requests) */
	uint32 bSuspendCrowdSimulation : 1;

	uint32 bEnableAnticipateTurns : 1;
	uint32 bEnableObstacleAvoidance : 1;
	uint32 bEnableSeparation : 1;
	uint32 bEnableOptimizeVisibility : 1;
	uint32 bEnableOptimizeTopology : 1;
	uint32 bEnablePathOffset : 1;

	/** if set, agent if moving on final path part, skip further updates (runtime flag) */
	uint32 bFinalPathPart : 1;

	/** if set, movement will be finished when velocity is opposite to path direction (runtime flag) */
	uint32 bCheckMovementAngle : 1;

	float SeparationWeight;
	float CollisionQueryRange;
	float PathOptimizationRange;

	/** start index of current path part */
	int32 PathStartIndex;

	/** last visited poly on path */
	int32 LastPathPolyIndex;

	TEnumAsByte<ECrowdAvoidanceQuality::Type> AvoidanceQuality;

	// PathFollowingComponent BEGIN
	virtual int32 DetermineStartingPathPoint(const FNavigationPath* ConsideredPath) const override;
	virtual void SetMoveSegment(int32 SegmentStartIndex) override;
	virtual void UpdatePathSegment() override;
	virtual void FollowPathSegment(float DeltaTime) override;
	virtual bool ShouldCheckPathOnResume() const override;
	virtual bool IsOnPath() const override;
	virtual bool UpdateMovementComponent(bool bForce) override;
	virtual void Reset() override;
	// PathFollowingComponent END

	void SwitchToNextPathPart();
	bool ShouldSwitchPathPart(int32 CorridorSize) const;
	bool HasMovedDuringPause() const;
	void UpdateCachedDirections(const FVector& NewVelocity, const FVector& NextPathCorner, bool bTraversingLink);

	friend class UCrowdManager;
};
