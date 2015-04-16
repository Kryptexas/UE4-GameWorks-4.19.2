// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SoundPreviewThread.h"
#include "SoundDefinitions.h"
#include "Sound/SoundWave.h"
#include "TargetPlatform.h"
#include "Dialogs/DlgSoundWaveOptions.h"

//////////////////////////////////////////////////////////////////////////
// FSoundPreviewThread

FSoundPreviewThread::FSoundPreviewThread(int32 PreviewCount, USoundWave* Wave, FPreviewInfo* Info)
	: Count(PreviewCount)
	, SoundWave(Wave)
	, PreviewInfo(Info)
	, TaskFinished(false)
	, CancelCalculations(false)
{
}

bool FSoundPreviewThread::Init()
{
	return true;
}

uint32 FSoundPreviewThread::Run()
{
	// Get the original wave
	SoundWave->RemoveAudioResource();
	SoundWave->InitAudioResource(SoundWave->RawData);

	for (Index = 0; Index < Count && !CancelCalculations; Index++)
	{
		SoundWaveQualityPreview(SoundWave, PreviewInfo + Index);
	}

	SoundWave->RemoveAudioResource();
	TaskFinished = true;
	return 0;
}

void FSoundPreviewThread::Stop()
{
	CancelCalculations = true;
}

void FSoundPreviewThread::Exit()
{
}

void FSoundPreviewThread::SoundWaveQualityPreview(USoundWave* SoundWave, FPreviewInfo* PreviewInfo)
{
	FWaveModInfo WaveInfo;
	FSoundQualityInfo QualityInfo = { 0 };

	// Extract the info from the wave header
	if (!WaveInfo.ReadWaveHeader((uint8*)SoundWave->ResourceData, SoundWave->ResourceSize, 0))
	{
		return;
	}

	SoundWave->NumChannels = *WaveInfo.pChannels;
	SoundWave->RawPCMDataSize = WaveInfo.SampleDataSize;

	// Extract the stats
	PreviewInfo->OriginalSize = SoundWave->ResourceSize;
	PreviewInfo->OggVorbisSize = 0;
	PreviewInfo->PS3Size = 0;
	PreviewInfo->XMASize = 0;

	QualityInfo.Quality = PreviewInfo->QualitySetting;
	QualityInfo.NumChannels = *WaveInfo.pChannels;
	QualityInfo.SampleRate = SoundWave->SampleRate;
	QualityInfo.SampleDataSize = WaveInfo.SampleDataSize;
	QualityInfo.DebugName = SoundWave->GetFullName();

	// PCM -> Vorbis -> PCM 
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	static FName NAME_OGG(TEXT("OGG"));
	const IAudioFormat* Compressor = TPM.FindAudioFormat(NAME_OGG); // why is this ogg specific?
	PreviewInfo->OggVorbisSize = 0;

	if (Compressor)
	{
		TArray<uint8> Input;
		Input.AddUninitialized(WaveInfo.SampleDataSize);
		FMemory::Memcpy(Input.GetData(), WaveInfo.SampleDataStart, Input.Num());
		TArray<uint8> Output;
		Compressor->Recompress(NAME_OGG, Input, QualityInfo, Output);
		if (Output.Num())
		{
			PreviewInfo->OggVorbisSize = Output.Num();
			PreviewInfo->DecompressedOggVorbis = (uint8*)FMemory::Malloc(Output.Num());
			FMemory::Memcpy(PreviewInfo->DecompressedOggVorbis, Output.GetData(), Output.Num());
		}
	}

	if (PreviewInfo->OggVorbisSize <= 0)
	{
		UE_LOG(LogAudio, Log, TEXT("PC decompression failed"));
	}
}
