// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DestructibleActor.cpp: ADestructibleActor methods.
=============================================================================*/


#include "EnginePrivate.h"
#include "PhysicsEngine/PhysXSupport.h"


ADestructibleActor::ADestructibleActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DestructibleComponent = PCIP.CreateDefaultSubobject<UDestructibleComponent>(this, TEXT("DestructibleComponent0"));
	RootComponent = DestructibleComponent;
}

#if WITH_EDITOR
bool ADestructibleActor::GetReferencedContentObjects( TArray<UObject*>& Objects ) const
{
	if (DestructibleComponent.IsValid() && DestructibleComponent->SkeletalMesh)
	{
		Objects.Add(DestructibleComponent->SkeletalMesh);
	}
	return true;
}
#endif // WITH_EDITOR


