// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacTargetSettings.h: Declares the UMacTargetSettings class.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MacTargetSettings.generated.h"

UENUM()
enum class EMacMetalShaderStandard : uint8
{
    /** Metal Shaders Compatible With OS X El Capitan 10.11.4 or later (std=osx-metal1.1) */
    MacMetalSLStandard_1_1 = 1 UMETA(DisplayName="Metal v1.1 (10.11.4+)"),
    
    /** Metal Shaders, supporting Tessellation Shaders & Fragment Shader UAVs, Compatible With macOS Sierra 10.12.0 or later (std=osx-metal1.2) */
    MacMetalSLStandard_1_2 = 2 UMETA(DisplayName="Metal v1.2 (10.12.0+)"),
};

/**
 * Implements the settings for the Mac target platform.
 */
UCLASS(config=Engine, defaultconfig)
class MACTARGETPLATFORM_API UMacTargetSettings
	: public UObject
{
public:

	GENERATED_UCLASS_BODY()
	
	/**
	 * The collection of RHI's we want to support on this platform.
	 * This is not always the full list of RHI we can support.
	 */
	UPROPERTY(EditAnywhere, config, Category=Rendering)
	TArray<FString> TargetedRHIs;
	
	/**
	 * The collection of shader formats we want to cache on this platform.
	 * This is not always the full list of RHI we can support.
	 */
	UPROPERTY(EditAnywhere, config, Category=Rendering)
    TArray<FString> CachedShaderFormats;
    
    /**
     * The maximum supported Metal shader langauge version. 
     * This defines what features may be used and OS versions supported.
     */
    UPROPERTY(EditAnywhere, config, Category=Rendering, meta = (DisplayName = "Max. Metal Shader Standard To Target"))
    uint8 MaxShaderLanguageVersion;
};
