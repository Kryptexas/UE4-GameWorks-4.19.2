// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#include "AudioDecompress.h"
#include "TargetPlatform.h"
#include "AudioDerivedData.h"
#include "SubtitleManager.h"

USoundWave::USoundWave(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Volume = 1.0;
	Pitch = 1.0;
	CompressionQuality = 40;
	bLoopableSound = true;
}

SIZE_T USoundWave::GetResourceSize(EResourceSizeMode::Type Mode)
{
	int32 CalculatedResourceSize = 0;

	if( DecompressionType == DTYPE_Native )
	{
		// If we've been decompressed, need to account for decompressed and also compressed
		CalculatedResourceSize += RawPCMDataSize;
	}

	if (GEngine && GEngine->GetAudioDevice())
	{
		CalculatedResourceSize += GetCompressedDataSize(GEngine->GetAudioDevice()->GetRuntimeFormat());
	}

	return CalculatedResourceSize;
}

int32 USoundWave::GetResourceSizeForFormat(FName Format)
{
	return GetCompressedDataSize(Format);
}


FName USoundWave::GetExporterName()
{
#if WITH_EDITORONLY_DATA
	if( ChannelOffsets.Num() > 0 && ChannelSizes.Num() > 0 )
	{
		return( FName( TEXT( "SoundSurroundExporterWAV" ) ) );
	}
#endif // WITH_EDITORONLY_DATA

	return( FName( TEXT( "SoundExporterWAV" ) ) );
}


FString USoundWave::GetDesc()
{
	FString Channels;

	if( NumChannels == 0 )
	{
		Channels = TEXT( "Unconverted" );
	}
#if WITH_EDITORONLY_DATA
	else if( ChannelSizes.Num() == 0 )
	{
		Channels = ( NumChannels == 1 ) ? TEXT( "Mono" ) : TEXT( "Stereo" );
	}
#endif // WITH_EDITORONLY_DATA
	else
	{
		Channels = FString::Printf( TEXT( "%d Channels" ), NumChannels );
	}

	return FString::Printf( TEXT( "%3.2fs %s" ), Duration, *Channels );
}

void USoundWave::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);

	// GetCompressedDataSize could technically modify this->CompressedFormatData therefore it is not const, however this information
	// is very useful in the asset registry so we will allow GetCompressedDataSize to be modified if the formats do not exist
	USoundWave* MutableThis = const_cast<USoundWave*>(this);
	const FString OggSize = FString::Printf(TEXT("%.2f"), MutableThis->GetCompressedDataSize("OGG") / 1024.0f );
	OutTags.Add( FAssetRegistryTag("OggSize", OggSize, UObject::FAssetRegistryTag::TT_Numerical) );
}

void USoundWave::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	bool bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (FPlatformProperties::RequiresCookedData() && !bCooked && Ar.IsLoading())
	{
		UE_LOG(LogAudio, Fatal, TEXT("This platform requires cooked packages, and audio data was not cooked into %s."), *GetFullName());
	}

	if (bCooked)
	{
		if (Ar.IsCooking())
		{
#if WITH_ENGINE
			TArray<FName> ActualFormatsToSave;
			if (!Ar.CookingTarget()->IsServerOnly())
			{
				// for now we only support one format per wav
				FName Format = Ar.CookingTarget()->GetWaveFormat(this);
				GetCompressedData(Format); // Get the data from the DDC or build it

				ActualFormatsToSave.Add(Format);
			}
			CompressedFormatData.Serialize(Ar, this, &ActualFormatsToSave);
#endif
		}
		else
		{
			CompressedFormatData.Serialize(Ar, this);
		}
	}
	else
	{
		// only save the raw data for non-cooked packages
		RawData.Serialize( Ar, this );
	}

	Ar << CompressedDataGuid;
}

/**
 * Prints the subtitle associated with the SoundWave to the console
 */
