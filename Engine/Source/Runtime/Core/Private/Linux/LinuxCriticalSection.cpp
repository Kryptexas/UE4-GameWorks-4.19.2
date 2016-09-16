// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "Linux/LinuxCriticalSection.h"
#include "LinuxPlatformRunnableThread.h"
#include "EngineVersion.h"
#include <spawn.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/ioctl.h> // ioctl
#include <sys/file.h>
#include <asm/ioctls.h> // FIONREAD
#include <sys/file.h> // flock
#include "LinuxApplication.h" // FLinuxApplication::IsForeground()

FLinuxSystemWideCriticalSection::FLinuxSystemWideCriticalSection(const FString& InName, FTimespan InTimeout)
{
	check(InName.Len() > 0)
	check(InTimeout >= FTimespan::Zero())
	check(InTimeout.GetTotalSeconds() < (double)FLT_MAX)

	const FString LockPath = FString(FLinuxPlatformProcess::ApplicationSettingsDir()) / InName;
	FString NormalizedFilepath(LockPath);
	NormalizedFilepath.ReplaceInline(TEXT("\\"), TEXT("/"));

	// Attempt to open a file and then lock with flock (NOTE: not an atomic operation, but best we can do)
	FileHandle = open(TCHAR_TO_UTF8(*NormalizedFilepath), O_CREAT | O_WRONLY | O_NONBLOCK, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (FileHandle == -1 && InTimeout != FTimespan::Zero())
	{
		FDateTime ExpireTime = FDateTime::UtcNow() + InTimeout;
		const float RetrySeconds = FMath::Min((float)InTimeout.GetTotalSeconds(), 0.25f);

		do
		{
			// retry until timeout
			FLinuxPlatformProcess::Sleep(RetrySeconds);
			FileHandle = open(TCHAR_TO_UTF8(*NormalizedFilepath), O_CREAT | O_WRONLY | O_NONBLOCK, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		} while (FileHandle == -1 && FDateTime::UtcNow() < ExpireTime);
	}
	if (FileHandle != -1)
	{
		flock(FileHandle, LOCK_EX);
	}
}

FLinuxSystemWideCriticalSection::~FLinuxSystemWideCriticalSection()
{
	Release();
}

bool FLinuxSystemWideCriticalSection::IsValid() const
{
	return FileHandle != -1;
}

void FLinuxSystemWideCriticalSection::Release()
{
	if (IsValid())
	{
		flock(FileHandle, LOCK_UN);
		close(FileHandle);
		FileHandle = -1;
	}
}
