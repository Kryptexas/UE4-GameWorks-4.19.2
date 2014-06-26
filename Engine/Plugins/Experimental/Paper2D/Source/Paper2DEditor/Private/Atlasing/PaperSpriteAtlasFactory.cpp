// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperSpriteAtlasFactory.h"

//////////////////////////////////////////////////////////////////////////
// UPaperSpriteAtlasFactory

UPaperSpriteAtlasFactory::UPaperSpriteAtlasFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SupportedClass = UPaperSpriteAtlas::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UPaperSpriteAtlasFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return ConstructObject<UObject>(SupportedClass, InParent, InName, Flags);
}

bool UPaperSpriteAtlasFactory::CanCreateNew() const
{
	return GetDefault<UPaperRuntimeSettings>()->bEnableSpriteAtlasGroups;
}
