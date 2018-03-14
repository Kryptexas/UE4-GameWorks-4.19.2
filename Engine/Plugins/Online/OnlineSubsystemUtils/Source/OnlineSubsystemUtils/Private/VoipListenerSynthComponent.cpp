// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "VoipListenerSynthComponent.h"

static float NumSecondsUntilIdling = 0.4f;

#if DEBUG_BUFFERING
static int64 StartingSample = 0;
static int16 DebugBuffer[2048];
#endif

#define DEBUG_NOISE 0

bool UVoipListenerSynthComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	MySampleRate = SampleRate;

#if DEBUG_BUFFERING
	FMToneGenerator = FDebugFMTone(MySampleRate, 660.0, 0.2, 0.8, 4.0);
#endif

	return true;
}

void UVoipListenerSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	if (PreDelaySampleCounter > 0)
	{
		//Pre delay sample counter is pretty loose, so we kind of round to the nearest buffer here.
		PreDelaySampleCounter -= NumSamples;
	}
	else if (PacketBuffer.IsValid())
	{
		PacketBuffer->PopAudio(OutAudio, NumSamples);
	}

#if DEBUG_NOISE
	for (int32 Index = 0; Index < NumSamples; Index++)
	{
		OutAudio[Index] = 0.5 * OutAudio[Index] + 0.5 * FMath::FRand();
	}
#endif
}

UVoipListenerSynthComponent::~UVoipListenerSynthComponent()
{
	ClosePacketStream();
}

void UVoipListenerSynthComponent::OpenPacketStream(uint64 BeginningSampleCount, int32 BufferSize, float JitterDelay)
{
	ClosePacketStream();
	PacketBuffer.Reset(new FVoicePacketBuffer(BufferSize, NumSecondsUntilIdling * MySampleRate, BeginningSampleCount));
	
	PreDelaySampleCounter = JitterDelay * MySampleRate;
}

void UVoipListenerSynthComponent::ClosePacketStream()
{
	PacketBuffer.Reset();
}

void UVoipListenerSynthComponent::ResetBuffer(int32 InStartSample, float JitterDelay)
{
	if (PacketBuffer.IsValid())
	{
		PacketBuffer->Reset(InStartSample);
		PreDelaySampleCounter = JitterDelay * MySampleRate;
	}
}

void UVoipListenerSynthComponent::SubmitPacket(void* InBuffer, int32 NumBytes, int64 InStartSample, EVoipStreamDataFormat DataFormat)
{
	if (PacketBuffer.IsValid())
	{
#if DEBUG_BUFFERING
		FMToneGenerator.Generate(DebugBuffer, NumBytes / sizeof(int16));
		StartingSample += NumBytes / sizeof(int16);
		PacketBuffer->PushPacket(DebugBuffer, NumBytes, StartingSample, DataFormat);
#else
		static int64 StartSample = 0;
		StartSample += NumBytes / sizeof(int16);
		PacketBuffer->PushPacket(InBuffer, NumBytes, StartSample, DataFormat);
#endif
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SubmitPacket called before OpenPacketStream was."));
	}
}

bool UVoipListenerSynthComponent::IsIdling()
{
	if (PacketBuffer.IsValid())
	{
		return PacketBuffer->IsIdle();
	}
	else
	{
		return true;
	}
}

uint64 UVoipListenerSynthComponent::GetSampleCounter()
{
	if (PacketBuffer.IsValid())
	{
		return PacketBuffer->GetCurrentSample();
	}
	else
	{
		return 0;
	}
}


#if DEBUG_BUFFERING
FDebugFMTone::FDebugFMTone(float InSampleRate, float InCarrierFreq, float InModFreq, float InCarrierAmp, float InModAmp)
	: SampleRate(InSampleRate)
	, CarrierFreq(InCarrierFreq)
	, CarrierAmp(InCarrierAmp)
	, ModFreq(InModFreq)
	, ModAmp(InModAmp)
{
}

void FDebugFMTone::Generate(int16* BufferPtr, int32 NumSamples)
{
	for (int32 Index = 0; Index < NumSamples; Index++)
	{
		float OutSample = CarrierAmp * FMath::Sin(2 * PI * (CarrierFreq + ModAmp * FMath::Sin(2 * PI * ModFreq * n / SampleRate)) * n / SampleRate);
		BufferPtr[Index] = (int16)(OutSample * 32767.0f);
		n++;
	}
}
#endif