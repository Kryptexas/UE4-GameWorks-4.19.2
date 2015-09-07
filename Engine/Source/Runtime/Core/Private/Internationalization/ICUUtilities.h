// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#if UE_ENABLE_ICU
#include "unicode/unistr.h"

namespace ICUUtilities
{
	/** 
	 * An object that can convert between FString and icu::UnicodeString
	 * Note: This object is not thread-safe.
	 */
	class FStringConverter
	{
	public:
		FStringConverter();
		~FStringConverter();

		/** Convert FString -> icu::UnicodeString */
		void ConvertString(const FString& Source, icu::UnicodeString& Destination, const bool ShouldNullTerminate = true);
		icu::UnicodeString ConvertString(const FString& Source, const bool ShouldNullTerminate = true);

		/** Convert icu::UnicodeString -> FString */
		void ConvertString(const icu::UnicodeString& Source, FString& Destination);
		FString ConvertString(const icu::UnicodeString& Source);

	private:
		/** Non-copyable */
		FStringConverter(const FStringConverter&);
		FStringConverter& operator=(const FStringConverter&);

		UConverter* ICUConverter;
	};

	/** Convert FString -> icu::UnicodeString */
	void ConvertString(const FString& Source, icu::UnicodeString& Destination, const bool ShouldNullTerminate = true);
	icu::UnicodeString ConvertString(const FString& Source, const bool ShouldNullTerminate = true);

	/** Convert icu::UnicodeString -> FString */
	void ConvertString(const icu::UnicodeString& Source, FString& Destination);
	FString ConvertString(const icu::UnicodeString& Source);
}
#endif
