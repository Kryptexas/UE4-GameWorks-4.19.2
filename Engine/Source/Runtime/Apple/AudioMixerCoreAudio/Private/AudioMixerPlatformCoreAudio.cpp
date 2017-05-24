// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerPlatformCoreAudio.h"
#include "ModuleManager.h"
#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "VorbisAudioInfo.h"

/*
	Unforunately there exists precious little CoreAudio documentation. Here are a few references
	https://developer.apple.com/library/content/technotes/tn2223/_index.html
	https://www.music.mcgill.ca/~gary/rtaudio/

*/

/**
* CoreAudio System Headers
*/
#include <CoreAudio/AudioHardware.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

DECLARE_LOG_CATEGORY_EXTERN(LogAudioMixerCoreAudio, Log, All);
DEFINE_LOG_CATEGORY(LogAudioMixerCoreAudio);

#define UNREAL_AUDIO_TEST_WHITE_NOISE 0

static const TCHAR* GetCoreAudioError(OSStatus Result)
{
	switch (Result)
	{
		case kAudioHardwareNotRunningError:				return TEXT("kAudioHardwareNotRunningError");
		case kAudioHardwareUnspecifiedError:			return TEXT("kAudioHardwareUnspecifiedError");
		case kAudioHardwareUnknownPropertyError:		return TEXT("kAudioHardwareUnknownPropertyError");
		case kAudioHardwareBadPropertySizeError:		return TEXT("kAudioHardwareBadPropertySizeError");
		case kAudioHardwareIllegalOperationError:		return TEXT("kAudioHardwareIllegalOperationError");
		case kAudioHardwareBadObjectError:				return TEXT("kAudioHardwareBadObjectError");
		case kAudioHardwareBadDeviceError:				return TEXT("kAudioHardwareBadDeviceError");
		case kAudioHardwareBadStreamError:				return TEXT("kAudioHardwareBadStreamError");
		case kAudioHardwareUnsupportedOperationError:	return TEXT("kAudioHardwareUnsupportedOperationError");
		case kAudioDeviceUnsupportedFormatError:		return TEXT("kAudioDeviceUnsupportedFormatError");
		case kAudioDevicePermissionsError:				return TEXT("kAudioDevicePermissionsError");
		default:										return TEXT("Unknown CoreAudio Error");
	}
}
/**
* Function called when an error occurs in the device code.
* @param Error The error type that occurred.
* @param ErrorDetails A string of specific details about the error.
* @param FileName The file that the error occurred in.
* @param LineNumber The line number that the error occurred in.
*/
static void OnDeviceError(FString ErrorMsg, FString ErrorDetails, FString FileName, int32 LineNumber)
{
	UE_LOG(LogAudioMixerCoreAudio, Error, TEXT("Audio Device Error: (%s) : %s (%s::%d)"), *ErrorMsg, *ErrorDetails, *FileName, LineNumber);
}

#define UA_DEVICE_PLATFORM_ERROR(INFO)	(OnDeviceError(TEXT("PLATFORM"), INFO, FString(__FILE__), __LINE__))
#define AU_DEVICE_PARAM_ERROR(INFO)		(OnDeviceError(TEXT("INVALID_PARAMETER"), INFO, FString(__FILE__), __LINE__))
#define AU_DEVICE_WARNING(INFO)			(OnDeviceError(TEXT("WARNING"), INFO, FString(__FILE__), __LINE__))

// Helper macro to report CoreAudio API errors
#define CORE_AUDIO_ERR(Status, Context)											\
	if (Status != noErr)														\
	{																			\
		const TCHAR* Err = GetCoreAudioError(Status);							\
		FString Message = FString::Printf(TEXT("%s: %s"), TEXT(Context), Err);	\
		UA_DEVICE_PLATFORM_ERROR(*Message);										\
		return false;															\
	}

// Helper macro to make a new global audio property address
#define NEW_GLOBAL_PROPERTY(SELECTOR)		\
	{										\
		SELECTOR,							\
		kAudioObjectPropertyScopeGlobal,	\
		kAudioObjectPropertyElementMaster	\
	}

// Helper macro to make a new output audio property address
#define NEW_OUTPUT_PROPERTY(SELECTOR)		\
	{										\
		SELECTOR,							\
		kAudioDevicePropertyScopeOutput,	\
		kAudioObjectPropertyElementMaster	\
	}

namespace Audio
{
	TArray<FMixerPlatformCoreAudio::FDevice>	FMixerPlatformCoreAudio::DeviceArray;
	
	FMixerPlatformCoreAudio::FMixerPlatformCoreAudio()
		: OutputDeviceId(INDEX_NONE)
		, OutputDeviceIndex(INDEX_NONE)
		, DefaultDeviceId(INDEX_NONE)
		, DefaultDeviceIndex(INDEX_NONE)
		, DeviceIOProcId(nullptr)
		, NumDeviceStreams(0)
		, bInitialized(false)
		, bInCallback(false)
		, coreAudioBuffer(nullptr)
		, coreAudioBufferByteSize(0)
	{
	}

	FMixerPlatformCoreAudio::~FMixerPlatformCoreAudio()
	{
		if (bInitialized)
		{
			TeardownHardware();
		}
	}

	bool FMixerPlatformCoreAudio::InitializeHardware()
	{
		if (bInitialized)
		{
			return false;
		}
		
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Closed;
		
		bool bSuccess = InitRunLoop();
		bSuccess &= GetOutputDevices();
		bSuccess &= GetDefaultOutputDevice();
		return (bInitialized = bSuccess);
	}

	bool FMixerPlatformCoreAudio::CheckAudioDeviceChange()
	{
		// Recheck the device list for changes
		
		return false;
	}

