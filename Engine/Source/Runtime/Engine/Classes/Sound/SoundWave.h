// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * Playable sound object for raw wave files
 */

#include "SoundWave.generated.h"

struct FActiveSound;
struct FSoundParseParameters;
struct FWaveInstance;

UENUM()
enum EDecompressionType
{
	DTYPE_Setup,
	DTYPE_Invalid,
	DTYPE_Preview,
	DTYPE_Native,
	DTYPE_RealTime,
	DTYPE_Procedural,
	DTYPE_Xenon,
	DTYPE_MAX,
};

UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, BlueprintType, dependson=USoundGroups)
class USoundWave : public USoundBase
{
	GENERATED_UCLASS_BODY()

	/** Platform agnostic compression quality. 1..100 with 1 being best compression and 100 being best quality. */
	UPROPERTY(EditAnywhere, Category=Compression, AssetRegistrySearchable)
	int32 CompressionQuality;

	/** If set, the compressor does everything required to make this a seamlessly looping sound. */
	UPROPERTY(EditAnywhere, Category=Compression )
	uint32 bLoopableSound:1;

	/** If set, when played directly (not through a sound cue) the wave will be played looping. */
	UPROPERTY(EditAnywhere, Category=Compression )
	uint32 bLooping:1;

	/** Set to true for programmatically-generated, streamed audio. */
	uint32 bProcedural:1;

	/** Whether to free the resource data after it has been uploaded to the hardware */
	uint32 bDynamicResource:1;

	/** If set to true if this sound is considered to contain mature/adult content. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles, AssetRegistrySearchable)
	uint32 bMature:1;

	/** If set to true will disable automatic generation of line breaks - use if the subtitles have been split manually. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles )
	uint32 bManualWordWrap:1;

	/** If set to true the subtitles display as a sequence of single lines as opposed to multiline. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles )
	uint32 bSingleLine:1;

	/** Whether this SoundWave was decompressed from OGG. */
	uint32 bDecompressedFromOgg:1;

	UPROPERTY(EditAnywhere, Category=SoundWave)
	TEnumAsByte<ESoundGroup> SoundGroup;

	/** A localized version of the text that is actually spoken phonetically in the audio. */
	UPROPERTY(EditAnywhere, Category=Subtitles )
	FString SpokenText;


	/** Playback volume of sound 0 to 1 - Default is 1.0. */
	UPROPERTY(Category=SoundWave, meta=(ClampMin = "0.0"), EditAnywhere)
	float Volume;

	/** Playback pitch for sound - Minimum is 0.4, maximum is 2.0 - it is a simple linear multiplier to the SampleRate. */
	UPROPERTY(Category=SoundWave, meta=(ClampMin = "0.4", ClampMax = "2.0"), EditAnywhere)
	float Pitch;

	/** Number of channels of multichannel data; 1 or 2 for regular mono and stereo files */
	UPROPERTY(Category=Info, AssetRegistrySearchable, VisibleAnywhere)
	int32 NumChannels;

	/** Cached sample rate for displaying in the tools */
	UPROPERTY(Category=Info, AssetRegistrySearchable, VisibleAnywhere)
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

#if WITH_EDITORONLY_DATA
	/** Provides contextual information for the sound to the translator. */
	UPROPERTY(EditAnywhere, localized, Category=Subtitles )
	FString Comment;

#endif // WITH_EDITORONLY_DATA

	/**
	 * The array of the subtitles for each language. Generated at cook time.
	 */
	UPROPERTY()
	TArray<struct FLocalizedSubtitle> LocalizedSubtitles;

#if WITH_EDITORONLY_DATA
	/** Path to the resource used to construct this sound node wave. Relative to the object's package, BaseDir() or absolute. */
	UPROPERTY(Category=Info, VisibleAnywhere)
	FString SourceFilePath;

	/** Date/Time-stamp of the file from the last import */
	UPROPERTY(Category=Info, VisibleAnywhere)
	FString SourceFileTimestamp;

#endif // WITH_EDITORONLY_DATA

public:	
	/** Async worker that decompresses the vorbis data on a different thread */
	typedef FAsyncTask< class FAsyncVorbisDecompressWorker > FAsyncVorbisDecompress;	// Forward declare typedef
	FAsyncVorbisDecompress*		VorbisDecompressor;

