// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "Internationalization/Text.h"

/** Rules used to format or parse a decimal number */
struct FDecimalNumberFormattingRules
{
	FDecimalNumberFormattingRules()
		: GroupingSeparatorCharacter(0)
		, DecimalSeparatorCharacter(0)
		, PrimaryGroupingSize(0)
		, SecondaryGroupingSize(0)
	{
		DigitCharacters[0] = '0';
		DigitCharacters[1] = '1';
		DigitCharacters[2] = '2';
		DigitCharacters[3] = '3';
		DigitCharacters[4] = '4';
		DigitCharacters[5] = '5';
		DigitCharacters[6] = '6';
		DigitCharacters[7] = '7';
		DigitCharacters[8] = '8';
		DigitCharacters[9] = '9';
	}

	/** Number formatting rules, typically extracted from the ICU decimal formatter for a given culture */
	FString NaNString;
	FString NegativePrefixString;
	FString NegativeSuffixString;
	FString PositivePrefixString;
	FString PositiveSuffixString;
	FString PlusString;
	FString MinusString;
	TCHAR GroupingSeparatorCharacter;
	TCHAR DecimalSeparatorCharacter;
	uint8 PrimaryGroupingSize;
	uint8 SecondaryGroupingSize;
	TCHAR DigitCharacters[10];

	/** Default number formatting options for a given culture */
	FNumberFormattingOptions CultureDefaultFormattingOptions;
};

/**
 * Provides efficient and culture aware number formatting and parsing.
 * You would call FastDecimalFormat::NumberToString to convert a number to the correct decimal representation based on the given formatting rules and options.
 * You would call FastDecimalFormat::StringToNumber to convert a string containing a culture correct decimal representation of a number into an actual number.
 * The primary consumer of this is FText, however you can use it for other things. GetCultureAgnosticFormattingRules can provide formatting rules for cases where you don't care about culture.
 * @note If you use the version of FastDecimalFormat::NumberToString that takes an output string, the formatted number will be appended to the existing contents of the string.
 */
