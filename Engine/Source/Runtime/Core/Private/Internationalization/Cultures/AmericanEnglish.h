// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Culture.h"

#define LOCTEXT_NAMESPACE "Internationalization"

class FAmericanEnglishCulture
{
public:
	static TSharedRef< FCulture > Create()
	{
#if UE_ENABLE_ICU
		TSharedRef< FCulture > Culture = MakeShareable( new FCulture( FString("en-US") ) );
#else
		TArray<FString> InNativeDigits;

		for (int i = 0; i<=10; ++i)
		{
			InNativeDigits.Add(FString(TEXT("%i"),i));
		}

		TArray<int> GroupSizes;
		GroupSizes.Add(3);

		FNumberFormattingRules AmericanEnglishNumberFormatting = FNumberFormattingRules(
			  2						// const int InCurrencyDecimalDigits
			, "."					// const FString CurrencyDecimalSeparator
			, ","					// const FString CurrencyGroupSeparator
			, GroupSizes			//const TArray<int>& CurrencyGroupSizes
			, TEXT("$")				//const FString CurrencySymbol
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
			, ECurrencyNegativePattern::BracketCurrencySymbolXBracket
			, ECurrencyPositivePattern::CurrencySymbolX
			, EPercentNegativePattern::NegativeSymbolXPct
			, EPercentPositivePattern::XPct
			, ENumberNegativePattern::NegativeSymbolX
			);

		FTextFormattingRules AmericanEnglishTextFormatting = FTextFormattingRules(false, FString(TEXT(",")), 1252, 37, 10000, 437);

		TArray<FString> AbbreviatedDayNames;
		AbbreviatedDayNames.Add(FString(TEXT("Sun")));
		AbbreviatedDayNames.Add(FString(TEXT("Mon")));
		AbbreviatedDayNames.Add(FString(TEXT("Tue")));
		AbbreviatedDayNames.Add(FString(TEXT("Wed")));
		AbbreviatedDayNames.Add(FString(TEXT("Thu")));
		AbbreviatedDayNames.Add(FString(TEXT("Fri")));
		AbbreviatedDayNames.Add(FString(TEXT("Sat")));

		TArray<FString> AbbreviatedMonthNames;
		AbbreviatedMonthNames.Add(FString(TEXT("Jan")));
		AbbreviatedMonthNames.Add(FString(TEXT("Feb")));
		AbbreviatedMonthNames.Add(FString(TEXT("Mar")));
		AbbreviatedMonthNames.Add(FString(TEXT("Apr")));
		AbbreviatedMonthNames.Add(FString(TEXT("May")));
		AbbreviatedMonthNames.Add(FString(TEXT("Jun")));
		AbbreviatedMonthNames.Add(FString(TEXT("Jul")));
		AbbreviatedMonthNames.Add(FString(TEXT("Aug")));
		AbbreviatedMonthNames.Add(FString(TEXT("Sep")));
		AbbreviatedMonthNames.Add(FString(TEXT("Oct")));
		AbbreviatedMonthNames.Add(FString(TEXT("Nov")));
		AbbreviatedMonthNames.Add(FString(TEXT("Dec")));

		TArray<FString> DayNames;
		DayNames.Add(FString(TEXT("Sunday")));
		DayNames.Add(FString(TEXT("Monday")));
		DayNames.Add(FString(TEXT("Tuesday")));
		DayNames.Add(FString(TEXT("Wednesday")));
		DayNames.Add(FString(TEXT("Thursday")));
		DayNames.Add(FString(TEXT("Friday")));
		DayNames.Add(FString(TEXT("Saturday")));

		TArray<FString> MonthNames;
		MonthNames.Add(FString(TEXT("January")));
		MonthNames.Add(FString(TEXT("February")));
		MonthNames.Add(FString(TEXT("March")));
		MonthNames.Add(FString(TEXT("April")));
		MonthNames.Add(FString(TEXT("May")));
		MonthNames.Add(FString(TEXT("June")));
		MonthNames.Add(FString(TEXT("July")));
		MonthNames.Add(FString(TEXT("August")));
		MonthNames.Add(FString(TEXT("September")));
		MonthNames.Add(FString(TEXT("October")));
		MonthNames.Add(FString(TEXT("November")));
		MonthNames.Add(FString(TEXT("December")));

		TArray<FString> ShortestDayNames;
		ShortestDayNames.Add(FString(TEXT("Su")));
		ShortestDayNames.Add(FString(TEXT("Mo")));
		ShortestDayNames.Add(FString(TEXT("Tu")));
		ShortestDayNames.Add(FString(TEXT("We")));
		ShortestDayNames.Add(FString(TEXT("Th")));
		ShortestDayNames.Add(FString(TEXT("Fr")));
		ShortestDayNames.Add(FString(TEXT("Sa")));

		FDateTimeFormattingRules AmericanEnglishDateFormatting = FDateTimeFormattingRules(
			  AbbreviatedDayNames		//const TArray<FString>& AbbreviatedDayNames
			, AbbreviatedMonthNames		//const TArray<FString>& AbbreviatedMonthNames
			, DayNames					//const TArray<FString>& DayNames
			, MonthNames				//const TArray<FString>& MonthNames
			, ShortestDayNames			//const TArray<FString>& ShortestDayNames
			, FString(TEXT("AM"))		//const FString AMDesignator
			, FString(TEXT("PM"))		//const FString PMDesignator
			, FString(TEXT("/"))		//const FString DateSeparator
			, FString(TEXT(":"))		//const FString TimeSeparator
			, 0							//const int FirstDayOfWeek
			, FString(TEXT("dddd, MMMM dd, yyyy h:mm:ss tt"))	//const FString FullDateTimePattern
			, FString(TEXT("dddd, MMMM dd, yyyy"))	//const FString LongDatePattern
			, FString(TEXT("h:mm:ss tt"))	//const FString LongTimePattern
			, FString(TEXT("MMMM dd")) //const FString MonthDayPattern
			, FString(TEXT("ddd, dd MMM yyyy HH':'mm':'ss 'GMT'"))	//const FString RFC1123Pattern
			, FString(TEXT("M/d/yyyy"))	//const FString ShortDatePattern
			, FString(TEXT("h:mm tt"))	//const FString ShortTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd'T'HH':'mm':'ss"))	//const FString SortableDateTimePattern
			, FString(TEXT("yyyy'-'MM'-'dd HH':'mm':'ss'Z'"))	//const FString UniversalSortableDateTimePattern
			, FString(TEXT("MMMM, yyyy"))	//const FString YearMonthPattern
			);


		TSharedRef< FCulture > Culture = MakeShareable( new FCulture(
			  LOCTEXT("AmericanEnglishDisplayName", "English (United States)")		//const FText DisplayName
			, FString(TEXT("English (United States)"))			//const FString EnglishName
			, 1033												//const int KeyboardLayoutId
			, 1033												//const int LCID
			, FString(TEXT("en-US"))							//const FString Name
			, FString(TEXT("English (United States)"))			//const FString NativeName
			, FString(TEXT("INT"))								//const FString UnrealLegacyThreeLetterISOLanguageName
			, FString(TEXT("eng"))								//const FString ThreeLetterISOLanguageName
			, FString(TEXT("en"))								//const FString TwoLetterISOLanguageName
			, AmericanEnglishNumberFormatting					//const FNumberFormattingRules NumberFormattingRule
			, AmericanEnglishTextFormatting						//const FTextFormattingRules TextFormattingRule
			, AmericanEnglishDateFormatting						//const FDateTimeFormattingRules DateTimeFormattingRule
			) );
#endif

		return Culture;
	}
};

#undef LOCTEXT_NAMESPACE