	bool FMixerPlatformCoreAudio::TeardownHardware()
	{
		if(!bInitialized)
		{
			return true;
		}
		
		StopAudioStream();
		CloseAudioStream();
		bInitialized = false;
		
		return true;;
	}

	bool FMixerPlatformCoreAudio::IsInitialized() const
	{
		return bInitialized;
	}

	bool FMixerPlatformCoreAudio::GetNumOutputDevices(uint32& OutNumOutputDevices)
	{
		OutNumOutputDevices = DeviceArray.Num();
		
		return true;
	}

	bool FMixerPlatformCoreAudio::GetOutputDeviceInfo(const uint32 InDeviceIndex, FAudioPlatformDeviceInfo& OutInfo)
	{
		if(InDeviceIndex >= DeviceArray.Num())
		{
			return false;
		}
		
		OutInfo = DeviceArray[InDeviceIndex].DeviceInfo;
		
		return true;
	}

	bool FMixerPlatformCoreAudio::GetDefaultOutputDeviceIndex(uint32& OutDefaultDeviceIndex) const
	{
		OutDefaultDeviceIndex = DefaultDeviceIndex;
		
		return true;
	}

	bool FMixerPlatformCoreAudio::OpenAudioStream(const FAudioMixerOpenStreamParams& Params)
	{
		if (!bInitialized || AudioStreamInfo.StreamState != EAudioOutputStreamState::Closed)
		{
			return false;
		}
		
		AudioStreamInfo.DeviceInfo = DeviceArray[Params.OutputDeviceIndex].DeviceInfo;

		bool bSampleRateChanged = false;
		bool bSuccess = InitDeviceOutputId(Params.OutputDeviceIndex);
		bSuccess &= InitDeviceFrameRate(Params.SampleRate, bSampleRateChanged);
		bSuccess &= InitDeviceVirtualFormat(bSampleRateChanged);
		bSuccess &= InitDevicePhysicalFormat();
		bSuccess &= InitDeviceNumDeviceStreams();
		bSuccess &= InitDeviceCallback(Params);
		bSuccess &= InitDeviceOverrunCallback();
		
		if(bSuccess)
		{
			AudioStreamInfo.StreamState = EAudioOutputStreamState::Open;

			AudioStreamInfo.OutputDeviceIndex = Params.OutputDeviceIndex;
			AudioStreamInfo.AudioMixer = Params.AudioMixer;
		}

		return bSuccess;
	}

	bool FMixerPlatformCoreAudio::CloseAudioStream()
	{
		if (!bInitialized || (AudioStreamInfo.StreamState != EAudioOutputStreamState::Open && AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped))
		{
			return false;
		}

		AudioObjectPropertyAddress Property = NEW_GLOBAL_PROPERTY(kAudioDeviceProcessorOverload);
		OSStatus Status = AudioObjectRemovePropertyListener(OutputDeviceId, &Property, OverrunPropertyListener, (void *)this);
		CORE_AUDIO_ERR(Status, "Failed to remove device overrun property listener");

		Status = AudioDeviceDestroyIOProcID(OutputDeviceId, DeviceIOProcId);
		CORE_AUDIO_ERR(Status, "Failed to stop audio destroy IOProcID");
		
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Closed;
		
		return true;
	}

	bool FMixerPlatformCoreAudio::StartAudioStream()
	{
		if (!bInitialized || (AudioStreamInfo.StreamState != EAudioOutputStreamState::Open && AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped))
		{
			return false;
		}

		coreAudioBuffer = NULL;
		coreAudioBufferByteSize = 0;
		BeginGeneratingAudio();

		// Start up the audio device stream
		OSStatus Status = AudioDeviceStart(OutputDeviceId, DeviceIOProcId);
		CORE_AUDIO_ERR(Status, "Failed to start audio device");

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Running;
		return true;
	}

	bool FMixerPlatformCoreAudio::StopAudioStream()
	{
		if(!bInitialized || AudioStreamInfo.StreamState != EAudioOutputStreamState::Running)
		{
			return false;
		}
		
		// Just use a simple mechanism here to ensure we don't collide with the os audio render callback, perhaps put a timeout in here to prevent any extreme edge case from deadlocking
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopping;
		
		while(bInCallback)
		{
			FPlatformProcess::Sleep(0.01f);
		}
		
		OSStatus Status = AudioDeviceStop(OutputDeviceId, DeviceIOProcId);
		CORE_AUDIO_ERR(Status, "Failed to stop audio device callback.");

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopped;
		
		return true;
	}

	bool FMixerPlatformCoreAudio::MoveAudioStreamToNewAudioDevice(const FString& InNewDeviceId)
	{
		return false;
	}

	FAudioPlatformDeviceInfo FMixerPlatformCoreAudio::GetPlatformDeviceInfo() const
	{
		return AudioStreamInfo.DeviceInfo;
	}

	void FMixerPlatformCoreAudio::SubmitBuffer(const uint8* Buffer)
	{
		// TODO: this shouldn't have to be a copy... could probably just point coreAudioBuffer to Buffer since Buffer is guaranteed to be valid for lifetime of the render!
		if(coreAudioBuffer != NULL && coreAudioBufferByteSize > 0)
		{
			memcpy(coreAudioBuffer, Buffer, coreAudioBufferByteSize);
		}
	}

	FName FMixerPlatformCoreAudio::GetRuntimeFormat(USoundWave* InSoundWave)
	{
		static FName NAME_OGG(TEXT("OGG"));
		return NAME_OGG;
	}

	bool FMixerPlatformCoreAudio::HasCompressedAudioInfoClass(USoundWave* InSoundWave)
	{
#if WITH_OGGVORBIS
		return true;
#else
		return false;
#endif
	}

