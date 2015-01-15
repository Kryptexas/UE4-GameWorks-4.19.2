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

	//@TODO: properly handle sub-levels
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(World);
	const UFoliageType* FoliageType = IFA->GetSettingsForMesh(Mesh);
	return IFA->GetOverlappingSphereCount(FoliageType, Sphere);
}