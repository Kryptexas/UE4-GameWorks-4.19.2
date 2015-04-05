// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperExtractSpritesSettings.h"

UPaperExtractSpritesSettings::UPaperExtractSpritesSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutlineColor = FLinearColor::Yellow;
	TextureTint = FLinearColor::Gray;
	BackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f);
}

void UPaperExtractSpritesSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPaperExtractSpritesSettings, SpriteExtractMode))
	{
		// Update bools
		bAutoMode = (SpriteExtractMode == ESpriteExtractMode::Auto);
		bGridMode = (SpriteExtractMode == ESpriteExtractMode::Grid);
	}
}


UPaperExtractSpriteGridSettings::UPaperExtractSpriteGridSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
