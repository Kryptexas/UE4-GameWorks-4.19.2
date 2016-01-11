// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once 
#include "GameFramework/HUD.h"
#include "FP_FirstPersonHUD.generated.h"

UCLASS()
class AFP_FirstPersonHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFP_FirstPersonHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

