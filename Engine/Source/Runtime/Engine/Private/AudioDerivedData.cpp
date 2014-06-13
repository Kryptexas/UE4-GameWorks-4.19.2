// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "EnginePrivate.h"
#include "AudioDerivedData.h"
#include "TargetPlatform.h"
#include "Sound/SoundWave.h"

DEFINE_LOG_CATEGORY_STATIC(LogAudioDerivedData, Log, All);

/*------------------------------------------------------------------------------
	Derived data key generation.
------------------------------------------------------------------------------*/

#if WITH_EDITORONLY_DATA

// If you want to bump this version, generate a new guid using
// VS->Tools->Create GUID and paste it here.
#define STREAMEDAUDIO_DERIVEDDATA_VER		TEXT("B9C1C21AEBE443B28C3B5546C8D5D742")

/**
 * Computes the derived data key suffix for a SoundWave's Streamed Audio.
 * @param SoundWave - The SoundWave for which to compute the derived data key.
 * @param AudioFormatName - The audio format we're creating the key for
 * @param OutKeySuffix - The derived data key suffix.
 */
static void GetStreamedAudioDerivedDataKeySuffix(
	const USoundWave& SoundWave,
	FName AudioFormatName,
	FString& OutKeySuffix
	)
{
	uint16 Version = 0;

	// get the version for this soundwave's platform format
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		const IAudioFormat* AudioFormat = TPM->FindAudioFormat(AudioFormatName);
		if (AudioFormat)
		{
			Version = AudioFormat->GetVersion(AudioFormatName);
		}
	}
	
	// build the key
	OutKeySuffix = FString::Printf(TEXT("%s_%d_%s"),
		*AudioFormatName.ToString(),
		Version,
		*SoundWave.CompressedDataGuid.ToString()
		);
}

/**
 * Constructs a derived data key from the key suffix.
 * @param KeySuffix - The key suffix.
 * @param OutKey - The full derived data key.
 */
static void GetStreamedAudioDerivedDataKeyFromSuffix(const FString& KeySuffix, FString& OutKey)
{
	OutKey = FDerivedDataCacheInterface::BuildCacheKey(
		TEXT("STREAMEDAUDIO"),
		STREAMEDAUDIO_DERIVEDDATA_VER,
		*KeySuffix
		);
}

/**
 * Constructs the derived data key for an individual audio chunk.
 * @param KeySuffix - The key suffix.
 * @param ChunkIndex - The chunk index.
 * @param OutKey - The full derived data key for the audio chunk.
 */
static void GetStreamedAudioDerivedChunkKey(
	int32 ChunkIndex,
	const FString& KeySuffix,
	FString& OutKey
	)
{
	OutKey = FDerivedDataCacheInterface::BuildCacheKey(
		TEXT("STREAMEDAUDIO"),
		STREAMEDAUDIO_DERIVEDDATA_VER,
		*FString::Printf(TEXT("%s_CHUNK%u"), *KeySuffix, ChunkIndex)
		);
}

/**
 * Computes the derived data key for Streamed Audio.
 * @param SoundWave - The soundwave for which to compute the derived data key.
 * @param AudioFormatName - The audio format we're creating the key for
 * @param OutKey - The derived data key.
 */
static void GetStreamedAudioDerivedDataKey(
	const USoundWave& SoundWave,
	FName AudioFormatName,
	FString& OutKey
	)
{
	FString KeySuffix;
	GetStreamedAudioDerivedDataKeySuffix(SoundWave, AudioFormatName, KeySuffix);
	GetStreamedAudioDerivedDataKeyFromSuffix(KeySuffix, OutKey);
}

/**
 * Gets Wave format for a SoundWave on the current running platform
 * @param SoundWave - The SoundWave to get format for.
 */
static FName GetWaveFormatForRunningPlatform(USoundWave& SoundWave)
{
	// Compress to whatever format the active target platform wants
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		ITargetPlatform* CurrentPlatform = NULL;
		const TArray<ITargetPlatform*>& Platforms = TPM->GetActiveTargetPlatforms();

		check(Platforms.Num());

		CurrentPlatform = Platforms[0];

		for (int32 Index = 1; Index < Platforms.Num(); Index++)
		{
			if (Platforms[Index]->IsRunningPlatform())
			{
				CurrentPlatform = Platforms[Index];
				break;
			}
		}

		check(CurrentPlatform != NULL);

		return CurrentPlatform->GetWaveFormat(&SoundWave);
	}

	return NAME_None;
}


