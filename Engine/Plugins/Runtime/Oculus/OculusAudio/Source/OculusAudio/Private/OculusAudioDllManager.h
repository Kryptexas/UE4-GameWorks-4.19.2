// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "OVR_Audio.h"
#include "CoreMinimal.h"
#include "Audio.h"

//---------------------------------------------------
// Oculus Audio DLL handling
//---------------------------------------------------

static const TCHAR* GetOculusErrorString(ovrResult Result)
{
	switch (Result)
	{
		default:
		case ovrError_AudioUnknown:			                return TEXT("Unknown Error");
		case ovrError_AudioInvalidParam:	                return TEXT("Invalid Param");
		case ovrError_AudioBadSampleRate:	                return TEXT("Bad Samplerate");
		case ovrError_AudioMissingDLL:		                return TEXT("Missing DLL");
		case ovrError_AudioBadAlignment:	                return TEXT("Pointers did not meet 16 byte alignment requirements");
		case ovrError_AudioUninitialized:	                return TEXT("Function called before initialization");
		case ovrError_AudioHRTFInitFailure:                 return TEXT("HRTF Profider initialization failed");
		case ovrError_AudioBadVersion:		                return TEXT("Bad audio version");
        case ovrError_AudioSymbolNotFound:	                return TEXT("DLL symbol not found");
        case ovrError_SharedReverbDisabled:	                return TEXT("Shared reverb disabled");
        case ovrError_AudioNoAvailableAmbisonicInstance:	return TEXT("No available Ambisonic");\
	}
}

#define OVR_AUDIO_CHECK(Result, Context)																\
	if (Result != ovrSuccess)																			\
	{																									\
		const TCHAR* ErrString = GetOculusErrorString(Result);											\
		UE_LOG(LogAudio, Error, TEXT("Oculus Audio SDK Error - %s: %s"), TEXT(Context), ErrString);	\
		return;																							\
	}


/************************************************************************/
/* FOculusAudioLibraryManager                                           */
/* This handles loading and unloading the Oculus Audio DLL at runtime.  */
/************************************************************************/
class FOculusAudioLibraryManager
{
public:
	static bool IsInitialized()
	{
		return bInitialized;
	}

	static void Initialize();
	

	static void Shutdown();

private:

	static bool LoadDll();

	static void ReleaseDll();

	static void* OculusAudioDllHandle;
	static uint32 NumInstances;
	static bool bInitialized;
};