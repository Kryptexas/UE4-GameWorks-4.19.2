// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Landscape.h"
#include "LandscapeInfoMap.h"

ULandscapeInfoMap::ULandscapeInfoMap(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULandscapeInfoMap::PostDuplicate(bool bDuplicateForPIE)
{
	// After duplication the pointers are duplicated, but the objects should
	// be duplicated.

	Super::PostDuplicate(bDuplicateForPIE);
	
	for (auto Iterator = Map.CreateConstIterator(); Iterator; ++Iterator)
	{
		Map[Iterator->Key] = Cast<ULandscapeInfo>(StaticDuplicateObject(
			Iterator->Value, Iterator->Value->GetOuter(), nullptr));
	}
}

void ULandscapeInfoMap::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << Map;
}

void ULandscapeInfoMap::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	ULandscapeInfoMap* This = CastChecked<ULandscapeInfoMap>(InThis);

	for (auto ReferencedMapInfo = This->Map.CreateIterator(); ReferencedMapInfo; ++ReferencedMapInfo)
	{
		Collector.AddReferencedObject(ReferencedMapInfo->Value, This);
	}
}
