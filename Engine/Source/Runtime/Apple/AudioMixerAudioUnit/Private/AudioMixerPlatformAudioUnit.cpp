// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerPlatformAudioUnit.h"
#include "ModuleManager.h"
#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "VorbisAudioInfo.h"
#include "ADPCMAudioInfo.h"

/*
	This implementation only depends on the audio units API which allows it to run on iOS. 
	Unfortunately, it is not clear yet if we can get the detailed channel configuration information
	that is needed on desktop from this API.
	
	For now just assume an iOS configuration (only 2 left and right channels on a single device)
*/

/**
* CoreAudio System Headers
*/
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <AVFoundation/AVAudioSession.h>

DECLARE_LOG_CATEGORY_EXTERN(LogAudioMixerAudioUnit, Log, All);
DEFINE_LOG_CATEGORY(LogAudioMixerAudioUnit);

#define UNREAL_AUDIO_TEST_WHITE_NOISE 0

const uint32	cSampleBufferBits = 12;
const uint32	cNumChannels = 2;
const uint32	cAudioMixerBufferSize = 1 << cSampleBufferBits;
const uint32	cSampleBufferSize = cAudioMixerBufferSize * 2 * cNumChannels;
const uint32	cSampleBufferSizeMask = cSampleBufferSize - 1;

namespace Audio
{	
	FMixerPlatformAudioUnit::FMixerPlatformAudioUnit()
		: bInitialized(false),
		  bInCallback(false),
		  sampleBufferHead(0),
		  sampleBufferTail(0)
	{
		sampleBuffer = (SInt16*)FMemory::Malloc(cSampleBufferSize);
	}

	FMixerPlatformAudioUnit::~FMixerPlatformAudioUnit()
	{
		if(sampleBuffer != NULL)
		{
			FMemory::Free(sampleBuffer);
			sampleBuffer = NULL;
		}
		
		if (bInitialized)
		{
			TeardownHardware();
		}
	}

	bool FMixerPlatformAudioUnit::InitializeHardware()
	{
		if (bInitialized)
		{
			return false;
		}
		
		size_t SampleSize = sizeof(SInt16);
		double GraphSampleRate = 44100.0;

		AVAudioSession* AudioSession = [AVAudioSession sharedInstance];
		[AudioSession setPreferredSampleRate:GraphSampleRate error:nil];
		[AudioSession setActive:true error:nil];

		// Retrieve the actual hardware sample rate
		GraphSampleRate = [AudioSession preferredSampleRate];
		
		DeviceInfo.NumChannels = 2;
		DeviceInfo.SampleRate = (int32)GraphSampleRate;
		DeviceInfo.Format = EAudioMixerStreamDataFormat::Int16;
		DeviceInfo.OutputChannelArray.SetNum(2);
		DeviceInfo.OutputChannelArray[0] = EAudioMixerChannel::FrontLeft;
		DeviceInfo.OutputChannelArray[1] = EAudioMixerChannel::FrontRight;
		DeviceInfo.bIsSystemDefault = true;
		AudioStreamInfo.DeviceInfo = DeviceInfo;

		// Linear PCM stream format
		OutputFormat.mFormatID         = kAudioFormatLinearPCM;
		OutputFormat.mFormatFlags	  = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
		OutputFormat.mChannelsPerFrame = 2;
		OutputFormat.mBytesPerFrame    = sizeof(SInt16) * OutputFormat.mChannelsPerFrame;
		OutputFormat.mFramesPerPacket  = 1;
		OutputFormat.mBytesPerPacket   = OutputFormat.mBytesPerFrame * OutputFormat.mFramesPerPacket;
		OutputFormat.mBitsPerChannel   = 8 * sizeof(SInt16);
		OutputFormat.mSampleRate       = GraphSampleRate;

		OSStatus Status = NewAUGraph(&AudioUnitGraph);
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to create audio unit graph!"));
			return false;
		}

		AudioComponentDescription UnitDescription;

