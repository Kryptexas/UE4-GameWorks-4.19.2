// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Culture.h"

#define LOCTEXT_NAMESPACE "Internationalization"

class FIndiaHindiCulture
{
public:
	static TSharedRef< FCulture > Create()
	{
#if UE_ENABLE_ICU
		TSharedRef< FCulture > Culture = MakeShareable( new FCulture( FString("hi-IN") ) );
#else
		TArray<FString> InNativeDigits;

		for (int i = 0; i<=10; ++i)
		{
			InNativeDigits.Add(FString(TEXT("%i"),i));
		}

		TArray<int> GroupSizes;
		GroupSizes.Add(3);
		GroupSizes.Add(2);

		FNumberFormattingRules NumberFormatting = FNumberFormattingRules(
			  2						// const int InCurrencyDecimalDigits
			, "."					// const FString CurrencyDecimalSeparator
			, ","					// const FString CurrencyGroupSeparator
			, GroupSizes			//const TArray<int>& CurrencyGroupSizes
			, FString(TEXT("\x20B9"))					//const FString CurrencySymbol
			, "NaN"					//const FString NaNSymbol
			, InNativeDigits		//const TArray<FString>& NativeDigits
			, "-"					//const FString NegativeSign
			, 3						//const int NumberDecimalDigits
			, "."					//const FString NumberDecimalSeparator
			, ","					//const FString NumberGroupSeparator
			, GroupSizes			//const TArray<int>& NumberGroupSizes
			, 0						//const int PercentDecimalDigits
			, "."					//const FString PercentDecimalSeparator
			, ","					//const FString PercentGroupSeparator
			, GroupSizes			//const TArray<int>& PercentGroupSizes
			, "%"					//const FString PercentSymbol
			, "‰"					//const FString PerMilleSymbol
			, ""					//const FString PositiveSign
			, ECurrencyNegativePattern::NegativeSymbolCurrencySymbolSpaceX
			, ECurrencyPositivePattern::CurrencySymbolSpaceX
			, EPercentNegativePattern::NegativeSymbolXPct
			, EPercentPositivePattern::XPct
			, ENumberNegativePattern::NegativeSymbolX
			);

		FTextFormattingRules TextFormatting = FTextFormattingRules(false, FString(TEXT(",")), 0, 500, 2, 1);

		TArray<FString> AbbreviatedDayNames;
		AbbreviatedDayNames.Add(FString(TEXT("रवि.")));
		AbbreviatedDayNames.Add(FString(TEXT("सोम.")));
		AbbreviatedDayNames.Add(FString(TEXT("मंगल.")));
		AbbreviatedDayNames.Add(FString(TEXT("बुध.")));
		AbbreviatedDayNames.Add(FString(TEXT("गुरु.")));
		AbbreviatedDayNames.Add(FString(TEXT("शुक्र.")));
		AbbreviatedDayNames.Add(FString(TEXT("शनि.")));

		TArray<FString> AbbreviatedMonthNames;
		AbbreviatedMonthNames.Add(FString(TEXT("जनवरी")));
		AbbreviatedMonthNames.Add(FString(TEXT("फरवरी")));
		AbbreviatedMonthNames.Add(FString(TEXT("मार्च")));
		AbbreviatedMonthNames.Add(FString(TEXT("अप्रैल")));
		AbbreviatedMonthNames.Add(FString(TEXT("मई")));
		AbbreviatedMonthNames.Add(FString(TEXT("जून")));
		AbbreviatedMonthNames.Add(FString(TEXT("जुलाई")));
		AbbreviatedMonthNames.Add(FString(TEXT("अगस्त")));
		AbbreviatedMonthNames.Add(FString(TEXT("सितम्बर")));
		AbbreviatedMonthNames.Add(FString(TEXT("अक्तूबर")));
		AbbreviatedMonthNames.Add(FString(TEXT("नवम्बर")));
		AbbreviatedMonthNames.Add(FString(TEXT("दिसम्बर")));

		TArray<FString> DayNames;
		DayNames.Add(FString(TEXT("रविवार")));
		DayNames.Add(FString(TEXT("सोमवार")));
		DayNames.Add(FString(TEXT("मंगलवार")));
		DayNames.Add(FString(TEXT("बुधवार")));
		DayNames.Add(FString(TEXT("गुरुवार")));
		DayNames.Add(FString(TEXT("शुक्रवार")));
		DayNames.Add(FString(TEXT("शनिवार")));

		TArray<FString> MonthNames;
		MonthNames.Add(FString(TEXT("जनवरी")));
		MonthNames.Add(FString(TEXT("फरवरी")));
		MonthNames.Add(FString(TEXT("मार्च")));
		MonthNames.Add(FString(TEXT("अप्रैल")));
		MonthNames.Add(FString(TEXT("मई")));
		MonthNames.Add(FString(TEXT("जून")));
		MonthNames.Add(FString(TEXT("जुलाई")));
		MonthNames.Add(FString(TEXT("अगस्त")));
		MonthNames.Add(FString(TEXT("सितम्बर")));
		MonthNames.Add(FString(TEXT("अक्तूबर")));
		MonthNames.Add(FString(TEXT("नवम्बर")));
		MonthNames.Add(FString(TEXT("दिसम्बर")));

		TArray<FString> ShortestDayNames;
		ShortestDayNames.Add(FString(TEXT("र")));
		ShortestDayNames.Add(FString(TEXT("स")));
		ShortestDayNames.Add(FString(TEXT("म")));
		ShortestDayNames.Add(FString(TEXT("ब")));
		ShortestDayNames.Add(FString(TEXT("ग")));
		ShortestDayNames.Add(FString(TEXT("श")));
		ShortestDayNames.Add(FString(TEXT("श")));


		FDateTimeFormattingRules DateFormatting = FDateTimeFormattingRules(
			AbbreviatedDayNames		//const TArray<FString>& AbbreviatedDayNames
			, AbbreviatedMonthNames		//const TArray<FString>& AbbreviatedMonthNames
			, DayNames					//const TArray<FString>& DayNames
			, MonthNames				//const TArray<FString>& MonthNames
			, ShortestDayNames			//const TArray<FString>& ShortestDayNames
			, FString(TEXT("पूर्वाह्न"))		//const FString AMDesignator
			, FString(TEXT("अपराह्न"))		//const FString PMDesignator
			, FString(TEXT("-"))		//const FString DateSeparator
			, FString(TEXT(":"))		//const FString TimeSeparator
			, 1							//const int FirstDayOfWeek
			, FString(TEXT("dd MMMM yyyy HH:mm:ss"))	//const FString FullDateTimePattern
			, FString(TEXT("dd MMMM yyyy"))	//const FString LongDatePattern
			, FString(TEXT("HH:mm:ss"))	//const FString LongTimePattern
			, FString(TEXT("dd MMMM")) //const FString MonthDayPattern
			, FString(TEXT("ddd, dd MMM yyyy HH':'mm':'ss 'GMT'"))	//const FString RFC1123Pattern
			, FString(TEXT("dd-MM-yyyy"))	//const FString ShortDatePattern
			, FString(TEXT("HH:mm"))	//const FString ShortTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd'T'HH':'mm':'ss"))	//const FString SortableDateTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd HH':'mm':'ss'Z'"))	//const FString UniversalSortableDateTimePattern
			, FString(TEXT("MMMM, yyyy"))	//const FString YearMonthPattern
			);


		TSharedRef< FCulture > Culture = MakeShareable( new FCulture(
			  LOCTEXT("HindiDisplayName", "Hindi (India)")					//const FText DisplayName
			, FString(TEXT("Hindi (India)"))		//const FString EnglishName
			, 1081									//const int KeyboardLayoutId
			, 1081									//const int LCID
			, FString(TEXT("hi-IN"))				//const FString Name
			, FString(TEXT("हिंदी (भारत)"))			//const FString NativeName
			, FString(TEXT("hin"))					//const FString UnrealLegacyThreeLetterISOLanguageName
			, FString(TEXT("hin"))					//const FString ThreeLetterISOLanguageName
			, FString(TEXT("hi"))					//const FString TwoLetterISOLanguageName
			, NumberFormatting						//const FNumberFormattingRules NumberFormattingRule
			, TextFormatting						//const FTextFormattingRules TextFormattingRule
			, DateFormatting						//const FDateTimeFormattingRules DateTimeFormattingRule
			) );
#endif

		return Culture;
	}
};

#undef LOCTEXT_NAMESPACE
