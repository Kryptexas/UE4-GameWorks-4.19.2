// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FoliagePrivate.h"
#include "FoliageStatistics.h"

//////////////////////////////////////////////////////////////////////////
// UFoliageStatics

UFoliageStatistics::UFoliageStatistics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 UFoliageStatistics::FoliageOverlappingSphereCount(UObject* WorldContextObject, const UStaticMesh* Mesh, FVector CenterPosition, float Radius)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	const FSphere Sphere(CenterPosition, Radius);

	int32 Count = 0;
	for (FConstLevelIterator LvlItr = World->GetLevelIterator(); LvlItr; ++LvlItr)
	{
		ULevel* Level = *LvlItr;
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level);
		const UFoliageType* FoliageType = IFA->GetSettingsForMesh(Mesh);
		Count += IFA->GetOverlappingSphereCount(FoliageType, Sphere);
	}

	return Count;
}