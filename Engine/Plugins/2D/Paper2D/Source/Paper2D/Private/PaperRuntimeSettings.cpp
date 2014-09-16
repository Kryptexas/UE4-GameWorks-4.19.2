// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRuntimeSettings.h"

//////////////////////////////////////////////////////////////////////////
// UPaperRuntimeSettings

UPaperRuntimeSettings::UPaperRuntimeSettings(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, DefaultPixelsPerUnrealUnit(2.56f)
	, bEnableSpriteAtlasGroups(false)
	, bEnableTileMapEditing(false)
	, bEnableTerrainSplineEditing(false)
{
#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet())
	{
		DefaultTranslucentSpriteMaterialName = FStringAssetReference("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial");
		DefaultOpaqueSpriteMaterialName = FStringAssetReference("/Paper2D/OpaqueUnlitSpriteMaterial.OpaqueUnlitSpriteMaterial");
	}
#endif
}