	ICompressedAudioInfo* FMixerPlatformCoreAudio::CreateCompressedAudioInfo(USoundWave* InSoundWave)
	{
#if WITH_OGGVORBIS
		return new FVorbisAudioInfo();
#else
		return NULL;
#endif
	}

	FString FMixerPlatformCoreAudio::GetDefaultDeviceName()
	{
		return FString();
	}

	FAudioPlatformSettings FMixerPlatformCoreAudio::GetPlatformSettings() const
	{
		return FAudioPlatformSettings();
	}

	bool FMixerPlatformCoreAudio::InitRunLoop()
	{
		CFRunLoopRef RunLoop = nullptr;
		AudioObjectPropertyAddress Property = NEW_GLOBAL_PROPERTY(kAudioHardwarePropertyRunLoop);
		OSStatus Status = AudioObjectSetPropertyData(kAudioObjectSystemObject, &Property, 0, nullptr, sizeof(CFRunLoopRef), &RunLoop);
		CORE_AUDIO_ERR(Status, "Failed to initialize run loop");
		return true;
	}

	bool FMixerPlatformCoreAudio::GetOutputDevices()
	{
		if(DeviceArray.Num() == 0)
		{
			// Only need to do this once
			
			UInt32 DataSize;
			AudioObjectPropertyAddress Property = NEW_GLOBAL_PROPERTY(kAudioHardwarePropertyDevices);
			OSStatus Status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &Property, 0, nullptr, &DataSize);
			CORE_AUDIO_ERR(Status, "Failed to get size of devices property");

			UInt32 NumDevices = DataSize / sizeof(AudioDeviceID);
			AudioDeviceID Devices[NumDevices];

			Status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &Property, 0, nullptr, &DataSize, (void*)&Devices);
			CORE_AUDIO_ERR(Status, "Failed to get device list");

			for (UInt32 i = 0; i < NumDevices; ++i)
			{
				FDevice	NewDevice;
				NewDevice.DeviceID = Devices[i];
				
				Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyStreamConfiguration);
				Status = AudioObjectGetPropertyDataSize(NewDevice.DeviceID, &Property, 0, nullptr, &DataSize);
				CORE_AUDIO_ERR(Status, "Failed to get stream configuration size");

				AudioBufferList* BufferList = (AudioBufferList*)alloca(DataSize);

				Status = AudioObjectGetPropertyData(NewDevice.DeviceID, &Property, 0, nullptr, &DataSize, BufferList);
				CORE_AUDIO_ERR(Status, "Failed to get stream configuration");

