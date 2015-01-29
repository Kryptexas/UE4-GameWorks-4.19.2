// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperExtractSpritesSettings.h"

UPaperExtractSpritesSettings::UPaperExtractSpritesSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutlineColor = FLinearColor::Yellow;
	TextureTint = FLinearColor::Gray;
}

void UPaperExtractSpritesSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPaperExtractSpritesSettings, Mode))
	{
		// Update bools
		bAutoMode = (Mode == ESpriteExtractMode::Auto);
		bGridMode = (Mode == ESpriteExtractMode::Grid);
	}
}
