// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OutputDeviceMemory.cpp: Ring buffer (memory only) output device
=============================================================================*/

#include "CorePrivatePCH.h"
#include "HAL/OutputDeviceMemory.h"

#define DUMP_LOG_ON_EXIT (!NO_LOGGING && PLATFORM_DESKTOP && (!UE_BUILD_SHIPPING || USE_LOGGING_IN_SHIPPING))

FOutputDeviceMemory::FOutputDeviceMemory(int32 InBufferSize /*= 1024 * 1024*/)
: BufferStartPos(0)
, BufferLength(0)
{
#if DUMP_LOG_ON_EXIT
	const FString LogFileName = FPlatformOutputDevices::GetAbsoluteLogFilename();
	FOutputDeviceFile::CreateBackupCopy(*LogFileName);
	IFileManager::Get().Delete(*LogFileName);
#endif // DUMP_LOG_ON_EXIT

	Buffer.AddUninitialized(InBufferSize);
	Logf(TEXT("Log file open, %s"), FPlatformTime::StrTimestamp());
}

void FOutputDeviceMemory::TearDown() 
{
	Logf(TEXT("Log file closed, %s"), FPlatformTime::StrTimestamp());

	// Dump on exit
#if DUMP_LOG_ON_EXIT
	const FString LogFileName = FPlatformOutputDevices::GetAbsoluteLogFilename();
	FArchive* LogFile = IFileManager::Get().CreateFileWriter(*LogFileName, FILEWRITE_AllowRead);
	if (LogFile)
	{
		Dump(*LogFile);
		LogFile->Flush();
		delete LogFile;
	}
#endif // DUMP_LOG_ON_EXIT
}

void FOutputDeviceMemory::Flush()
{
	// Do nothing, buffer is always flushed
}

void FOutputDeviceMemory::Serialize(const TCHAR* Data, ELogVerbosity::Type Verbosity, const class FName& Category, const double Time)
{
#if !NO_LOGGING
	FormatAndSerialize(Data, Verbosity, Category, Time);
#endif
}

void FOutputDeviceMemory::Serialize(const TCHAR* Data, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	Serialize(Data, Verbosity, Category, -1.0);
}

void FOutputDeviceMemory::SerializeToBuffer(ANSICHAR* Data, int32 Length)
{	
	const int32 BufferCapacity = Buffer.Num(); // Never changes
	// Given the size of the buffer (usually 1MB) this should never happen
	check(Length <= BufferCapacity);

	int32 WritePos = 0;
	{
		// We only need to lock long enough to update all state variables. Copy doesn't need a lock, at least for large buffers.
		FScopeLock WriteLock(&BufferPosCritical);
		WritePos = (BufferStartPos + BufferLength) % BufferCapacity;
		BufferStartPos = (BufferLength + Length) > BufferCapacity ? ((BufferStartPos + Length) % BufferCapacity) : BufferStartPos;
		BufferLength = FMath::Min(BufferLength + Length, BufferCapacity);
	}

	if ((WritePos + Length) <= BufferCapacity)
	{
		FMemory::Memcpy(Buffer.GetData() + WritePos, Data, Length * sizeof(ANSICHAR));
	}
	else
	{
		const int32 ChunkALength = BufferCapacity - WritePos;
		FMemory::Memcpy(Buffer.GetData() + WritePos, Data, ChunkALength * sizeof(ANSICHAR));
		const int32 ChunkBLength = Length - ChunkALength;
		FMemory::Memcpy(Buffer.GetData(), Data + ChunkALength, ChunkBLength * sizeof(ANSICHAR));
	}
}

void FOutputDeviceMemory::FormatAndSerialize(const TCHAR* Data, ELogVerbosity::Type Verbosity, const class FName& Category, const double Time)
{
	if (!bSuppressEventTag)
	{
		FString Prefix = FOutputDevice::FormatLogLine(Verbosity, Category, NULL, GPrintLogTimes, Time);
		CastAndSerializeToBuffer(*Prefix);
	}

	CastAndSerializeToBuffer(Data);

	if (bAutoEmitLineTerminator)
	{
#if PLATFORM_LINUX
		// on Linux, we still want to have logs with Windows line endings so they can be opened with Windows tools like infamous notepad.exe
		static ANSICHAR WindowsTerminator[] = { '\r', '\n' };
		SerializeToBuffer((ANSICHAR*)WindowsTerminator, sizeof(WindowsTerminator));
#else
		CastAndSerializeToBuffer(LINE_TERMINATOR);
#endif // PLATFORM_LINUX
	}
}

void FOutputDeviceMemory::CastAndSerializeToBuffer(const TCHAR* Data)
{
	FTCHARToUTF8 ConvertedData(Data);
	if (ConvertedData.Length())
	{
		SerializeToBuffer((ANSICHAR*)ConvertedData.Get(), ConvertedData.Length());
	}
}

void FOutputDeviceMemory::Dump(FArchive& Ar)
{
	Ar.Serialize(Buffer.GetData() + BufferStartPos, (BufferLength - BufferStartPos) * sizeof(ANSICHAR));
	Ar.Serialize(Buffer.GetData(), BufferStartPos * sizeof(ANSICHAR));
}