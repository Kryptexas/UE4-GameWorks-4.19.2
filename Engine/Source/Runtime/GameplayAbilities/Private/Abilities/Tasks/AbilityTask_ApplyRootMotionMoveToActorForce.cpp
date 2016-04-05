// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToActorForce.h"
#include "Net/UnrealNetwork.h"

UAbilityTask_ApplyRootMotionMoveToActorForce::UAbilityTask_ApplyRootMotionMoveToActorForce(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bTickingTask = true;
	bSimulatedTask = true;
	bSetNewMovementMode = false;
	NewMovementMode = EMovementMode::MOVE_Walking;
	PreviousMovementMode = EMovementMode::MOVE_None;
	RootMotionSourceID = (uint16)ERootMotionSourceID::Invalid;
	bIsFinished = false;
	MovementComponent = nullptr;
	bRestrictSpeedToExpected = false;
	PathOffsetCurve = nullptr;
	VelocityOnFinishMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	SetVelocityOnFinish = FVector::ZeroVector;
}

UAbilityTask_ApplyRootMotionMoveToActorForce* UAbilityTask_ApplyRootMotionMoveToActorForce::ApplyRootMotionMoveToActorForce(UObject* WorldContextObject, FName TaskInstanceName, FVector TargetLocation, float Duration, bool bSetNewMovementMode, EMovementMode MovementMode, bool bRestrictSpeedToExpected, UCurveVector* PathOffsetCurve, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish)
{
	auto MyTask = NewAbilityTask<UAbilityTask_ApplyRootMotionMoveToActorForce>(WorldContextObject, TaskInstanceName);

	MyTask->ForceName = TaskInstanceName;
	MyTask->TargetLocation = TargetLocation;
	MyTask->Duration = FMath::Max(Duration, KINDA_SMALL_NUMBER); // Avoid negative or divide-by-zero cases
	MyTask->bSetNewMovementMode = bSetNewMovementMode;
	MyTask->NewMovementMode = MovementMode;
	MyTask->bRestrictSpeedToExpected = bRestrictSpeedToExpected;
	MyTask->PathOffsetCurve = PathOffsetCurve;
	MyTask->VelocityOnFinishMode = VelocityOnFinishMode;
	MyTask->SetVelocityOnFinish = SetVelocityOnFinish;
	if (MyTask->GetAvatarActor() != nullptr)
	{
		MyTask->StartLocation = MyTask->GetAvatarActor()->GetActorLocation();
	}
	else
	{
		checkf(false, TEXT("UAbilityTask_ApplyRootMotionMoveToActorForce called without valid avatar actor to get start location from."));
		MyTask->StartLocation = TargetLocation;
	}
	MyTask->SharedInitAndApply();

	return MyTask;
}

void UAbilityTask_ApplyRootMotionMoveToActorForce::Activate()
{
}

void UAbilityTask_ApplyRootMotionMoveToActorForce::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	Super::InitSimulatedTask(InGameplayTasksComponent);

	SharedInitAndApply();
}

