// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "TextureEditorPrivatePCH.h"


UTextureEditorSettings::UTextureEditorSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, Background(TextureEditorBackground_SolidColor)
	, BackgroundColor(FColor::Black)
	, CheckerColorOne(FColor(128, 128, 128))
	, CheckerColorTwo(FColor(64, 64, 64))
	, CheckerSize(32)
	, FitToViewport(true)
	, TextureBorderColor(FColor::White)
	, TextureBorderEnabled(true)
{ }
