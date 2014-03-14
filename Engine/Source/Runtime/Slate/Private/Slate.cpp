// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "ModuleManager.h"

#include "Slate.generated.inl"

IMPLEMENT_MODULE( FDefaultModuleImpl, Slate );


DEFINE_STAT(STAT_SlateBuildLines);
DEFINE_STAT(STAT_SlateBuildBoxes);
DEFINE_STAT(STAT_SlateBuildBorders);
DEFINE_STAT(STAT_SlateBuildText);
DEFINE_STAT(STAT_SlateBuildGradients);
DEFINE_STAT(STAT_SlateBuildSplines);
DEFINE_STAT(STAT_SlateBuildViewports);
DEFINE_STAT(STAT_SlateBuildElements);
DEFINE_STAT(STAT_SlateFindBatchTime);
DEFINE_STAT(STAT_SlateUpdateBufferGTTime);
DEFINE_STAT(STAT_SlateFontCachingTime);
DEFINE_STAT(STAT_SlateRenderingGTTime);
DEFINE_STAT(STAT_SlateMeasureStringTime);


DEFINE_STAT(STAT_SlateCacheDesiredSize_STextBlock);
DEFINE_STAT(STAT_SlateCacheDesiredSize);

DEFINE_STAT(STAT_SlateOnPaint_SGraphPanel);
DEFINE_STAT(STAT_SlateOnPaint_STextBlock);
DEFINE_STAT(STAT_SlateOnPaint_SRichTextBlock);
DEFINE_STAT(STAT_SlateOnPaint_SEditableText);
DEFINE_STAT(STAT_SlateOnPaint_SOverlay);
DEFINE_STAT(STAT_SlateOnPaint_SPanel);
DEFINE_STAT(STAT_SlateOnPaint_SBorder);
DEFINE_STAT(STAT_SlateOnPaint_SCompoundWidget);
DEFINE_STAT(STAT_SlateOnPaint_SBox);
DEFINE_STAT(STAT_SlateOnPaint_SImage);
DEFINE_STAT(STAT_SlateOnPaint_SViewport);
DEFINE_STAT(STAT_SlateOnPaint);
DEFINE_STAT(STAT_SlateOnPaint_SSlider);

DEFINE_STAT(STAT_SlateDrawWindowTime);

DEFINE_STAT(STAT_SlateTickWidgets);

DEFINE_STAT(STAT_SlateTickTime);

DEFINE_STAT(STAT_SlateDrawTime);
DEFINE_STAT(STAT_SlateUpdateBufferRTTime);
DEFINE_STAT(STAT_SlateRenderingRTTime);

// Useful for locally pref testing miscellaneous Slate areas.  Usage of this should not be checked in 
DEFINE_STAT(STAT_SlateMiscTime);


DEFINE_STAT(STAT_SlateNumLayers);
DEFINE_STAT(STAT_SlateNumBatches);
DEFINE_STAT(STAT_SlateVertexCount);

DEFINE_STAT(STAT_SlateVertexBatchMemory);
DEFINE_STAT(STAT_SlateIndexBatchMemory);
DEFINE_STAT(STAT_SlateVertexBufferMemory);
DEFINE_STAT(STAT_SlateIndexBufferMemory);
DEFINE_STAT(STAT_SlateTextureAtlasMemory);
DEFINE_STAT(STAT_SlateFontKerningTableMemory);
DEFINE_STAT(STAT_SlateFontMeasureCacheMemory);
DEFINE_STAT(STAT_SlateNumTextureAtlases);
DEFINE_STAT(STAT_SlateNumFontAtlases);
DEFINE_STAT(STAT_SlateNumNonAtlasedTextures);
DEFINE_STAT(STAT_SlateTextureDataMemory);
DEFINE_STAT(STAT_SlateNumDynamicTextures);