	/** Pointer to 16 bit PCM data - used to decompress data to and preview sounds */
	uint8*						RawPCMData;

	/** Memory containing the data copied from the compressed bulk data */
	uint8*						ResourceData;

	/** Uncompressed wav data 16 bit in mono or stereo - stereo not allowed for multichannel data */
	FByteBulkData				RawData;

	/** GUID used to uniquely identify this node so it can be found in the DDC */
	FGuid						CompressedDataGuid;

	FFormatContainer			CompressedFormatData;

	/** Type of buffer this wave uses. Set once on load */
	TEnumAsByte<enum EDecompressionType> DecompressionType;

	/** Resource index to cross reference with buffers */
	int32 ResourceID;

	/** Size of resource copied from the bulk data */
	int32 ResourceSize;

	/** Cache the total used memory recorded for this SoundWave to keep INC/DEC consistent */
	int32 TrackedMemoryUsage;

	// Begin UObject interface. 
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	virtual void PostInitProperties() OVERRIDE;
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	virtual void FinishDestroy() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void CookerWillNeverCookAgain() OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;	
#endif // WITH_EDITOR
	virtual bool IsLocalizedResource() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	virtual FName GetExporterName() OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;
	// End UObject interface. 

	// Begin USoundBase interface.
	virtual bool IsPlayable() const OVERRIDE;
	virtual void Parse( class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual float GetMaxAudibleDistance() OVERRIDE;
	virtual float GetDuration() OVERRIDE;
	// End USoundBase interface.

	/**
	 *	@param		Format		Format to check
	 *
	 *	@return		Sum of the size of waves referenced by this cue for the given platform.
	 */
	virtual int32 GetResourceSizeForFormat(FName Format);

	/**
	 * Frees up all the resources allocated in this class
	 */
	ENGINE_API void FreeResources();

	/** 
	 * Copy the compressed audio data from the bulk data
	 */
	virtual ENGINE_API void InitAudioResource( FByteBulkData& CompressedData );

	/** 
	 * Copy the compressed audio data from derived data cache
	 *
	 * @param Format to get the compressed audio in
	 * @return true if the resource has been successfully initialized or it was already initialized.
	 */
	virtual ENGINE_API bool InitAudioResource(FName Format);

	/** 
	 * Remove the compressed audio data associated with the passed in wave
	 */
	ENGINE_API void RemoveAudioResource();

	/** 
	 * Prints the subtitle associated with the SoundWave to the console
	 */
	void LogSubtitle( FOutputDevice& Ar );

	/** 
	 * Handle any special requirements when the sound starts (e.g. subtitles)
	 */
	FWaveInstance* HandleStart( FActiveSound& ActiveSound, const UPTRINT WaveInstanceHash );

	/** 
	 * This is only for DTYPE_Procedural audio. Override this function.
	 *  Put SamplesNeeded PCM samples into Buffer. If put less,
	 *  silence will be filled in at the end of the buffer. If you
	 *  put more, data will be truncated.
	 *  Please note that "samples" means individual channels, not
	 *  sample frames! If you have stereo data and SamplesNeeded
	 *  is 1, you're writing two SWORDs, not four!
	 */
	virtual int32 GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded) { ensure(false); return 0; }

	/** 
	 * Gets the compressed data size from derived data cache for the specified format
	 * 
	 * @param Format	format of compressed data
	 * @return			compressed data size, or zero if it could not be obtained
	 */ 
	int32 GetCompressedDataSize(FName Format)
	{
		FByteBulkData* Data = GetCompressedData(Format);
		return Data ? Data->GetBulkDataSize() : 0;
	}

	/** 
	 * Gets the compressed data from derived data cache for the specified platform
	 * Warning, the returned pointer isn't valid after we add new formats
	 * 
	 * @param Format	format of compressed data
	 * @return	compressed data, if it could be obtained
	 */ 
	virtual ENGINE_API FByteBulkData* GetCompressedData(FName Format);

	/** 
	 * Change the guid and flush all compressed data
	 */ 
	ENGINE_API void InvalidateCompressedData();


};



