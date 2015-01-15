// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FoliageStatistics.generated.h"

UCLASS()
class FOLIAGE_API UFoliageStatistics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/**
	* Counts how many foliage instances overlap a given sphere
	*
	* @param	Mesh			The static mesh we are interested in counting
	* @param	CenterPosition	The center position of the sphere
	* @param	Radius			The radius of the sphere.
	*
	* return number of foliage instances with their mesh set to Mesh that overlap the sphere
	*/
	UFUNCTION(BlueprintCallable, Category = "Foliage", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static int32 FoliageOverlappingSphereCount(UObject* WorldContextObject, const UStaticMesh* StaticMesh, FVector CenterPosition, float Radius);
};

