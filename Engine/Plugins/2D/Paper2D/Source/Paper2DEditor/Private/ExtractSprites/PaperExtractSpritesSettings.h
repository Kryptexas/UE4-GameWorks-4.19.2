// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperExtractSpritesSettings.generated.h"

UENUM()
enum class ESpriteExtractMode : uint8
{
	// Automatically extract sprites by detecting using alpha
	Auto,

	// Extract sprites in a grid defined in the properties below
	Grid,	
};



/**
*
*/
UCLASS()
class UPaperExtractSpritesSettings : public UObject
{
	GENERATED_BODY()

public:
	// Sprite extract mode
	UPROPERTY(Category = Settings, EditAnywhere)
	ESpriteExtractMode SpriteExtractMode;

	// The color of the sprite boundary outlines
	UPROPERTY(Category = Settings, EditAnywhere)
	FLinearColor OutlineColor;

	// Tint the texture to help increase outline visibility
	UPROPERTY(Category = Settings, EditAnywhere)
	FLinearColor TextureTint;

	// The viewport background color
	UPROPERTY(Category = Settings, EditAnywhere)
	FLinearColor BackgroundColor;

	UPROPERTY()
	bool bAutoMode;

	UPROPERTY()
	bool bGridMode;


	UPaperExtractSpritesSettings(const FObjectInitializer& ObjectInitializer);

	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface
};

UCLASS()
class UPaperExtractSpriteGridSettings : public UObject
{
	GENERATED_BODY()

	//////////////////////////////////////////////////////////////////////////
	// Grid mode

	// The name of the sprite that will be created. {0} will get replaced by the sprite number.
	UPROPERTY(Category = Naming, EditAnywhere)
	FString NamingTemplate;

	// The number to start naming with
	UPROPERTY(Category = Naming, EditAnywhere)
	int32 NamingStartIndex;

	// The width of each sprite in grid mode
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
	int32 CellWidth;

	// The height of each sprite in grid mode
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
	int32 CellHeight;

	// Number of cells extracted horizontally. Can be used to limit the number of sprites extracted. Set to 0 to extract all sprites
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
	int32 NumCellsX;

	// Number of cells extracted vertically. Can be used to limit the number of sprites extracted. Set to 0 to extract all sprites
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
	int32 NumCellsY;

	// Margin from the left of the texture to the first sprite
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
	int32 MarginX;

	// Margin from the top of the texture to the first sprite
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
	int32 MarginY;

	// Horizontal spacing between sprites
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
	int32 SpacingX;

	// Vertical spacing between sprites
	UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
	int32 SpacingY;

	UPaperExtractSpriteGridSettings(const FObjectInitializer& ObjectInitializer);
};
