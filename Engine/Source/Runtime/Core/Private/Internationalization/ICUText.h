// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ICUUtilities.h"
#include "ICUCulture.h"
#include "unicode/utypes.h"
#include <unicode/numfmt.h>
#include "unicode/fmtable.h"
#include "unicode/unistr.h"

template<typename T1, typename T2>
FText FText::AsNumberTemplate(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	const TSharedRef<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture.ToSharedRef() : FInternationalization::GetCurrentCulture();
	UErrorCode ICUStatus = U_ZERO_ERROR;
	const TSharedRef<const icu::DecimalFormat> ICUDecimalFormat( Culture->Implementation->GetDecimalFormatter(Options) );
	icu::Formattable FormattableVal(static_cast<T2>(Val));
	icu::UnicodeString FormattedString;
	ICUDecimalFormat->format(FormattableVal, FormattedString, ICUStatus);

	FString NativeString;
	ICUUtilities::Convert(FormattedString, NativeString);

	return FText::CreateNumericalText(NativeString);
}

template<typename T1, typename T2>
FText FText::AsCurrencyTemplate(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	const TSharedRef<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture.ToSharedRef() : FInternationalization::GetCurrentCulture();
	UErrorCode ICUStatus = U_ZERO_ERROR;
	const TSharedRef<const icu::DecimalFormat> ICUDecimalFormat( Culture->Implementation->GetCurrencyFormatter(Options) );
	icu::Formattable FormattableVal(static_cast<T2>(Val));
	icu::UnicodeString FormattedString;
	ICUDecimalFormat->format(FormattableVal, FormattedString, ICUStatus);

	FString NativeString;
	ICUUtilities::Convert(FormattedString, NativeString);

	return FText::CreateNumericalText(NativeString);
}

template<typename T1, typename T2>
FText FText::AsPercentTemplate(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	const TSharedRef<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture.ToSharedRef() : FInternationalization::GetCurrentCulture();
	UErrorCode ICUStatus = U_ZERO_ERROR;
	const TSharedRef<const icu::DecimalFormat> ICUDecimalFormat( Culture->Implementation->GetPercentFormatter(Options) );
	icu::Formattable FormattableVal(static_cast<T2>(Val));
	icu::UnicodeString FormattedString;
	ICUDecimalFormat->format(FormattableVal, FormattedString, ICUStatus);

	FString NativeString;
	ICUUtilities::Convert(FormattedString, NativeString);

	return FText::CreateNumericalText(NativeString);
}