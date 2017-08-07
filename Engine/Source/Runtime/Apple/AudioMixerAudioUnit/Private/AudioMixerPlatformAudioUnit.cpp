// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerPlatformAudioUnit.h"
#include "ModuleManager.h"
#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "CoreGlobals.h"
#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"
#include "VorbisAudioInfo.h"
#include "ADPCMAudioInfo.h"

/*
	This implementation only depends on the audio units API which allows it to run on MacOS, iOS and tvOS. 
	
	For now just assume an iOS configuration (only 2 left and right channels on a single device)
*/

/**
* CoreAudio System Headers
*/
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#if PLATFORM_IOS || PLATFORM_TVOS
#include <AVFoundation/AVAudioSession.h>
#elif  PLATFORM_MAC
#include <CoreAudio/AudioHardware.h>
#else
#error Invalid Platform!
#endif // #if PLATFORM_IOS || PLATFORM_TVOS

DECLARE_LOG_CATEGORY_EXTERN(LogAudioMixerAudioUnit, Log, All);
DEFINE_LOG_CATEGORY(LogAudioMixerAudioUnit);

namespace Audio
{
#if PLATFORM_IOS || PLATFORM_TVOS
    static const int32 DefaultBufferSize = 4096;
#else
    static const int32 DefaultBufferSize = 512;
#endif //#if PLATFORM_IOS || PLATFORM_TVOS
    static const double DefaultSampleRate = 48000.0;
    
	FMixerPlatformAudioUnit::FMixerPlatformAudioUnit()
		: bInitialized(false)
        , bInCallback(false)
        , SubmittedBufferPtr(nullptr)
#if PLATFORM_IOS || PLATFORM_TVOS
        , RemainingBytesInCurrentSubmittedBuffer(0)
        , BytesPerSubmittedBuffer(0)
#endif //#if PLATFORM_IOS || PLATFORM_TVOS
	{
	}

	FMixerPlatformAudioUnit::~FMixerPlatformAudioUnit()
	{
		if (bInitialized)
		{
			TeardownHardware();
		}
	}
    
    int32 FMixerPlatformAudioUnit::GetNumFrames(const int32 InNumReqestedFrames)
    {
        //On iOS and MacOS, we hardcode buffer sizes.
        return DefaultBufferSize;
    }

	bool FMixerPlatformAudioUnit::InitializeHardware()
	{
        if (bInitialized)
		{
			return false;
		}
        
		OSStatus Status;
        double GraphSampleRate = (double) AudioStreamInfo.DeviceInfo.SampleRate;
        UInt32 BufferSize = (UInt32) GetNumFrames(OpenStreamParams.NumFrames);
        const int32 NumChannels = 2;

        if (GraphSampleRate == 0)
        {
            GraphSampleRate = DefaultSampleRate;
        }
        
        if (BufferSize == 0)
        {
            BufferSize = DefaultBufferSize;
        }
        
#if PLATFORM_IOS || PLATFORM_TVOS
        NSError* error;
        
        AVAudioSession* AudioSession = [AVAudioSession sharedInstance];
        [AudioSession setPreferredSampleRate:GraphSampleRate error:&error];
        
        //Set the buffer size.
        Float32 bufferSizeInSec = (float)BufferSize / OpenStreamParams.SampleRate;
        [AudioSession setPreferredIOBufferDuration:bufferSizeInSec error:&error];
        
        if (error != nil)
        {
            UE_LOG(LogAudioMixerAudioUnit, Display, TEXT("Error setting buffer size to: %f"), bufferSizeInSec);
        }
        
        BytesPerSubmittedBuffer = BufferSize * NumChannels * sizeof(float);
        check(BytesPerSubmittedBuffer != 0);
        UE_LOG(LogAudioMixerAudioUnit, Display, TEXT("Bytes per submitted buffer: %d"), BytesPerSubmittedBuffer);
        
        [AudioSession setActive:true error:&error];
        
        // Retrieve the actual hardware sample rate
        GraphSampleRate = [AudioSession preferredSampleRate];
        UE_LOG(LogAudioMixerAudioUnit, Display, TEXT("Device Sample Rate: %f"), GraphSampleRate);
        check(GraphSampleRate != 0);
#else
        AudioObjectID DeviceAudioObjectID;
        AudioObjectPropertyAddress DevicePropertyAddress;
        UInt32 AudioDeviceQuerySize;
        
        //Get Audio Device ID- this will be used throughout initialization to query the audio hardware.
        DevicePropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        DevicePropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
        DevicePropertyAddress.mElement = 0;
        AudioDeviceQuerySize = sizeof(AudioDeviceID);
        Status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &DevicePropertyAddress, 0, nullptr, &AudioDeviceQuerySize, &DeviceAudioObjectID);
        
