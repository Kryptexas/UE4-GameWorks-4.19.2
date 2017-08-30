// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WmfMediaPrivate.h"

#if WMFMEDIA_SUPPORTED_PLATFORM

#include "CoreTypes.h"
#include "Delegates/Delegate.h"
#include "HAL/CriticalSection.h"
#include "Misc/Timespan.h"

class FWmfMediaSink;


/** Declares a delegate that gets fired when the sink received a new sample. */
DECLARE_DELEGATE_FiveParams(FOnWmfMediaStreamSinkSample,
	IMFSample& /*Sample*/,
	DWORD /*NumBuffers*/,
	FTimespan /*Time*/,
	FTimespan /*Duration*/,
	DWORD /*Flags*/
);


/** Declares a delegate that gets fired when the sink received a new sample. */
DECLARE_DELEGATE_FiveParams(FOnWmfMediaStreamSinkBuffer,
	const BYTE* /*Buffer*/,
	DWORD /*Size*/,
	FTimespan /*Time*/,
	FTimespan /*Duration*/,
	DWORD /*Flags*/
);


namespace WmfMediaStreamSink
{
	const DWORD FixedStreamId = 1;
}


/**
 * Implements a stream sink object for the WMF pipeline.
 */
class FWmfMediaStreamSink
	: public IMFMediaTypeHandler
	, public IMFStreamSink
{
public:

	/**
	 * Create and initialize a new instance.
	 *
	 * @param InMajorType The sink's major media type, i.e. audio or video.
	 * @param InStreamId The sink's identifier.
	 */
	FWmfMediaStreamSink(const GUID& InMajorType, DWORD InStreamId);

public:

	/**
	 * Initialize this sink.
	 *
	 * @param InOwner The media sink that owns this stream sink.
	 * @return true on success, false otherwise.
	 * @see Shutdown
	 */
	bool Initialize(FWmfMediaSink& InOwner);

	/** Get a delegate that is executed when the sink received a new sample. */
	FOnWmfMediaStreamSinkBuffer& OnBuffer();

	/** Get a delegate that is executed when the sink received a new sample. */
	FOnWmfMediaStreamSinkSample& OnSample();

	/**
	 * Shut down this sink.
	 *
	 * @see Initialize
	 */
	void Shutdown();

public:

	//~ IMFMediaEventGenerator interface

	STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* pState);
	STDMETHODIMP EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
	STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent);
	STDMETHODIMP QueueEvent(MediaEventType met, REFGUID extendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

public:

	//~ IMFMediaTypeHandler

	STDMETHODIMP GetCurrentMediaType(_Outptr_ IMFMediaType** ppMediaType);
	STDMETHODIMP GetMajorType(__RPC__out GUID* pguidMajorType);
	STDMETHODIMP GetMediaTypeByIndex(DWORD dwIndex, _Outptr_ IMFMediaType** ppType);
	STDMETHODIMP GetMediaTypeCount(__RPC__out DWORD* pdwTypeCount);
	STDMETHODIMP IsMediaTypeSupported(IMFMediaType* pMediaType, _Outptr_opt_result_maybenull_ IMFMediaType** ppMediaType);
	STDMETHODIMP SetCurrentMediaType(IMFMediaType* pMediaType);

public:

	//~ IMFStreamSink interface

	STDMETHODIMP Flush();
	STDMETHODIMP GetIdentifier(__RPC__out DWORD* pdwIdentifier);
	STDMETHODIMP GetMediaSink(__RPC__deref_out_opt IMFMediaSink** ppMediaSink);
	STDMETHODIMP GetMediaTypeHandler(__RPC__deref_out_opt IMFMediaTypeHandler** ppHandler);
	STDMETHODIMP PlaceMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType, __RPC__in const PROPVARIANT* pvarMarkerValue, __RPC__in const PROPVARIANT* pvarContextValue);
	STDMETHODIMP ProcessSample(__RPC__in_opt IMFSample* pSample);

public:

	//~ IUnknown interface

	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP QueryInterface(REFIID RefID, void** Object);
	STDMETHODIMP_(ULONG) Release();

private:

	/** Hidden destructor (this class is reference counted). */
	virtual ~FWmfMediaStreamSink();

private:

	/** The delegate that gets fired when the sink received a new sample. */
	FOnWmfMediaStreamSinkBuffer BufferDelegate;

	/** Critical section for synchronizing access to this sink. */
	FCriticalSection CriticalSection;

	/** The event queue. */
	TComPtr<IMFMediaEventQueue> EventQueue;

	/** The sink's current media type. */
	TComPtr<IMFMediaType> CurrentMediaType;

	/** The media sink that owns this stream sink. */
	TComPtr<FWmfMediaSink> Owner;

	/** Holds a reference counter for this instance. */
	int32 RefCount;

	/** The delegate that gets fired when the sink received a new sample. */
	FOnWmfMediaStreamSinkSample SampleDelegate;

	/** The stream identifier (currently fixed). */
	DWORD StreamId;

	/** The sink's major media type. */
	const GUID StreamType;
};

#endif
