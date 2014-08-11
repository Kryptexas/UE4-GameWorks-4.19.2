// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/NavigationModifier.h"
#include "AI/Navigation/NavModifierComponent.h"
#include "AI/Navigation/NavAreas/NavArea_Null.h"

UNavModifierComponent::UNavModifierComponent(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	AreaClass = UNavArea_Null::StaticClass();
	Bounds = FNavigationSystem::InvalidBoundingBox;
}

void UNavModifierComponent::OnOwnerRegistered()
{
	if (AActor* MyOwner = GetOwner())
	{
		MyOwner->UpdateNavigationRelevancy();
	}
}

void UNavModifierComponent::OnOwnerUnregistered()
{
	if (AActor* MyOwner = GetOwner())
	{
		MyOwner->UpdateNavigationRelevancy();
	}
}

void UNavModifierComponent::OnRegister()
{
	Super::OnRegister();

	const AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		const float Radius = MyOwner->GetSimpleCollisionRadius();
		const float HalfHeght = MyOwner->GetSimpleCollisionHalfHeight();
		const FVector Loc = MyOwner->GetActorLocation();
		ObstacleExtent = FVector(Radius, Radius, HalfHeght);
		Bounds = FBox(Loc + ObstacleExtent, Loc - ObstacleExtent);
	}
}

void UNavModifierComponent::OnApplyModifiers(FCompositeNavModifier& Modifiers)
{
	const AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		const FVector Loc = MyOwner->GetActorLocation();
		Bounds = FBox(Loc + ObstacleExtent, Loc - ObstacleExtent);
		Modifiers.Add(FAreaNavModifier(Bounds, FTransform::Identity, AreaClass));
	}
}
