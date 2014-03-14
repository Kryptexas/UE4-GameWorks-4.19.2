// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "VoicePrivatePCH.h"
#include "VoiceCaptureWindows.h"

#include "AllowWindowsPlatformTypes.h"

/** Calculate silence in an audio buffer by using a RMS threshold */
template<typename T>
bool IsSilence(T* Buffer, int32 BuffSize)
{
	if (BuffSize == 0)
	{
		return true;
	} 

	const int32 IterSize = BuffSize / sizeof(T);

	double Total = 0.0f;
	for (int32 i = 0; i < IterSize; i++) 
	{
		Total += Buffer[i];
	}

	const double Average = Total / IterSize;

	double SumMeanSquare = 0.0f;
	double Diff = 0.0f;
	for (int32 i = 0; i < IterSize; i++) 
	{
		Diff = (Buffer[i] - Average);
		SumMeanSquare += (Diff * Diff);
	}

	double AverageMeanSquare = SumMeanSquare / IterSize;

	static double Threshold = 75.0 * 75.0;
	return AverageMeanSquare < Threshold;
}

/** Callback to access all the voice capture devices on the platform */
BOOL CALLBACK CaptureDeviceCallback(
	LPGUID lpGuid,
	LPCSTR lpcstrDescription,
	LPCSTR lpcstrModule,
	LPVOID lpContext
	)
{
	// @todo identify the proper device
	FVoiceCaptureWindows* VCPtr = (FVoiceCaptureWindows*)(lpContext);
	UE_LOG(LogVoice, Display, TEXT("Device: %s Desc: %s"), lpcstrDescription, lpcstrModule);
	return true;
}

FVoiceCaptureWindows::FVoiceCaptureWindows() :
	VoiceCaptureState(EVoiceCaptureState::UnInitialized),
	DirectSound(NULL),
	VoiceCaptureDev(NULL),
	VoiceCaptureBuffer8(NULL),
	LastEventTriggered(STOP_EVENT - 1),
	LastBufferWritten(1),
	NextCaptureOffset(0)
{
	FMemory::Memzero(&WavFormat, sizeof(WavFormat));
	FMemory::Memzero(&VoiceCaptureBufferDesc, sizeof(VoiceCaptureBufferDesc));
	FMemory::Memzero(&VoiceCaptureDevCaps, sizeof(VoiceCaptureDevCaps));
	FMemory::Memzero(&VoiceCaptureBufferCaps8, sizeof(VoiceCaptureBufferCaps8));
	FMemory::Memzero(&Events, ARRAY_COUNT(Events) * sizeof(HANDLE));

	for (int i = 0; i < NUM_EVENTS; i++)
	{
		Events[i] = INVALID_HANDLE_VALUE;
	}

	AudioBuffers[0].Empty(MAX_UNCOMPRESSED_VOICE_BUFFER_SIZE);
	AudioBuffers[1].Empty(MAX_UNCOMPRESSED_VOICE_BUFFER_SIZE);
}

FVoiceCaptureWindows::~FVoiceCaptureWindows()
{
	Shutdown();
}

