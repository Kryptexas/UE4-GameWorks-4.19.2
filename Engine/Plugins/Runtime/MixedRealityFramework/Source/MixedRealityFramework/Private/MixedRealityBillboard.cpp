// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityBillboard.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"

//------------------------------------------------------------------------------
UMixedRealityBillboard::UMixedRealityBillboard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

//------------------------------------------------------------------------------
void UMixedRealityBillboard::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (ensure(DepthMatchTarget != nullptr)) // should only be ticking when we have a depth target
	{
		FVector TargetWorldPos = DepthMatchTarget->GetComponentLocation();

		if (USceneComponent* Parent = GetAttachParent())
		{
			const FVector ParentFwdVec = Parent->GetForwardVector();
			const FVector ToTargetVect = TargetWorldPos - Parent->GetComponentLocation();
			const FVector DepthOffsetVec = ParentFwdVec * (/*DotProduct: */ParentFwdVec | ToTargetVect);

			TargetWorldPos = Parent->GetComponentLocation() + DepthOffsetVec;
		}
		// assume we're attached to the mixed reality capture component
		SetWorldLocation(TargetWorldPos);
	}
}

//------------------------------------------------------------------------------
void UMixedRealityBillboard::SetTargetPawn(const APawn* PlayerPawn)
{
	USceneComponent* NewDepthTarget = nullptr;
	if (PlayerPawn)
	{
		TArray<UCameraComponent*> PawnCameras;
		PlayerPawn->GetComponents<UCameraComponent>(PawnCameras);

		for (UCameraComponent* Camera : PawnCameras)
		{
			if (!NewDepthTarget)
			{
				NewDepthTarget = Camera;
			}
			else if (Camera->bLockToHmd)
			{
				NewDepthTarget = Camera;
				break;
			}
		}
		if (NewDepthTarget == nullptr)
		{
			NewDepthTarget = PlayerPawn->GetRootComponent();
		}
	}
	SetDepthTarget(NewDepthTarget);
}

//------------------------------------------------------------------------------
void UMixedRealityBillboard::SetDepthTarget(USceneComponent* NewDepthTarget)
{
	if (DepthMatchTarget)
	{
		RemoveTickPrerequisiteComponent(DepthMatchTarget);
	}

	DepthMatchTarget = NewDepthTarget;
	if (NewDepthTarget)
	{
		AddTickPrerequisiteComponent(NewDepthTarget);
	}

	SetComponentTickEnabled(NewDepthTarget != nullptr);
}