				if (BufferList->mNumberBuffers != 0)
				{
					if (!GetDeviceInfo(NewDevice.DeviceID, NewDevice.DeviceInfo))
					{
						return false;
					}
					
					DeviceArray.Add(NewDevice);
				}
			}
		}
		
		return true;
	}

	bool FMixerPlatformCoreAudio::GetDefaultOutputDevice()
	{
		UInt32 PropertySize = sizeof(AudioDeviceID);
		AudioObjectPropertyAddress Property = NEW_GLOBAL_PROPERTY(kAudioHardwarePropertyDefaultOutputDevice);
		OSStatus Result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &Property, 0, nullptr, &PropertySize, &DefaultDeviceId);
		CORE_AUDIO_ERR(Result, "Failed to get default output device");
		
		for (int32 i = 0; i < DeviceArray.Num(); ++i)
		{

			if (DeviceArray[i].DeviceID == DefaultDeviceId)
			{
				DeviceArray[i].DeviceInfo.bIsSystemDefault = true;
				DefaultDeviceIndex = i;
				AudioStreamInfo.DeviceInfo = DeviceArray[i].DeviceInfo;
			}
		}
		
		return true;
	}

	bool FMixerPlatformCoreAudio::GetDeviceInfo(AudioDeviceID DeviceID, FAudioPlatformDeviceInfo& DeviceInfo)
	{
		DeviceInfo.Format = EAudioMixerStreamDataFormat::Float;
		DeviceInfo.DeviceId = FString::Printf(TEXT("%x"), DeviceID);

		bool bSuccess = GetDeviceName(DeviceID, DeviceInfo.Name);
		bSuccess &= GetDeviceSampleRate(DeviceID, DeviceInfo.SampleRate);
		bSuccess &=	GetDeviceChannels(DeviceID, DeviceInfo.OutputChannelArray);
		
		DeviceInfo.NumChannels = DeviceInfo.OutputChannelArray.Num();
		
		AudioValueRange BufferSizeRange;
		UInt32 DataSize = sizeof(AudioValueRange);
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyBufferFrameSizeRange);
		OSStatus Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &DataSize, &BufferSizeRange);
		CORE_AUDIO_ERR(Status, "Failed to get callback buffer size range");
	
		return bSuccess;
	}

	bool FMixerPlatformCoreAudio::GetDeviceName(AudioDeviceID DeviceID, FString& OutName)
	{
		AudioObjectPropertyAddress Property = NEW_GLOBAL_PROPERTY(kAudioObjectPropertyManufacturer);

		CFStringRef ManufacturerName;
		UInt32 DataSize = sizeof(CFStringRef);
		OSStatus Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &DataSize, &ManufacturerName);
		CORE_AUDIO_ERR(Status, "Failed to get device manufacturer name");

		// Convert the Apple CFStringRef to an Unreal FString
		TCHAR ManufacturerNameBuff[256];
		FPlatformString::CFStringToTCHAR(ManufacturerName, ManufacturerNameBuff);
		CFRelease(ManufacturerName);

		CFStringRef DeviceName;
		DataSize = sizeof(CFStringRef);
		Property.mSelector = kAudioObjectPropertyName;
		Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &DataSize, &DeviceName);
		CORE_AUDIO_ERR(Status, "Failed to get device name");

		// Convert the Apple CFStringRef to an Unreal FString
		TCHAR DeviceNameBuff[256];
		FPlatformString::CFStringToTCHAR(DeviceName, DeviceNameBuff);
		CFRelease(DeviceName);

		OutName = FString::Printf(TEXT("%s - %s"), ManufacturerNameBuff, DeviceNameBuff);
		return true;
	}

	bool FMixerPlatformCoreAudio::GetDeviceLatency(AudioDeviceID DeviceID, UInt32& Latency)
	{
		UInt32 DataSize = sizeof(UInt32);
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyLatency);
		if (AudioObjectHasProperty(DeviceID, &Property))
		{
			OSStatus Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &DataSize, &Latency);
			CORE_AUDIO_ERR(Status, "Failed to get device latency");
		}
		return true;
	}

	bool FMixerPlatformCoreAudio::GetDeviceSampleRate(AudioDeviceID DeviceID, int32& OutSampleRate)
	{
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyAvailableNominalSampleRates);

		UInt32 DataSize;
		OSStatus Status = AudioObjectGetPropertyDataSize(DeviceID, &Property, 0, nullptr, &DataSize);
		CORE_AUDIO_ERR(Status, "Failed to get nominal sample rates size");

		UInt32 NumSampleRates = DataSize / sizeof(AudioValueRange);
		AudioValueRange SampleRates[NumSampleRates];

		Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &DataSize, &SampleRates);
		CORE_AUDIO_ERR(Status, "Failed to get nominal sample rates");

		// The only sample rates we care about are 48k and 44.1k, find the largest one supported
		UInt32 LargestSampleRate = 0;
		for (uint32 i = 0; i < NumSampleRates; ++i)
		{
			const AudioValueRange& SampleRate = SampleRates[i];

			// If the min and max sample rate is the same, then just add this to the list of frame rates
			if (SampleRate.mMinimum == SampleRate.mMaximum)
			{
				uint32 TargetSampleRate = (uint32)SampleRate.mMinimum;
				if(TargetSampleRate > LargestSampleRate && TargetSampleRate <= 48000)
				{
					LargestSampleRate = TargetSampleRate;
				}
			}
			else
			{
				if(SampleRate.mMinimum <= 48000 && 48000 <= SampleRate.mMaximum)
				{
					LargestSampleRate = 48000;
					break;
				}
				
				if(SampleRate.mMinimum <= 44100 && 44100 <= SampleRate.mMaximum && 44100 >= LargestSampleRate)
				{
					LargestSampleRate = 44100;
				}
				
			}
		}

		if (LargestSampleRate != 44100 && LargestSampleRate != 48000)
		{
			UA_DEVICE_PLATFORM_ERROR(TEXT("Audio device doesn't support 48k or 44.1k sample rates."));
			return false;
		}
		
		OutSampleRate = LargestSampleRate;
		
		return true;
	}

	bool FMixerPlatformCoreAudio::GetDeviceChannels(AudioDeviceID DeviceID, TArray<EAudioMixerChannel::Type>& OutChannels)
	{
		// First get number of channels by getting the buffer list
		UInt32 PropertySize;
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyStreamConfiguration);
		OSStatus Status = AudioObjectGetPropertyDataSize(DeviceID, &Property, 0, nullptr, &PropertySize);
		CORE_AUDIO_ERR(Status, "Failed to get stream configuration property size");

		AudioBufferList* BufferList = (AudioBufferList*)alloca(PropertySize);
		Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &PropertySize, BufferList);
		CORE_AUDIO_ERR(Status, "Failed to get stream configuration property");

		UInt32 NumChannels = 0;
		for (UInt32 i = 0; i < BufferList->mNumberBuffers; ++i)
		{
			NumChannels += BufferList->mBuffers[i].mNumberChannels;
		}

		if (NumChannels == 0)
		{
			UA_DEVICE_PLATFORM_ERROR(TEXT("Output device has 0 channels"));
			return false;
		}

		// CoreAudio has a different property for stereo speaker layouts
		if (NumChannels == 2)
		{
			return GetDeviceChannelsForStereo(DeviceID, OutChannels);
		}

		// Now CORE_AUDIO_ERR multi-channel layouts
		Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyPreferredChannelLayout);
		if (!AudioObjectHasProperty(DeviceID, &Property))
		{
			return false;
		}

		// Bool used to try different methods of getting channel layout (which is surprisingly complex!)
		bool bSuccess = true;
		Status = AudioObjectGetPropertyDataSize(DeviceID, &Property, 0, nullptr, &PropertySize);
		CORE_AUDIO_ERR(Status, "Failed to get preferred channel layout size");

		AudioChannelLayout* ChannelLayout = (AudioChannelLayout*)alloca(PropertySize);
		Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &PropertySize, ChannelLayout);
		CORE_AUDIO_ERR(Status, "Failed to get preferred channel layout");

		if (ChannelLayout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
		{
			bSuccess = GetDeviceChannelsForLayoutDescriptions(ChannelLayout, OutChannels);
		}
		else if (ChannelLayout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap)
		{
			bSuccess = GetDeviceChannelsForBitMap(ChannelLayout->mChannelBitmap, OutChannels);
		}
		else
		{
			bSuccess = GetDeviceChannelsForLayoutTag(ChannelLayout->mChannelLayoutTag, OutChannels);
		}

		// If everything failed, then lets just get the number of channels and "guess" the layout
		if (!bSuccess)
		{
			bSuccess = GetDeviceChannelsForChannelCount(NumChannels, OutChannels);
		}

		return bSuccess;
	}

	bool FMixerPlatformCoreAudio::GetDeviceChannelsForStereo(AudioDeviceID DeviceID, TArray<EAudioMixerChannel::Type>& OutChannels)
	{
		UInt32 ChannelIndices[2];
		UInt32 PropertySize = sizeof(ChannelIndices);
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyPreferredChannelsForStereo);
		OSStatus Status = AudioObjectGetPropertyData(DeviceID, &Property, 0, nullptr, &PropertySize, ChannelIndices);
		CORE_AUDIO_ERR(Status, "Failed to get preferred channels for stereo property");

		if (ChannelIndices[0] == kAudioChannelLabel_Left)
		{
			OutChannels.Add(EAudioMixerChannel::FrontLeft);
			OutChannels.Add(EAudioMixerChannel::FrontRight);
		}
		else
		{
			OutChannels.Add(EAudioMixerChannel::FrontRight);
			OutChannels.Add(EAudioMixerChannel::FrontLeft);
		}
		return true;
	}

	bool FMixerPlatformCoreAudio::GetDeviceChannelsForLayoutDescriptions(AudioChannelLayout* ChannelLayout, TArray<EAudioMixerChannel::Type>& OutChannels)
	{
		bool bSuccess = true;
		if (ChannelLayout->mNumberChannelDescriptions == 0)
		{
			bSuccess = false;
		}
		else
		{
			for (UInt32 Channel = 0; Channel < ChannelLayout->mNumberChannelDescriptions; ++Channel)
			{
				AudioChannelDescription Description = ChannelLayout->mChannelDescriptions[Channel];
				EAudioMixerChannel::Type Speaker;
				switch(Description.mChannelLabel)
				{
					case kAudioChannelLabel_Left:					Speaker = EAudioMixerChannel::FrontLeft;			break;
					case kAudioChannelLabel_Right:					Speaker = EAudioMixerChannel::FrontRight;			break;
					case kAudioChannelLabel_Center:					Speaker = EAudioMixerChannel::FrontCenter;			break;
					case kAudioChannelLabel_LFEScreen:				Speaker = EAudioMixerChannel::LowFrequency;			break;
					case kAudioChannelLabel_LeftSurround:			Speaker = EAudioMixerChannel::SideLeft;				break;
					case kAudioChannelLabel_RightSurround:			Speaker = EAudioMixerChannel::SideRight;			break;
					case kAudioChannelLabel_LeftCenter:				Speaker = EAudioMixerChannel::FrontLeftOfCenter;	break;
					case kAudioChannelLabel_RightCenter:			Speaker = EAudioMixerChannel::FrontRightOfCenter;	break;
					case kAudioChannelLabel_CenterSurround:			Speaker = EAudioMixerChannel::BackCenter;			break;
					case kAudioChannelLabel_LeftSurroundDirect:		Speaker = EAudioMixerChannel::SideLeft;				break;
					case kAudioChannelLabel_RightSurroundDirect:	Speaker = EAudioMixerChannel::SideRight;			break;
					case kAudioChannelLabel_TopCenterSurround:		Speaker = EAudioMixerChannel::TopCenter;			break;
					case kAudioChannelLabel_VerticalHeightLeft:		Speaker = EAudioMixerChannel::TopFrontLeft;			break;
					case kAudioChannelLabel_VerticalHeightCenter:	Speaker = EAudioMixerChannel::TopFrontCenter;		break;
					case kAudioChannelLabel_VerticalHeightRight:	Speaker = EAudioMixerChannel::TopFrontRight;		break;
					case kAudioChannelLabel_TopBackLeft:			Speaker = EAudioMixerChannel::TopBackLeft;			break;
					case kAudioChannelLabel_TopBackCenter:			Speaker = EAudioMixerChannel::TopBackCenter;		break;
					case kAudioChannelLabel_TopBackRight:			Speaker = EAudioMixerChannel::TopBackRight;			break;
					case kAudioChannelLabel_RearSurroundLeft:		Speaker = EAudioMixerChannel::BackLeft;				break;
					case kAudioChannelLabel_RearSurroundRight:		Speaker = EAudioMixerChannel::BackRight;			break;
					case kAudioChannelLabel_Unused:					Speaker = EAudioMixerChannel::Unknown;				break;
					case kAudioChannelLabel_Unknown:				Speaker = EAudioMixerChannel::Unknown;				break;

					default:
						UA_DEVICE_PLATFORM_ERROR(TEXT("Unknown or unsupported channel label"));
						OutChannels.Empty();
						return false;
				}
				OutChannels.Add(Speaker);
			}
		}
		return bSuccess;
	}

	bool FMixerPlatformCoreAudio::GetDeviceChannelsForBitMap(UInt32 BitMap, TArray<EAudioMixerChannel::Type>& OutChannels)
	{
		bool bSuccess = true;
		// Bit maps for standard speaker layouts
		static const UInt32 BitMapMono				= kAudioChannelBit_Center;
		static const UInt32 BitMapStereo			= kAudioChannelBit_Left | kAudioChannelBit_Right;
		static const UInt32 BitMapStereoPoint1		= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_LFEScreen;
		static const UInt32 BitMapSurround			= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center | kAudioChannelBit_CenterSurround;
		static const UInt32 BitMapQuad				= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround;
		static const UInt32 BitMap4Point1			= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround | kAudioChannelBit_LFEScreen;
		static const UInt32 BitMap5Point1			= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center | kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround | kAudioChannelBit_LFEScreen;
		static const UInt32 BitMap7Point1			= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center | kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround | kAudioChannelBit_LFEScreen | kAudioChannelBit_LeftCenter | kAudioChannelBit_RightCenter;
		static const UInt32 BitMap5Point1Surround	= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center | kAudioChannelBit_LeftSurroundDirect | kAudioChannelBit_RightSurroundDirect | kAudioChannelBit_LFEScreen;
		static const UInt32 BitMap7Point1Surround	= kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center | kAudioChannelBit_LeftSurroundDirect | kAudioChannelBit_RightSurroundDirect | kAudioChannelBit_LFEScreen | kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround;

		switch(BitMap)
		{
			case BitMapMono:
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				break;

			case BitMapStereo:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				break;

			case BitMapStereoPoint1:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				break;

			case BitMapSurround:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::BackCenter);
				break;

			case BitMapQuad:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				break;

			case BitMap4Point1:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				break;

			case BitMap5Point1:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				break;

			case BitMap7Point1:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::FrontLeftOfCenter);
				OutChannels.Add(EAudioMixerChannel::FrontRightOfCenter);
				break;

			case BitMap5Point1Surround:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				OutChannels.Add(EAudioMixerChannel::SideLeft);
				OutChannels.Add(EAudioMixerChannel::SideRight);
				break;

			case BitMap7Point1Surround:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::SideLeft);
				OutChannels.Add(EAudioMixerChannel::SideRight);
				break;

			default:
				UA_DEVICE_PLATFORM_ERROR(TEXT("Unknown or unsupported channel bitmap"));
				bSuccess = false;
				break;
		}
		return bSuccess;
	}

	bool FMixerPlatformCoreAudio::GetDeviceChannelsForLayoutTag(AudioChannelLayoutTag LayoutTag, TArray<EAudioMixerChannel::Type>& OutChannels)
	{
		bool bSuccess = true;
		switch (LayoutTag)
		{
			case kAudioChannelLayoutTag_Mono:
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				break;

			case kAudioChannelLayoutTag_Stereo:
			case kAudioChannelLayoutTag_StereoHeadphones:
			case kAudioChannelLayoutTag_MatrixStereo:
			case kAudioChannelLayoutTag_MidSide:
			case kAudioChannelLayoutTag_XY:
			case kAudioChannelLayoutTag_Binaural:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				break;

			case kAudioChannelLayoutTag_Quadraphonic:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				break;

			case kAudioChannelLayoutTag_Pentagonal:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				break;

			case kAudioChannelLayoutTag_Hexagonal:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				break;

			case kAudioChannelLayoutTag_Octagonal:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				OutChannels.Add(EAudioMixerChannel::SideLeft);
				OutChannels.Add(EAudioMixerChannel::SideRight);
				break;

			default:
				bSuccess = false;
				break;
		}
		return bSuccess;
	}

	bool FMixerPlatformCoreAudio::GetDeviceChannelsForChannelCount(UInt32 NumChannels, TArray<EAudioMixerChannel::Type>& OutChannels)
	{
		bool bSuccess = true;
		switch (NumChannels)
		{
			case 1:
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				break;

			case 2:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				break;

			case 3:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				break;

			case 4:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				break;

			case 5:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				break;

			case 6:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				break;

			case 8:
				OutChannels.Add(EAudioMixerChannel::FrontLeft);
				OutChannels.Add(EAudioMixerChannel::FrontRight);
				OutChannels.Add(EAudioMixerChannel::BackLeft);
				OutChannels.Add(EAudioMixerChannel::BackRight);
				OutChannels.Add(EAudioMixerChannel::FrontCenter);
				OutChannels.Add(EAudioMixerChannel::LowFrequency);
				OutChannels.Add(EAudioMixerChannel::SideLeft);
				OutChannels.Add(EAudioMixerChannel::SideRight);
				break;

			default:
				bSuccess = false;
				UA_DEVICE_PLATFORM_ERROR(TEXT("Failed to get a speaker array from number of channels"));
				break;
		}
		return bSuccess;
	}

	bool FMixerPlatformCoreAudio::InitDeviceOutputId(UInt32 DeviceIndex)
	{
		if (DeviceIndex >= DeviceArray.Num())
		{
			return false;
		}
		OutputDeviceId = DeviceArray[DeviceIndex].DeviceID;
		OutputDeviceIndex = DeviceIndex;
		return true;
	}

	bool FMixerPlatformCoreAudio::InitDeviceFrameRate(const UInt32 RequestedFrameRate, bool& bChanged)
	{
		bool bSupportedFrameRate = false;
		AudioStreamInfo.DeviceInfo.SampleRate = RequestedFrameRate;
		FAudioPlatformDeviceInfo& DeviceInfo = DeviceArray[OutputDeviceIndex].DeviceInfo;

		// CORE_AUDIO_ERR what the sample rate of the device is currently set to
		Float64 NominalSampleRate;
		UInt32 DataSize = sizeof(Float64);
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyNominalSampleRate);
		OSStatus Status = AudioObjectGetPropertyData(OutputDeviceId, &Property, 0, nullptr, &DataSize, &NominalSampleRate);
		CORE_AUDIO_ERR(Status, "Failed to get nominal sample rate");

		// If the desired sample rate is different than the nominal sample rate, then we change it
		if (FMath::Abs(NominalSampleRate - (Float64)RequestedFrameRate) > 1.0)
		{
			// Set a property listener to make sure we set the new sample rate correctly
			Float64 ReportedSampleRate = 0.0;
			AudioObjectPropertyAddress Property2 = NEW_GLOBAL_PROPERTY(kAudioDevicePropertyNominalSampleRate);
			Status = AudioObjectAddPropertyListener(OutputDeviceId, &Property2, SampleRatePropertyListener, (void*)&ReportedSampleRate);
			CORE_AUDIO_ERR(Status, "Failed to add a sample rate property listener");

			NominalSampleRate = (Float64)RequestedFrameRate;
			Status = AudioObjectSetPropertyData(OutputDeviceId, &Property, 0, nullptr, DataSize, &NominalSampleRate);
			CORE_AUDIO_ERR(Status, "Failed to set a sample rate on device");

			// Wait until the sample rate has changed to the requested rate with a timeout
			UInt32 Counter = 0;
			const UInt32 Inc = 5000;
			const UInt32 Timeout = 5000000;
			bool bTimedout = false;
			while (ReportedSampleRate != NominalSampleRate)
			{
				Counter += Inc;
				if (Counter > Timeout)
				{
					bTimedout = true;
					break;
				}
				usleep(Inc);
			}

			Status = AudioObjectRemovePropertyListener(OutputDeviceId, &Property2, SampleRatePropertyListener, (void*)&ReportedSampleRate);
			CORE_AUDIO_ERR(Status, "Failed to remove the sample rate property listener");

			if (bTimedout)
			{
				UA_DEVICE_PLATFORM_ERROR(TEXT("Timed out while setting sample rate of audio device."));
				return false;
			}

			bChanged = true;
		}
		return true;
	}

	bool FMixerPlatformCoreAudio::InitDeviceVirtualFormat(bool bSampleRateChanged)
	{
		// Change the output stream virtual format if we need to
		AudioStreamBasicDescription Format;
		UInt32 DataSize = sizeof(AudioStreamBasicDescription);
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioStreamPropertyVirtualFormat);
		OSStatus Status = AudioObjectGetPropertyData(OutputDeviceId, &Property, 0, nullptr, &DataSize, &Format);
		CORE_AUDIO_ERR(Status, "Failed to get audio stream virtual format");

		// we'll only change the virtual format if we need to
		bool bChangeFormat = false;
		if (bSampleRateChanged)
		{
			bChangeFormat = true;
			Format.mSampleRate = (Float64)AudioStreamInfo.DeviceInfo.SampleRate;
		}

		if (Format.mFormatID != kAudioFormatLinearPCM)
		{
			bChangeFormat = true;
			Format.mFormatID = kAudioFormatLinearPCM;
		}

		if (bChangeFormat)
		{
			Status = AudioObjectSetPropertyData(OutputDeviceId, &Property, 0, nullptr, DataSize, &Format);
			CORE_AUDIO_ERR(Status, "Failed to set the virtual format on device");
		}

		return true;
	}

	bool FMixerPlatformCoreAudio::InitDevicePhysicalFormat()
	{
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioStreamPropertyPhysicalFormat);
		AudioStreamBasicDescription Format;
		UInt32 DataSize = sizeof(AudioStreamBasicDescription);
		OSStatus Status = AudioObjectGetPropertyData(OutputDeviceId, &Property, 0, nullptr, &DataSize, &Format);
		CORE_AUDIO_ERR(Status, "Failed to get audio stream physical format");

		if (Format.mFormatID != kAudioFormatLinearPCM || Format.mBitsPerChannel < 16)
		{
			bool bSuccess = false;
			Format.mFormatID = kAudioFormatLinearPCM;

			// Make a copy of the old format
			AudioStreamBasicDescription NewFormat = Format;

			// Try setting the physical format to Float32
			NewFormat.mFormatFlags = Format.mFormatFlags;
			NewFormat.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
			NewFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsSignedInteger;

			NewFormat.mBitsPerChannel = 32;
			NewFormat.mBytesPerFrame = 4 * Format.mChannelsPerFrame;
			NewFormat.mBytesPerPacket = NewFormat.mBytesPerFrame * NewFormat.mFramesPerPacket;

			Status = AudioObjectSetPropertyData(OutputDeviceId, &Property, 0, nullptr, DataSize, &NewFormat);
			if (Status == noErr)
			{
				bSuccess = true;
			}

			// Lets try signed 32 bit integer format
			if (!bSuccess)
			{
				NewFormat.mFormatFlags = Format.mFormatFlags;
				NewFormat.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
				NewFormat.mFormatFlags |= kAudioFormatFlagIsPacked;
				NewFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsFloat;

				Status = AudioObjectSetPropertyData(OutputDeviceId, &Property, 0, nullptr, DataSize, &NewFormat);
				if (Status == noErr)
				{
					bSuccess = true;
				}
			}

			// Now try 24 bit signed integer
			if (!bSuccess)
			{
				NewFormat.mBitsPerChannel = 24;
				NewFormat.mBytesPerFrame = 3 * Format.mChannelsPerFrame;
				NewFormat.mBytesPerPacket = NewFormat.mBytesPerFrame * NewFormat.mFramesPerPacket;

				Status = AudioObjectSetPropertyData(OutputDeviceId, &Property, 0, nullptr, DataSize, &NewFormat);
				if (Status == noErr)
				{
					bSuccess = true;
				}
			}

			if (!bSuccess)
			{
				// TODO: look at supporting other physical formats if we fail here
				UA_DEVICE_PLATFORM_ERROR(TEXT("Failed to set physical format of the audio device to something reasonable."));
				return false;
			}
		}

		return true;
	}

	bool FMixerPlatformCoreAudio::InitDeviceNumDeviceStreams()
	{

		UInt32 DataSize;
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyStreamConfiguration);
		OSStatus Status = AudioObjectGetPropertyDataSize(OutputDeviceId, &Property, 0, nullptr, &DataSize);
		CORE_AUDIO_ERR(Status, "Failed to get stream config size");

		AudioBufferList* BufferList = (AudioBufferList*)alloca(DataSize);
		Status = AudioObjectGetPropertyData(OutputDeviceId, &Property, 0, nullptr, &DataSize, BufferList);
		CORE_AUDIO_ERR(Status, "Failed to get output stream configuration");

		NumDeviceStreams = BufferList->mNumberBuffers;

		return true;
	}

	bool FMixerPlatformCoreAudio::InitDeviceOverrunCallback()
	{
		AudioObjectPropertyAddress Property = NEW_GLOBAL_PROPERTY(kAudioDeviceProcessorOverload);
		OSStatus Status = AudioObjectAddPropertyListener(OutputDeviceId, &Property, OverrunPropertyListener, (void *)this);
		CORE_AUDIO_ERR(Status, "Failed to set device overrun property listener");
		return true;
	}

	bool FMixerPlatformCoreAudio::InitDeviceCallback(const FAudioMixerOpenStreamParams& Params)
	{
		// Determine device callback buffer size range and set the best callback buffer size
		AudioValueRange BufferSizeRange;
		UInt32 DataSize = sizeof(AudioValueRange);
		AudioObjectPropertyAddress Property = NEW_OUTPUT_PROPERTY(kAudioDevicePropertyBufferFrameSizeRange);
		OSStatus Status = AudioObjectGetPropertyData(OutputDeviceId, &Property, 0, nullptr, &DataSize, &BufferSizeRange);
		CORE_AUDIO_ERR(Status, "Failed to get callback buffer size range");

		AudioStreamInfo.NumOutputFrames = (UInt32)FMath::Clamp(Params.NumFrames, (uint32)BufferSizeRange.mMinimum, (uint32)BufferSizeRange.mMaximum);

		DataSize = sizeof(UInt32);
		Property.mSelector = kAudioDevicePropertyBufferFrameSize;
		Status = AudioObjectSetPropertyData(OutputDeviceId, &Property, 0, nullptr, DataSize, &AudioStreamInfo.NumOutputFrames);
		CORE_AUDIO_ERR(Status, "Failed to set the callback buffer size");

		Status = AudioDeviceCreateIOProcID(OutputDeviceId, CoreAudioCallback, (void*)this, &DeviceIOProcId);
		CORE_AUDIO_ERR(Status, "Failed to set device callback function");
		
		return true;
	}

	bool FMixerPlatformCoreAudio::PerformCallback(AudioDeviceID DeviceId, const AudioBufferList* OutputBuffer)
	{
		bInCallback = true;
		
		if (AudioStreamInfo.StreamState == EAudioOutputStreamState::Running)
		{
			#if UNREAL_AUDIO_TEST_WHITE_NOISE
			
				for (uint32 bufferItr = 0; bufferItr < OutputBuffer->mNumberBuffers; ++bufferItr)
				{
					float*	floatSampleData = (float*)OutputBuffer->mBuffers[bufferItr].mData;
					for (uint32 Sample = 0; Sample < OutputBuffer->mBuffers[bufferItr].mDataByteSize / sizeof(float); ++Sample)
					{
						floatSampleData[Sample] = 0.5f * FMath::FRandRange(-1.0f, 1.0f);
					}
				}
			
			#else // UNREAL_AUDIO_TEST_WHITE_NOISE

				coreAudioBuffer = OutputBuffer->mBuffers[0].mData;
				coreAudioBufferByteSize = OutputBuffer->mBuffers[0].mDataByteSize;
				ReadNextBuffer();	// This will trigger a call to SubmitBuffer() which will copy the audio generated by the mixer into the core audio buffer
				
			#endif // UNREAL_AUDIO_TEST_WHITE_NOISE
		}
		else
		{
			for (uint32 bufferItr = 0; bufferItr < OutputBuffer->mNumberBuffers; ++bufferItr)
			{
				memset(OutputBuffer->mBuffers[bufferItr].mData, 0, OutputBuffer->mBuffers[bufferItr].mDataByteSize);
			}
		}
		
		bInCallback = false;
		
		return true;
	}

	/** Core audio callback function called when the output device is ready for more data. */
	OSStatus FMixerPlatformCoreAudio::CoreAudioCallback(	AudioDeviceID DeviceId,
										const AudioTimeStamp* CurrentTimeStamp,
										const AudioBufferList* InputBufferData,
										const AudioTimeStamp* InputTime,
										AudioBufferList* OutputBufferData,
										const AudioTimeStamp* OutputTime,
										void* UserData)
	{
		// Get the user data and cast to our FMixerPlatformCoreAudio object
		FMixerPlatformCoreAudio* me = (FMixerPlatformCoreAudio*) UserData;

		// Call our callback function
		if (!me->PerformCallback(DeviceId, OutputBufferData))
		{
			// Something went wrong...
			return kAudioHardwareUnspecifiedError;
		}
		
		// Everything went cool...
		return kAudioHardwareNoError;
	}

	/** A CoreAudio Property Listener to listen to whether or not the audio callback is overloaded (resulting in hitching or buffer over/underun). */
	OSStatus FMixerPlatformCoreAudio::OverrunPropertyListener(AudioObjectID DeviceId,
											UInt32 NumAddresses,
											const AudioObjectPropertyAddress* Addresses,
											void* UserData)
	{
		// Looks like nothing to do here for now
		
		return kAudioHardwareNoError;
	}

	/** A CoreAudio Property Listener to listen to the sample rate property when it gets changed. */
	OSStatus FMixerPlatformCoreAudio::SampleRatePropertyListener(	AudioObjectID DeviceId,
												UInt32 NumAddresses,
												const AudioObjectPropertyAddress* Addresses,
												void* SampleRatePtr)
	{
		Float64* SampleRate = (Float64*)SampleRatePtr;
		UInt32 DataSize = sizeof(Float64);
		AudioObjectPropertyAddress PropertyAddress = NEW_GLOBAL_PROPERTY(kAudioDevicePropertyNominalSampleRate);
		OSStatus Status = AudioObjectGetPropertyData(DeviceId, &PropertyAddress, 0, nullptr, &DataSize, SampleRate);
		return Status;
	}


}
