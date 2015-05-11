// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
// Automation
#include "AutomationCommon.h"
#include "Tests/AutomationTestSettings.h"

//Materials

class FEditorPromotionTestUtilities
{

public:

	/**
	* Gets the base path for this asset
	*/
	UNREALED_API static FString GetGamePath();


	/**
	* Creates a material from an existing texture
	*
	* @param InTexture - The texture to use as the diffuse for the new material
	*/
	UNREALED_API static UMaterial* CreateMaterialFromTexture(UTexture* InTexture);
};