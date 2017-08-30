// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../WmfMediaPrivate.h"

#if WMFMEDIA_SUPPORTED_PLATFORM

#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"

#include "AllowWindowsPlatformTypes.h"


namespace WmfMedia
{
	/**
	 * Create an output media type for the given input media type.
	 *
	 * @param MajorType The output's major type, i.e. audio or video.
	 * @param SubType The output's sub type, i.e. PCM or RGB32.
	 * @param AllowNonStandardCodecs Whether non-standard codecs should be allowed.
	 * @return The output type, or NULL on error.
	 */
	TComPtr<IMFMediaType> CreateOutputType(const GUID& MajorType, const GUID& SubType, bool AllowNonStandardCodecs);

	/**
	 * Enumerate the available capture devices.
	 *
	 * The supported device types are MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID for audio
	 * capture devices and MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID for video capture devices.
	 *
	 * @param DeviceType The type of devices to enumerate.
	 * @param OutDevices Will contain the collection of found device source activators.
	 */
	void EnumerateCaptureDevices(GUID DeviceType, TArray<TComPtr<IMFActivate>>& OutDevices);

	/**
	 * Convert a floating point frame rate to an integer ratio.
	 *
	 * @param OutNumerator Will contain the ratio's numerator.
	 * @param OutDenominator Will contain the ratio's denominator.
	 * @return true on success, false if the frame rate cannot be represented as a ratio.
	 * @see RatioToFrameRate
	 */
	bool FrameRateToRatio(float FrameRate, int32& OutNumerator, int32& OutDenominator);

	/** 
	 * Convert a FOURCC code to string.
	 *
	 * @param Fourcc The code to convert.
	 * @return The corresponding string.
	 */
	FString FourccToString(unsigned long Fourcc);

	/**
	 * Get information about the given capture device.
	 *
	 * @param Device The device.
	 * @param OutDisplayName Will contain the device's human readable name.
	 * @param OutInfo Will contain a device specific information string.
	 * @param OutSoftwareDevice Will contain a flag indicating whether the device is software.
	 * @param OutUrl Will contain the device's media URL string.
	 * @return true on success, false otherwise.
	 */
	bool GetCaptureDeviceInfo(IMFActivate& Device, FText& OutDisplayName, FString& OutInfo, bool& OutSoftwareDevice, FString& OutUrl);

	/**
	 * Get the list of supported media types for the specified major type.
	 *
	 * @param MajorType The major type to query, i.e. audio or video.
	 * @return Media type collection.
	 * @see IsSupportedMajorType
	 */
	const TArray<TComPtr<IMFMediaType>>& GetSupportedMediaTypes(const GUID& MajorType);

	/**
	 * Convert a Windows GUID to string.
	 *
	 * @param Guid The GUID to convert.
	 * @return The corresponding string.
	 */
	FString GuidToString(const GUID& Guid);

	/**
	 * Check whether the given major media type is supported.
	 *
	 * @param Type The major type to check.
	 * @return true if the type is supported, false otherwise.
	 * @see GetSupportedMediaTypes
	 */
	bool IsSupportedMajorType(const GUID& MajorType);

	/**
	 * Convert a media major type to string.
	 *
	 * @param MajorType The major type GUID to convert.
	 * @return The corresponding string.
	 */
	FString MajorTypeToString(const GUID& MajorType);

	/**
	 * Convert a media event to string.
	 *
	 * @param Event The event code to convert.
	 * @return The corresponding string.
	 */
	FString MediaEventToString(MediaEventType Event);

	/**
	 * Convert an integer ratio to a floating point frame rate.
	 *
	 * @param Numerator The ratio's numerator.
	 * @param Denominator The ratio's denominator.
	 * @return The corresponding floating point value.
	 * @see FrameRateToRatio
	 */
	float RatioToFrameRate(int32 Numerator, int32 Denominator);

	/**
	 * Resolve a media source from an archive or URL.
	 *
	 * @param Archive The archive to read media data from (optional).
	 * @param Url The media source URL.
	 * @param Precache Whether to precache media into RAM if URL is a local file.
	 * @return The media source object, or NULL if it couldn't be resolved.
	 */
	TComPtr<IMFMediaSource> ResolveMediaSource(TSharedPtr<FArchive, ESPMode::ThreadSafe> Archive, const FString& Url, bool Precache);

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


#include "HideWindowsPlatformTypes.h"

#endif //WMFMEDIA_SUPPORTED_PLATFORM