void USoundWave::LogSubtitle( FOutputDevice& Ar )
{
	FString Subtitle = "";
	for( int32 i = 0; i < Subtitles.Num(); i++ )
	{
		Subtitle += Subtitles[ i ].Text.ToString();
	}

	if( Subtitle.Len() == 0 )
	{
		Subtitle = SpokenText;
	}

	if( Subtitle.Len() == 0 )
	{
		Subtitle = "<NO SUBTITLE>";
	}

	Ar.Logf( TEXT( "Subtitle:  %s" ), *Subtitle );
#if WITH_EDITORONLY_DATA
	Ar.Logf( TEXT( "Comment:   %s" ), *Comment );
#endif // WITH_EDITORONLY_DATA
	Ar.Logf( bMature ? TEXT( "Mature:    Yes" ) : TEXT( "Mature:    No" ) );
}

void USoundWave::PostInitProperties()
{
	Super::PostInitProperties();

	if(!IsTemplate())
	{
		InvalidateCompressedData();
	}
}

FByteBulkData* USoundWave::GetCompressedData(FName Format)
{
	if (IsTemplate() || IsRunningDedicatedServer())
	{
		return NULL;
	}
	bool bContainedData = CompressedFormatData.Contains(Format);
	FByteBulkData* Result = &CompressedFormatData.GetFormat(Format);
	if (!bContainedData)
	{
		if (!FPlatformProperties::RequiresCookedData() && GetDerivedDataCache())
		{
			TArray<uint8> OutData;
			FDerivedAudioDataCompressor* DeriveAudioData = new FDerivedAudioDataCompressor(this, Format);
			GetDerivedDataCacheRef().GetSynchronous(DeriveAudioData, OutData);
			if (OutData.Num())
			{
				Result->Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(Result->Realloc(OutData.Num()), OutData.GetTypedData(), OutData.Num());
				Result->Unlock();
			}
		}
		else
		{
			UE_LOG(LogAudio, Error, TEXT("Attempt to access the DDC when there is none available on sound '%s', format = %s. Should have been cooked."), *GetFullName(), *Format.ToString());
		}
	}
	check(Result);
	return Result->GetBulkDataSize() > 0 ? Result : NULL; // we don't return empty bulk data...but we save it to avoid thrashing the DDC
}

void USoundWave::InvalidateCompressedData()
{
	CompressedDataGuid = FGuid::NewGuid();
	CompressedFormatData.FlushData();
}

void USoundWave::PostLoad()
{
	Super::PostLoad();

	if (GetOutermost()->PackageFlags & PKG_ReloadingForCooker)
	{
		return;
	}

	// Compress to whatever formats the active target platforms want
	// static here as an optimization
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		const TArray<ITargetPlatform*>& Platforms = TPM->GetActiveTargetPlatforms();

		for (int32 Index = 0; Index < Platforms.Num(); Index++)
		{
			GetCompressedData(Platforms[Index]->GetWaveFormat(this));
		}
	}

	// We don't precache default objects and we don't precache in the Editor as the latter will
	// most likely cause us to run out of memory.
	if( !GIsEditor && !IsTemplate( RF_ClassDefaultObject ) && GEngine )
	{
		FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
		if( AudioDevice && AudioDevice->bStartupSoundsPreCached)
		{
			// Upload the data to the hardware, but only if we've precached startup sounds already
			AudioDevice->Precache( this );
		}
		// remove bulk data if no AudioDevice is used and no sounds were initialized
		else if( IsRunningGame() )
		{
			RawData.RemoveBulkData();
		}
	}

	INC_FLOAT_STAT_BY( STAT_AudioBufferTime, Duration );
	INC_FLOAT_STAT_BY( STAT_AudioBufferTimeChannels, NumChannels * Duration );
}


