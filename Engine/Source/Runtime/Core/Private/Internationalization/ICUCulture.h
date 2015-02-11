// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#if UE_ENABLE_ICU
#include "Internationalization/Text.h"
#include <unicode/locid.h>
#include <unicode/brkiter.h>
#include <unicode/coll.h>
#include <unicode/numfmt.h>
#include <unicode/decimfmt.h>
#include <unicode/datefmt.h>

inline UColAttributeValue UEToICU(const ETextComparisonLevel::Type ComparisonLevel)
{
	UColAttributeValue Value;
	switch(ComparisonLevel)
	{
	case ETextComparisonLevel::Default:
		Value = UColAttributeValue::UCOL_DEFAULT;
		break;
	case ETextComparisonLevel::Primary:
		Value = UColAttributeValue::UCOL_PRIMARY;
		break;
	case ETextComparisonLevel::Secondary:
		Value = UColAttributeValue::UCOL_SECONDARY;
		break;
	case ETextComparisonLevel::Tertiary:
		Value = UColAttributeValue::UCOL_TERTIARY;
		break;
	case ETextComparisonLevel::Quaternary:
		Value = UColAttributeValue::UCOL_QUATERNARY;
		break;
	case ETextComparisonLevel::Quinary:
		Value = UColAttributeValue::UCOL_IDENTICAL;
		break;
	default:
		Value = UColAttributeValue::UCOL_DEFAULT;
		break;
	}
	return Value;
}

inline icu::DateFormat::EStyle UEToICU(const EDateTimeStyle::Type DateTimeStyle)
{
	icu::DateFormat::EStyle Value;
	switch(DateTimeStyle)
	{
	case EDateTimeStyle::Short:
		Value = icu::DateFormat::kShort;
		break;
	case EDateTimeStyle::Medium:
		Value = icu::DateFormat::kMedium;
		break;
	case EDateTimeStyle::Long:
		Value = icu::DateFormat::kLong;
		break;
	case EDateTimeStyle::Full:
		Value = icu::DateFormat::kFull;
		break;
	case EDateTimeStyle::Default:
		Value = icu::DateFormat::kDefault;
		break;
	default:
		Value = icu::DateFormat::kDefault;
		break;
	}
	return Value;
}

inline icu::DecimalFormat::ERoundingMode UEToICU(const ERoundingMode RoundingMode)
{
	icu::DecimalFormat::ERoundingMode Value;
	switch(RoundingMode)
	{
	case ERoundingMode::HalfToEven:
		Value = icu::DecimalFormat::ERoundingMode::kRoundHalfEven;
		break;
	case ERoundingMode::HalfFromZero:
		Value = icu::DecimalFormat::ERoundingMode::kRoundHalfUp;
		break;
	case ERoundingMode::HalfToZero:
		Value = icu::DecimalFormat::ERoundingMode::kRoundHalfDown;
		break;
	case ERoundingMode::FromZero:
		Value = icu::DecimalFormat::ERoundingMode::kRoundUp;
		break;
	case ERoundingMode::ToZero:
		Value = icu::DecimalFormat::ERoundingMode::kRoundDown;
		break;
	case ERoundingMode::ToNegativeInfinity:
		Value = icu::DecimalFormat::ERoundingMode::kRoundFloor;
		break;
	case ERoundingMode::ToPositiveInfinity:
		Value = icu::DecimalFormat::ERoundingMode::kRoundCeiling;
		break;
	default:
		Value = icu::DecimalFormat::ERoundingMode::kRoundHalfEven;
		break;
	}
	return Value;
}

enum class EBreakIteratorType
{
	Grapheme,
	Word,
	Line,
	Sentence,
	Title
};

class FCulture::FICUCultureImplementation
{
	friend FCulture;
	friend FText;
	friend class FICUBreakIteratorManager;

	FICUCultureImplementation(const FString& LocaleName);

	FString GetDisplayName() const;

	FString GetEnglishName() const;

	int GetKeyboardLayoutId() const;

	int GetLCID() const;

	static FString GetCanonicalName(const FString& Name);
	
	FString GetName() const;

	FString GetNativeName() const;

	FString GetUnrealLegacyThreeLetterISOLanguageName() const;

	FString GetThreeLetterISOLanguageName() const;

	FString GetTwoLetterISOLanguageName() const;

	FString GetNativeLanguage() const;

	FString GetRegion() const;

	FString GetNativeRegion() const;

	FString GetScript() const;

	FString GetVariant() const;

	TSharedRef<const icu::BreakIterator> GetBreakIterator(const EBreakIteratorType Type);
	TSharedRef<const icu::Collator, ESPMode::ThreadSafe> GetCollator(const ETextComparisonLevel::Type ComparisonLevel);
	TSharedRef<const icu::DecimalFormat> GetDecimalFormatter(const FNumberFormattingOptions* const Options = NULL);
	TSharedRef<const icu::DecimalFormat> GetCurrencyFormatter(const FString& CurrencyCode = FString(), const FNumberFormattingOptions* const Options = NULL);
	TSharedRef<const icu::DecimalFormat> GetPercentFormatter(const FNumberFormattingOptions* const Options = NULL);
	TSharedRef<const icu::DateFormat> GetDateFormatter(const EDateTimeStyle::Type DateStyle, const FString& TimeZone);
	TSharedRef<const icu::DateFormat> GetTimeFormatter(const EDateTimeStyle::Type TimeStyle, const FString& TimeZone);
	TSharedRef<const icu::DateFormat> GetDateTimeFormatter(const EDateTimeStyle::Type DateStyle, const EDateTimeStyle::Type TimeStyle, const FString& TimeZone);

	icu::Locale ICULocale;
	TSharedPtr<const icu::BreakIterator> ICUGraphemeBreakIterator;
	TSharedPtr<const icu::BreakIterator> ICUWordBreakIterator;
	TSharedPtr<const icu::BreakIterator> ICULineBreakIterator;
	TSharedPtr<const icu::BreakIterator> ICUSentenceBreakIterator;
	TSharedPtr<const icu::BreakIterator> ICUTitleBreakIterator;
	TSharedPtr<const icu::Collator, ESPMode::ThreadSafe> ICUCollator;
	TSharedPtr<const icu::DecimalFormat> ICUDecimalFormat;
	TSharedPtr<const icu::DecimalFormat> ICUCurrencyFormat;
	TSharedPtr<const icu::DecimalFormat> ICUPercentFormat;
	TSharedPtr<const icu::DateFormat> ICUDateFormat;
	TSharedPtr<const icu::DateFormat> ICUTimeFormat;
	TSharedPtr<const icu::DateFormat> ICUDateTimeFormat;
};
#endif