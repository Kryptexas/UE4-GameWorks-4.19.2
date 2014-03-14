// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "CurlHttp.h"
#include "LinuxHttpManager.h"

CURLM * GMultiHandle = NULL;

namespace
{
	/** 
	 * A callback that libcurl will use to allocate memory
	 *
	 * @param Size size of allocation in bytes
	 * @return Pointer to memory chunk or NULL if failed
	 */
	void * CurlMalloc(size_t Size)
	{
		check(Size);
		return FMemory::Malloc(Size);
	}

	/** 
	 * A callback that libcurl will use to free memory
	 *
	 * @param Ptr pointer to memory chunk (may be NULL)
	 */
	void CurlFree(void * Ptr)
	{
		FMemory::Free(Ptr);
	}

	/** 
	 * A callback that libcurl will use to reallocate memory
	 *
	 * @param Ptr pointer to existing memory chunk (may be NULL)
	 * @param Size size of allocation in bytes
	 * @return Pointer to memory chunk or NULL if failed
	 */
	void * CurlRealloc(void * Ptr, size_t Size)
	{
		check(Size);
		return FMemory::Realloc(Ptr, Size);
	}

	/** 
	 * A callback that libcurl will use to duplicate a string
	 *
	 * @param ZeroTerminatedString pointer to string (ANSI or UTF-8, but this does not matter in this case)
	 * @return Pointer to a copy of string
	 */
	char * CurlStrdup(const char * ZeroTerminatedString)
	{
		char * Copy = NULL;
		check(ZeroTerminatedString);
		if (ZeroTerminatedString)
		{
			SIZE_T StrLen = FCStringAnsi::Strlen(ZeroTerminatedString);
			Copy = reinterpret_cast<char*>( FMemory::Malloc(StrLen + 1) );
			if (Copy)
			{
				FCStringAnsi::Strcpy(Copy, StrLen, ZeroTerminatedString);
				check(FCStringAnsi::Strcmp(Copy, ZeroTerminatedString) == 0);
			}
		}
		return Copy;
	}

	/** 
	 * A callback that libcurl will use to allocate zero-initialized memory
	 *
	 * @param NumElems number of elements to allocate (may be 0, then NULL should be returned)
	 * @param ElemSize size of each element in bytes (may be 0)
	 * @return Pointer to memory chunk, filled with zeroes or NULL if failed
	 */
	void * CurlCalloc(size_t NumElems, size_t ElemSize)
	{
		void * Return = NULL;
		const size_t Size = NumElems * ElemSize;
		if (Size)
		{
			Return = FMemory::Malloc(Size);

			if (Return)
			{
				FMemory::Memzero(Return, Size);
			}
		}

		return Return;
	}
}

void FLinuxPlatformHttp::Init()
{
	GMultiHandle = NULL;

	CURLcode InitResult = curl_global_init_mem(CURL_GLOBAL_ALL, CurlMalloc, CurlFree, CurlRealloc, CurlStrdup, CurlCalloc);
	if (InitResult == 0)
	{
		curl_version_info_data * VersionInfo = curl_version_info( CURLVERSION_NOW );
		if (VersionInfo)
		{
			UE_LOG(LogInit, Log, TEXT("Using libcurl %s"), ANSI_TO_TCHAR(VersionInfo->version));
			UE_LOG(LogInit, Log, TEXT(" - built for %s"), ANSI_TO_TCHAR(VersionInfo->host));

			if( VersionInfo->features & CURL_VERSION_SSL )
			{
				UE_LOG(LogInit, Log, TEXT(" - supports SSL with %s"), ANSI_TO_TCHAR(VersionInfo->ssl_version));
			}
			else
			{
				// No SSL
				UE_LOG(LogInit, Log, TEXT(" - NO SSL SUPPORT!"));
			}

			if( VersionInfo->features & CURL_VERSION_LIBZ )
			{
				UE_LOG(LogInit, Log, TEXT(" - supports HTTP deflate (compression) using libz %s"), ANSI_TO_TCHAR(VersionInfo->libz_version));
			}

			UE_LOG(LogInit, Log, TEXT(" - other features:"));

#define PrintCurlFeature(Feature)	\
			if ( VersionInfo->features & Feature ) \
			{ \
				UE_LOG(LogInit, Log, TEXT("     %s"), TEXT(#Feature));	\
			}

			PrintCurlFeature(CURL_VERSION_SSL);
			PrintCurlFeature(CURL_VERSION_LIBZ);

			PrintCurlFeature(CURL_VERSION_DEBUG);
			PrintCurlFeature(CURL_VERSION_IPV6);
			PrintCurlFeature(CURL_VERSION_ASYNCHDNS);
			PrintCurlFeature(CURL_VERSION_LARGEFILE);
			PrintCurlFeature(CURL_VERSION_IDN);
			PrintCurlFeature(CURL_VERSION_CONV);
			PrintCurlFeature(CURL_VERSION_TLSAUTH_SRP);

#undef PrintCurlFeature
		}

		GMultiHandle = curl_multi_init();
		if (NULL == GMultiHandle)
		{
			UE_LOG(LogInit, Fatal, TEXT("Could not initialize create libcurl multi handle! HTTP transfers will not function properly."));
		}

		UE_LOG(LogInit, Log, TEXT("Libcurl will %s"), FParse::Param(FCommandLine::Get(), TEXT("reuseconn")) ? TEXT("reuse connections") : TEXT("NOT reuse connections"));
	}
	else
	{
		UE_LOG(LogInit, Fatal, TEXT("Could not initialize libcurl (result=%d), HTTP transfers will not function properly."), InitResult);
	}
}

class FHttpManager * FLinuxPlatformHttp::CreatePlatformHttpManager()
{
	// multi handle should be inited by this time (i.e. Init() called)
	check(GMultiHandle);
	return new FLinuxHttpManager(GMultiHandle);
}


void FLinuxPlatformHttp::Shutdown()
{
	if (NULL != GMultiHandle)
	{
		curl_multi_cleanup(GMultiHandle);
		GMultiHandle = NULL;
	}

	curl_global_cleanup();
}

IHttpRequest* FLinuxPlatformHttp::ConstructRequest()
{
	return new FCurlHttpRequest(GMultiHandle);
}

