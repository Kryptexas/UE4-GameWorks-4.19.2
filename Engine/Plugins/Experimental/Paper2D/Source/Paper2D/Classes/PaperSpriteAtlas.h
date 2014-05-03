// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperSpriteAtlas.generated.h"

// Groups together a set of sprites that will try to share the same texture atlas (allowing them to be combined into a single draw call)
UCLASS(MinimalAPI, BlueprintType, meta=(DisplayThumbnail = "true"))
class UPaperSpriteAtlas : public UObject
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	// Description of this atlas, which shows up in the content browser tooltip
	UPROPERTY(EditAnywhere, Category=General)
	FString AtlasDescription;
#endif

	// UObject interface
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;
	// End of UObject interface
};
