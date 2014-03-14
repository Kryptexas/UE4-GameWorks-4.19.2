// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LogVisualizerHUD.generated.h"

UCLASS()
class ALogVisualizerHUD : public ADebugCameraHUD
{
	GENERATED_UCLASS_BODY()

	FFontRenderInfo TextRenderInfo;

	virtual bool DisplayMaterials( float X, float& Y, float DY, UMeshComponent* MeshComp ) OVERRIDE;

	// Begin AActor Interface
	virtual void PostRender() OVERRIDE;
	// End AActor Interface
};



