// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
/** 
 * Sound node that contains sample data
 */

#pragma once
#include "SoundNodeWave.generated.h"

UCLASS(deprecated, PerObjectConfig, hidecategories=Object, editinlinenew, MinimalAPI)
class UDEPRECATED_SoundNodeWave : public UDEPRECATED_SoundNodeDeprecated
{
	GENERATED_UCLASS_BODY()

	/** Platform agnostic compression quality. 1..100 with 1 being best compression (smallest size) and 100 being best quality (largest size). */
	UPROPERTY(EditAnywhere, Category=Compression, AssetRegistrySearchable)
	int32 CompressionQuality;

	/** If set, the compressor does everything required to make this a seamlessly looping sound. */
	UPROPERTY(EditAnywhere, Category=Compression )
	uint32 bLoopingSound:1;

	/** A localized version of the text that is actually phonetically spoken in the audio. */
	UPROPERTY(EditAnywhere, Category=Subtitles )
	FString SpokenText;

	/** Playback volume of sound 0 to 1 - Default is 0.75. */
	UPROPERTY(Category=Info, VisibleAnywhere, BlueprintReadOnly)
	float Volume;

	/** Playback pitch for sound 0.4 to 2.0  - it is a simple linear multiplier to the SampleRate. */
	UPROPERTY(Category=Info, VisibleAnywhere, BlueprintReadOnly)
	float Pitch;

	/** Duration of sound in seconds. */
	UPROPERTY(Category=Info, AssetRegistrySearchable, VisibleAnywhere, BlueprintReadOnly)
	float Duration;

	/** Number of channels of multichannel data; 1 or 2 for regular mono and stereo files */
	UPROPERTY(Category=Info, AssetRegistrySearchable, VisibleAnywhere, BlueprintReadOnly)
	int32 NumChannels;

	/** Cached sample rate for displaying in the tools */
	UPROPERTY(Category=Info, AssetRegistrySearchable, VisibleAnywhere, BlueprintReadOnly)
	int32 SampleRate;

#if WITH_EDITORONLY_DATA
	/** Offsets into the bulk data for the source wav data */
	UPROPERTY()
	TArray<int32> ChannelOffsets;

	/** Sizes of the bulk data for the source wav data */
	UPROPERTY()
	TArray<int32> ChannelSizes;

#endif // WITH_EDITORONLY_DATA

	/** Size of RawPCMData, or what RawPCMData would be if the sound was fully decompressed */
	UPROPERTY()
	int32 RawPCMDataSize;

	/**
	 * Subtitle cues.  If empty, use SpokenText as the subtitle.  Will often be empty,
	 * as the contents of the subtitle is commonly identical to what is spoken.
	 */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles)
	TArray<struct FSubtitleCue> Subtitles;

	/** Set to true if this sound is considered to contain mature content. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles, AssetRegistrySearchable)
	uint32 bMature:1;

#if WITH_EDITORONLY_DATA
	/** Provides contextual information for the sound to the translator. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles )
	FString Comment;

#endif // WITH_EDITORONLY_DATA
	/** Set to true if the subtitles have been split manually - disables automatic generation of line breaks. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles )
	uint32 bManualWordWrap:1;

	/** Set to true if the subtitles display as a sequence of single lines as opposed to multiline. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles )
	uint32 bSingleLine:1;

	/**
	 * The array of the subtitles for each language. Generated at cook time.
	 */
	UPROPERTY()
	TArray<struct FLocalizedSubtitle> LocalizedSubtitles;

#if WITH_EDITORONLY_DATA
	/** Path to the resource used to construct this sound node wave. Relative to the object's package, BaseDir() or absolute. */
	UPROPERTY(Category=Wave, VisibleAnywhere, BlueprintReadOnly)
	FString SourceFilePath;

	/** Date/Time-stamp of the file from the last import */
	UPROPERTY(Category=Wave, VisibleAnywhere, BlueprintReadOnly)
	FString SourceFileTimestamp;

#endif // WITH_EDITORONLY_DATA

public:	
	/** Uncompressed wav data 16 bit in mono or stereo - stereo not allowed for multichannel data */
	FByteBulkData				RawData;

	// Begin UObject interface. 
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	// End UObject interface. 

};



