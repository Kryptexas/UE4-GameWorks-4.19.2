// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperExtractSpritesSettings.generated.h"

UENUM()
enum class ESpriteExtractMode : uint8
{
	Auto,	// Automatically extract sprites
	Grid,	// Extract sprites by grid
};

/**
*
*/
UCLASS()
class UPaperExtractSpritesSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Settings, EditAnywhere)
	ESpriteExtractMode Mode;

	UPROPERTY(Category = Settings, EditAnywhere)
	FLinearColor OutlineColor;

	UPROPERTY(Category = Settings, EditAnywhere)
	FLinearColor TextureTint;

	UPROPERTY()
	bool bAutoMode;

	UPROPERTY()
	bool bGridMode;


	//////////////////////////////////////////////////////////////////////////
	// Grid mode

	UPROPERTY(Category = Grid, EditAnywhere, meta = (EditCondition = "bGridMode", UIMin = 1, ClampMin = 1))
	int32 GridWidth;

	UPROPERTY(Category = Grid, EditAnywhere, meta = (EditCondition = "bGridMode", UIMin = 1, ClampMin = 1))
	int32 GridHeight;

	UPROPERTY(Category = Grid, EditAnywhere, meta = (EditCondition = "bGridMode", UIMin = 0, ClampMin = 0))
	int32 Margin;

	UPROPERTY(Category = Grid, EditAnywhere, meta = (EditCondition = "bGridMode", UIMin = 0, ClampMin = 0))
	int32 Spacing;
	


	UPaperExtractSpritesSettings(const FObjectInitializer& ObjectInitializer);

	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface
};
