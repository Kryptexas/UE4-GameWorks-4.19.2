// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "WindowsCriticalSection.h"
#include "AllowWindowsPlatformTypes.h"

FWindowsSystemWideCriticalSection::FWindowsSystemWideCriticalSection(const FString& InName, FTimespan InTimeout)
{
	check(InName.Len() > 0)
	check(InName.Len() < MAX_PATH - 1)
	check(InTimeout >= FTimespan::Zero())
	check(InTimeout.GetTotalMilliseconds() < (double)0x0FFFFFFF)

	TCHAR MutexName[MAX_PATH] = TEXT("");
	FCString::Strcpy(MutexName, MAX_SPRINTF, *InName);

	Mutex = CreateMutex(NULL, true, MutexName);

	if (Mutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		bool bMutexOwned = false;

		if (InTimeout != FTimespan::Zero())
		{
			DWORD WaitResult = WaitForSingleObject(Mutex, FMath::TruncToInt((float)InTimeout.GetTotalMilliseconds()));

			if (WaitResult == WAIT_OBJECT_0)
			{
				bMutexOwned = true;
			}
		}

		if (!bMutexOwned)
		{
			Mutex = NULL;
		}
	}
}

FWindowsSystemWideCriticalSection::~FWindowsSystemWideCriticalSection()
{
	Release();
}

bool FWindowsSystemWideCriticalSection::IsValid() const
{
	return Mutex != NULL;
}

void FWindowsSystemWideCriticalSection::Release()
{
	if (IsValid())
	{
		ReleaseMutex(Mutex);
		Mutex = NULL;
	}
}

#include "HideWindowsPlatformTypes.h"
