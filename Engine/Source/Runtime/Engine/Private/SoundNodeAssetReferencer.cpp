// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Sound/SoundNodeAssetReferencer.h"

#if WITH_EDITOR
void USoundNodeAssetReferencer::PostEditImport()
{
	Super::PostEditImport();

	LoadAsset();
}
#endif