/**
 * Stores derived data in the DDC.
 * @param DerivedData - The data to store in the DDC.
 * @param DerivedDataKeySuffix - The key suffix at which to store derived data.
 */
static void PutDerivedDataInCache(
	FStreamedAudioPlatformData* DerivedData,
	const FString& DerivedDataKeySuffix
	)
{
	TArray<uint8> RawDerivedData;
	FString DerivedDataKey;

	// Build the key with which to cache derived data.
	GetStreamedAudioDerivedDataKeyFromSuffix(DerivedDataKeySuffix, DerivedDataKey);

	FString LogString;
	if (UE_LOG_ACTIVE(LogAudio,Verbose))
	{
		LogString = FString::Printf(
			TEXT("Storing Streamed Audio in DDC:\n  Key: %s\n  Format: %s\n"),
			*DerivedDataKey,
			*DerivedData->AudioFormat.ToString()
			);
	}

	// Write out individual chunks to the derived data cache.
	const int32 ChunkCount = DerivedData->Chunks.Num();
	for (int32 ChunkIndex = 0; ChunkIndex < ChunkCount; ++ChunkIndex)
	{
		FString ChunkDerivedDataKey;
		FStreamedAudioChunk& Chunk = DerivedData->Chunks[ChunkIndex];
		GetStreamedAudioDerivedChunkKey(ChunkIndex, DerivedDataKeySuffix, ChunkDerivedDataKey);

		if (UE_LOG_ACTIVE(LogAudio,Verbose))
		{
			LogString += FString::Printf(TEXT("  Chunk%d %d bytes %s\n"),
				ChunkIndex,
				Chunk.BulkData.GetBulkDataSize(),
				*ChunkDerivedDataKey
				);
		}

		Chunk.StoreInDerivedDataCache(ChunkDerivedDataKey);
	}

	// Store derived data.
	FMemoryWriter Ar(RawDerivedData, /*bIsPersistent=*/ true);
	DerivedData->Serialize(Ar, NULL);
	GetDerivedDataCacheRef().Put(*DerivedDataKey, RawDerivedData);
	UE_LOG(LogAudio,Verbose,TEXT("%s  Derived Data: %d bytes"),*LogString,RawDerivedData.Num());
}

#endif // #if WITH_EDITORONLY_DATA

#if WITH_EDITORONLY_DATA

namespace EStreamedAudioCacheFlags
{
	enum Type
	{
		None			= 0x0,
		Async			= 0x1,
		ForceRebuild	= 0x2,
		AllowAsyncBuild	= 0x4,
		ForDDCBuild		= 0x8,
	};
};

/**
 * Worker used to cache streamed audio derived data.
 */
class FStreamedAudioCacheDerivedDataWorker : public FNonAbandonableTask
{
	/** Where to store derived data. */
	FStreamedAudioPlatformData* DerivedData;
	/** The SoundWave for which derived data is being cached. */
	USoundWave& SoundWave;
	/** Audio Format Name */
	FName AudioFormatName;
	/** Derived data key suffix. */
	FString KeySuffix;
	/** Streamed Audio cache flags. */
	uint32 CacheFlags;
	/** true if caching has succeeded. */
	bool bSucceeded;

