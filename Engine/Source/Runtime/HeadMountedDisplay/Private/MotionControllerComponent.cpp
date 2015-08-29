// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
//
#include "HeadMountedDisplayPrivate.h"
#include "MotionControllerComponent.h"

UMotionControllerComponent::UMotionControllerComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	PlayerIndex = 0;
	Hand = EControllerHand::Left;
}

void UMotionControllerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bTracked = false;
	FVector Position = FVector::ZeroVector;
	FRotator Orientation = FRotator::ZeroRotator;

	if (PlayerIndex != INDEX_NONE)
	{
		for (auto MotionController : GEngine->MotionControllerDevices)
		{
			if ((MotionController != nullptr) && MotionController->GetControllerOrientationAndPosition(PlayerIndex, Hand, Orientation, Position))
			{
				SetRelativeLocationAndRotation(Position, Orientation);
				bTracked = true;
				break;
			}
		}
	}
}
