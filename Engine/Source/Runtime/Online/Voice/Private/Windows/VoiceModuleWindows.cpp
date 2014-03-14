// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "VoicePrivatePCH.h"
#include "VoiceCaptureWindows.h"
#include "VoiceCodecOpus.h"
#include "Voice.h"

IVoiceCapture* CreateVoiceCaptureObject()
{
	FVoiceCaptureWindows* NewVoiceCapture = new FVoiceCaptureWindows();
	if (!NewVoiceCapture->Init(VOICE_SAMPLE_RATE, NUM_VOICE_CHANNELS))
	{
		delete NewVoiceCapture;
		NewVoiceCapture = NULL;
	}

	return NewVoiceCapture; 
}

IVoiceEncoder* CreateVoiceEncoderObject()
{
	FVoiceEncoderOpus* NewEncoder = new FVoiceEncoderOpus();
	if (!NewEncoder->Init(VOICE_SAMPLE_RATE, NUM_VOICE_CHANNELS))
	{
		delete NewEncoder;
		NewEncoder = NULL;
	}

	return NewEncoder; 
}

IVoiceDecoder* CreateVoiceDecoderObject()
{
	FVoiceDecoderOpus* NewDecoder = new FVoiceDecoderOpus();
	if (!NewDecoder->Init(VOICE_SAMPLE_RATE, NUM_VOICE_CHANNELS))
	{
		delete NewDecoder;
		NewDecoder = NULL;
	}

	return NewDecoder; 
}