// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UAssetImportData::UAssetImportData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bDirty = false;
}

#if WITH_EDITOR
void UAssetImportData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bDirty = true;
}
#endif // WITH_EDITOR