void UAbilityTask_ApplyRootMotionMoveToActorForce::SharedInitAndApply()
{
	if (AbilitySystemComponent->AbilityActorInfo->MovementComponent.IsValid())
	{
		MovementComponent = Cast<UCharacterMovementComponent>(AbilitySystemComponent->AbilityActorInfo->MovementComponent.Get());
		StartTime = GetWorld()->GetTimeSeconds();
		EndTime = StartTime + Duration;

		if (MovementComponent)
		{
			if (bSetNewMovementMode)
			{
				PreviousMovementMode = MovementComponent->MovementMode;
				MovementComponent->SetMovementMode(NewMovementMode);
			}

			ForceName = ForceName.IsNone() ? FName("AbilityTaskApplyRootMotionMoveToActorForce") : ForceName;
			FRootMotionSource_MoveToForce* MoveToActorForce = new FRootMotionSource_MoveToForce();
			MoveToActorForce->InstanceName = ForceName;
			MoveToActorForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			MoveToActorForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
			MoveToActorForce->Priority = 1000;
			MoveToActorForce->TargetLocation = TargetLocation;
			MoveToActorForce->StartLocation = StartLocation;
			MoveToActorForce->Duration = Duration;
			MoveToActorForce->bRestrictSpeedToExpected = bRestrictSpeedToExpected;
			MoveToActorForce->PathOffsetCurve = PathOffsetCurve;
			RootMotionSourceID = MovementComponent->ApplyRootMotionSource(MoveToActorForce);

			if (Ability)
			{
				Ability->SetMovementSyncPoint(ForceName);
			}
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_ApplyRootMotionMoveToActorForce called in Ability %s with null MovementComponent; Task Instance Name %s."), 
			Ability ? *Ability->GetName() : TEXT("NULL"), 
			*InstanceName.ToString());
	}
}

void UAbilityTask_ApplyRootMotionMoveToActorForce::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	AActor* MyActor = GetAvatarActor();
	if (MyActor)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		const bool bTimedOut = CurrentTime >= EndTime;

		const float ReachedDestinationDistanceSqr = 50.f * 50.f;
		const bool bReachedDestination = FVector::DistSquared(TargetLocation, MyActor->GetActorLocation()) < ReachedDestinationDistanceSqr;

		if (bTimedOut)
		{
			// Task has finished
			bIsFinished = true;
			if (!bIsSimulating)
			{
				MyActor->ForceNetUpdate();
				if (bReachedDestination)
				{
					OnTimedOutAndDestinationReached.Broadcast();
				}
				else
				{
					OnTimedOut.Broadcast();
				}
				EndTask();
			}
		}
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UAbilityTask_ApplyRootMotionMoveToActorForce::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, ForceName);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, StartLocation);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, TargetLocation);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, Duration);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, bSetNewMovementMode);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, NewMovementMode);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, bRestrictSpeedToExpected);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, PathOffsetCurve);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, VelocityOnFinishMode);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionMoveToActorForce, SetVelocityOnFinish);
}

void UAbilityTask_ApplyRootMotionMoveToActorForce::PreDestroyFromReplication()
{
	bIsFinished = true;
	EndTask();
}

void UAbilityTask_ApplyRootMotionMoveToActorForce::OnDestroy(bool AbilityIsEnding)
{
	if (MovementComponent)
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);

		if (bSetNewMovementMode)
		{
			MovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
		}

		if (VelocityOnFinishMode == ERootMotionFinishVelocityMode::SetVelocity)
		{
			FVector EndVelocity;
			ACharacter* Character = MovementComponent->GetCharacterOwner();
			if (Character)
			{
				EndVelocity = Character->GetActorRotation().RotateVector(SetVelocityOnFinish);
			}
			else
			{
				EndVelocity = SetVelocityOnFinish;
			}

			// When we mean to SetVelocity when finishing a MoveTo, we apply a short-duration low-priority
			// root motion velocity override. This ensures that the velocity we set is replicated properly
			// and takes effect.
			{
				const FName OnFinishForceName = FName("AbilityTaskApplyRootMotionMoveToActorForce_EndForce");
				FRootMotionSource_ConstantForce* ConstantForce = new FRootMotionSource_ConstantForce();
				ConstantForce->InstanceName = OnFinishForceName;
				ConstantForce->AccumulateMode = ERootMotionAccumulateMode::Override;
				ConstantForce->Priority = 1; // Low priority so any other override root motion sources stomp it
				ConstantForce->Force = EndVelocity;
				ConstantForce->Duration = 0.001f;
				MovementComponent->ApplyRootMotionSource(ConstantForce);

				if (Ability)
				{
					Ability->SetMovementSyncPoint(OnFinishForceName);
				}
			}
		}
	}

	Super::OnDestroy(AbilityIsEnding);
}
