// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoundDefinitions.h"

class FAssetTypeActions_DialogueVoice : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_DialogueVoice", "Dialogue Voice"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,0,255); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UDialogueVoice::StaticClass(); }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Sounds; }
	virtual bool CanFilter() OVERRIDE { return true; }
};