void USoundWave::InitAudioResource( FByteBulkData& CompressedData )
{
	if( !ResourceSize )
	{
		// Grab the compressed vorbis data from the bulk data
		ResourceSize = CompressedData.GetBulkDataSize();
		if( ResourceSize > 0 )
		{
			check(!ResourceData);
			CompressedData.GetCopy( ( void** )&ResourceData, true );
		}
	}
}

bool USoundWave::InitAudioResource(FName Format)
{
	if( !ResourceSize )
	{
		FByteBulkData* Bulk = GetCompressedData(Format);
		if (Bulk)
		{
			ResourceSize = Bulk->GetBulkDataSize();
			check(ResourceSize > 0);
			check(!ResourceData);
			Bulk->GetCopy((void**)&ResourceData, true);
		}
	}

	return ResourceSize > 0;
}

void USoundWave::RemoveAudioResource()
{
	if(ResourceData)
	{
		FMemory::Free(ResourceData);
		ResourceSize = 0;
		ResourceData = NULL;
	}
}


bool USoundWave::IsLocalizedResource()
{
	FString FullPathName;
	bool bIsLocalised = false;

	//@todo-packageloc Handle this based on the appropriate localized package name.

	return( Super::IsLocalizedResource() || Subtitles.Num() > 0 || bIsLocalised );
}

#if WITH_EDITOR

void USoundWave::CookerWillNeverCookAgain()
{
	Super::CookerWillNeverCookAgain();
	RawData.RemoveBulkData();
	CompressedFormatData.FlushData();
}

void USoundWave::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	static FName CompressionQualityFName = FName( TEXT( "CompressionQuality" ) );
	static FName LoopableSoundFName = FName( TEXT( "bLoopableSound" ) );

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	// Regenerate on save any compressed sound formats
	if( PropertyThatChanged &&
		( PropertyThatChanged->GetFName() == CompressionQualityFName
		|| PropertyThatChanged->GetFName() == LoopableSoundFName
		) )
	{
		InvalidateCompressedData();
		MarkPackageDirty();
	}
}
#endif // WITH_EDITOR

void USoundWave::FreeResources()
{
	// Housekeeping of stats
	DEC_FLOAT_STAT_BY( STAT_AudioBufferTime, Duration );
	DEC_FLOAT_STAT_BY( STAT_AudioBufferTimeChannels, NumChannels * Duration );

	// GEngine is NULL during script compilation and GEngine->Client and its audio device might be
	// destroyed first during the exit purge.
	if( GEngine && !GExitPurge )
	{
		// Notify the audio device to free the bulk data associated with this wave.
		FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
		if (AudioDevice != NULL)
		{
			AudioDevice->FreeResource( this );
		}
	}

	NumChannels = 0;
	SampleRate = 0;
	Duration = 0.0f;
	ResourceID = 0;
	bDynamicResource = false;
}

FWaveInstance* USoundWave::HandleStart( FActiveSound& ActiveSound, const UPTRINT WaveInstanceHash )
{
	// Create a new wave instance and associate with the ActiveSound
	FWaveInstance* WaveInstance = new FWaveInstance( &ActiveSound );
	WaveInstance->WaveInstanceHash = WaveInstanceHash;
	ActiveSound.WaveInstances.Add( WaveInstance );

	// Add in the subtitle if they exist
	if (ActiveSound.bHandleSubtitles && Subtitles.Num() > 0)
	{
		// TODO - Audio Threading. This would need to be a call back to the main thread.
		if (ActiveSound.AudioComponent.IsValid() && ActiveSound.AudioComponent->OnQueueSubtitles.IsBound())
		{
			// intercept the subtitles if the delegate is set
			ActiveSound.AudioComponent->OnQueueSubtitles.ExecuteIfBound( Subtitles, Duration );
		}
		else
		{
			// otherwise, pass them on to the subtitle manager for display
			// Subtitles are hashed based on the associated sound (wave instance).
			if( ActiveSound.World.IsValid() )
			{
				FSubtitleManager::GetSubtitleManager()->QueueSubtitles( ( PTRINT )WaveInstance, ActiveSound.SubtitlePriority, bManualWordWrap, bSingleLine, Duration, Subtitles, ActiveSound.World->GetAudioTimeSeconds() );
			}
		}
	}

	return WaveInstance;
}

