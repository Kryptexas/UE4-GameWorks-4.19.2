// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FoliagePrivate.h"
#include "ProceduralFoliageActor.h"
#include "ProceduralFoliageComponent.h"
#include "ProceduralFoliage.h"

AProceduralFoliageActor::AProceduralFoliageActor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ProceduralComponent = ObjectInitializer.CreateDefaultSubobject<UProceduralFoliageComponent>(this, TEXT("ProceduralFoliageComponent"));
	ProceduralComponent->SpawningVolume = this;

	if (UBrushComponent* BrushComponent = GetBrushComponent())
	{
		BrushComponent->SetCollisionObjectType(ECC_WorldStatic);
		BrushComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

#if WITH_EDITOR

bool AProceduralFoliageActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	Super::GetReferencedContentObjects(Objects);

	if (ProceduralComponent && ProceduralComponent->ProceduralFoliage)
	{
		Objects.Add(ProceduralComponent->ProceduralFoliage);
	}
	return true;
}

#endif