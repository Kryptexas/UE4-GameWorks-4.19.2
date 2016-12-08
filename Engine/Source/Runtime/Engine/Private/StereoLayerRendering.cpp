// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenRendering.cpp: Screen rendering implementation.
=============================================================================*/

#include "StereoLayerRendering.h"

IMPLEMENT_SHADER_TYPE(,FStereoLayerVS,TEXT("StereoLayerShader"),TEXT("MainVS"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FStereoLayerPS,TEXT("StereoLayerShader"),TEXT("MainPS"),SF_Pixel);
