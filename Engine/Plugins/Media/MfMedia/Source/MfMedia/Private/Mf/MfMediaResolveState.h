// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMfMediaResolverCallbacks.h"
#include "Misc/ScopeLock.h"

#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif


/** Enumerates media source resolve types. */
enum class EMfMediaResolveType
{
	/** Not resolving right now. */
	None,

	/** Resolving a byte stream. */
	ByteStream,

	/** Resolving a file or web URL. */
	Url
};


/**
 * Implements media source resolve state information.
 */
struct FMfMediaResolveState
	: public IUnknown
{
	/** The type of the media source being resolved. */
	EMfMediaResolveType Type;

	/** The URL of the media source being resolved. */
	FString Url;

public:

	/** Creates and initializes a new instance. */
	FMfMediaResolveState(EMfMediaResolveType InType, const FString& InUrl, IMfMediaResolverCallbacks& InCallbacks)
		: Callbacks(&InCallbacks)
		, Type(InType)
		, Url(InUrl)
		, RefCount(0)
	{ }

public:

	/** Invalidate the resolve state (resolve events will no longer be forwarded). */
	void Invalidate()
	{
		FScopeLock Lock(&CriticalSection);
		Callbacks = nullptr;
	}

	/**
	 * Forward the event for a completed media resolve.
	 *
	 * @param SourceObject The media source object that was resolved.
	 * @param ResolvedUrl The resolved media URL.
	 */
	void ResolveComplete(TComPtr<IUnknown> SourceObject, FString ResolvedUrl)
	{
		FScopeLock Lock(&CriticalSection);

		if (Callbacks != nullptr)
		{
			Callbacks->ProcessResolveComplete(SourceObject, ResolvedUrl);
		}
	}

	/**
	 * Forward the event for a failed media resolve.
	 *
	 * @param FailedUrl The media URL that couldn't be resolved.
	 */
	void ResolveFailed(FString FailedUrl)
	{
		FScopeLock Lock(&CriticalSection);

		if (Callbacks != nullptr)
		{
			Callbacks->ProcessResolveFailed(FailedUrl);
		}
	}

public:

	//~ IUnknown interface

	STDMETHODIMP_(ULONG) AddRef()
	{
		return FPlatformAtomics::InterlockedIncrement(&RefCount);
	}

	STDMETHODIMP QueryInterface(REFIID RefID, void** Object)
	{
		if (Object == NULL)
		{
			return E_INVALIDARG;
		}

		if (RefID == IID_IUnknown)
		{
			*Object = (LPVOID)this;
			AddRef();

			return NOERROR;
		}

		*Object = NULL;

		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		int32 CurrentRefCount = FPlatformAtomics::InterlockedDecrement(&RefCount);
	
		if (CurrentRefCount == 0)
		{
			delete this;
		}

		return CurrentRefCount;
	}

private:

	/** Object that receives event callbacks. */
	IMfMediaResolverCallbacks* Callbacks;

	/** Critical section for gating access to Callbacks. */
	FCriticalSection CriticalSection;

	/** Holds a reference counter for this instance. */
	int32 RefCount;
};


#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif
