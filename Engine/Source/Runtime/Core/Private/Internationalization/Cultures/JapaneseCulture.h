// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Culture.h"

#define LOCTEXT_NAMESPACE "Internationalization"

class FJapaneseCulture
{
public:
	static TSharedRef< FCulture > Create()
	{
#if UE_ENABLE_ICU
		TSharedRef< FCulture > Culture = MakeShareable( new FCulture( FString("ja-JP") ) );
#else
		TArray<FString> NativeDigits;

		NativeDigits.Add(FString(TEXT("0")));
		NativeDigits.Add(FString(TEXT("1")));
		NativeDigits.Add(FString(TEXT("2")));
		NativeDigits.Add(FString(TEXT("3")));
		NativeDigits.Add(FString(TEXT("4")));
		NativeDigits.Add(FString(TEXT("5")));
		NativeDigits.Add(FString(TEXT("6")));
		NativeDigits.Add(FString(TEXT("7")));
		NativeDigits.Add(FString(TEXT("8")));
		NativeDigits.Add(FString(TEXT("9")));

		TArray<int> CurrencyGroupSizes;
		TArray<int> NumberGroupSizes;
		TArray<int> PercentGroupSizes;
		CurrencyGroupSizes.Add(3);
		NumberGroupSizes.Add(3);
		PercentGroupSizes.Add(3);

		FNumberFormattingRules NumberFormatting = FNumberFormattingRules(
			  0	//const int CurrencyDecimalDigits
			, FString(TEXT("."))	//const FString CurrencyDecimalSeparator
			, FString(TEXT(","))	//const FString CurrencyGroupSeparator
			, CurrencyGroupSizes	//const TArray<int>& CurrencyGroupSizes
			, FString(TEXT("¥"))	//const FString CurrencySymbol
			, FString(TEXT("NaN (非数値)"))	//const FString NaNSymbol
			, NativeDigits	//const TArray<int>& NativeDigits
			, FString(TEXT("-"))	//const FString NegativeSign
			, 2	//const int NumberDecimalDigits
			, FString(TEXT("."))	//const FString NumberDecimalSeparator
			, FString(TEXT(","))	//const FString NumberGroupSeparator
			, NumberGroupSizes	//const TArray<int>& NumberGroupSizes
			, 2	//const int PercentDecimalDigits
			, FString(TEXT("."))	//const FString PercentDecimalSeparator
			, FString(TEXT(","))	//const FString PercentGroupSeparator
			, PercentGroupSizes	//const TArray<int>& PercentGroupSizes
			, FString(TEXT("%"))	//const FString PercentSymbol
			, FString(TEXT("‰"))	//const FString PerMilleSymbol
			, FString(TEXT("+"))	//const FString PositiveSign
			, ECurrencyNegativePattern::NegativeSymbolCurrencySymbolX	//const ECurrencyNegativePattern::Type CurrencyNegativePattern
			, ECurrencyPositivePattern::CurrencySymbolX	//const ECurrencyPositivePattern::Type CurrencyPositivePattern
			, EPercentNegativePattern::NegativeSymbolXPct	//const EPercentNegativePattern::Type PercentNegativePattern
			, EPercentPositivePattern::XPct	//const EPercentPositivePattern::Type PercentPositivePattern
			, ENumberNegativePattern::NegativeSymbolX	//const ENumberNegativePattern::Type NumberNegativePattern
			);

		FTextFormattingRules TextFormatting = FTextFormattingRules(
			  false	//const bool IsRightToLeft
			, FString(TEXT(","))	//const FString ListSeparator
			, 932	//const int ANSICodePage
			, 20290	//const int EBCDICCodePage
			, 10001	//const int MacCodePage
			, 932	//const int OEMCodePage
			);

		TArray<FString> AbbreviatedDayNames;
		TArray<FString> AbbreviatedMonthNames;
		TArray<FString> DayNames;
		TArray<FString> MonthNames;
		TArray<FString> ShortestDayNames;

		AbbreviatedDayNames.Add(FString(TEXT("日")));
		AbbreviatedDayNames.Add(FString(TEXT("月")));
		AbbreviatedDayNames.Add(FString(TEXT("火")));
		AbbreviatedDayNames.Add(FString(TEXT("水")));
		AbbreviatedDayNames.Add(FString(TEXT("木")));
		AbbreviatedDayNames.Add(FString(TEXT("金")));
		AbbreviatedDayNames.Add(FString(TEXT("土")));
		AbbreviatedMonthNames.Add(FString(TEXT("1")));
		AbbreviatedMonthNames.Add(FString(TEXT("2")));
		AbbreviatedMonthNames.Add(FString(TEXT("3")));
		AbbreviatedMonthNames.Add(FString(TEXT("4")));
		AbbreviatedMonthNames.Add(FString(TEXT("5")));
		AbbreviatedMonthNames.Add(FString(TEXT("6")));
		AbbreviatedMonthNames.Add(FString(TEXT("7")));
		AbbreviatedMonthNames.Add(FString(TEXT("8")));
		AbbreviatedMonthNames.Add(FString(TEXT("9")));
		AbbreviatedMonthNames.Add(FString(TEXT("10")));
		AbbreviatedMonthNames.Add(FString(TEXT("11")));
		AbbreviatedMonthNames.Add(FString(TEXT("12")));
		AbbreviatedMonthNames.Add(FString(TEXT("")));
		DayNames.Add(FString(TEXT("日曜日")));
		DayNames.Add(FString(TEXT("月曜日")));
		DayNames.Add(FString(TEXT("火曜日")));
		DayNames.Add(FString(TEXT("水曜日")));
		DayNames.Add(FString(TEXT("木曜日")));
		DayNames.Add(FString(TEXT("金曜日")));
		DayNames.Add(FString(TEXT("土曜日")));
		MonthNames.Add(FString(TEXT("1月")));
		MonthNames.Add(FString(TEXT("2月")));
		MonthNames.Add(FString(TEXT("3月")));
		MonthNames.Add(FString(TEXT("4月")));
		MonthNames.Add(FString(TEXT("5月")));
		MonthNames.Add(FString(TEXT("6月")));
		MonthNames.Add(FString(TEXT("7月")));
		MonthNames.Add(FString(TEXT("8月")));
		MonthNames.Add(FString(TEXT("9月")));
		MonthNames.Add(FString(TEXT("10月")));
		MonthNames.Add(FString(TEXT("11月")));
		MonthNames.Add(FString(TEXT("12月")));
		MonthNames.Add(FString(TEXT("")));
		ShortestDayNames.Add(FString(TEXT("日")));
		ShortestDayNames.Add(FString(TEXT("月")));
		ShortestDayNames.Add(FString(TEXT("火")));
		ShortestDayNames.Add(FString(TEXT("水")));
		ShortestDayNames.Add(FString(TEXT("木")));
		ShortestDayNames.Add(FString(TEXT("金")));
		ShortestDayNames.Add(FString(TEXT("土")));

		FDateTimeFormattingRules DateFormatting = FDateTimeFormattingRules(
			  AbbreviatedDayNames	//const TArray<FString>& AbbreviatedDayNames
			, AbbreviatedMonthNames	//const TArray<FString>& AbbreviatedMonthNames
			, DayNames	//const TArray<FString>& DayNames
			, MonthNames	//const TArray<FString>& MonthNames
			, ShortestDayNames	//const TArray<FString>& ShortestDayNames
			, FString(TEXT("午前"))	//const FString AMDesignator
			, FString(TEXT("午後"))	//const FString PMDesignator
			, FString(TEXT("/"))	//const FString DateSeparator
			, FString(TEXT(":"))	//const FString TimeSeparator
			, 0	//const int FirstDayOfWeek
			, FString(TEXT("yyyy'年'M'月'd'日' H:mm:ss"))	//const FString FullDateTimePattern
			, FString(TEXT("yyyy'年'M'月'd'日'"))	//const FString LongDatePattern
			, FString(TEXT("H:mm:ss"))	//const FString LongTimePattern
			, FString(TEXT("M'月'd'日'"))	//const FString MonthDayPattern
			, FString(TEXT("ddd, dd MMM yyyy HH':'mm':'ss 'GMT'"))	//const FString RFC1123Pattern
			, FString(TEXT("yyyy/MM/dd"))	//const FString ShortDatePattern
			, FString(TEXT("H:mm"))	//const FString ShortTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd'T'HH':'mm':'ss"))	//const FString SortableDateTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd HH':'mm':'ss'Z'"))	//const FString UniversalSortableDateTimePattern
			, FString(TEXT("yyyy'年'M'月'"))	//const FString YearMonthPattern
			);


		TSharedRef< FCulture > Culture = MakeShareable( new FCulture(
			  LOCTEXT("JapaneseDisplayName", "Japanese (Japan)")	//const FText DisplayName
			, FString(TEXT("Japanese (Japan)"))	//const FString EnglishName
			, 1041	//const int KeyboardLayoutId
			, 1041	//const int LCID
			, FString(TEXT("ja-JP"))	//const FString Name
			, FString(TEXT("日本語 (日本)"))	//const FString NativeName
			, FString(TEXT("jpn"))	//const FString UnrealLegacyThreeLetterISOLanguageName
			, FString(TEXT("jpn"))	//const FString ThreeLetterISOLanguageName
			, FString(TEXT("ja"))	//const FString TwoLetterISOLanguageName
			, NumberFormatting					//const FNumberFormattingRules NumberFormattingRule
			, TextFormatting						//const FTextFormattingRules TextFormattingRule
			, DateFormatting						//const FDateTimeFormattingRules DateTimeFormattingRule
			) );
#endif

		return Culture;
	}
};

#undef LOCTEXT_NAMESPACE
