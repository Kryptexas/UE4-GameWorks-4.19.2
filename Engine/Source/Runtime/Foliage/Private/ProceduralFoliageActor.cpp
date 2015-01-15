// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FoliagePrivate.h"
#include "ProceduralFoliageActor.h"
#include "ProceduralFoliageComponent.h"

AProceduralFoliageActor::AProceduralFoliageActor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ProceduralComponent = ObjectInitializer.CreateDefaultSubobject<UProceduralFoliageComponent>(this, TEXT("ProceduralFoliageComponent"));
	RootComponent = ProceduralComponent;
}
