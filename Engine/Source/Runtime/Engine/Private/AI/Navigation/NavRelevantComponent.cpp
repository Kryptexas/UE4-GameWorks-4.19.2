// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineNavigationClasses.h"

UNavRelevantComponent::UNavRelevantComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	bNavigationRelevant = true;
}

void UNavRelevantComponent::OnOwnerRegistered()
{

}

void UNavRelevantComponent::OnOwnerUnregistered()
{

}

void UNavRelevantComponent::OnApplyModifiers(FCompositeNavModifier& Modifiers)
{

}

void UNavRelevantComponent::SetNavigationRelevancy(bool bRelevant)
{
	if (bNavigationRelevant != bRelevant)
	{
		bNavigationRelevant = bRelevant;

		if (bRelevant)
		{
			OnOwnerRegistered();
		}
		else
		{
			OnOwnerUnregistered();
		}

		RefreshNavigationModifiers();
	}
}

void UNavRelevantComponent::RefreshNavigationModifiers()
{
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	if (NavSys && GetOwner())
	{
		INavRelevantActorInterface* NavRelevantOwner = InterfaceCast<INavRelevantActorInterface>(GetOwner());
		if (NavRelevantOwner && NavRelevantOwner->DoesSupplyPerComponentNavigationCollision())
		{
			NavSys->UpdateNavOctree(this, UNavigationSystem::OctreeUpdate_Modifiers);
		}
		else
		{
			NavSys->UpdateNavOctree(GetOwner(), UNavigationSystem::OctreeUpdate_Modifiers);
		}
	}
}
