// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Culture.h"

#define LOCTEXT_NAMESPACE "Internationalization"

class FKoreanCulture
{
public:
	static TSharedRef< FCulture > Create()
	{
#if UE_ENABLE_ICU
		TSharedRef< FCulture > Culture = MakeShareable( new FCulture( FString("ko-KR") ) );
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
			, FString(TEXT("₩"))	//const FString CurrencySymbol
			, FString(TEXT("NaN"))	//const FString NaNSymbol
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
			, EPercentNegativePattern::NegativeSymbolXSpacePct	//const EPercentNegativePattern::Type PercentNegativePattern
			, EPercentPositivePattern::XSpacePct	//const EPercentPositivePattern::Type PercentPositivePattern
			, ENumberNegativePattern::NegativeSymbolX	//const ENumberNegativePattern::Type NumberNegativePattern
			);

		FTextFormattingRules TextFormatting = FTextFormattingRules(
			  false	//const bool IsRightToLeft
			, FString(TEXT(","))	//const FString ListSeparator
			, 949	//const int ANSICodePage
			, 20833	//const int EBCDICCodePage
			, 10003	//const int MacCodePage
			, 949	//const int OEMCodePage
			);

		TArray<FString> AbbreviatedDayNames;
		TArray<FString> AbbreviatedMonthNames;
		TArray<FString> DayNames;
		TArray<FString> MonthNames;
		TArray<FString> ShortestDayNames;

		AbbreviatedDayNames.Add(FString(TEXT("일")));
		AbbreviatedDayNames.Add(FString(TEXT("월")));
		AbbreviatedDayNames.Add(FString(TEXT("화")));
		AbbreviatedDayNames.Add(FString(TEXT("수")));
		AbbreviatedDayNames.Add(FString(TEXT("목")));
		AbbreviatedDayNames.Add(FString(TEXT("금")));
		AbbreviatedDayNames.Add(FString(TEXT("토")));
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
		DayNames.Add(FString(TEXT("일요일")));
		DayNames.Add(FString(TEXT("월요일")));
		DayNames.Add(FString(TEXT("화요일")));
		DayNames.Add(FString(TEXT("수요일")));
		DayNames.Add(FString(TEXT("목요일")));
		DayNames.Add(FString(TEXT("금요일")));
		DayNames.Add(FString(TEXT("토요일")));
		MonthNames.Add(FString(TEXT("1월")));
		MonthNames.Add(FString(TEXT("2월")));
		MonthNames.Add(FString(TEXT("3월")));
		MonthNames.Add(FString(TEXT("4월")));
		MonthNames.Add(FString(TEXT("5월")));
		MonthNames.Add(FString(TEXT("6월")));
		MonthNames.Add(FString(TEXT("7월")));
		MonthNames.Add(FString(TEXT("8월")));
		MonthNames.Add(FString(TEXT("9월")));
		MonthNames.Add(FString(TEXT("10월")));
		MonthNames.Add(FString(TEXT("11월")));
		MonthNames.Add(FString(TEXT("12월")));
		MonthNames.Add(FString(TEXT("")));
		ShortestDayNames.Add(FString(TEXT("일")));
		ShortestDayNames.Add(FString(TEXT("월")));
		ShortestDayNames.Add(FString(TEXT("화")));
		ShortestDayNames.Add(FString(TEXT("수")));
		ShortestDayNames.Add(FString(TEXT("목")));
		ShortestDayNames.Add(FString(TEXT("금")));
		ShortestDayNames.Add(FString(TEXT("토")));

		FDateTimeFormattingRules DateFormatting = FDateTimeFormattingRules(
			  AbbreviatedDayNames	//const TArray<FString>& AbbreviatedDayNames
			, AbbreviatedMonthNames	//const TArray<FString>& AbbreviatedMonthNames
			, DayNames	//const TArray<FString>& DayNames
			, MonthNames	//const TArray<FString>& MonthNames
			, ShortestDayNames	//const TArray<FString>& ShortestDayNames
			, FString(TEXT("오전"))	//const FString AMDesignator
			, FString(TEXT("오후"))	//const FString PMDesignator
			, FString(TEXT("-"))	//const FString DateSeparator
			, FString(TEXT(":"))	//const FString TimeSeparator
			, 0	//const int FirstDayOfWeek
			, FString(TEXT("yyyy'년' M'월' d'일' dddd tt h:mm:ss"))	//const FString FullDateTimePattern
			, FString(TEXT("yyyy'년' M'월' d'일' dddd"))	//const FString LongDatePattern
			, FString(TEXT("tt h:mm:ss"))	//const FString LongTimePattern
			, FString(TEXT("M'월' d'일'"))	//const FString MonthDayPattern
			, FString(TEXT("ddd, dd MMM yyyy HH':'mm':'ss 'GMT'"))	//const FString RFC1123Pattern
			, FString(TEXT("yyyy-MM-dd"))	//const FString ShortDatePattern
			, FString(TEXT("tt h:mm"))	//const FString ShortTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd'T'HH':'mm':'ss"))	//const FString SortableDateTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd HH':'mm':'ss'Z'"))	//const FString UniversalSortableDateTimePattern
			, FString(TEXT("yyyy'년' M'월'"))	//const FString YearMonthPattern
			);


		TSharedRef< FCulture > Culture = MakeShareable( new FCulture(
			  LOCTEXT("KoreanDisplayName", "Korean (Korea)")	//const FText DisplayName
			, FString(TEXT("Korean (Korea)"))	//const FString EnglishName
			, 1042	//const int KeyboardLayoutId
			, 1042	//const int LCID
			, FString(TEXT("ko-KR"))	//const FString Name
			, FString(TEXT("한국어 (대한민국)"))	//const FString NativeName
			, FString(TEXT("kor"))	//const FString UnrealLegacyThreeLetterISOLanguageName
			, FString(TEXT("kor"))	//const FString ThreeLetterISOLanguageName
			, FString(TEXT("ko"))	//const FString TwoLetterISOLanguageName
			, NumberFormatting					//const FNumberFormattingRules NumberFormattingRule
			, TextFormatting						//const FTextFormattingRules TextFormattingRule
			, DateFormatting						//const FDateTimeFormattingRules DateTimeFormattingRule
			) );
#endif

		return Culture;
	}
};

#undef LOCTEXT_NAMESPACE