        //Query hardware sample rate:
        DevicePropertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
        DevicePropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
        DevicePropertyAddress.mElement = 0;
        AudioDeviceQuerySize = sizeof(Float64);
        Status = AudioObjectSetPropertyData(DeviceAudioObjectID, &DevicePropertyAddress, 0, nullptr, AudioDeviceQuerySize, &GraphSampleRate);
        
        if(Status != 0)
        {
            UE_LOG(LogAudioMixerAudioUnit, Display, TEXT("ERROR setting sample rate to %f"), GraphSampleRate);
        }
        
        Status = AudioObjectGetPropertyData(DeviceAudioObjectID, &DevicePropertyAddress, 0, nullptr, &AudioDeviceQuerySize, &GraphSampleRate);
        
        if(Status == 0)
        {
            UE_LOG(LogAudioMixerAudioUnit, Display, TEXT("Sample Rate: %f"), GraphSampleRate);
        }

#endif // #if PLATFORM_IOS || PLATFORM_TVOS
        
        AudioStreamInfo.DeviceInfo.NumChannels           = NumChannels;
		AudioStreamInfo.DeviceInfo.SampleRate            = (int32)GraphSampleRate;
		AudioStreamInfo.DeviceInfo.Format                = EAudioMixerStreamDataFormat::Float;
		AudioStreamInfo.DeviceInfo.OutputChannelArray.SetNum(NumChannels);
		AudioStreamInfo.DeviceInfo.OutputChannelArray[0] = EAudioMixerChannel::FrontLeft;
		AudioStreamInfo.DeviceInfo.OutputChannelArray[1] = EAudioMixerChannel::FrontRight;
		AudioStreamInfo.DeviceInfo.bIsSystemDefault      = true;

		// Linear PCM stream format
		OutputFormat.mFormatID         = kAudioFormatLinearPCM;
		OutputFormat.mFormatFlags	   = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
		OutputFormat.mChannelsPerFrame = 2;
		OutputFormat.mBytesPerFrame    = sizeof(float) * OutputFormat.mChannelsPerFrame;
		OutputFormat.mFramesPerPacket  = 1;
		OutputFormat.mBytesPerPacket   = OutputFormat.mBytesPerFrame * OutputFormat.mFramesPerPacket;
		OutputFormat.mBitsPerChannel   = 8 * sizeof(float);
		OutputFormat.mSampleRate       = GraphSampleRate;

		Status = NewAUGraph(&AudioUnitGraph);
		if (Status != noErr)
		{
			HandleError(TEXT("Failed to create audio unit graph!"));
			return false;
		}

		AudioComponentDescription UnitDescription;

		// Setup audio output unit
		UnitDescription.componentType         = kAudioUnitType_Output;
#if PLATFORM_IOS || PLATFORM_TVOS
        //On iOS, we'll use the RemoteIO AudioUnit.
		UnitDescription.componentSubType      = kAudioUnitSubType_RemoteIO;
#else //PLATFORM_MAC
        //On MacOS, we'll use the DefaultOutput AudioUnit.
        UnitDescription.componentSubType      = kAudioUnitSubType_DefaultOutput;
#endif // #if PLATFORM_IOS || PLATFORM_TVOS
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
		
