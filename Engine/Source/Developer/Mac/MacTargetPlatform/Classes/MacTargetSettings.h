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
     * The maximum supported Metal shader langauge version. 
     * This defines what features may be used and OS versions supported.
     */
    UPROPERTY(EditAnywhere, config, Category=Rendering, meta = (DisplayName = "Max. Metal Shader Standard To Target", ConfigRestartRequired = true))
    uint8 MaxShaderLanguageVersion;

	/** Sample rate to run the audio mixer with. */
	UPROPERTY(config, EditAnywhere, Category = "Audio", Meta = (DisplayName = "Audio Mixer Sample Rate"))
	int32 AudioSampleRate;

	/** The amount of audio to compute each callback block. Lower values decrease latency but may increase CPU cost. */
	UPROPERTY(config, EditAnywhere, Category = "Audio", meta = (ClampMin = "512", ClampMax = "4096", DisplayName = "Callback Buffer Size"))
	int32 AudioCallbackBufferFrameSize;

	/** The number of buffers to keep enqueued. More buffers increases latency, but can compensate for variable compute availability in audio callbacks on some platforms. */
	UPROPERTY(config, EditAnywhere, Category = "Audio", meta = (ClampMin = "1", UIMin = "1", DisplayName = "Number of Buffers To Enqueue"))
	int32 AudioNumBuffersToEnqueue;

	/** The max number of channels (voices) to limit for this platform. The max channels used will be the minimum of this value and the global audio quality settings. A value of 0 will not apply a platform channel count max. */
	UPROPERTY(config, EditAnywhere, Category = "Audio", meta = (ClampMin = "0", UIMin = "0", DisplayName = "Max Channels"))
	int32 AudioMaxChannels;

	/** The number of workers to use to compute source audio. Will only use up to the max number of sources. Will evenly divide sources to each source worker. */
	UPROPERTY(config, EditAnywhere, Category = "Audio", meta = (ClampMin = "0", UIMin = "0", DisplayName = "Number of Source Workers"))
	int32 AudioNumSourceWorkers;
};