namespace FastDecimalFormat
{

namespace Internal
{

CORE_API void IntegralToString(const bool bIsNegative, const uint64 InVal, const FDecimalNumberFormattingRules& InFormattingRules, FNumberFormattingOptions InFormattingOptions, FString& OutString);
CORE_API void FractionalToString(const double InVal, const FDecimalNumberFormattingRules& InFormattingRules, FNumberFormattingOptions InFormattingOptions, FString& OutString);

CORE_API bool StringToIntegral(const TCHAR* InStr, const int32 InStrLen, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberParsingOptions& InParsingOptions, bool& OutIsNegative, uint64& OutVal, int32* OutParsedLen);
CORE_API bool StringToFractional(const TCHAR* InStr, const int32 InStrLen, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberParsingOptions& InParsingOptions, double& OutVal, int32* OutParsedLen);

} // namespace Internal

#define FAST_DECIMAL_FORMAT_SIGNED_IMPL(NUMBER_TYPE)																																				\
	FORCEINLINE void NumberToString(const NUMBER_TYPE InVal, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberFormattingOptions& InFormattingOptions, FString& OutString)		\
	{																																																\
		const bool bIsNegative = InVal < 0;																																							\
		Internal::IntegralToString(bIsNegative, static_cast<uint64>((bIsNegative) ? -InVal : InVal), InFormattingRules, InFormattingOptions, OutString);											\
	}																																																\
	FORCEINLINE FString NumberToString(const NUMBER_TYPE InVal, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberFormattingOptions& InFormattingOptions)						\
	{																																																\
		FString Result;																																												\
		NumberToString(InVal, InFormattingRules, InFormattingOptions, Result);																														\
		return Result;																																												\
	}

#define FAST_DECIMAL_FORMAT_UNSIGNED_IMPL(NUMBER_TYPE)																																				\
	FORCEINLINE void NumberToString(const NUMBER_TYPE InVal, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberFormattingOptions& InFormattingOptions, FString& OutString)		\
	{																																																\
		Internal::IntegralToString(false, static_cast<uint64>(InVal), InFormattingRules, InFormattingOptions, OutString);																			\
	}																																																\
	FORCEINLINE FString NumberToString(const NUMBER_TYPE InVal, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberFormattingOptions& InFormattingOptions)						\
	{																																																\
		FString Result;																																												\
		NumberToString(InVal, InFormattingRules, InFormattingOptions, Result);																														\
		return Result;																																												\
	}

#define FAST_DECIMAL_FORMAT_FRACTIONAL_IMPL(NUMBER_TYPE)																																			\
	FORCEINLINE void NumberToString(const NUMBER_TYPE InVal, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberFormattingOptions& InFormattingOptions, FString& OutString)		\
	{																																																\
		Internal::FractionalToString(static_cast<double>(InVal), InFormattingRules, InFormattingOptions, OutString);																				\
	}																																																\
	FORCEINLINE FString NumberToString(const NUMBER_TYPE InVal, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberFormattingOptions& InFormattingOptions)						\
	{																																																\
		FString Result;																																												\
		NumberToString(InVal, InFormattingRules, InFormattingOptions, Result);																														\
		return Result;																																												\
	}

#define FAST_DECIMAL_PARSE_INTEGER_IMPL(NUMBER_TYPE)																																														\
	FORCEINLINE bool StringToNumber(const TCHAR* InStr, const int32 InStrLen, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberParsingOptions& InParsingOptions, NUMBER_TYPE& OutVal, int32* OutParsedLen = nullptr)	\
	{																																																										\
		bool bIsNegative = false;																																																			\
		uint64 Val = 0;																																																						\
		const bool bResult = Internal::StringToIntegral(InStr, InStrLen, InFormattingRules, InParsingOptions, bIsNegative, Val, OutParsedLen);																								\
		OutVal = static_cast<NUMBER_TYPE>(Val);																																																\
		OutVal *= (bIsNegative ? -1 : 1);																																																	\
		return bResult;																																																						\
	}																																																										\
	FORCEINLINE bool StringToNumber(const TCHAR* InStr, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberParsingOptions& InParsingOptions, NUMBER_TYPE& OutVal, int32* OutParsedLen = nullptr)							\
	{																																																										\
		return StringToNumber(InStr, FCString::Strlen(InStr), InFormattingRules, InParsingOptions, OutVal, OutParsedLen);																													\
	}

#define FAST_DECIMAL_PARSE_FRACTIONAL_IMPL(NUMBER_TYPE)																																														\
	FORCEINLINE bool StringToNumber(const TCHAR* InStr, const int32 InStrLen, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberParsingOptions& InParsingOptions, NUMBER_TYPE& OutVal, int32* OutParsedLen = nullptr)	\
	{																																																										\
		double Val = 0.0;																																																					\
		const bool bResult = Internal::StringToFractional(InStr, InStrLen, InFormattingRules, InParsingOptions, Val, OutParsedLen);																											\
		OutVal = static_cast<NUMBER_TYPE>(Val);																																																\
		return bResult;																																																						\
	}																																																										\
	FORCEINLINE bool StringToNumber(const TCHAR* InStr, const FDecimalNumberFormattingRules& InFormattingRules, const FNumberParsingOptions& InParsingOptions, NUMBER_TYPE& OutVal, int32* OutParsedLen = nullptr)							\
	{																																																										\
		return StringToNumber(InStr, FCString::Strlen(InStr), InFormattingRules, InParsingOptions, OutVal, OutParsedLen);																													\
	}

FAST_DECIMAL_FORMAT_SIGNED_IMPL(int8)
FAST_DECIMAL_FORMAT_SIGNED_IMPL(int16)
FAST_DECIMAL_FORMAT_SIGNED_IMPL(int32)
FAST_DECIMAL_FORMAT_SIGNED_IMPL(int64)

FAST_DECIMAL_FORMAT_UNSIGNED_IMPL(uint8)
FAST_DECIMAL_FORMAT_UNSIGNED_IMPL(uint16)
FAST_DECIMAL_FORMAT_UNSIGNED_IMPL(uint32)
FAST_DECIMAL_FORMAT_UNSIGNED_IMPL(uint64)

FAST_DECIMAL_FORMAT_FRACTIONAL_IMPL(float)
FAST_DECIMAL_FORMAT_FRACTIONAL_IMPL(double)

FAST_DECIMAL_PARSE_INTEGER_IMPL(int8)
FAST_DECIMAL_PARSE_INTEGER_IMPL(int16)
FAST_DECIMAL_PARSE_INTEGER_IMPL(int32)
FAST_DECIMAL_PARSE_INTEGER_IMPL(int64)

FAST_DECIMAL_PARSE_INTEGER_IMPL(uint8)
FAST_DECIMAL_PARSE_INTEGER_IMPL(uint16)
FAST_DECIMAL_PARSE_INTEGER_IMPL(uint32)
FAST_DECIMAL_PARSE_INTEGER_IMPL(uint64)

FAST_DECIMAL_PARSE_FRACTIONAL_IMPL(float)
FAST_DECIMAL_PARSE_FRACTIONAL_IMPL(double)

#undef FAST_DECIMAL_FORMAT_SIGNED_IMPL
#undef FAST_DECIMAL_FORMAT_UNSIGNED_IMPL
#undef FAST_DECIMAL_FORMAT_FRACTIONAL_IMPL
#undef FAST_DECIMAL_PARSE_INTEGER_IMPL
#undef FAST_DECIMAL_PARSE_FRACTIONAL_IMPL

/**
 * Get the formatting rules to use when you don't care about culture.
 */
CORE_API const FDecimalNumberFormattingRules& GetCultureAgnosticFormattingRules();

} // namespace FastDecimalFormat
