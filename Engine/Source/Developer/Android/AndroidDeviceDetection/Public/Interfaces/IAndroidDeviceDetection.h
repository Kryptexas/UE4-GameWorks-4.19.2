// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IAndroidDeviceDetection.h: Declares the IAndroidDeviceDetection interface.
=============================================================================*/

#pragma once


struct FAndroidDeviceInfo
{
	FString SerialNumber;
	FString Model;
	FString DeviceName;
	FString GLESExtenstions;		//@todo android: currently retrieved from SurfaceFlinger running 1.1, but so far seems ok for supported ES2 compression formats
	int GLESVersion;
};


/**
 * Interface for AndroidDeviceDetection module.
 */
class IAndroidDeviceDetection
{
public:
	virtual const TMap<FString,FAndroidDeviceInfo>& GetDeviceMap() = 0;
	virtual FCriticalSection* GetDeviceMapLock() = 0;

protected:

	/**
	 * Virtual destructor
	 */
	virtual ~IAndroidDeviceDetection() { }
};
