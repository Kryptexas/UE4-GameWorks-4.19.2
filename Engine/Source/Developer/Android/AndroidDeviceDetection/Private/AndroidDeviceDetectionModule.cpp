// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidDeviceDetectionModule.cpp: Implements the FAndroidDeviceDetectionModule class.
=============================================================================*/

#include "AndroidDeviceDetectionPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FAndroidDeviceDetectionModule" 


class FAndroidDeviceDetectionRunnable : public FRunnable
{
public:
	FAndroidDeviceDetectionRunnable(const FString& InADBPath, TMap<FString,FAndroidDeviceInfo>& InDeviceMap, FCriticalSection* InDeviceMapLock) :
		ADBPath(InADBPath),
		StopTaskCounter(0),
		DeviceMap(InDeviceMap),
		DeviceMapLock(InDeviceMapLock)
	{
	}

public:

	// FRunnable interface.
	virtual bool Init(void) 
	{ 
		return true; 
	}

	virtual void Exit(void) 
	{
	}

	virtual void Stop(void)
	{
		StopTaskCounter.Increment();
	}

	virtual uint32 Run(void)
	{
		int LoopCount = 10;

		while (StopTaskCounter.GetValue() == 0)
		{
			// query every 10 seconds
			if (LoopCount++ >= 10)
			{
				QueryConnectedDevices();
				LoopCount = 0;
			}

			FPlatformProcess::Sleep(1.0f);
		}

		return 0;
	}

private:

	bool ExecuteAdbCommand( const FString& CommandLine, FString* OutStdOut, FString* OutStdErr ) const
	{
		// execute the command
		int32 ReturnCode;
		FString DefaultError;

		// make sure there's a place for error output to go if the caller specified nullptr
		if (!OutStdErr)
		{
			OutStdErr = &DefaultError;
		}

		FPlatformProcess::ExecProcess(*ADBPath, *CommandLine, &ReturnCode, OutStdOut, OutStdErr);

		if (ReturnCode != 0)
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("The Android SDK command '%s' failed to run. Return code: %d, Error: %s\n"), *CommandLine, ReturnCode, **OutStdErr);

			return false;
		}

		return true;
	}

	void QueryConnectedDevices()
	{
		// grab the list of devices via adb
		FString StdOut;
		if (!ExecuteAdbCommand(TEXT("devices -l"), &StdOut, nullptr))
		{
			return;
		}

		// separate out each line
		TArray<FString> DeviceStrings;
		StdOut = StdOut.Replace(TEXT("\r"), TEXT("\n"));
		StdOut.ParseIntoArray(&DeviceStrings, TEXT("\n"), true);

		// a list containing all devices found this time, so we can remove anything not in this list
		TArray<FString> CurrentlyConnectedDevices;

		for (int32 StringIndex = 0; StringIndex < DeviceStrings.Num(); ++StringIndex)
		{
			const FString& DeviceString = DeviceStrings[StringIndex];

			// skip over non-device lines
			if (DeviceString.StartsWith("* ") || DeviceString.StartsWith("List "))
			{
				continue;
			}

			// grab the device serial number
			int32 TabIndex;

			if (!DeviceString.FindChar(TCHAR(' '), TabIndex))
			{
				continue;
			}

			FString SerialNumber = DeviceString.Left(TabIndex);

			// add it to our list of currently connected devices
			CurrentlyConnectedDevices.Add(SerialNumber);

			// move on to next device if this one is already a known device
			if (DeviceMap.Contains(SerialNumber))
			{
				continue;
			}

			// get the GL extensions string (and a bunch of other stuff)
			FString GLESExtensions;

			FString ExtensionsCommand = FString::Printf(TEXT("-s %s shell dumpsys SurfaceFlinger"), *SerialNumber);
			if (!ExecuteAdbCommand(*ExtensionsCommand, &GLESExtensions, nullptr))
			{
				continue;
			}

			// grab the GL ES version
			FString GLESVersionString;

			FString VersionCommand = FString::Printf(TEXT("-s %s shell getprop ro.opengles.version"), *SerialNumber);
			if (!ExecuteAdbCommand(*VersionCommand, &GLESVersionString, nullptr))
			{
				continue;
			}

			int GLESVersion = FCString::Atoi(*GLESVersionString);

			// parse the device model
			FString Model;
			FParse::Value(*DeviceString, TEXT("model:"), Model);

			// parse the device model
			FString DeviceName;
			FParse::Value(*DeviceString, TEXT("device:"), DeviceName);

			// add the device to the map
			{
				FScopeLock ScopeLock(DeviceMapLock);

				FAndroidDeviceInfo& DeviceInfo = DeviceMap.Add(SerialNumber);
				DeviceInfo.SerialNumber = SerialNumber;
				DeviceInfo.Model = Model;
				DeviceInfo.DeviceName = DeviceName;
				DeviceInfo.GLESExtensions = GLESExtensions;
				DeviceInfo.GLESVersion = GLESVersion;
			}
		}

		// loop through the previously connected devices list and remove any that aren't still connected from the updated DeviceMap
		TArray<FString> DevicesToRemove;

		for (auto It = DeviceMap.CreateConstIterator(); It; ++It)
		{
			if (!CurrentlyConnectedDevices.Contains(It.Key()))
			{
				DevicesToRemove.Add(It.Key());
			}
		}

		{
			// enter the critical section and remove the devices from the map
			FScopeLock ScopeLock(DeviceMapLock);

			for (auto It = DevicesToRemove.CreateConstIterator(); It; ++It)
			{
				DeviceMap.Remove(*It);
			}
		}
	}

private:

	// path to the adb command
	FString ADBPath;

	// > 0 if we've been asked to abort work in progress at the next opportunity
	FThreadSafeCounter StopTaskCounter;

	TMap<FString,FAndroidDeviceInfo>& DeviceMap;
	FCriticalSection* DeviceMapLock;
};

class FAndroidDeviceDetection : public IAndroidDeviceDetection
{
public:

	FAndroidDeviceDetection() :
		DetectionThread(nullptr),
		DetectionThreadRunnable(nullptr)
	{
		// get the SDK binaries folder
		TCHAR AndroidDirectory[32768] = { 0 };
		FPlatformMisc::GetEnvironmentVariable(TEXT("ANDROID_HOME"), AndroidDirectory, 32768);

		if (AndroidDirectory[0] == 0)
		{
			return;
		}

		FString ADBPath = FString::Printf(TEXT("%s\\platform-tools\\adb.exe"), AndroidDirectory);

		// if it doesn't exist, no SDK installed, so don't bother creating a thread to look for devices
		if (!FPaths::FileExists(*ADBPath))
		{
			ADBPath.Empty();
			return;
		}

		// create and fire off our device detection thread
		DetectionThreadRunnable = new FAndroidDeviceDetectionRunnable(ADBPath, DeviceMap, &DeviceMapLock);
		DetectionThread = FRunnableThread::Create(DetectionThreadRunnable, TEXT("FAndroidDeviceDetectionRunnable"));
	}

	virtual ~FAndroidDeviceDetection()
	{
		if (DetectionThreadRunnable && DetectionThread)
		{
			DetectionThreadRunnable->Stop();
			DetectionThread->WaitForCompletion();
		}
	}

	virtual const TMap<FString,FAndroidDeviceInfo>& GetDeviceMap() OVERRIDE
	{
		return DeviceMap;
	}

	virtual FCriticalSection* GetDeviceMapLock() OVERRIDE
	{
		return &DeviceMapLock;
	}

private:

	FRunnableThread* DetectionThread;
	FAndroidDeviceDetectionRunnable* DetectionThreadRunnable;

	TMap<FString,FAndroidDeviceInfo> DeviceMap;
	FCriticalSection DeviceMapLock;
};


/**
 * Holds the target platform singleton.
 */
static FAndroidDeviceDetection* AndroidDeviceDetectionSingleton = nullptr;


/**
 * Module for detecting android devices.
 */
class FAndroidDeviceDetectionModule : public IAndroidDeviceDetectionModule
{
public:

	/**
	 * Destructor.
	 */
	~FAndroidDeviceDetectionModule( )
	{
		if (AndroidDeviceDetectionSingleton != nullptr)
		{
			delete AndroidDeviceDetectionSingleton;
		}

		AndroidDeviceDetectionSingleton = nullptr;
	}

	virtual IAndroidDeviceDetection* GetAndroidDeviceDetection() OVERRIDE
	{
		if (AndroidDeviceDetectionSingleton == nullptr)
		{
			AndroidDeviceDetectionSingleton = new FAndroidDeviceDetection();
		}

		return AndroidDeviceDetectionSingleton;
	}
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FAndroidDeviceDetectionModule, AndroidDeviceDetection);
