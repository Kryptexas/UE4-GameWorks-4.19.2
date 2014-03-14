// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#include "AudioDerivedData.h"
#include "TargetPlatform.h"

DEFINE_LOG_CATEGORY_STATIC(LogAudioDerivedData, Log, All);

/**
 * Helper class to display a status update message in the editor.
 */
class FAudioStatusMessageContext
{
public:

	/**
	 * Updates the status message displayed to the user.
	 */
	explicit FAudioStatusMessageContext( const FText& InMessage )
	{
		if ( GIsEditor && !IsRunningCommandlet() )
		{
			GWarn->PushStatus();
			GWarn->StatusUpdate(-1, -1, InMessage);
		}
		DEFINE_LOG_CATEGORY_STATIC(LogAudioDerivedData, Log, All);
		UE_LOG(LogAudioDerivedData, Display, TEXT("%s"), *InMessage.ToString());
	}

	/**
	 * Ensures the status context is popped off the stack.
	 */
	~FAudioStatusMessageContext()
	{
		if ( GIsEditor && !IsRunningCommandlet() )
		{
			GWarn->PopStatus();
		}
	}
};

/**
 * Cook a simple mono or stereo wave
 */
static void CookSimpleWave(USoundWave* SoundWave, FName FormatName, const IAudioFormat& Format, TArray<uint8>& Output)
{
	FWaveModInfo WaveInfo;
	TArray<uint8> Input;
	check(!Output.Num());

	bool bWasLocked = false;

	// check if there is any raw sound data
	if( SoundWave->RawData.GetBulkDataSize() > 0 )
	{
		// Lock raw wave data.
		uint8* RawWaveData = ( uint8 * )SoundWave->RawData.Lock( LOCK_READ_ONLY );
		bWasLocked = true;
		int32 RawDataSize = SoundWave->RawData.GetBulkDataSize();

		// parse the wave data
		if( !WaveInfo.ReadWaveHeader( RawWaveData, RawDataSize, 0 ) )
		{
			UE_LOG(LogAudioDerivedData, Warning, TEXT( "Only mono or stereo 16 bit waves allowed: %s (%d bytes)" ), *SoundWave->GetFullName(), RawDataSize );
		}
		else
		{
			Input.AddUninitialized(WaveInfo.SampleDataSize);
			FMemory::Memcpy(Input.GetTypedData(), WaveInfo.SampleDataStart, WaveInfo.SampleDataSize);
		}
	}

	if(!Input.Num())
	{
		UE_LOG(LogAudioDerivedData, Warning, TEXT( "Can't cook %s because there is no source compressed or uncompressed PC sound data" ), *SoundWave->GetFullName() );
	}
	else
	{
		FSoundQualityInfo QualityInfo = { 0 };

		QualityInfo.Quality = SoundWave->CompressionQuality;
		QualityInfo.NumChannels = *WaveInfo.pChannels;
		QualityInfo.SampleRate = *WaveInfo.pSamplesPerSec;
		QualityInfo.SampleDataSize = Input.Num();
		QualityInfo.bLoopableSound = SoundWave->bLoopableSound;
		QualityInfo.DebugName = SoundWave->GetFullName();

		// Cook the data.
		if(Format.Cook(FormatName, Input, QualityInfo, Output)) 
		{
			//@todo tighten up the checking for empty results here
			if (SoundWave->SampleRate != *WaveInfo.pSamplesPerSec)
			{
				UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->SampleRate during cooking %s." ), *SoundWave->GetFullName() );
				SoundWave->SampleRate = *WaveInfo.pSamplesPerSec;
			}
			if (SoundWave->NumChannels != *WaveInfo.pChannels)
			{
				UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->NumChannels during cooking %s." ), *SoundWave->GetFullName() );
				SoundWave->NumChannels = *WaveInfo.pChannels;
			}
			if (SoundWave->RawPCMDataSize != Input.Num())
			{
				UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->RawPCMDataSize during cooking %s." ), *SoundWave->GetFullName() );
				SoundWave->RawPCMDataSize = Input.Num();
			}
			if (SoundWave->Duration != ( float )SoundWave->RawPCMDataSize / (SoundWave->SampleRate * sizeof( int16 ) * SoundWave->NumChannels))
			{
				UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->Duration during cooking %s." ), *SoundWave->GetFullName() );
				SoundWave->Duration = ( float )SoundWave->RawPCMDataSize / (SoundWave->SampleRate * sizeof( int16 ) * SoundWave->NumChannels);
			}
		}
	}
	if (bWasLocked)
	{
		SoundWave->RawData.Unlock();
	}
}

/**
 * Cook a multistream (normally 5.1) wave
 */
