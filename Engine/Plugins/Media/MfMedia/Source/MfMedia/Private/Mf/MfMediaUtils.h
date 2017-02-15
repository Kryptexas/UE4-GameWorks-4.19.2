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


namespace MfMedia
{
	/** 
	 * Convert a FOURCC code to string.
	 *
	 * @param Fourcc The code to convert.
	 * @return The corresponding string.
	 */
	FString FourccToString(unsigned long Fourcc);

	/**
	 * Convert a Windows GUID to string.
	 *
	 * @param Guid The GUID to convert.
	 * @return The corresponding string.
	 */
	FString GuidToString(const GUID& Guid);

	/**
	 * Convert a media major type to string.
	 *
	 * @param MajorType The major type GUID to convert.
	 * @return The corresponding string.
	 */
	FString MajorTypeToString(const GUID& MajorType);

	/**
	 * Convert an WMF HRESULT code to string.
	 *
	 * @param Result The result code to convert.
	 * @return The corresponding string.
	 */
	FString ResultToString(HRESULT Result);

	/**
	 * Convert a media sub-type to string.
	 *
	 * @param SubType The sub-type GUID to convert.
	 * @return The corresponding string.
	 */
	FString SubTypeToString(const GUID& SubType);
}


#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif
