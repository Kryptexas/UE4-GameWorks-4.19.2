// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaStreamSink.h"


#if WMFMEDIA_SUPPORTED_PLATFORM

#include "Misc/ScopeLock.h"

#include "WmfMediaSink.h"
#include "WmfMediaUtils.h"


/* FWmfMediaStreamSink structors
 *****************************************************************************/

FWmfMediaStreamSink::FWmfMediaStreamSink(const GUID& InMajorType, DWORD InStreamId)
	: RefCount(0)
	, StreamId(InStreamId)
	, StreamType(InMajorType)
{ }


FWmfMediaStreamSink::~FWmfMediaStreamSink()
{
	check(RefCount == 0);
}


/* FWmfMediaStreamSink interface
 *****************************************************************************/

bool FWmfMediaStreamSink::Initialize(FWmfMediaSink& InOwner)
{
	FScopeLock Lock(&CriticalSection);

	const HRESULT Result = ::MFCreateEventQueue(&EventQueue);

	if (FAILED(Result))
	{
		UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Failed to create event queue for stream sink (%s)"), *WmfMedia::ResultToString(Result));
		return false;
	}

	Owner = &InOwner;

	return true;
}


FOnWmfMediaStreamSinkBuffer& FWmfMediaStreamSink::OnBuffer()
{
	FScopeLock Lock(&CriticalSection);
	return BufferDelegate;
}


FOnWmfMediaStreamSinkSample& FWmfMediaStreamSink::OnSample()
{
	FScopeLock Lock(&CriticalSection);
	return SampleDelegate;
}


void FWmfMediaStreamSink::Shutdown()
{
	FScopeLock Lock(&CriticalSection);

	if (EventQueue.IsValid())
	{
		EventQueue->Shutdown();
		EventQueue.Reset();
	}
}


/* IMFMediaEventGenerator interface
 *****************************************************************************/

STDMETHODIMP FWmfMediaStreamSink::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* pState)
{
	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	return EventQueue->BeginGetEvent(pCallback, pState);
}


STDMETHODIMP FWmfMediaStreamSink::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	return EventQueue->EndGetEvent(pResult, ppEvent);
}


STDMETHODIMP FWmfMediaStreamSink::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
	TComPtr<IMFMediaEventQueue> TempQueue;
	{
		FScopeLock Lock(&CriticalSection);

		if (!EventQueue.IsValid())
		{
			return MF_E_SHUTDOWN;
		}

		TempQueue = EventQueue;
	}

	return TempQueue->GetEvent(dwFlags, ppEvent);
}


STDMETHODIMP FWmfMediaStreamSink::QueueEvent(MediaEventType met, REFGUID extendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	return EventQueue->QueueEventParamVar(met, extendedType, hrStatus, pvValue);
}


/* IMFMediaTypeHandler interface
 *****************************************************************************/

STDMETHODIMP FWmfMediaStreamSink::GetCurrentMediaType(_Outptr_ IMFMediaType** ppMediaType)
{
	FScopeLock Lock(&CriticalSection);

	if (ppMediaType == NULL)
	{
		return E_POINTER;
	}

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	if (!CurrentMediaType.IsValid())
	{
		return MF_E_NOT_INITIALIZED;
	}

	*ppMediaType = CurrentMediaType;
	(*ppMediaType)->AddRef();

	return S_OK;
}


