// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#if UE_ENABLE_ICU
#include "unicode/unistr.h"

namespace ICUUtilities
{
	void Convert(const FString& Source, icu::UnicodeString& Destination, const bool ShouldNullTerminate = true);
	icu::UnicodeString Convert(const FString& Source, const bool ShouldNullTerminate = true);
	void Convert(const icu::UnicodeString& Source, FString& Destination);
	FString Convert(const icu::UnicodeString& Source);

	void Convert(const TCHAR Source, UChar32& Destination);
	UChar32 Convert(const TCHAR Source);
	void Convert(const UChar32 Source, TCHAR& Destination);
	TCHAR Convert(const UChar32 Source);
}
#endif