	/** Build the streamed audio. This function is safe to call from any thread. */
	void BuildStreamedAudio()
	{
		GetStreamedAudioDerivedDataKeySuffix(SoundWave, AudioFormatName, KeySuffix);

		DerivedData->Chunks.Empty();
		
		ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
		const IAudioFormat* AudioFormat = NULL;
		if (TPM)
		{
			AudioFormat = TPM->FindAudioFormat(AudioFormatName);
		}

		if (AudioFormat)
		{
			DerivedData->AudioFormat = AudioFormatName;

			FByteBulkData* CompressedData = SoundWave.GetCompressedData(AudioFormatName);
			TArray<uint8> CompressedBuffer;
			CompressedBuffer.Empty(CompressedData->GetBulkDataSize());
			CompressedBuffer.AddUninitialized(CompressedData->GetBulkDataSize());
			void* BufferData = CompressedBuffer.GetTypedData();
			CompressedData->GetCopy(&BufferData, false);
			TArray<TArray<uint8>> ChunkBuffers;

			if (AudioFormat->SplitDataForStreaming(CompressedBuffer, ChunkBuffers))
			{
				for (int32 ChunkIndex = 0; ChunkIndex < ChunkBuffers.Num(); ++ChunkIndex)
				{
					FStreamedAudioChunk* NewChunk = new(DerivedData->Chunks) FStreamedAudioChunk();
					NewChunk->BulkData.Lock(LOCK_READ_WRITE);
					void* NewChunkData = NewChunk->BulkData.Realloc(ChunkBuffers[ChunkIndex].Num());
					FMemory::Memcpy(NewChunkData, ChunkBuffers[ChunkIndex].GetTypedData(), ChunkBuffers[ChunkIndex].Num());
					NewChunk->BulkData.Unlock();
				}
			}
			else
			{
				// Could not split so copy compressed data into a single chunk
				FStreamedAudioChunk* NewChunk = new(DerivedData->Chunks) FStreamedAudioChunk();
				NewChunk->BulkData.Lock(LOCK_READ_WRITE);
				void* NewChunkData = NewChunk->BulkData.Realloc(CompressedBuffer.Num());
				FMemory::Memcpy(NewChunkData, CompressedBuffer.GetTypedData(), CompressedBuffer.Num());
				NewChunk->BulkData.Unlock();
			}

			DerivedData->NumChunks = DerivedData->Chunks.Num();

			// Store it in the cache.
			PutDerivedDataInCache(DerivedData, KeySuffix);
		}
		
		if (DerivedData->Chunks.Num())
		{
			bSucceeded = true;
		}
		else
		{
			UE_LOG(LogAudio, Warning, TEXT("Failed to build %s derived data for %s"),
				*AudioFormatName.GetPlainNameString(),
				*SoundWave.GetPathName()
				);
		}
	}

public:
	/** Initialization constructor. */
	FStreamedAudioCacheDerivedDataWorker(
		FStreamedAudioPlatformData* InDerivedData,
		USoundWave* InSoundWave,
		FName InAudioFormatName,
		uint32 InCacheFlags
		)
		: DerivedData(InDerivedData)
		, SoundWave(*InSoundWave)
		, AudioFormatName(InAudioFormatName)
		, CacheFlags(InCacheFlags)
		, bSucceeded(false)
	{
	}

	/** Does the work to cache derived data. Safe to call from any thread. */
	void DoWork()
	{
		TArray<uint8> RawDerivedData;
		bool bForceRebuild = (CacheFlags & EStreamedAudioCacheFlags::ForceRebuild) != 0;
		bool bForDDC = (CacheFlags & EStreamedAudioCacheFlags::ForDDCBuild) != 0;
		bool bAllowAsyncBuild = (CacheFlags & EStreamedAudioCacheFlags::AllowAsyncBuild) != 0;

		if (!bForceRebuild && GetDerivedDataCacheRef().GetSynchronous(*DerivedData->DerivedDataKey, RawDerivedData))
		{
			FMemoryReader Ar(RawDerivedData, /*bIsPersistent=*/ true);
			DerivedData->Serialize(Ar, NULL);
			bSucceeded = true;
			if (bForDDC)
			{
				bSucceeded = DerivedData->TryLoadChunks(0,NULL);
			}
			else
			{
				bSucceeded = DerivedData->AreDerivedChunksAvailable();
			}
		}
		else if (bAllowAsyncBuild)
		{
			BuildStreamedAudio();
		}
	}

	/** Finalize work. Must be called ONLY by the game thread! */
	void Finalize()
	{
		check(IsInGameThread());
		if (!bSucceeded)
		{
			BuildStreamedAudio();
		}
	}

	/** Interface for FAsyncTask. */
	static const TCHAR* Name()
	{
		return TEXT("FStreamedAudioAsyncCacheDerivedDataTask");
	}
};

struct FStreamedAudioAsyncCacheDerivedDataTask : public FAsyncTask<FStreamedAudioCacheDerivedDataWorker>
{
	FStreamedAudioAsyncCacheDerivedDataTask(
		FStreamedAudioPlatformData* InDerivedData,
		USoundWave* InSoundWave,
		FName InAudioFormatName,
		uint32 InCacheFlags
		)
		: FAsyncTask<FStreamedAudioCacheDerivedDataWorker>(
			InDerivedData,
			InSoundWave,
			InAudioFormatName,
			InCacheFlags
			)
	{
	}
};