		// Setup audio output unit
		UnitDescription.componentType         = kAudioUnitType_Output;
		UnitDescription.componentSubType      = kAudioUnitSubType_RemoteIO;
		UnitDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
		UnitDescription.componentFlags        = 0;
		UnitDescription.componentFlagsMask    = 0;
		Status = AUGraphAddNode(AudioUnitGraph, &UnitDescription, &OutputNode);
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to initialize audio output node!"), true);
			return false;
		}
		
		Status = AUGraphOpen(AudioUnitGraph);
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to open audio unit graph"), true);
			return false;
		}
		
		Status = AUGraphNodeInfo(AudioUnitGraph, OutputNode, NULL, &OutputUnit);
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to retrieve output unit reference!"), true);
			return false;
		}

		Status = AudioUnitSetProperty(OutputUnit,
		                              kAudioUnitProperty_StreamFormat,
		                              kAudioUnitScope_Input,
		                              0,
		                              &OutputFormat,
		                              sizeof(AudioStreamBasicDescription));
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to set output format!"), true);
			return false;
		}

		// Set the approimate callback rate for iOS, this can't be banked on so we must buffer to handle varibale sized requests from the os
		Float32 bufferSizeInSec = (float)cAudioMixerBufferSize / GraphSampleRate;
		NSError* error;
		[AudioSession setPreferredIOBufferDuration:bufferSizeInSec error:&error];
		
		AURenderCallbackStruct InputCallback;
		InputCallback.inputProc = &IOSAudioRenderCallback;
		InputCallback.inputProcRefCon = this;
		Status = AUGraphSetNodeInputCallback(AudioUnitGraph,
		                                     OutputNode,
		                                     0,
		                                     &InputCallback);
		UE_CLOG(Status != noErr, LogAudioMixerAudioUnit, Error, TEXT("Failed to set input callback for audio output node"));

		
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Closed;
		
		bInitialized = true;

		return true;
	}

	bool FMixerPlatformAudioUnit::CheckAudioDeviceChange()
	{
		// only ever one device currently		
		return false;
	}

	bool FMixerPlatformAudioUnit::TeardownHardware()
	{
		if(!bInitialized)
		{
			return true;
		}
		
		StopAudioStream();
		CloseAudioStream();

		DisposeAUGraph(AudioUnitGraph);

		AudioUnitGraph = NULL;
		OutputNode = -1;
		OutputUnit = NULL;

		bInitialized = false;
		
		return true;
	}

	bool FMixerPlatformAudioUnit::IsInitialized() const
	{
		return bInitialized;
	}

	bool FMixerPlatformAudioUnit::GetNumOutputDevices(uint32& OutNumOutputDevices)
	{
		OutNumOutputDevices = 1;
		
		return true;
	}

	bool FMixerPlatformAudioUnit::GetOutputDeviceInfo(const uint32 InDeviceIndex, FAudioPlatformDeviceInfo& OutInfo)
	{
		OutInfo = DeviceInfo;
		return true;
	}

	bool FMixerPlatformAudioUnit::GetDefaultOutputDeviceIndex(uint32& OutDefaultDeviceIndex) const
	{
		OutDefaultDeviceIndex = 0;
		
		return true;
	}

	bool FMixerPlatformAudioUnit::OpenAudioStream(const FAudioMixerOpenStreamParams& Params)
	{
		if (!bInitialized || AudioStreamInfo.StreamState != EAudioOutputStreamState::Closed)
		{
			return false;
		}
		
		AudioStreamInfo.DeviceInfo = DeviceInfo;
		AudioStreamInfo.OutputDeviceIndex = Params.OutputDeviceIndex;
		AudioStreamInfo.AudioMixer = Params.AudioMixer;
		
		// Initialize the audio unit graph
		OSStatus Status = AUGraphInitialize(AudioUnitGraph);
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to initialize audio graph!"), true);
			return false;
		}

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Open;

		return true;
	}

	bool FMixerPlatformAudioUnit::CloseAudioStream()
	{
		if (!bInitialized || (AudioStreamInfo.StreamState != EAudioOutputStreamState::Open && AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped))
		{
			return false;
		}
		
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Closed;
		
		return true;
	}

	bool FMixerPlatformAudioUnit::StartAudioStream()
	{
		if (!bInitialized || (AudioStreamInfo.StreamState != EAudioOutputStreamState::Open && AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped))
		{
			return false;
		}
		
		sampleBufferHead = sampleBufferTail = 0;
		BeginGeneratingAudio();
		ReadNextBuffer();
		
		// This will start the render audio callback
		OSStatus Status = AUGraphStart(AudioUnitGraph);
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to start audio graph!"), true);
			return false;
		}

		return true;
	}

	bool FMixerPlatformAudioUnit::StopAudioStream()
	{
		if(!bInitialized || AudioStreamInfo.StreamState != EAudioOutputStreamState::Running)
		{
			return false;
		}
		
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopping;
		
		AUGraphStop(AudioUnitGraph);

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopped;
		
		return true;
	}

	bool FMixerPlatformAudioUnit::MoveAudioStreamToNewAudioDevice(const FString& InNewDeviceId)
	{
		return false;
	}

	FAudioPlatformDeviceInfo FMixerPlatformAudioUnit::GetPlatformDeviceInfo() const
	{
		return AudioStreamInfo.DeviceInfo;
	}

	void FMixerPlatformAudioUnit::SubmitBuffer(const uint8* Buffer)
	{
		int32			sampleCount = AudioStreamInfo.NumOutputFrames * AudioStreamInfo.DeviceInfo.NumChannels;
		float const*	curSample = (const float*)Buffer;
		float const*	lastSample = curSample + sampleCount;
		
		while(curSample < lastSample)
		{
			if(sampleBufferHead - sampleBufferTail < cSampleBufferSize)
			{
				sampleBuffer[sampleBufferHead++ & cSampleBufferSizeMask] = (SInt16)(*curSample++ * 32767.0f);
			}
			else
			{
				// Overrun
				check(false);
			}
		}
	}

	FName FMixerPlatformAudioUnit::GetRuntimeFormat(USoundWave* InSoundWave)
	{
		static FName NAME_ADPCM(TEXT("ADPCM"));
		return NAME_ADPCM;
	}

	bool FMixerPlatformAudioUnit::HasCompressedAudioInfoClass(USoundWave* InSoundWave)
	{
		return true;
	}

	ICompressedAudioInfo* FMixerPlatformAudioUnit::CreateCompressedAudioInfo(USoundWave* InSoundWave)
	{
		return new FADPCMAudioInfo();
	}

	FString FMixerPlatformAudioUnit::GetDefaultDeviceName()
	{
		return FString();
	}

	FAudioPlatformSettings FMixerPlatformAudioUnit::GetPlatformSettings() const
	{
		return FAudioPlatformSettings();
	}

	void FMixerPlatformAudioUnit::ResumeContext()
	{
		if (bSuspended)
		{
			AUGraphStart(AudioUnitGraph);
			UE_LOG(LogAudioMixerAudioUnit, Display, TEXT("Resuming Audio"));
			bSuspended = false;
		}
	}
	
	void FMixerPlatformAudioUnit::SuspendContext()
	{
		if (!bSuspended)
		{
			AUGraphStop(AudioUnitGraph);
			UE_LOG(LogAudioMixerAudioUnit, Display, TEXT("Suspending Audio"));
			bSuspended = true;
		}
	}

	void FMixerPlatformAudioUnit::HandleError(const TCHAR* InLogOutput, bool bTeardown)
	{
		UE_LOG(LogAudioMixerAudioUnit, Log, TEXT("%s"), InLogOutput);
		if (bTeardown)
		{
			TeardownHardware();
		}
	}

	bool FMixerPlatformAudioUnit::PerformCallback(UInt32 NumFrames, const AudioBufferList* OutputBufferData)
	{
		bInCallback = true;
		
		if (AudioStreamInfo.StreamState == EAudioOutputStreamState::Running)
		{
			
			#if UNREAL_AUDIO_TEST_WHITE_NOISE
			
				for (uint32 bufferItr = 0; bufferItr < OutputBufferData->mNumberBuffers; ++bufferItr)
				{
					SInt16*	floatSampleData = (SInt16*)OutputBufferData->mBuffers[bufferItr].mData;
					for (uint32 Sample = 0; Sample < OutputBufferData->mBuffers[bufferItr].mDataByteSize / sizeof(SInt16); ++Sample)
					{
						floatSampleData[Sample] = (SInt16)FMath::FRandRange(-10000.0f, 10000.0f);
					}
				}
			
			#else // UNREAL_AUDIO_TEST_WHITE_NOISE

				// We can not count on getting predicable buffer sizes on iOS so we have to do some buffering
				int			outputBufferSize = OutputBufferData->mBuffers[0].mDataByteSize / sizeof(SInt16);
				SInt16*		curOutputSample = (SInt16*)OutputBufferData->mBuffers[0].mData;
				SInt16*		lastOutputSample = curOutputSample + outputBufferSize;
			
				while(curOutputSample < lastOutputSample)
				{
					if(sampleBufferHead - sampleBufferTail <= cSampleBufferSize / 2)
					{
						ReadNextBuffer();	// This will trigger a call to SubmitBuffer() which will copy the audio generated by the mixer into audioMixerSampleBuffer
					}
					
					if(sampleBufferHead > sampleBufferTail)
					{
						*curOutputSample++ = sampleBuffer[sampleBufferTail++ & cSampleBufferSizeMask];
					}
					else
					{
						// Underrun
						*curOutputSample++ = 0;
						check(false);
					}
				}
			
			#endif // UNREAL_AUDIO_TEST_WHITE_NOISE
		}
		else
		{
			for (uint32 bufferItr = 0; bufferItr < OutputBufferData->mNumberBuffers; ++bufferItr)
			{
				memset(OutputBufferData->mBuffers[bufferItr].mData, 0, OutputBufferData->mBuffers[bufferItr].mDataByteSize);
			}
		}
		
		bInCallback = false;
		
		return true;
	}

	OSStatus FMixerPlatformAudioUnit::IOSAudioRenderCallback(void* RefCon, AudioUnitRenderActionFlags* ActionFlags,
														  const AudioTimeStamp* TimeStamp, UInt32 BusNumber,
														  UInt32 NumFrames, AudioBufferList* IOData)
	{
		// Get the user data and cast to our FMixerPlatformCoreAudio object
		FMixerPlatformAudioUnit* me = (FMixerPlatformAudioUnit*) RefCon;
		
		me->PerformCallback(NumFrames, IOData);
		
		return noErr;
	}
}
