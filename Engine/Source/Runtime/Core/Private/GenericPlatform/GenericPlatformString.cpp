// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericPlatformString.cpp: Generic implementations of string functions
=============================================================================*/

#include "CorePrivate.h"

DEFINE_LOG_CATEGORY_STATIC(LogGenericPlatformString, Log, All);

template <> const TCHAR* FGenericPlatformString::GetEncodingName<ANSICHAR>() { return TEXT("ANSICHAR"); }
template <> const TCHAR* FGenericPlatformString::GetEncodingName<WIDECHAR>() { return TEXT("WIDECHAR"); }
template <> const TCHAR* FGenericPlatformString::GetEncodingName<UCS2CHAR>() { return TEXT("UCS2CHAR"); }

void* FGenericPlatformString::Memcpy(void* Dest, const void* Src, SIZE_T Count)
{
	return FMemory::Memcpy(Dest, Src, Count);
}

namespace
{
	void TrimStringAndLogBogusCharsError(FString& SrcStr, const TCHAR* SourceCharName, const TCHAR* DestCharName)
	{
		SrcStr.Trim();
		// @todo: Put this back in once GLog becomes a #define, or is replaced with GLog::GetLog()
		//UE_LOG(LogGenericPlatformString, Warning, TEXT("Bad chars found when trying to convert \"%s\" from %s to %s"), *SrcStr, SourceCharName, DestCharName);
	}
}

template <typename DestEncoding, typename SourceEncoding>
void FGenericPlatformString::LogBogusChars(const SourceEncoding* Src, int32 SrcSize)
{
	FString SrcStr;
	bool    bFoundBogusChars = false;
	for (; SrcSize; --SrcSize)
	{
		SourceEncoding SrcCh = *Src++;
		if (!CanConvertChar<DestEncoding>(SrcCh))
		{
			SrcStr += FString::Printf(TEXT("[0x%X]"), (int32)SrcCh);
			bFoundBogusChars = true;
		}
		else if (CanConvertChar<TCHAR>(SrcCh))
		{
			if (TChar<SourceEncoding>::IsLinebreak(SrcCh))
			{
				if (bFoundBogusChars)
				{
					TrimStringAndLogBogusCharsError(SrcStr, GetEncodingName<SourceEncoding>(), GetEncodingName<DestEncoding>());
					bFoundBogusChars = false;
				}
				SrcStr.Empty();
			}
			else
			{
				SrcStr.AppendChar((TCHAR)SrcCh);
			}
		}
		else
		{
			SrcStr.AppendChar((TCHAR)'?');
		}
	}

	if (bFoundBogusChars)
	{
		TrimStringAndLogBogusCharsError(SrcStr, GetEncodingName<SourceEncoding>(), GetEncodingName<DestEncoding>());
	}
}

template CORE_API void FGenericPlatformString::LogBogusChars<ANSICHAR, WIDECHAR>(const WIDECHAR* Src, int32 SrcSize);
template CORE_API void FGenericPlatformString::LogBogusChars<ANSICHAR, UCS2CHAR>(const UCS2CHAR* Src, int32 SrcSize);
template CORE_API void FGenericPlatformString::LogBogusChars<WIDECHAR, ANSICHAR>(const ANSICHAR* Src, int32 SrcSize);
template CORE_API void FGenericPlatformString::LogBogusChars<WIDECHAR, UCS2CHAR>(const UCS2CHAR* Src, int32 SrcSize);
template CORE_API void FGenericPlatformString::LogBogusChars<UCS2CHAR, ANSICHAR>(const ANSICHAR* Src, int32 SrcSize);
template CORE_API void FGenericPlatformString::LogBogusChars<UCS2CHAR, WIDECHAR>(const WIDECHAR* Src, int32 SrcSize);
