// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LandscapePrivatePCH.h"
#include "LandscapeProxy.h"

#if WITH_EDITOR

LANDSCAPE_API FLandscapeImportLayerInfo::FLandscapeImportLayerInfo(const FLandscapeInfoLayerSettings& InLayerSettings)
	: LayerName(InLayerSettings.GetLayerName())
	, LayerInfo(InLayerSettings.LayerInfoObj)
	, SourceFilePath(InLayerSettings.GetEditorSettings().ReimportLayerFilePath)
{
}

#endif // WITH_EDITOR
