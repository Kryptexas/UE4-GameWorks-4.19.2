// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../MfMediaPrivate.h"

#if MFMEDIA_SUPPORTED_PLATFORM

#if PLATFORM_WINDOWS
	#include "WindowsHWrapper.h"
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif


class IMfMediaResolverCallbacks;
struct FMfMediaResolveState;


/**
 * Implements an asynchronous callback object for resolving media URLs.
 */
class FMfMediaResolver
	: public IMFAsyncCallback
{
public:

	/** Default constructor. */
	FMfMediaResolver();

public:

	/**
	 * Cancel the current media source resolve.
	 *
	 * Note: Because resolving media sources is an asynchronous operation, the
	 * callback delegates may still get invoked even after calling this method.
	 *
	 * @see IsResolving, ResolveByteStream, ResolveUrl
	 */
	void Cancel();

	/**
	 * Check whether a media URL is currently being resolved.
	 *
	 * @return true if a resolve is in progress, false otherwise.
	 * @see Cancel, ResolveByteStream, ResolveUrl
	 */
	bool IsResolving()
	{
		return (ResolveState != nullptr);
	}

	/**
	 * Resolves a media source from a byte stream.
	 *
	 * @param Archive The archive holding the media data.
	 * @param OriginalUrl The original URL of the media that was loaded into the buffer.
	 * @param Callbacks The object that handles resolver event callbacks.
	 * @return true if the byte stream will be resolved, false otherwise.
	 * @see Cancel, IsResolving, ResolveUrl
	 */
	bool ResolveByteStream(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, IMfMediaResolverCallbacks& Callbacks);

	/**
	 * Resolves a media source from a file or internet URL.
	 *
	 * @param Url The URL of the media to open (file name or web address).
	 * @param Callbacks The object that handles resolver event callbacks.
	 * @return true if the URL will be resolved, false otherwise.
	 * @see Cancel, IsResolving, ResolveByteStream
	 */
	bool ResolveUrl(const FString& Url, IMfMediaResolverCallbacks& Callbacks);

public:

	//~ IMFAsyncCallback interface

	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP GetParameters(unsigned long*, unsigned long*);
	STDMETHODIMP Invoke(IMFAsyncResult* AsyncResult);
	STDMETHODIMP QueryInterface(REFIID RefID, void** Object);
	STDMETHODIMP_(ULONG) Release();

private:

	/** Cancellation cookie object. */
	TComPtr<IUnknown> CancelCookie;

	/** Holds a reference counter for this instance. */
	int32 RefCount;

	/** The current resolve state. */
	TComPtr<FMfMediaResolveState> ResolveState;

	/** Holds the actual source resolver. */
	TComPtr<IMFSourceResolver> SourceResolver;
};


#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif
