// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LandscapeInfo.h"

#include "LandscapeInfoMap.generated.h"

UCLASS()
class ULandscapeInfoMap : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void PostDuplicate(bool bDuplicateForPIE) override;
	void Serialize(FArchive& Ar) override;

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	TMap<FGuid, ULandscapeInfo*> Map;


#if WITH_EDITORONLY_DATA
	/**
	* Gets landscape-specific data for given world.
	*
	* @param World A pointer to world that return data should be associated with.
	*
	* @returns Landscape-specific data associated with given world.
	*/
	LANDSCAPE_API static ULandscapeInfoMap& GetLandscapeInfoMap(UWorld* World);
#endif // WITH_EDITORONLY_DATA
};
