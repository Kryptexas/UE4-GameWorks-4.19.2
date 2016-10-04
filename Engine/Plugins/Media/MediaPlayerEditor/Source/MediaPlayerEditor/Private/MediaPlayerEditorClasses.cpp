// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayerEditorPCH.h"
#include "MediaPlayerEditorSettings.h"


UMediaPlayerEditorSettings::UMediaPlayerEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ShowTextOverlays(true)
	, ViewportScale(EMediaPlayerEditorScale::Fit)
{ }