bool USoundWave::IsReadyForFinishDestroy()
{
	// Wait till vorbis decompression finishes before deleting resource.
	return( ( VorbisDecompressor == NULL ) || VorbisDecompressor->IsDone() );
}


void USoundWave::FinishDestroy()
{
	FreeResources();

	Super::FinishDestroy();
}

void USoundWave::Parse( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	FWaveInstance* WaveInstance = ActiveSound.FindWaveInstance( this, NodeWaveInstanceHash);

	// Create a new WaveInstance if this SoundWave doesn't already have one associated with it.
	if( WaveInstance == NULL )
	{
		if( !ActiveSound.bRadioFilterSelected )
		{
			ActiveSound.ApplyRadioFilter( AudioDevice, ParseParams );
		}

		WaveInstance = HandleStart( ActiveSound, NodeWaveInstanceHash);
	}

	// Looping sounds are never actually finished
	if (bLooping || ParseParams.bLooping)
	{
		WaveInstance->bIsFinished = false;
#if !NO_LOGGING
		if (!ActiveSound.bWarnedAboutOrphanedLooping && !ActiveSound.AudioComponent.IsValid())
		{
			UE_LOG(LogAudio, Warning, TEXT("Detected orphaned looping sound '%s'."), *ActiveSound.Sound->GetName());
			ActiveSound.bWarnedAboutOrphanedLooping = true;
		}
#endif
	}

	// Check for finished paths.
	if( !WaveInstance->bIsFinished )
	{
		// Propagate properties and add WaveInstance to outgoing array of FWaveInstances.
		WaveInstance->Volume = ParseParams.Volume * Volume;
		WaveInstance->VolumeMultiplier = ParseParams.VolumeMultiplier;
		WaveInstance->Pitch = ParseParams.Pitch * Pitch;
		WaveInstance->HighFrequencyGain = ParseParams.HighFrequencyGain;
		WaveInstance->bApplyRadioFilter = ActiveSound.bApplyRadioFilter;
		WaveInstance->StartTime = ParseParams.StartTime;

		bool bAlwaysPlay = false;

		// Properties from the sound class
		WaveInstance->SoundClass = ParseParams.SoundClass;
		if (ParseParams.SoundClass)
		{
			FSoundClassProperties* SoundClassProperties = AudioDevice->GetSoundClassCurrentProperties(ParseParams.SoundClass);
			// Use values from "parsed/ propagated" sound class properties
			WaveInstance->VolumeMultiplier *= SoundClassProperties->Volume;
			WaveInstance->Pitch *= SoundClassProperties->Pitch;
			//TODO: Add in HighFrequencyGainMultiplier property to sound classes


			WaveInstance->VoiceCenterChannelVolume = SoundClassProperties->VoiceCenterChannelVolume;
			WaveInstance->RadioFilterVolume = SoundClassProperties->RadioFilterVolume * ParseParams.VolumeMultiplier;
			WaveInstance->RadioFilterVolumeThreshold = SoundClassProperties->RadioFilterVolumeThreshold * ParseParams.VolumeMultiplier;
			WaveInstance->StereoBleed = SoundClassProperties->StereoBleed;
			WaveInstance->LFEBleed = SoundClassProperties->LFEBleed;
			
			WaveInstance->bIsUISound = ActiveSound.bIsUISound || SoundClassProperties->bIsUISound;
			WaveInstance->bIsMusic = ActiveSound.bIsMusic || SoundClassProperties->bIsMusic;
			WaveInstance->bCenterChannelOnly = ActiveSound.bCenterChannelOnly || SoundClassProperties->bCenterChannelOnly;
			WaveInstance->bEQFilterApplied = ActiveSound.bEQFilterApplied || SoundClassProperties->bApplyEffects;
			WaveInstance->bReverb = ActiveSound.bReverb || SoundClassProperties->bReverb;
			WaveInstance->OutputTarget = SoundClassProperties->OutputTarget;

			bAlwaysPlay = ActiveSound.bAlwaysPlay || SoundClassProperties->bAlwaysPlay;
		}
		else
		{
			WaveInstance->VoiceCenterChannelVolume = 0.f;
			WaveInstance->RadioFilterVolume = 0.f;
			WaveInstance->RadioFilterVolumeThreshold = 0.f;
			WaveInstance->StereoBleed = 0.f;
			WaveInstance->LFEBleed = 0.f;
			WaveInstance->bEQFilterApplied = ActiveSound.bEQFilterApplied;
			WaveInstance->bIsUISound = ActiveSound.bIsUISound;
			WaveInstance->bIsMusic = ActiveSound.bIsMusic;
			WaveInstance->bReverb = ActiveSound.bReverb;
			WaveInstance->bCenterChannelOnly = ActiveSound.bCenterChannelOnly;

			bAlwaysPlay = ActiveSound.bAlwaysPlay;
		}

		WaveInstance->PlayPriority = WaveInstance->Volume + ( bAlwaysPlay ? 1.0f : 0.0f ) + WaveInstance->RadioFilterVolume;
		WaveInstance->Location = ParseParams.Transform.GetTranslation();
		WaveInstance->bIsStarted = true;
		WaveInstance->bAlreadyNotifiedHook = false;
		WaveInstance->bUseSpatialization = ParseParams.bUseSpatialization;
		WaveInstance->WaveData = this;
		WaveInstance->NotifyBufferFinishedHooks = ParseParams.NotifyBufferFinishedHooks;
		WaveInstance->LoopingMode = ((bLooping || ParseParams.bLooping) ? LOOP_Forever : LOOP_Never);

		// Don't add wave instances that are not going to be played at this point.
		if( WaveInstance->PlayPriority > KINDA_SMALL_NUMBER )
		{
			WaveInstances.Add( WaveInstance );
		}

		// We're still alive.
		ActiveSound.bFinished = false;

		// Sanity check
		if( NumChannels > 2 && WaveInstance->bUseSpatialization && !WaveInstance->bReportedSpatializationWarning)
		{
			FString SoundWarningInfo = FString::Printf(TEXT("Spatialisation on stereo and multichannel sounds is not supported. SoundWave: %s"), *GetName());
			if (ActiveSound.Sound != this)
			{
				SoundWarningInfo += FString::Printf(TEXT(" SoundCue: %s"), *ActiveSound.Sound->GetName());
			}

			if (ActiveSound.AudioComponent.IsValid())
			{
				// TODO - Audio Threading. This log would have to be a task back to game thread
				AActor* SoundOwner = ActiveSound.AudioComponent->GetOwner();
				UE_LOG(LogAudio, Warning, TEXT( "%s Actor: %s AudioComponent: %s" ), *SoundWarningInfo, (SoundOwner ? *SoundOwner->GetName() : TEXT("None")), *ActiveSound.AudioComponent->GetName() );
			}
			else
			{
				UE_LOG(LogAudio, Warning, TEXT("%s"), *SoundWarningInfo );
			}
			WaveInstance->bReportedSpatializationWarning = true;
		}
	}
}

bool USoundWave::IsPlayable() const
{
	return true;
}

float USoundWave::GetMaxAudibleDistance()
{
	return (AttenuationSettings ? AttenuationSettings->Attenuation.GetMaxDimension() : WORLD_MAX);
}

float USoundWave::GetDuration()
{
	return ( bLooping ? INDEFINITELY_LOOPING_DURATION : Duration);
}
