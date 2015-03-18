// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "EnginePrivate.h"
#include "VisualLogger/VisualLoggerCircularBufferDevice.h"

#if ENABLE_VISUAL_LOG

FVisualLoggerCircularBufferDevice::FVisualLoggerCircularBufferDevice()
{
	Cleanup();

	int32 DefaultCircularBufferSize = 0;
	GConfig->GetInt(TEXT("VisualLogger"), TEXT("CircularBufferSize"), DefaultCircularBufferSize, GEngineIni);
	CircularBufferSize = DefaultCircularBufferSize;

	bool DefaultFrameCacheLenght = 0;
	GConfig->GetBool(TEXT("VisualLogger"), TEXT("FrameCacheLenght"), DefaultFrameCacheLenght, GEngineIni);
	FrameCacheLenght = DefaultFrameCacheLenght;

	bool UseCompression = false;
	GConfig->GetBool(TEXT("VisualLogger"), TEXT("UseCompression"), UseCompression, GEngineIni);
	bUseCompression = UseCompression;

	FrameCache.Reserve(CircularBufferSize);
}

void FVisualLoggerCircularBufferDevice::Cleanup(bool bReleaseMemory)
{
	if (bReleaseMemory)
	{
		FrameCache.Empty();
	}
	else
	{
		FrameCache.Reset();
	}
}

void FVisualLoggerCircularBufferDevice::Serialize(const class UObject* LogOwner, FName OwnerName, FName OwnerClassName, const FVisualLogEntry& LogEntry)
{
	if (CircularBufferSize <= 0)
	{
		return;
	}

	if (FrameCache.Num() >= CircularBufferSize)
	{
		FrameCache.RemoveAt(0, FrameCache.Num() - CircularBufferSize + 1, false);
	}
	FrameCache.Add(FVisualLogEntryItem(OwnerName, OwnerClassName, LogEntry));
}

void FVisualLoggerCircularBufferDevice::DumpBuffer()
{
	if (FrameCache.Num() && CircularBufferSize > 0)
	{
		FString TempFileName = FVisualLoggerHelpers::GenerateTemporaryFilename(VISLOG_FILENAME_EXT);
		const FString FullFilename = FString::Printf(TEXT("%slogs/%s"), *FPaths::GameSavedDir(), *TempFileName);

		FArchive* FileArchive = IFileManager::Get().CreateFileWriter(*FullFilename);
		FVisualLoggerHelpers::Serialize(*FileArchive, FrameCache);
		FileArchive->Close();
		delete FileArchive;
		FileArchive = NULL;

		const FString MapName = GWorld.GetReference() ? GWorld->GetMapName() : TEXT("");
		FString OutputFileName = FString::Printf(TEXT("%s_%s"), TEXT("VisualLog"), *MapName);

		const FString NewFileName = FVisualLoggerHelpers::GenerateFilename(TempFileName, OutputFileName, FrameCache[0].Entry.TimeStamp, FrameCache[FrameCache.Num()-1].Entry.TimeStamp);

		// rename file when we serialized some data
		IFileManager::Get().Move(*NewFileName, *FullFilename, true);
	}
}

#endif
