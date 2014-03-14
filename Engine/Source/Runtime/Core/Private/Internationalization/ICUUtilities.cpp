// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#if UE_ENABLE_ICU
#include "ICUUtilities.h"
#include <unicode/ucnv.h>

namespace ICUUtilities
{
	void Convert(const FString& Source, icu::UnicodeString& Destination, const bool ShouldNullTerminate)
	{
		const char* const SourceEncoding = FPlatformString::GetEncodingName();
	
		UErrorCode ICUStatus = U_ZERO_ERROR;
		
		UConverter* ICUConverter = ucnv_open(SourceEncoding, &(ICUStatus));
		if( U_FAILURE(ICUStatus) ) 
		{
			return;
		}

		int32_t DestinationCapacity = Source.Len() * 2;
		UChar* ICUChars = new UChar[DestinationCapacity];
		const char* NativeChars = reinterpret_cast<const char*>(*Source);
		int32_t NativeCharsLen = Source.Len() * sizeof(TCHAR);
		int32_t DestinationLength = ucnv_toUChars(ICUConverter, ICUChars, DestinationCapacity, NativeChars, NativeCharsLen, &(ICUStatus));
		Destination.setTo(ICUChars, DestinationLength);
		delete[] ICUChars;

		ucnv_close(ICUConverter);
	}

	icu::UnicodeString Convert(const FString& Source, const bool ShouldNullTerminate)
	{
		const char* const SourceEncoding = FPlatformString::GetEncodingName();

		UErrorCode ICUStatus = U_ZERO_ERROR;

		UConverter* ICUConverter = ucnv_open(SourceEncoding, &(ICUStatus));
		if( U_FAILURE(ICUStatus) ) 
		{
			return icu::UnicodeString();
		}

		int32_t DestinationCapacity = Source.Len() * 2;
		UChar* ICUChars = new UChar[DestinationCapacity];
		const char* NativeChars = reinterpret_cast<const char*>(*Source);
		int32_t NativeCharsLen = Source.Len() * sizeof(TCHAR);
		int32_t DestinationLength = ucnv_toUChars(ICUConverter, ICUChars, DestinationCapacity, NativeChars, NativeCharsLen, &(ICUStatus));
		icu::UnicodeString Destination(ICUChars, DestinationLength);
		delete[] ICUChars;

		ucnv_close(ICUConverter);

		return Destination;
	}

	void Convert(const icu::UnicodeString& Source, FString& Destination)
	{
		const char* const DestinationEncoding = FPlatformString::GetEncodingName();

		UErrorCode ICUStatus = U_ZERO_ERROR;

		UConverter* ICUConverter = ucnv_open(DestinationEncoding, &(ICUStatus));
		if( U_FAILURE(ICUStatus) ) 
		{
			return;
		}

		int32_t DestinationCapacity = UCNV_GET_MAX_BYTES_FOR_STRING(Source.length(), ucnv_getMaxCharSize(ICUConverter));
		char* NativeChars = reinterpret_cast<char*>(malloc(DestinationCapacity));
		int32_t DestinationLength = ucnv_fromUChars(ICUConverter, NativeChars, DestinationCapacity, Source.getBuffer(), Source.length(), &(ICUStatus)) / sizeof(TCHAR);

		Destination = FString(DestinationLength, reinterpret_cast<TCHAR*>(NativeChars));
		free(NativeChars);

		ucnv_close(ICUConverter);
	}

	FString Convert(const icu::UnicodeString& Source)
	{
		const char* const DestinationEncoding = FPlatformString::GetEncodingName();

		UErrorCode ICUStatus = U_ZERO_ERROR;

		UConverter* ICUConverter = ucnv_open(DestinationEncoding, &(ICUStatus));
		if( U_FAILURE(ICUStatus) ) 
		{
			return FString();
		}

		int32_t DestinationCapacity = UCNV_GET_MAX_BYTES_FOR_STRING(Source.length(), ucnv_getMaxCharSize(ICUConverter));
		char* NativeChars = reinterpret_cast<char*>(malloc(DestinationCapacity));
		int32_t DestinationLength = ucnv_fromUChars(ICUConverter, NativeChars, DestinationCapacity, Source.getBuffer(), Source.length(), &(ICUStatus)) / sizeof(TCHAR);

		FString Destination(DestinationLength, reinterpret_cast<TCHAR*>(NativeChars));
		free(NativeChars);

		ucnv_close(ICUConverter);

		return Destination;
	}
}
#endif