bool FVoiceCaptureWindows::Init(int32 SampleRate, int32 NumChannels)
{
	if (SampleRate < 8000 || SampleRate > 48000)
	{
		UE_LOG(LogVoice, Warning, TEXT("Voice capture doesn't support %d hz"), SampleRate);
		return false;
	}

	if (NumChannels < 0 || NumChannels > 2)
	{
		UE_LOG(LogVoice, Warning, TEXT("Voice capture only supports 1 or 2 channels"));
		return false;
	}

	// @todo move this to voice module (only init/deinit once ever)
	if (FAILED(DirectSoundCreate8(NULL, &DirectSound, NULL)))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to init DirectSound"));
		return false;
	}

	if (FAILED(DirectSoundCaptureEnumerate((LPDSENUMCALLBACK)CaptureDeviceCallback, this)))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to enumerate devices"));
		return false;
	}

	// DSDEVID_DefaultCapture WAVEINCAPS 
	if (FAILED(DirectSoundCaptureCreate8(&DSDEVID_DefaultVoiceCapture, &VoiceCaptureDev, NULL)))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to create capture device"));
		return false;
	}

	// Device capabilities
	VoiceCaptureDevCaps.dwSize = sizeof(DSCCAPS);
	if (FAILED(VoiceCaptureDev->GetCaps(&VoiceCaptureDevCaps)))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to get mic device caps"));
		return false;
	}

	// Wave format setup
	WavFormat.wFormatTag = WAVE_FORMAT_PCM;
	WavFormat.nChannels = NumChannels;
	WavFormat.wBitsPerSample = 16;
	WavFormat.nSamplesPerSec = SampleRate;
	WavFormat.nBlockAlign = (WavFormat.nChannels * WavFormat.wBitsPerSample) / 8;
	WavFormat.nAvgBytesPerSec = WavFormat.nBlockAlign * WavFormat.nSamplesPerSec;
	WavFormat.cbSize = 0;

	// Buffer setup
	VoiceCaptureBufferDesc.dwSize = sizeof(DSCBUFFERDESC);
	VoiceCaptureBufferDesc.dwFlags = 0;
	VoiceCaptureBufferDesc.dwBufferBytes = WavFormat.nAvgBytesPerSec / 2; // .5 sec buffer
	VoiceCaptureBufferDesc.dwReserved = 0;
	VoiceCaptureBufferDesc.lpwfxFormat = &WavFormat;
	VoiceCaptureBufferDesc.dwFXCount = 0;
	VoiceCaptureBufferDesc.lpDSCFXDesc = NULL;

	LPDIRECTSOUNDCAPTUREBUFFER VoiceBuffer = NULL;
	if (FAILED(VoiceCaptureDev->CreateCaptureBuffer(&VoiceCaptureBufferDesc, &VoiceBuffer, NULL)))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to create voice capture buffer"));
		return false;
	}

	HRESULT BufferCreateHR = VoiceBuffer->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&VoiceCaptureBuffer8);
	VoiceBuffer->Release(); 
	VoiceBuffer = NULL;
	if (FAILED(BufferCreateHR))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to create voice capture buffer"));
		return false;
	}

	VoiceCaptureBufferCaps8.dwSize = sizeof(DSCBCAPS);
	if (FAILED(VoiceCaptureBuffer8->GetCaps(&VoiceCaptureBufferCaps8)))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to get voice buffer caps"));
		return false;
	}

	// TEST ------------------------
	if (0)
	{
		DWORD SizeWritten8 = 0;
		VoiceCaptureBuffer8->GetFormat(NULL, sizeof(WAVEFORMATEX), &SizeWritten8);

		LPWAVEFORMATEX BufferFormat8 = (WAVEFORMATEX*)FMemory::Malloc(SizeWritten8);
		VoiceCaptureBuffer8->GetFormat(BufferFormat8, SizeWritten8, &SizeWritten8);
		FMemory::Free(BufferFormat8);
	}
	// TEST ------------------------

	if (!CreateNotifications(VoiceCaptureBufferCaps8.dwBufferBytes))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to create voice buffer notifications"));
		return false;
	}

	VoiceCaptureState = EVoiceCaptureState::NotCapturing;
	return true;
}

void FVoiceCaptureWindows::Shutdown()
{
	Stop();

	if (VoiceCaptureBuffer8)
	{
		VoiceCaptureBuffer8->Release();
		VoiceCaptureBuffer8 = NULL;
	}

	if (VoiceCaptureDev)
	{
		VoiceCaptureDev->Release();
		VoiceCaptureDev = NULL;
	}

	// @todo move to voice module
	if (DirectSound)
	{
		DirectSound->Release();
		DirectSound = NULL;
	}

	for (int i = 0; i < ARRAY_COUNT(Events); i++)
	{
		if (Events[i] != INVALID_HANDLE_VALUE)
		{
			CloseHandle(Events[i]);
			Events[i] = INVALID_HANDLE_VALUE;
		}
	}

	VoiceCaptureState = EVoiceCaptureState::UnInitialized;
}

bool FVoiceCaptureWindows::Start()
{
	check(VoiceCaptureState != EVoiceCaptureState::UnInitialized);

	if (FAILED(VoiceCaptureBuffer8->Start(DSCBSTART_LOOPING)))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to start capture"));
		return false;
	}

	VoiceCaptureState = EVoiceCaptureState::NoData;
	return true;
}

void FVoiceCaptureWindows::Stop()
{
	if (VoiceCaptureBuffer8 && 
		VoiceCaptureState != EVoiceCaptureState::Stopping && 
		VoiceCaptureState != EVoiceCaptureState::NotCapturing)
	{
		VoiceCaptureBuffer8->Stop();
		VoiceCaptureState = EVoiceCaptureState::Stopping;
	}
}

bool FVoiceCaptureWindows::IsCapturing()
{		
	if (VoiceCaptureBuffer8)
	{
		DWORD Status = 0;
		if (FAILED(VoiceCaptureBuffer8->GetStatus(&Status)))
		{
			UE_LOG(LogVoice, Warning, TEXT("Failed to get voice buffer status"));
		}

		// Status & DSCBSTATUS_LOOPING
		return Status & DSCBSTATUS_CAPTURING ? true : false;
	}

	return false;
}

