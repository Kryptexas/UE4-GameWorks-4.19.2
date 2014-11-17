// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "Paper2D"

/////////////////////////////////////////////////////
// UPaperSpriteFactory

UPaperSpriteFactory::UPaperSpriteFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bUseSourceRegion(false)
	, InitialSourceUV(0, 0)
	, InitialSourceDimension(0, 0)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UPaperSprite::StaticClass();
}

bool UPaperSpriteFactory::ConfigureProperties()
{
	//@TODO: Maybe create a texture picker here?
	return true;
}

UObject* UPaperSpriteFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UPaperSprite* NewSprite = ConstructObject<UPaperSprite>(Class, InParent, Name, Flags | RF_Transactional);

	if (bUseSourceRegion)
	{
		NewSprite->InitializeSprite(InitialTexture, InitialSourceUV, InitialSourceDimension);
	}
	else
	{
		NewSprite->InitializeSprite(InitialTexture);
	}

	return NewSprite;
}

#undef LOCTEXT_NAMESPACE