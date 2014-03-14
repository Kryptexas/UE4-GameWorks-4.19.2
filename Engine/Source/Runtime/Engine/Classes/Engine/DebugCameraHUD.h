// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "DebugCameraHUD.generated.h"

UCLASS(config=Game)
class ADebugCameraHUD : public AHUD
{
	GENERATED_UCLASS_BODY()

	/** @todo document */
	virtual bool DisplayMaterials( float X, float& Y, float DY, class UMeshComponent* MeshComp );
	
	// Begin AActor Interface
	virtual void PostRender() OVERRIDE;
	// End AActor Interface

};



