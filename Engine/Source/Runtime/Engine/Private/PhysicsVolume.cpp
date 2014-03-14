// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

APhysicsVolume::APhysicsVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = true;
	static FName CollisionProfileName(TEXT("OverlapAllDynamic"));
	BrushComponent->SetCollisionProfileName(CollisionProfileName);

	FluidFriction = 0.3f;
	TerminalVelocity = 4000.0f;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 0.1f;
	bReplicateMovement = false;
}

#if WITH_EDITOR
void APhysicsVolume::LoadedFromAnotherClass(const FName& OldClassName)
{
	Super::LoadedFromAnotherClass(OldClassName);

	if(GetLinkerUE4Version() < VER_UE4_REMOVE_DYNAMIC_VOLUME_CLASSES)
	{
		static FName DynamicPhysicsVolume_NAME(TEXT("DynamicPhysicsVolume"));

		if(OldClassName == DynamicPhysicsVolume_NAME)
		{
			BrushComponent->Mobility = EComponentMobility::Movable;
		}
	}
}
#endif

bool APhysicsVolume::IsOverlapInVolume(const class USceneComponent& TestComponent) const
{
	bool bInsideVolume = true;
	if (!bPhysicsOnContact)
	{
		FVector ClosestPoint(0.f);
		UPrimitiveComponent* RootPrimitive = GetRootPrimitiveComponent();
		const float DistToCollision = RootPrimitive ? RootPrimitive->GetDistanceToCollision(TestComponent.GetComponentLocation(), ClosestPoint) : 0.f;
		bInsideVolume = (DistToCollision == 0.f);
	}

	return bInsideVolume;
}

float APhysicsVolume::GetGravityZ() const
{
	return GetWorld()->GetGravityZ();
}

void APhysicsVolume::ActorEnteredVolume(AActor* Other) {}

void APhysicsVolume::ActorLeavingVolume(AActor* Other) {}