void CookSurroundWave( USoundWave* SoundWave, FName FormatName, const IAudioFormat& Format, TArray<uint8>& Output)
{
	check(!Output.Num());
#if WITH_EDITORONLY_DATA
	int32						i, ChannelCount;
	uint32					SampleDataSize;
	FWaveModInfo			WaveInfo;
	TArray<TArray<uint8> >	SourceBuffers;

	uint8* RawWaveData = ( uint8 * )SoundWave->RawData.Lock( LOCK_READ_ONLY );

	// Front left channel is the master
	ChannelCount = 1;
	checkAtCompileTime(SPEAKER_FrontLeft == 0, frontleft_must_be_first);
	if( WaveInfo.ReadWaveHeader( RawWaveData, SoundWave->ChannelSizes[ SPEAKER_FrontLeft ], SoundWave->ChannelOffsets[ SPEAKER_FrontLeft ] ) )
	{
		{
			TArray<uint8>& Input = *new (SourceBuffers) TArray<uint8>;
			Input.AddUninitialized(WaveInfo.SampleDataSize);
			FMemory::Memcpy(Input.GetTypedData(), WaveInfo.SampleDataStart, WaveInfo.SampleDataSize);
		}

		SampleDataSize = WaveInfo.SampleDataSize;
		// Extract all the info for the other channels (may be blank)
		for( i = 1; i < SPEAKER_Count; i++ )
		{
			FWaveModInfo WaveInfoInner;
			if( WaveInfoInner.ReadWaveHeader( RawWaveData, SoundWave->ChannelSizes[ i ], SoundWave->ChannelOffsets[ i ] ) )
			{
				// Only mono files allowed
				if( *WaveInfoInner.pChannels == 1 )
				{
					ChannelCount++;
					TArray<uint8>& Input = *new (SourceBuffers) TArray<uint8>;
					Input.AddUninitialized(WaveInfoInner.SampleDataSize);
					FMemory::Memcpy(Input.GetTypedData(), WaveInfoInner.SampleDataStart, WaveInfoInner.SampleDataSize);
					SampleDataSize = FMath::Min<uint32>(WaveInfoInner.SampleDataSize, SampleDataSize);
				}
			}
		}
	
		// Only allow the formats that can be played back through
		if( ChannelCount == 4 || ChannelCount == 6 || ChannelCount == 7 || ChannelCount == 8 )
		{
			UE_LOG(LogAudioDerivedData, Log,  TEXT( "Cooking %d channels for: %s" ), ChannelCount, *SoundWave->GetFullName() );

			FSoundQualityInfo QualityInfo = { 0 };

			QualityInfo.Quality = SoundWave->CompressionQuality;
			QualityInfo.NumChannels = ChannelCount;
			QualityInfo.SampleRate = *WaveInfo.pSamplesPerSec;
			QualityInfo.SampleDataSize = SampleDataSize;
			QualityInfo.bLoopableSound = SoundWave->bLoopableSound;
			QualityInfo.DebugName = SoundWave->GetFullName();
			//@todo tighten up the checking for empty results here
			if(Format.CookSurround(FormatName, SourceBuffers, QualityInfo, Output)) 
			{
				if (SoundWave->SampleRate != *WaveInfo.pSamplesPerSec)
				{
					UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->SampleRate during cooking %s." ), *SoundWave->GetFullName() );
					SoundWave->SampleRate = *WaveInfo.pSamplesPerSec;
				}
				if (SoundWave->NumChannels != ChannelCount)
				{
					UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->NumChannels during cooking %s." ), *SoundWave->GetFullName() );
					SoundWave->NumChannels = ChannelCount;
				}
				if (SoundWave->RawPCMDataSize != SampleDataSize * ChannelCount)
				{
					UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->RawPCMDataSize during cooking %s." ), *SoundWave->GetFullName() );
					SoundWave->RawPCMDataSize = SampleDataSize * ChannelCount;
				}
				if (SoundWave->Duration != ( float )SampleDataSize / (SoundWave->SampleRate * sizeof( int16 )))
				{
					UE_LOG(LogAudioDerivedData, Warning, TEXT( "Updated SoundWave->Duration during cooking %s." ), *SoundWave->GetFullName() );
					SoundWave->Duration = ( float )SampleDataSize / (SoundWave->SampleRate * sizeof( int16 ));
				}			
			}
		}
		else
		{
			UE_LOG(LogAudioDerivedData, Warning, TEXT( "No format available for a %d channel surround sound: %s" ), ChannelCount, *SoundWave->GetFullName() );
		}
	}
	else
	{
		UE_LOG(LogAudioDerivedData, Warning, TEXT( "Cooking surround sound failed: %s" ), *SoundWave->GetPathName() );
	}
	SoundWave->RawData.Unlock();
#endif
}

FDerivedAudioDataCompressor::FDerivedAudioDataCompressor(USoundWave* InSoundNode, FName InFormat)
	: SoundNode(InSoundNode)
	, Format(InFormat)
	, Compressor(NULL)
{
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		Compressor = TPM->FindAudioFormat(InFormat);
	}
}

bool FDerivedAudioDataCompressor::Build(TArray<uint8>& OutData) 
{
#if WITH_EDITORONLY_DATA
	if (!Compressor)
	{
		UE_LOG(LogAudioDerivedData, Warning, TEXT( "Could not find audio format to cook: %s" ), *Format.ToString());
		return false;
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("AudioFormat"), FText::FromName( Format ) );
	Args.Add( TEXT("SoundNodeName"), FText::FromString( SoundNode->GetName() ) );
	FAudioStatusMessageContext StatusMessage( FText::Format( NSLOCTEXT("Engine", "BuildingCompressedAudioTaskStatus", "Building compressed audio format {AudioFormat} wave {SoundNodeName}..."), Args ) );		

	if (!SoundNode->ChannelSizes.Num())
	{
		check(!SoundNode->ChannelOffsets.Num());
		CookSimpleWave(SoundNode, Format, *Compressor, OutData);
	}
	else
	{
		check(SoundNode->ChannelOffsets.Num() == SPEAKER_Count);
		check(SoundNode->ChannelSizes.Num() == SPEAKER_Count);
		CookSurroundWave(SoundNode, Format, *Compressor, OutData);
	}
#endif
	return OutData.Num() > 0;
}