void FStreamedAudioPlatformData::Cache(USoundWave& InSoundWave, FName AudioFormatName, uint32 InFlags)
{
	// Flush any existing async task and ignore results.
	FinishCache();

	uint32 Flags = InFlags;

	static bool bForDDC = FString(FCommandLine::Get()).Contains(TEXT("DerivedDataCache"));
	if (bForDDC)
	{
		Flags |= EStreamedAudioCacheFlags::ForDDCBuild;
	}

	bool bForceRebuild = (Flags & EStreamedAudioCacheFlags::ForceRebuild) != 0;
	bool bAsync = !bForDDC && (Flags & EStreamedAudioCacheFlags::Async) != 0;
	GetStreamedAudioDerivedDataKey(InSoundWave, AudioFormatName, DerivedDataKey);

	if (bAsync && !bForceRebuild)
	{
		AsyncTask = new FStreamedAudioAsyncCacheDerivedDataTask(this, &InSoundWave, AudioFormatName, Flags);
		AsyncTask->StartBackgroundTask();
	}
	else
	{
		FStreamedAudioCacheDerivedDataWorker Worker(this, &InSoundWave, AudioFormatName, Flags);
		Worker.DoWork();
		Worker.Finalize();
	}
}

void FStreamedAudioPlatformData::FinishCache()
{
	if (AsyncTask)
	{
		{
			AsyncTask->EnsureCompletion();
		}
		FStreamedAudioCacheDerivedDataWorker& Worker = AsyncTask->GetTask();
		Worker.Finalize();
		delete AsyncTask;
		AsyncTask = NULL;
	}
}

/**
 * Executes async DDC gets for chunks stored in the derived data cache.
 * @param Chunks - Chunks to retrieve.
 * @param FirstChunkToLoad - Index of the first chunk to retrieve.
 * @param OutHandles - Handles to the asynchronous DDC gets.
 */
static void BeginLoadDerivedChunks(TIndirectArray<FStreamedAudioChunk>& Chunks, int32 FirstChunkToLoad, TArray<uint32>& OutHandles)
{
	FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();
	OutHandles.AddZeroed(Chunks.Num());
	for (int32 ChunkIndex = FirstChunkToLoad; ChunkIndex < Chunks.Num(); ++ChunkIndex)
	{
		const FStreamedAudioChunk& Chunk = Chunks[ChunkIndex];
		if (!Chunk.DerivedDataKey.IsEmpty())
		{
			OutHandles[ChunkIndex] = DDC.GetAsynchronous(*Chunk.DerivedDataKey);
		}
	}
}

#endif //WITH_EDITORONLY_DATA

FStreamedAudioPlatformData::FStreamedAudioPlatformData()
#if WITH_EDITORONLY_DATA
: AsyncTask(NULL)
#endif // #if WITH_EDITORONLY_DATA
{
}

FStreamedAudioPlatformData::~FStreamedAudioPlatformData()
{
#if WITH_EDITORONLY_DATA
	if (AsyncTask)
	{
		AsyncTask->EnsureCompletion();
		delete AsyncTask;
		AsyncTask = NULL;
	}
#endif
}