STDMETHODIMP FWmfMediaStreamSink::GetMajorType(__RPC__out GUID* pguidMajorType)
{
	if (pguidMajorType == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	if (!CurrentMediaType.IsValid())
	{
		return MF_E_NOT_INITIALIZED;
	}

	return CurrentMediaType->GetGUID(MF_MT_MAJOR_TYPE, pguidMajorType);
}


STDMETHODIMP FWmfMediaStreamSink::GetMediaTypeByIndex(DWORD dwIndex, _Outptr_ IMFMediaType** ppType)
{
	if (ppType == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	// get supported media type
	TArray<TComPtr<IMFMediaType>> SupportedTypes = WmfMedia::GetSupportedMediaTypes(StreamType);

	if (!SupportedTypes.IsValidIndex(dwIndex))
	{
		return MF_E_NO_MORE_TYPES;
	}

	TComPtr<IMFMediaType> SupportedType = SupportedTypes[dwIndex];

	if (!SupportedType.IsValid())
	{
		return MF_E_INVALIDMEDIATYPE;
	}

	// create result type
	TComPtr<IMFMediaType> MediaType;
	{
		HRESULT Result = ::MFCreateMediaType(&MediaType);

		if (FAILED(Result))
		{
			return Result;
		}

		Result = SupportedType->CopyAllItems(MediaType);

		if (FAILED(Result))
		{
			return Result;
		}
	}

	*ppType = MediaType;
	(*ppType)->AddRef();

	return S_OK;
}


STDMETHODIMP FWmfMediaStreamSink::GetMediaTypeCount(__RPC__out DWORD* pdwTypeCount)
{
	if (pdwTypeCount == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	*pdwTypeCount = (DWORD)WmfMedia::GetSupportedMediaTypes(StreamType).Num();

	return S_OK;
}


STDMETHODIMP FWmfMediaStreamSink::IsMediaTypeSupported(IMFMediaType* pMediaType, _Outptr_opt_result_maybenull_ IMFMediaType** ppMediaType)
{
	if (ppMediaType != NULL)
	{
		*ppMediaType = NULL;
	}

	if (pMediaType == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	// get requested major type
	GUID MajorType;
	{
		const HRESULT Result = pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &MajorType);

		if (FAILED(Result))
		{
			return Result;
		}
	}

	if (MajorType != StreamType)
	{
		return MF_E_INVALIDMEDIATYPE;
	}

	// compare media type
	const DWORD CompareFlags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA;

	for (const TComPtr<IMFMediaType>& MediaType : WmfMedia::GetSupportedMediaTypes(StreamType))
	{
		if (!MediaType.IsValid())
		{
			continue;
		}

		DWORD OutFlags = 0;
		const HRESULT Result = MediaType->IsEqual(pMediaType, &OutFlags);

		if (SUCCEEDED(Result) && ((OutFlags & CompareFlags) == CompareFlags))
		{
			return S_OK;
		}
	}

	return MF_E_INVALIDMEDIATYPE;
}


STDMETHODIMP FWmfMediaStreamSink::SetCurrentMediaType(IMFMediaType* pMediaType)
{
	if (pMediaType == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	const HRESULT Result = IsMediaTypeSupported(pMediaType, NULL);

	if (FAILED(Result))
	{
		return Result;
	}

	CurrentMediaType = pMediaType;

	return S_OK;
}


/* IMFStreamSink interface
 *****************************************************************************/

STDMETHODIMP FWmfMediaStreamSink::Flush()
{
	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}


	return S_OK;
}


STDMETHODIMP FWmfMediaStreamSink::GetIdentifier(__RPC__out DWORD* pdwIdentifier)
{
	if (pdwIdentifier == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	*pdwIdentifier = StreamId;

	return S_OK;
}


STDMETHODIMP FWmfMediaStreamSink::GetMediaSink(__RPC__deref_out_opt IMFMediaSink** ppMediaSink)
{
	if (ppMediaSink == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	*ppMediaSink = Owner;
	(*ppMediaSink)->AddRef();

	return S_OK;
}


STDMETHODIMP FWmfMediaStreamSink::GetMediaTypeHandler(__RPC__deref_out_opt IMFMediaTypeHandler** ppHandler)
{
	if (ppHandler == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	return QueryInterface(IID_IMFMediaTypeHandler, (void**)ppHandler);
}


STDMETHODIMP FWmfMediaStreamSink::PlaceMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType, __RPC__in const PROPVARIANT* pvarMarkerValue, __RPC__in const PROPVARIANT* pvarContextValue)
{
	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	return S_OK;
}


STDMETHODIMP FWmfMediaStreamSink::ProcessSample(__RPC__in_opt IMFSample* pSample)
{
	if (pSample == NULL)
	{
		return E_POINTER;
	}

	FScopeLock Lock(&CriticalSection);

	if (!EventQueue.IsValid())
	{
		return MF_E_SHUTDOWN;
	}

	DWORD NumBuffers;
	{
		const HRESULT Result = pSample->GetBufferCount(&NumBuffers);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Failed to get buffer count from media sample (%s)"), *WmfMedia::ResultToString(Result));
			return Result;
		}
	}

	FTimespan Duration;
	{
		LONGLONG SampleDuration = 0;
		const HRESULT Result = pSample->GetSampleDuration(&SampleDuration);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Failed to get sample duration from media sample (%s)"), *WmfMedia::ResultToString(Result));
			return Result;
		}

		Duration = FTimespan(SampleDuration);
	}

	DWORD Flags;
	{
		const HRESULT Result = pSample->GetSampleFlags(&Flags);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Failed to get sample flags from media sample (%s)"), *WmfMedia::ResultToString(Result));
			return Result;
		}
	}

	FTimespan Time;
	{
		LONGLONG SampleTime = 0;
		const HRESULT Result = pSample->GetSampleTime(&SampleTime);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Failed to get sample time from media sample (%s)"), *WmfMedia::ResultToString(Result));
			return Result;
		}

		Time = FTimespan(SampleTime);
	}

	if (BufferDelegate.IsBound())
	{
		TComPtr<IMFMediaBuffer> Buffer = NULL;
		{
			const HRESULT Result = pSample->ConvertToContiguousBuffer(&Buffer);

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Failed to get contiguous buffer from media sample (%s)"), *WmfMedia::ResultToString(Result));
				return Result;
			}
		}

		BYTE* ByteBuffer = NULL;
		DWORD Size = 0;
		{
			const HRESULT Result = Buffer->Lock(&ByteBuffer, NULL, &Size);

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Failed to lock contiguous buffer from media sample (%s)"), *WmfMedia::ResultToString(Result));
				return Result;
			}
		}

		BufferDelegate.Execute(ByteBuffer, Size, Time, Duration, Flags);
		Buffer->Unlock();
	}

	if (SampleDelegate.IsBound())
	{
		SampleDelegate.Execute(*pSample, NumBuffers, Time, Duration, Flags);
	}

	return S_OK;
}


/* IUnknown interface
 *****************************************************************************/

STDMETHODIMP_(ULONG) FWmfMediaStreamSink::AddRef()
{
	return FPlatformAtomics::InterlockedIncrement(&RefCount);
}


#if _MSC_VER == 1900
	#pragma warning(push)
	#pragma warning(disable:4838)
#endif

STDMETHODIMP FWmfMediaStreamSink::QueryInterface(REFIID RefID, void** Object)
{
	static const QITAB QITab[] =
	{
		QITABENT(FWmfMediaStreamSink, IMFStreamSink),
		{ 0 }
	};

	return QISearch(this, QITab, RefID, Object);
}

#if _MSC_VER == 1900
	#pragma warning(pop)
#endif


STDMETHODIMP_(ULONG) FWmfMediaStreamSink::Release()
{
	int32 CurrentRefCount = FPlatformAtomics::InterlockedDecrement(&RefCount);

	if (CurrentRefCount == 0)
	{
		delete this;
	}

	return CurrentRefCount;
}


#endif
