// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateModule.cpp: Implements the FSlateModule class.
=============================================================================*/

#include "SlatePrivatePCH.h"
#include "ModuleManager.h"


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
DEFINE_STAT(STAT_SlateOnPaint_SSlider);

DEFINE_STAT(STAT_SlateDrawWindowTime);
DEFINE_STAT(STAT_SlateTickWidgets);
DEFINE_STAT(STAT_SlateTickTime);

DEFINE_STAT(STAT_SlateMiscTime); // Useful for locally pref testing miscellaneous Slate areas. Usage of this should not be checked in!


/**
 * Implements the Slate module.
 */
class FSlateModule
	: public IModuleInterface
{
};


IMPLEMENT_MODULE(FSlateModule, Slate);