bool FStreamedAudioPlatformData::TryLoadChunks(int32 FirstChunkToLoad, void** OutChunkData)
{
	int32 NumChunksCached = 0;

#if WITH_EDITORONLY_DATA
	TArray<uint8> TempData;
	TArray<uint32> AsyncHandles;
	FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();
	BeginLoadDerivedChunks(Chunks, FirstChunkToLoad, AsyncHandles);
#endif // #if WITH_EDITORONLY_DATA

	// Load remaining chunks (if any) from bulk data.
	for (int32 ChunkIndex = FirstChunkToLoad; ChunkIndex < Chunks.Num(); ++ChunkIndex)
	{
		FStreamedAudioChunk& Chunk = Chunks[ChunkIndex];
		if (Chunk.BulkData.GetBulkDataSize() > 0)
		{
			if (OutChunkData)
			{
				OutChunkData[ChunkIndex - FirstChunkToLoad] = FMemory::Malloc(Chunk.BulkData.GetBulkDataSize());
				Chunk.BulkData.GetCopy(&OutChunkData[ChunkIndex - FirstChunkToLoad]);
			}
			NumChunksCached++;
		}
	}

#if WITH_EDITORONLY_DATA
	// Wait for async DDC gets.
	for (int32 ChunkIndex = FirstChunkToLoad; ChunkIndex < Chunks.Num(); ++ChunkIndex)
	{
		FStreamedAudioChunk& Chunk = Chunks[ChunkIndex];
		if (Chunk.DerivedDataKey.IsEmpty() == false)
		{
			uint32 AsyncHandle = AsyncHandles[ChunkIndex];
			DDC.WaitAsynchronousCompletion(AsyncHandle);
			if (DDC.GetAsynchronousResults(AsyncHandle, TempData))
			{
				FMemoryReader Ar(TempData, /*bIsPersistent=*/ true);
				NumChunksCached++;

				if (OutChunkData)
				{
					OutChunkData[ChunkIndex - FirstChunkToLoad] = FMemory::Malloc(TempData.Num());
					Ar.Serialize(OutChunkData[ChunkIndex - FirstChunkToLoad], TempData.Num());
				}
			}
			TempData.Reset();
		}
	}
#endif // #if WITH_EDITORONLY_DATA

	if (NumChunksCached != (Chunks.Num() - FirstChunkToLoad))
	{
		// Unable to cache all chunks. Release memory for those that were cached.
		for (int32 ChunkIndex = FirstChunkToLoad; ChunkIndex < Chunks.Num(); ++ChunkIndex)
		{
			if (OutChunkData && OutChunkData[ChunkIndex - FirstChunkToLoad])
			{
				FMemory::Free(OutChunkData[ChunkIndex - FirstChunkToLoad]);
				OutChunkData[ChunkIndex - FirstChunkToLoad] = NULL;
			}
		}
		return false;
	}

	return true;
}

#if WITH_EDITORONLY_DATA
bool FStreamedAudioPlatformData::AreDerivedChunksAvailable() const
{
	bool bChunksAvailable = true;
	FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();
	for (int32 ChunkIndex = 0; bChunksAvailable && ChunkIndex < Chunks.Num(); ++ChunkIndex)
	{
		const FStreamedAudioChunk& Chunk = Chunks[ChunkIndex];
		if (Chunk.DerivedDataKey.IsEmpty() == false)
		{
			bChunksAvailable = DDC.CachedDataProbablyExists(*Chunk.DerivedDataKey);
		}
	}
	return bChunksAvailable;
}
#endif // #if WITH_EDITORONLY_DATA

void FStreamedAudioPlatformData::Serialize(FArchive& Ar, USoundWave* Owner)
{
	Ar << NumChunks;
	Ar << AudioFormat;

	if (Ar.IsLoading())
	{
		Chunks.Empty(NumChunks);
		for (int32 ChunkIndex = 0; ChunkIndex < NumChunks; ++ChunkIndex)
		{
			new(Chunks) FStreamedAudioChunk();
		}
	}
	for (int32 ChunkIndex = 0; ChunkIndex < NumChunks; ++ChunkIndex)
	{
		Chunks[ChunkIndex].Serialize(Ar, Owner, ChunkIndex);
	}
}

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

