// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "VoicePrivatePCH.h"
#include "VoiceModule.h"
#include "VoiceCapture.h"
#include "VoiceCodec.h"
#include "VoiceTests.h"

IMPLEMENT_MODULE(FVoiceModule, Voice);

DEFINE_LOG_CATEGORY(LogVoice);

DEFINE_STAT(STAT_Voice_Encoding);
DEFINE_STAT(STAT_Voice_Decoding);

#if PLATFORM_SUPPORTS_VOICE_CAPTURE
/** Implement these functions per platform to create the voice objects */
extern IVoiceCapture* CreateVoiceCaptureObject();
extern IVoiceEncoder* CreateVoiceEncoderObject();
extern IVoiceDecoder* CreateVoiceDecoderObject();
#endif

void FVoiceModule::StartupModule()
{	
	bEnabled = false;
	if (!GConfig->GetBool(TEXT("Voice"), TEXT("bEnabled"), bEnabled, GEngineIni))
	{
		UE_LOG(LogVoice, Warning, TEXT("Missing bEnabled key in Voice of DefaultEngine.ini"));
	}
}

void FVoiceModule::ShutdownModule()
{
}

bool FVoiceModule::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	// Ignore any execs that don't start with Voice
	if (FParse::Command(&Cmd, TEXT("Voice")))
	{
		return false;
	}

	return false;
}

TSharedPtr<class IVoiceCapture> FVoiceModule::CreateVoiceCapture()
{
#if PLATFORM_SUPPORTS_VOICE_CAPTURE
	if (bEnabled)
	{
		// Create the platform specific instance
		return TSharedPtr<IVoiceCapture>(CreateVoiceCaptureObject());
	}
#endif
	return NULL;
}

TSharedPtr<class IVoiceEncoder> FVoiceModule::CreateVoiceEncoder()
{
#if PLATFORM_SUPPORTS_VOICE_CAPTURE
	if (bEnabled)
	{
		// Create the platform specific instance
		return TSharedPtr<IVoiceEncoder>(CreateVoiceEncoderObject());
	}
#endif

	return NULL;
}

TSharedPtr<class IVoiceDecoder> FVoiceModule::CreateVoiceDecoder()
{
#if PLATFORM_SUPPORTS_VOICE_CAPTURE
	if (bEnabled)
	{
		// Create the platform specific instance
		return TSharedPtr<IVoiceDecoder>(CreateVoiceDecoderObject());
	}
#endif

	return NULL;
}


