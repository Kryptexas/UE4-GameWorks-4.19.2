// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WmfMediaPrivate.h"

#if WMFMEDIA_SUPPORTED_PLATFORM

#include "CoreTypes.h"
#include "HAL/CriticalSection.h"

class FWmfMediaStreamSink;


/**
 * Implements a media sink object for the WMF pipeline.
 */
class FWmfMediaSink
	: public IMFClockStateSink
	, public IMFMediaSink
{
public:

	/** Default constructor. */
	FWmfMediaSink();

public:

	/**
	 * Initialize this sink.
	 *
	 * @param InStreamSink The stream sink to use for the fixed stream.
	 * @return true on success, false otherwise.
	 */
	bool Initialize(FWmfMediaStreamSink& InStreamSink);

public:

	//~ IMFClockStateSink interface

	STDMETHODIMP OnClockPause(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockRestart(MFTIME hnsSystemTime);
	STDMETHODIMP OnClockSetRate(MFTIME hnsSystemTime, float flRate);
	STDMETHODIMP OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
	STDMETHODIMP OnClockStop(MFTIME hnsSystemTime);

public:

	// ~IMFMediaSink interface

	STDMETHODIMP AddStreamSink(DWORD dwStreamSinkIdentifier, __RPC__in_opt IMFMediaType* pMediaType, __RPC__deref_out_opt IMFStreamSink** ppStreamSink);
	STDMETHODIMP GetCharacteristics(__RPC__out DWORD* pdwCharacteristics);
	STDMETHODIMP GetPresentationClock(__RPC__deref_out_opt IMFPresentationClock** ppPresentationClock);
	STDMETHODIMP GetStreamSinkById(DWORD dwIdentifier, __RPC__deref_out_opt IMFStreamSink** ppStreamSink);
	STDMETHODIMP GetStreamSinkByIndex(DWORD dwIndex, __RPC__deref_out_opt IMFStreamSink** ppStreamSink);
	STDMETHODIMP GetStreamSinkCount(__RPC__out DWORD* pcStreamSinkCount);
	STDMETHODIMP RemoveStreamSink(DWORD dwStreamSinkIdentifier);
	STDMETHODIMP SetPresentationClock(__RPC__in_opt IMFPresentationClock* pPresentationClock);
	STDMETHODIMP Shutdown();

public:

	//~ IUnknown interface

	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP QueryInterface(REFIID RefID, void** Object);
	STDMETHODIMP_(ULONG) Release();

private:

	/** Hidden destructor (this class is reference counted). */
	virtual ~FWmfMediaSink();

private:

	/** Critical section for synchronizing access to this sink. */
	FCriticalSection CriticalSection;

	/** Holds a reference counter for this instance. */
	int32 RefCount;

	/** The presentation clock used by this sink. */
	TComPtr<IMFPresentationClock> PresentationClock;

	/** The stream sink. */
	TComPtr<FWmfMediaStreamSink> StreamSink;
};

#endif