FString FDerivedAudioDataCompressor::GetPluginSpecificCacheKeySuffix() const
{
	int32 FormatVersion = 0xffff; // if the compressor is NULL, this will be used as the version...and in that case we expect everything to fail anyway
	if (Compressor)
	{
		FormatVersion = (int32)Compressor->GetVersion(Format);
	}
	FString FormatString = Format.ToString().ToUpper();
	check(SoundNode->CompressedDataGuid.IsValid());
	return FString::Printf(TEXT("%s_%04X_%s"), *FormatString, FormatVersion, *SoundNode->CompressedDataGuid.ToString());
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

/*---------------------------------------
	USoundWave Derived Data functions
---------------------------------------*/

void USoundWave::CleanupCachedRunningPlatformData()
{
	if (RunningPlatformData != NULL)
	{
		delete RunningPlatformData;
		RunningPlatformData = NULL;
	}
}

void USoundWave::CleanupCachedCookedPlatformData()
{
	for (auto It : CookedPlatformData)
	{
		delete It.Value;
	}

	CookedPlatformData.Empty();
}

void USoundWave::SerializeCookedPlatformData(FArchive& Ar)
{
	if (IsTemplate())
	{
		return;
	}

#if WITH_EDITORONLY_DATA
	if (Ar.IsCooking() && Ar.IsPersistent())
	{
		if (!Ar.CookingTarget()->IsServerOnly())
		{
			FName PlatformFormat = Ar.CookingTarget()->GetWaveFormat(this);
			FString DerivedDataKey;
			GetStreamedAudioDerivedDataKey(*this, PlatformFormat, DerivedDataKey);

			FStreamedAudioPlatformData *PlatformDataToSave = CookedPlatformData.FindRef(DerivedDataKey);

			if (PlatformDataToSave == NULL)
			{
				PlatformDataToSave = new FStreamedAudioPlatformData();
				PlatformDataToSave->Cache(*this, PlatformFormat, EStreamedAudioCacheFlags::Async);

				CookedPlatformData.Add(DerivedDataKey, PlatformDataToSave);
			}

			PlatformDataToSave->FinishCache();
			PlatformDataToSave->Serialize(Ar, this);
		}
	}
	else
#endif // #if WITH_EDITORONLY_DATA
	{
		CleanupCachedRunningPlatformData();
		check(RunningPlatformData == NULL);

		RunningPlatformData = new FStreamedAudioPlatformData();
		RunningPlatformData->Serialize(Ar, this);
	}
}

#if WITH_EDITORONLY_DATA
void USoundWave::CachePlatformData(bool bAsyncCache)
{
	FString DerivedDataKey;
	FName AudioFormat = GetWaveFormatForRunningPlatform(*this);
	GetStreamedAudioDerivedDataKey(*this, AudioFormat, DerivedDataKey);

	if (RunningPlatformData == NULL || RunningPlatformData->DerivedDataKey != DerivedDataKey)
	{
		if (RunningPlatformData == NULL)
		{
			RunningPlatformData = new FStreamedAudioPlatformData();
		}
		RunningPlatformData->Cache(*this, AudioFormat, bAsyncCache ? EStreamedAudioCacheFlags::Async : EStreamedAudioCacheFlags::None);
	}
}

void USoundWave::BeginCachePlatformData()
{
	CachePlatformData(true);

	// enable caching in postload for derived data cache commandlet and cook by the book
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM && (TPM->RestrictFormatsToRuntimeOnly() == false))
	{
		ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
		TArray<ITargetPlatform*> Platforms = TPM->GetActiveTargetPlatforms();
		// Cache for all the audio formats that the cooking target requires
		for (int32 FormatIndex = 0; FormatIndex < Platforms.Num(); FormatIndex++)
		{
			BeginCacheForCookedPlatformData(Platforms[FormatIndex]);
		}
	}
}

void USoundWave::BeginCacheForCookedPlatformData(const ITargetPlatform *TargetPlatform)
{
	if (IsStreaming())
	{
		// Retrieve format to cache for targetplatform.
		FName PlatformFormat = TargetPlatform->GetWaveFormat(this);

		uint32 CacheFlags = EStreamedAudioCacheFlags::Async;

		// If source data is resident in memory then allow the streamed audio to be built
		// in a background thread.
		if (GetCompressedData(PlatformFormat)->IsBulkDataLoaded())
		{
			CacheFlags |= EStreamedAudioCacheFlags::AllowAsyncBuild;
		}

		// find format data by comparing derived data keys.
		FString DerivedDataKey;
		GetStreamedAudioDerivedDataKeySuffix(*this, PlatformFormat, DerivedDataKey);

		FStreamedAudioPlatformData *PlatformData = CookedPlatformData.FindRef(DerivedDataKey);

		if (PlatformData == NULL)
		{
			PlatformData = new FStreamedAudioPlatformData();
			PlatformData->Cache(
				*this,
				PlatformFormat,
				CacheFlags
				);
			CookedPlatformData.Add(DerivedDataKey, PlatformData);
		}
	}

	Super::BeginCacheForCookedPlatformData(TargetPlatform);
}

void USoundWave::FinishCachePlatformData()
{
	if (RunningPlatformData == NULL)
	{
		// begin cache never called
		CachePlatformData();
	}
	else
	{
		// make sure async requests are finished
		RunningPlatformData->FinishCache();
	}

#if DO_CHECK
	FString DerivedDataKey;
	FName AudioFormat = GetWaveFormatForRunningPlatform(*this);
	GetStreamedAudioDerivedDataKey(*this, AudioFormat, DerivedDataKey);

	check(RunningPlatformData->DerivedDataKey == DerivedDataKey);
#endif
}
#endif //WITH_EDITORONLY_DATA