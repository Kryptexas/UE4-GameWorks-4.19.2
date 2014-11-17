// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Landscape/LandscapeProxy.h"
#include "Landscape/LandscapeInfo.h"

#if WITH_EDITOR

ENGINE_API FLandscapeImportLayerInfo::FLandscapeImportLayerInfo(const struct FLandscapeInfoLayerSettings& InLayerSettings)
:	LayerName(InLayerSettings.GetLayerName())
,	LayerInfo(InLayerSettings.LayerInfoObj)
,	ThumbnailMIC(NULL)
,	SourceFilePath(InLayerSettings.GetEditorSettings().ReimportLayerFilePath)
{ }

#endif // WITH_EDITOR