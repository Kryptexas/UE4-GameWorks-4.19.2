// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerPlatformAndroid.h"
#include "ModuleManager.h"
#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "VorbisAudioInfo.h"
#include "ADPCMAudioInfo.h"

#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"


DECLARE_LOG_CATEGORY_EXTERN(LogAudioMixerAndroid, Log, All);
DEFINE_LOG_CATEGORY(LogAudioMixerAndroid);

#define UNREAL_AUDIO_TEST_WHITE_NOISE 0

const uint32	cSampleBufferBits = 12;
const uint32	cNumChannels = 2;
const uint32	cAudioMixerBufferSize = 1 << cSampleBufferBits;
const uint32	cSampleBufferSize = cAudioMixerBufferSize * cNumChannels;

namespace Audio
{	
	FMixerPlatformAndroid::FMixerPlatformAndroid()
	:
	bInitialized(false),
	bInCallback(false)
	{
		tempBuffer = (int16*)FMemory::Malloc(cSampleBufferSize * sizeof(int16));
	}

	FMixerPlatformAndroid::~FMixerPlatformAndroid()
	{		
		if (bInitialized)
		{
			TeardownHardware();
		}

		if(tempBuffer != nullptr)
		{
			FMemory::Free(tempBuffer);
		}
	}

	//~ Begin IAudioMixerPlatformInterface
	bool FMixerPlatformAndroid::InitializeHardware()
	{
		if (bInitialized)
		{
			return false;
		}
				
		DeviceInfo.NumChannels = 2;
		DeviceInfo.SampleRate = 44100;
		DeviceInfo.DefaultSampleRate = DeviceInfo.SampleRate;
		DeviceInfo.NumFrames = cAudioMixerBufferSize;
		DeviceInfo.NumSamples = DeviceInfo.NumFrames * DeviceInfo.NumChannels;
		DeviceInfo.Format = EAudioMixerStreamDataFormat::Float;
		DeviceInfo.OutputChannelArray.SetNum(2);
		DeviceInfo.OutputChannelArray[0] = EAudioMixerChannel::FrontLeft;
		DeviceInfo.OutputChannelArray[1] = EAudioMixerChannel::FrontRight;
		DeviceInfo.Latency = 0;
		DeviceInfo.bIsSystemDefault = true;
		AudioStreamInfo.DeviceInfo = DeviceInfo;
		
		SLresult result;
		SLEngineOption EngineOption[] = { {(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE} };
	
		// create engine
		result = slCreateEngine( &SL_EngineObject, 1, EngineOption, 0, NULL, NULL);
		//check(SL_RESULT_SUCCESS == result);
		if (SL_RESULT_SUCCESS != result)
		{
			UE_LOG( LogAudioMixerAndroid, Error, TEXT("Engine create failed %d"), int32(result));
		}
	
		// realize the engine
		result = (*SL_EngineObject)->Realize(SL_EngineObject, SL_BOOLEAN_FALSE);
		check(SL_RESULT_SUCCESS == result);
	
		// get the engine interface, which is needed in order to create other objects
		result = (*SL_EngineObject)->GetInterface(SL_EngineObject, SL_IID_ENGINE, &SL_EngineEngine);
		check(SL_RESULT_SUCCESS == result);
	
		// create output mix 
		result = (*SL_EngineEngine)->CreateOutputMix(SL_EngineEngine, &SL_OutputMixObject, 0, NULL, NULL );
		check(SL_RESULT_SUCCESS == result);
	
		// realize the output mix
		result = (*SL_OutputMixObject)->Realize(SL_OutputMixObject, SL_BOOLEAN_FALSE);
		check(SL_RESULT_SUCCESS == result);

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Closed;
		
		bInitialized = true;

		return true;
	}

	bool FMixerPlatformAndroid::CheckAudioDeviceChange()
	{
		// only ever one device currently		
		return false;
	}

	bool FMixerPlatformAndroid::TeardownHardware()
	{
		if(!bInitialized)
		{
			return true;
		}
		
		StopAudioStream();
		CloseAudioStream();

		// Teardown OpenSLES..
		// Destroy the SLES objects in reverse order of creation:
		if (SL_OutputMixObject)
		{
			(*SL_OutputMixObject)->Destroy(SL_OutputMixObject);
			SL_OutputMixObject = NULL;
		}
		if (SL_EngineObject)
		{
			(*SL_EngineObject)->Destroy(SL_EngineObject);
			SL_EngineObject = NULL;
			SL_EngineEngine = NULL;
		}

		bInitialized = false;
		
		return true;
	}

	bool FMixerPlatformAndroid::IsInitialized() const
	{
		return bInitialized;
	}

	bool FMixerPlatformAndroid::GetNumOutputDevices(uint32& OutNumOutputDevices)
	{
		OutNumOutputDevices = 1;
		
		return true;
	}

	bool FMixerPlatformAndroid::GetOutputDeviceInfo(const uint32 InDeviceIndex, FAudioPlatformDeviceInfo& OutInfo)
	{
		OutInfo = DeviceInfo;
		return true;
	}

	bool FMixerPlatformAndroid::GetDefaultOutputDeviceIndex(uint32& OutDefaultDeviceIndex) const
	{
		OutDefaultDeviceIndex = 0;
		
		return true;
	}

	bool FMixerPlatformAndroid::OpenAudioStream(const FAudioMixerOpenStreamParams& Params)
	{
		if (!bInitialized || AudioStreamInfo.StreamState != EAudioOutputStreamState::Closed)
		{
			return false;
		}
		
		AudioStreamInfo.DeviceInfo = DeviceInfo;
		AudioStreamInfo.OutputDeviceIndex = Params.OutputDeviceIndex;
		AudioStreamInfo.AudioMixer = Params.AudioMixer;

		SLresult	result;

		// data info
		SLDataLocator_AndroidSimpleBufferQueue LocationBuffer = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
		
		// PCM Info
		SLDataFormat_PCM PCM_Format = {
			SL_DATAFORMAT_PCM, 
			2, 
			Params.SampleRate * 1000 ,	
			SL_PCMSAMPLEFORMAT_FIXED_16, 
			SL_PCMSAMPLEFORMAT_FIXED_16, 
			SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, 
			SL_BYTEORDER_LITTLEENDIAN 
		};
		
		SLDataSource SoundDataSource = { &LocationBuffer, &PCM_Format };
		
		// configure audio sink
		SLDataLocator_OutputMix Output_Mix = { SL_DATALOCATOR_OUTPUTMIX, SL_OutputMixObject };
		SLDataSink AudioSink = { &Output_Mix, NULL };

		// create audio player
		const SLInterfaceID	ids[] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
		const SLboolean		req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
		result = 
			(*SL_EngineEngine)->CreateAudioPlayer(
				SL_EngineEngine, 
				&SL_PlayerObject, 
				&SoundDataSource, 
				&AudioSink, 
				sizeof(ids) / sizeof(SLInterfaceID), 
				ids, 
				req );
		if(result != SL_RESULT_SUCCESS)
		{
			UE_LOG(LogAudioMixerAndroid, Warning, TEXT("FAILED OPENSL BUFFER CreateAudioPlayer 0x%x"), result);
			return false;
		}

		// realize the player
		result = (*SL_PlayerObject)->Realize(SL_PlayerObject, SL_BOOLEAN_FALSE);
		if (result != SL_RESULT_SUCCESS) { UE_LOG(LogAudioMixerAndroid, Warning, TEXT("FAILED OPENSL BUFFER Realize 0x%x"), result); return false; }
		
		// get the play interface
		result = (*SL_PlayerObject)->GetInterface(SL_PlayerObject, SL_IID_PLAY, &SL_PlayerPlayInterface);
		if (result != SL_RESULT_SUCCESS) { UE_LOG(LogAudioMixerAndroid, Warning, TEXT("FAILED OPENSL BUFFER GetInterface SL_IID_PLAY 0x%x"), result); return false; }
		
		// buffer system
		result = (*SL_PlayerObject)->GetInterface(SL_PlayerObject, SL_IID_BUFFERQUEUE, &SL_PlayerBufferQueue);
		if (result != SL_RESULT_SUCCESS) { UE_LOG(LogAudioMixerAndroid, Warning, TEXT("FAILED OPENSL BUFFER GetInterface SL_IID_BUFFERQUEUE 0x%x"), result); return false; }

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Open;

		return true;
	}

	bool FMixerPlatformAndroid::CloseAudioStream()
	{
		if (!bInitialized || (AudioStreamInfo.StreamState != EAudioOutputStreamState::Open && AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped))
		{
			return false;
		}
		
		(*SL_PlayerBufferQueue)->RegisterCallback(SL_PlayerBufferQueue, NULL, NULL);

		(*SL_PlayerObject)->Destroy(SL_PlayerObject);			
		SL_PlayerObject			= NULL;
		SL_PlayerPlayInterface	= NULL;
		SL_PlayerBufferQueue	= NULL;

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Closed;
		
		return true;
	}

	bool FMixerPlatformAndroid::StartAudioStream()
	{
		if (!bInitialized || (AudioStreamInfo.StreamState != EAudioOutputStreamState::Open && AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped))
		{
			return false;
		}

		SLresult	result;

		result = (*SL_PlayerBufferQueue)->RegisterCallback(SL_PlayerBufferQueue, OpenSLBufferQueueCallback, (void*)this);
		if (result != SL_RESULT_SUCCESS) { UE_LOG(LogAudioMixerAndroid, Warning, TEXT("FAILED OPENSL BUFFER QUEUE RegisterCallback 0x%x "), result); return false; }
		
		// Setup the output buffers
		for (int32 Index = 0; Index < NumMixerBuffers; ++Index)
		{
			OutputBuffers[Index].SetNumZeroed(DeviceInfo.NumSamples);
		}
		
		// Lets prime the first buffer
		FPlatformMemory::Memzero(OutputBuffers[0].GetData(), OutputBuffers[0].Num() * sizeof(float));
		AudioStreamInfo.AudioMixer->OnProcessAudioStream(OutputBuffers[0]);

		// Have the second buffer ready for submission when the first callback happens
		FPlatformMemory::Memzero(OutputBuffers[1].GetData(), OutputBuffers[1].Num() * sizeof(float));
		AudioStreamInfo.AudioMixer->OnProcessAudioStream(OutputBuffers[1]);

		CurrentBufferIndex = 1;

		// Give the platform audio device this first buffer to start the callback process
		SubmitBuffer(OutputBuffers[0]);

		// set the player's state to playing
		result = (*SL_PlayerPlayInterface)->SetPlayState(SL_PlayerPlayInterface, SL_PLAYSTATE_PLAYING);
		check(SL_RESULT_SUCCESS == result);

		return true;
	}

	bool FMixerPlatformAndroid::StopAudioStream()
	{
		if(!bInitialized || AudioStreamInfo.StreamState != EAudioOutputStreamState::Running)
		{
			return false;
		}
		
		AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopping;
		
		// set the player's state to stopped
		SLresult result = (*SL_PlayerPlayInterface)->SetPlayState(SL_PlayerPlayInterface, SL_PLAYSTATE_STOPPED);
		check(SL_RESULT_SUCCESS == result);

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopped;
		
		return true;
	}

	bool FMixerPlatformAndroid::MoveAudioStreamToNewAudioDevice(const FString& InNewDeviceId)
	{
		return false;
	}

	FAudioPlatformDeviceInfo FMixerPlatformAndroid::GetPlatformDeviceInfo() const
	{
		return AudioStreamInfo.DeviceInfo;
	}

	void FMixerPlatformAndroid::SubmitBuffer(const TArray<float>& Buffer)
	{
		float const*	floatData = Buffer.GetData();

		check(Buffer.Num() <= cSampleBufferSize);

		for(int i = 0; i < Buffer.Num(); ++i)
		{
			tempBuffer[i] = (int16)(floatData[i] * 32767.0f);
		}

		SLresult result = (*SL_PlayerBufferQueue)->Enqueue(SL_PlayerBufferQueue, tempBuffer, Buffer.Num() * sizeof(int16));
		if(result != SL_RESULT_SUCCESS) 
		{ 
			UE_LOG( LogAudioMixerAndroid, Warning, TEXT("FAILED OPENSL BUFFER Enqueue SL_PlayerBufferQueue (Requeing)"));  
		}
	}

	FName FMixerPlatformAndroid::GetRuntimeFormat(USoundWave* InSoundWave)
	{
		#if WITH_OGGVORBIS
		static FName NAME_OGG(TEXT("OGG"));
		if (InSoundWave->HasCompressedData(NAME_OGG))
		{
			return NAME_OGG;
		}
		#endif

		static FName NAME_ADPCM(TEXT("ADPCM"));

		return NAME_ADPCM;
	}

	bool FMixerPlatformAndroid::HasCompressedAudioInfoClass(USoundWave* InSoundWave)
	{
		return true;
	}

	ICompressedAudioInfo* FMixerPlatformAndroid::CreateCompressedAudioInfo(USoundWave* InSoundWave)
	{
		#if WITH_OGGVORBIS
		static FName NAME_OGG(TEXT("OGG"));
		if (InSoundWave->HasCompressedData(NAME_OGG))
		{
			return new FVorbisAudioInfo();
		}
		#endif

		static FName NAME_ADPCM(TEXT("ADPCM"));

		return new FADPCMAudioInfo();
	}

	FString FMixerPlatformAndroid::GetDefaultDeviceName()
	{
		return FString();
	}
	//~ End IAudioMixerPlatformInterface

	//~ Begin IAudioMixerDeviceChangedLister
	void FMixerPlatformAndroid::RegisterDeviceChangedListener()
	{

	}

	void FMixerPlatformAndroid::UnRegisterDeviceChangedListener()
	{

	}

	void FMixerPlatformAndroid::OnDefaultCaptureDeviceChanged(const EAudioDeviceRole InAudioDeviceRole, const FString& DeviceId)
	{

	}

	void FMixerPlatformAndroid::OnDefaultRenderDeviceChanged(const EAudioDeviceRole InAudioDeviceRole, const FString& DeviceId)
	{

	}

	void FMixerPlatformAndroid::OnDeviceAdded(const FString& DeviceId)
	{

	}

	void FMixerPlatformAndroid::OnDeviceRemoved(const FString& DeviceId)
	{

	}

	void FMixerPlatformAndroid::OnDeviceStateChanged(const FString& DeviceId, const EAudioDeviceState InState)
	{
		
	}
	//~ End IAudioMixerDeviceChangedLister

	void FMixerPlatformAndroid::ResumeContext()
	{
		if (bSuspended)
		{
			UE_LOG(LogAudioMixerAndroid, Display, TEXT("Resuming Audio"));
			bSuspended = false;
		}
	}
	
	void FMixerPlatformAndroid::SuspendContext()
	{
		if (!bSuspended)
		{
			UE_LOG(LogAudioMixerAndroid, Display, TEXT("Suspending Audio"));
			bSuspended = true;
		}
	}

	void FMixerPlatformAndroid::HandleCallback(void)
	{
		// Submit the ready buffer
		SubmitBuffer(OutputBuffers[CurrentBufferIndex]);
		CurrentBufferIndex = !CurrentBufferIndex;

		// Start processing the new buffer
		FPlatformMemory::Memzero(OutputBuffers[CurrentBufferIndex].GetData(), OutputBuffers[CurrentBufferIndex].Num() * sizeof(float));
		AudioStreamInfo.AudioMixer->OnProcessAudioStream(OutputBuffers[CurrentBufferIndex]);
	}

	void FMixerPlatformAndroid::OpenSLBufferQueueCallback( SLAndroidSimpleBufferQueueItf InQueueInterface, void* pContext ) 
	{
		FMixerPlatformAndroid* me = (FMixerPlatformAndroid*)pContext;
		if( me != nullptr )
		{
			me->HandleCallback();
		}
	}

}