		Status = AUGraphNodeInfo(AudioUnitGraph, OutputNode, nullptr, &OutputUnit);
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

#if PLATFORM_MAC
        DevicePropertyAddress.mSelector = kAudioDevicePropertyBufferFrameSize;
        DevicePropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
        DevicePropertyAddress.mElement = 0;
        AudioDeviceQuerySize = sizeof(BufferSize);
        Status = AudioObjectSetPropertyData(DeviceAudioObjectID, &DevicePropertyAddress, 0, nullptr, AudioDeviceQuerySize, &BufferSize);
        if(Status != 0)
        {
            HandleError(TEXT("Failed to set output format!"), true);
            return false;
        }
        
#endif //#if PLATFORM_MAC
        
        AudioStreamInfo.NumOutputFrames = BufferSize;

		AURenderCallbackStruct InputCallback;
		InputCallback.inputProc = &AudioRenderCallback;
		InputCallback.inputProcRefCon = this;
		Status = AUGraphSetNodeInputCallback(AudioUnitGraph,
		                                     OutputNode,
		                                     0,
		                                     &InputCallback);
		UE_CLOG(Status != noErr, LogAudioMixerAudioUnit, Error, TEXT("Failed to set input callback for audio output node"));

        OpenStreamParams.NumFrames = BufferSize;
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Closed;
        
		bInitialized = true;

		return true;
	}

	bool FMixerPlatformAudioUnit::CheckAudioDeviceChange()
	{
		//TODO
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

		AudioUnitGraph = nullptr;
		OutputNode = -1;
		OutputUnit = nullptr;

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
		OutInfo = AudioStreamInfo.DeviceInfo;
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
        
		BeginGeneratingAudio();
		
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
        //TODO
        
		return false;
	}

	FAudioPlatformDeviceInfo FMixerPlatformAudioUnit::GetPlatformDeviceInfo() const
	{
		return AudioStreamInfo.DeviceInfo;
	}

	void FMixerPlatformAudioUnit::SubmitBuffer(const uint8* Buffer)
	{
        SubmittedBufferPtr = (uint8*) Buffer;
#if PLATFORM_IOS || PLATFORM_TVOS
        RemainingBytesInCurrentSubmittedBuffer = BytesPerSubmittedBuffer;
#endif //#if PLATFORM_IOS || PLATFORM_TVOS
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
        FAudioPlatformSettings Settings;
        Settings.CallbackBufferFrameSize = DefaultBufferSize;
        Settings.NumBuffers = 2;
#if PLATFORM_IOS || PLATFORM_TVOS
        Settings.MaxChannels = 16;
#endif //#if PLATFORM_IOS || PLATFORM_TVOS
        
        return Settings;
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

	bool FMixerPlatformAudioUnit::PerformCallback(AudioBufferList* OutputBufferData)
	{
		bInCallback = true;
		
		if (AudioStreamInfo.StreamState == EAudioOutputStreamState::Running)
		{
            if (!SubmittedBufferPtr)
            {
                ReadNextBuffer();
            }

            OutputBufferData->mBuffers[0].mData = (void*) SubmittedBufferPtr;
            SubmittedBufferPtr = nullptr;
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

	OSStatus FMixerPlatformAudioUnit::AudioRenderCallback(void* RefCon, AudioUnitRenderActionFlags* ActionFlags,
														  const AudioTimeStamp* TimeStamp, UInt32 BusNumber,
														  UInt32 NumFrames, AudioBufferList* IOData)
	{
		// Get the user data and cast to our FMixerPlatformCoreAudio object
		FMixerPlatformAudioUnit* me = (FMixerPlatformAudioUnit*) RefCon;
		
		me->PerformCallback(IOData);
		
		return noErr;
	}
}
