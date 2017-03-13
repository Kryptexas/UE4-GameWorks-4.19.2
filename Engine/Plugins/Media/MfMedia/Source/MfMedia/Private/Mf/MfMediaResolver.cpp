// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MfMediaResolver.h"

#if MFMEDIA_SUPPORTED_PLATFORM

#include "MfMediaByteStream.h"
#include "MfMediaResolveState.h"

#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif


/* MfMediaUrlResolver structors
 *****************************************************************************/

FMfMediaResolver::FMfMediaResolver()
	: RefCount(0)
{
	if (FAILED(::MFCreateSourceResolver(&SourceResolver)))
	{
		UE_LOG(LogMfMedia, Error, TEXT("Failed to create media source resolver."));
	}
}


/* MfMediaUrlResolver interface
 *****************************************************************************/

void FMfMediaResolver::Cancel()
{
	if (SourceResolver == NULL)
	{
		return;
	}

	if (ResolveState != NULL)
	{
		ResolveState->Invalidate();
		ResolveState.Reset();
	}

	TComPtr<IUnknown> CancelCookieCopy = CancelCookie;

	if (CancelCookieCopy != NULL)
	{
		SourceResolver->CancelObjectCreation(CancelCookieCopy);
		CancelCookie.Reset();
	}
}


bool FMfMediaResolver::ResolveByteStream(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, IMfMediaResolverCallbacks& Callbacks)
{
	if (SourceResolver == NULL)
	{
		return false;
	}

	Cancel();

	TComPtr<FMfMediaByteStream> ByteStream = new FMfMediaByteStream(Archive);
	TComPtr<FMfMediaResolveState> NewResolveState = new(std::nothrow) FMfMediaResolveState(EMfMediaResolveType::ByteStream, OriginalUrl, Callbacks);
	HRESULT Result = SourceResolver->BeginCreateObjectFromByteStream(ByteStream, *OriginalUrl, MF_RESOLUTION_MEDIASOURCE, NULL, &CancelCookie, this, NewResolveState);

	if (FAILED(Result))
	{
		UE_LOG(LogMfMedia, Warning, TEXT("Failed to resolve byte stream: %s (%i)"), *OriginalUrl, Result);

		return false;
	}

	ResolveState = NewResolveState;

	return true;
}


bool FMfMediaResolver::ResolveUrl(const FString& Url, IMfMediaResolverCallbacks& Callbacks)
{
	if (SourceResolver == NULL)
	{
		return false;
	}

	Cancel();

	TComPtr<FMfMediaResolveState> NewResolveState = new(std::nothrow) FMfMediaResolveState(EMfMediaResolveType::Url, Url, Callbacks);
	HRESULT Result = SourceResolver->BeginCreateObjectFromURL(*Url, MF_RESOLUTION_MEDIASOURCE, NULL, &CancelCookie, this, NewResolveState);

	if (FAILED(Result))
	{
		UE_LOG(LogMfMedia, Warning, TEXT("Failed to resolve URL: %s (%08x)"), *Url, Result);

		return false;
	}

	ResolveState = NewResolveState;

	return true;
}


/* IMFAsyncCallback interface
 *****************************************************************************/

STDMETHODIMP_(ULONG) FMfMediaResolver::AddRef()
{
	return FPlatformAtomics::InterlockedIncrement(&RefCount);
}


STDMETHODIMP FMfMediaResolver::GetParameters(unsigned long*, unsigned long*)
{
	return E_NOTIMPL;
}


STDMETHODIMP FMfMediaResolver::Invoke(IMFAsyncResult* AsyncResult)
{
	// are we currently resolving anything?
	TComPtr<FMfMediaResolveState> ResolveStateCopy = ResolveState;

	if (ResolveStateCopy == NULL)
	{
		return S_OK;
	}

	// is this the resolve state we care about?
	FMfMediaResolveState* State = NULL;

	if (FAILED(AsyncResult->GetState((IUnknown**)&State)) || (State != ResolveStateCopy))
	{
		return S_OK;
	}

	CancelCookie.Reset();

	// get the asynchronous result
	MF_OBJECT_TYPE ObjectType;
	TComPtr<IUnknown> SourceObject;
	HRESULT Result = S_FALSE;

	if (State->Type == EMfMediaResolveType::ByteStream)
	{
		Result = SourceResolver->EndCreateObjectFromByteStream(AsyncResult, &ObjectType, &SourceObject);
	}
	else if (State->Type == EMfMediaResolveType::Url)
	{
		Result = SourceResolver->EndCreateObjectFromURL(AsyncResult, &ObjectType, &SourceObject);
	}

	if (SUCCEEDED(Result))
	{
		State->ResolveComplete(SourceObject, State->Url);
	}
	else
	{
		UE_LOG(LogMfMedia, Warning, TEXT("Failed to finish resolve: %s (%08x, %08x)"), *State->Url, Result, AsyncResult->GetStatus());

		State->ResolveFailed(State->Url);
	}

	ResolveState.Reset();

	return S_OK;
}


STDMETHODIMP FMfMediaResolver::QueryInterface(REFIID RefID, void** Object)
{
	if (Object == NULL)
	{
		return E_INVALIDARG;
	}

	if ((RefID == IID_IUnknown) || (RefID == IID_IMFAsyncCallback))
	{
		*Object = (LPVOID)this;
		AddRef();

		return NOERROR;
	}

	*Object = NULL;

	return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) FMfMediaResolver::Release()
{
	int32 CurrentRefCount = FPlatformAtomics::InterlockedDecrement(&RefCount);
	
	if (CurrentRefCount == 0)
	{
		delete this;
	}

	return CurrentRefCount;
}


#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif //MFMEDIA_SUPPORTED_PLATFORM