EVoiceCaptureState::Type FVoiceCaptureWindows::GetCaptureState(uint32& OutAvailableVoiceData) const
{
	if (VoiceCaptureState != EVoiceCaptureState::UnInitialized &&
		VoiceCaptureState != EVoiceCaptureState::Error)
	{
		OutAvailableVoiceData = AudioBuffers[LastBufferWritten].Num();
	}
	else
	{
		OutAvailableVoiceData = 0;
	}

	return VoiceCaptureState;
}

void FVoiceCaptureWindows::ProcessData()
{
	DWORD CurrentCapturePos = 0;
	DWORD CurrentReadPos = 0;

	HRESULT HR = VoiceCaptureBuffer8 ? VoiceCaptureBuffer8->GetCurrentPosition(&CurrentCapturePos, &CurrentReadPos) : E_FAIL;
	if (FAILED(HR))
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to get voice buffer cursor position 0x%08x"), HR);
		VoiceCaptureState = EVoiceCaptureState::Error;
		return;
	}

	DWORD LockSize = ((CurrentReadPos - NextCaptureOffset) + VoiceCaptureBufferCaps8.dwBufferBytes) % VoiceCaptureBufferCaps8.dwBufferBytes;
	if(LockSize != 0) 
	{ 
		DWORD CaptureFlags = 0;
		DWORD CaptureLength = 0;
		void* CaptureData = NULL;
		DWORD CaptureLength2 = 0;
		void* CaptureData2 = NULL;
		HR = VoiceCaptureBuffer8->Lock(NextCaptureOffset, LockSize,
			&CaptureData, &CaptureLength, 
			&CaptureData2, &CaptureLength2, CaptureFlags);
		if (SUCCEEDED(HR))
		{
			LastBufferWritten = 0;//!LastBufferWritten;

			CaptureLength = FMath::Min(CaptureLength, (DWORD)MAX_UNCOMPRESSED_VOICE_BUFFER_SIZE);
			CaptureLength2 = FMath::Min(CaptureLength2, (DWORD)MAX_UNCOMPRESSED_VOICE_BUFFER_SIZE - CaptureLength);

			AudioBuffers[LastBufferWritten].Empty(MAX_UNCOMPRESSED_VOICE_BUFFER_SIZE);
			AudioBuffers[LastBufferWritten].AddUninitialized(CaptureLength + CaptureLength2);

			uint8* AudioBuffer = AudioBuffers[LastBufferWritten].GetTypedData();
			FMemory::Memcpy(AudioBuffer, CaptureData, CaptureLength);

			if (CaptureData2 && CaptureLength2 > 0)
			{
				FMemory::Memcpy(AudioBuffer + CaptureLength, CaptureData2, CaptureLength2);
			}

			VoiceCaptureBuffer8->Unlock(CaptureData, CaptureLength, CaptureData2, CaptureLength2);

			// Move the capture offset forward.
			NextCaptureOffset = (NextCaptureOffset + CaptureLength) % VoiceCaptureBufferCaps8.dwBufferBytes; 
			NextCaptureOffset = (NextCaptureOffset + CaptureLength2) % VoiceCaptureBufferCaps8.dwBufferBytes; 

			if (IsSilence((int16*)AudioBuffer, AudioBuffers[LastBufferWritten].Num()))
			{
				VoiceCaptureState = EVoiceCaptureState::NoData;
			}
			else
			{
				VoiceCaptureState = EVoiceCaptureState::Ok;
			}

#if !UE_BUILD_SHIPPING
			static double LastCapture = 0.0;
			double NewTime = FPlatformTime::Seconds();
			UE_LOG(LogVoice, VeryVerbose, TEXT("LastCapture: %f %s"), (NewTime - LastCapture) * 1000.0, EVoiceCaptureState::ToString(VoiceCaptureState));
			LastCapture = NewTime;
#endif
		}
		else
		{
			UE_LOG(LogVoice, Warning, TEXT("Failed to lock voice buffer 0x%08x"), HR);
			VoiceCaptureState = EVoiceCaptureState::Error;
		}
	}
}

EVoiceCaptureState::Type FVoiceCaptureWindows::GetVoiceData(uint8* OutVoiceBuffer, const uint32 InVoiceBufferSize, uint32& OutAvailableVoiceData)
{
	EVoiceCaptureState::Type NewMicState = VoiceCaptureState;
	OutAvailableVoiceData = 0;

	if (VoiceCaptureState == EVoiceCaptureState::Ok ||
		VoiceCaptureState == EVoiceCaptureState::Stopping)
	{
		OutAvailableVoiceData = AudioBuffers[LastBufferWritten].Num();
		if (InVoiceBufferSize >= OutAvailableVoiceData)
		{
			FMemory::Memcpy(OutVoiceBuffer, AudioBuffers[LastBufferWritten].GetTypedData(), OutAvailableVoiceData);
			VoiceCaptureState = EVoiceCaptureState::NoData;
		}
		else
		{
			NewMicState = EVoiceCaptureState::BufferTooSmall;
		}
	}

	return NewMicState;
}

bool FVoiceCaptureWindows::CreateNotifications(uint32 BufferSize)
{
	bool bSuccess = true;

	LPDIRECTSOUNDNOTIFY8 NotifyInt = NULL;

	HRESULT NotifyHR = VoiceCaptureBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&NotifyInt);
	if (SUCCEEDED(NotifyHR))
	{
		DSBPOSITIONNOTIFY NotifyEvents[NUM_EVENTS];

		// Create events.
		for (int i = 0; i < NUM_EVENTS; i++)
		{
			Events[i] = CreateEvent(NULL, true, false, NULL);
			if (Events[i] == INVALID_HANDLE_VALUE)
			{
				UE_LOG(LogVoice, Warning, TEXT("Error creating sync event"));
				bSuccess = false;
			}
		}

		if (bSuccess)
		{
			// Evenly distribute notifications throughout the buffer
			uint32 NumSegments = NUM_EVENTS - 1; 
			uint32 BufferSegSize = BufferSize / NumSegments;
			for (uint32 i = 0; i < NumSegments; i++)
			{
				NotifyEvents[i].dwOffset = ((i + 1) * BufferSegSize) - 1;
				NotifyEvents[i].hEventNotify = Events[i];
			}

			// when buffer stops
			NotifyEvents[STOP_EVENT].dwOffset = DSBPN_OFFSETSTOP;
			NotifyEvents[STOP_EVENT].hEventNotify = Events[STOP_EVENT];

			if (FAILED(NotifyInt->SetNotificationPositions(NUM_EVENTS, NotifyEvents)))
			{
				UE_LOG(LogVoice, Warning, TEXT("Failed to set notifications"));
				bSuccess = false;
			}

			float BufferSegMs = BufferSegSize / ((WavFormat.nSamplesPerSec / 1000.0) * (WavFormat.wBitsPerSample / 8.0));
			UE_LOG(LogVoice, Display, TEXT("%d notifications, every %d bytes [%f ms]"), NumSegments, BufferSegSize, BufferSegMs);
		}

		NotifyInt->Release();
	}
	else
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to create voice notification interface 0x%08x"), NotifyHR);
	}

	if (!bSuccess)
	{
		for (int i = 0; i < NUM_EVENTS; i++)
		{
			if (Events[i] != INVALID_HANDLE_VALUE)
			{
				CloseHandle(Events[i]);
				Events[i] = INVALID_HANDLE_VALUE;
			}
		}
	}

	return bSuccess;
}

bool FVoiceCaptureWindows::Tick(float DeltaTime)
{
	if (VoiceCaptureState != EVoiceCaptureState::UnInitialized &&
		VoiceCaptureState != EVoiceCaptureState::NotCapturing)
	{
#if 1
		uint32 NextEvent = (LastEventTriggered + 1) % (NUM_EVENTS - 1);
		if (WaitForSingleObject(Events[NextEvent], 0) == WAIT_OBJECT_0)
		{
			UE_LOG(LogVoice, VeryVerbose, TEXT("Event %d Signalled"), NextEvent);
			ProcessData();

			LastEventTriggered = NextEvent;
			ResetEvent(Events[NextEvent]);
		}
#else
		// All "non stop" events
		for (int32 EventIdx = 0; EventIdx < STOP_EVENT; EventIdx++)
		{
			if (WaitForSingleObject(Events[EventIdx], 0) == WAIT_OBJECT_0)
			{
				UE_LOG(LogVoice, VeryVerbose, TEXT("Event %d Signalled"), EventIdx);
				if (LastEventTriggered != EventIdx)
				{
					ProcessData();
				}

				LastEventTriggered = EventIdx;
				ResetEvent(Events[EventIdx]);
			}
		}
#endif

		if (Events[STOP_EVENT] != INVALID_HANDLE_VALUE && WaitForSingleObject(Events[STOP_EVENT], 0) == WAIT_OBJECT_0)
		{
			UE_LOG(LogVoice, Verbose, TEXT("Voice capture stopped"));
			ResetEvent(Events[STOP_EVENT]);
			VoiceCaptureState = EVoiceCaptureState::NotCapturing;
		}
	}

	return true;
}

#include "HideWindowsPlatformTypes.h"
