// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Culture.h"

#define LOCTEXT_NAMESPACE "Internationalization"

class FLEETCulture
{
public:
	static TSharedRef< FCulture > Create()
	{
#if UE_ENABLE_ICU
		TSharedRef< FCulture > Culture = MakeShareable( new FCulture( FString() ) );
#else
		TArray<FString> InNativeDigits;

		for (int i = 0; i<=10; ++i)
		{
			InNativeDigits.Add(FString(TEXT("%i"),i));
		}

		TArray<int> GroupSizes;
		GroupSizes.Add(1);
		GroupSizes.Add(2);
		GroupSizes.Add(3);

		FNumberFormattingRules LEETCultureNumberFormatting = FNumberFormattingRules(
			  4						// const int InCurrencyDecimalDigits
			, "'"					// const FString CurrencyDecimalSeparator
			, "`"					// const FString CurrencyGroupSeparator
			, GroupSizes			//const TArray<int>& CurrencyGroupSizes
			, TEXT("@")				//const FString CurrencySymbol
			, "n4n"					//const FString NaNSymbol
			, InNativeDigits		//const TArray<FString>& NativeDigits
			, "--"					//const FString NegativeSign
			, 4						//const int NumberDecimalDigits
			, "'"					//const FString NumberDecimalSeparator
			, "`"					//const FString NumberGroupSeparator
			, GroupSizes			//const TArray<int>& NumberGroupSizes
			, 4						//const int PercentDecimalDigits
			, "'"					//const FString PercentDecimalSeparator
			, "`"					//const FString PercentGroupSeparator
			, GroupSizes			//const TArray<int>& PercentGroupSizes
			, "%%"					//const FString PercentSymbol
			, "‰‰"					//const FString PerMilleSymbol
			, "++"					//const FString PositiveSign
			, ECurrencyNegativePattern::NegativeSymbolCurrencySymbolX
			, ECurrencyPositivePattern::CurrencySymbolX
			, EPercentNegativePattern::PctSpaceXNegativeSymbol
			, EPercentPositivePattern::PctSpaceX
			, ENumberNegativePattern::NegativeSymbolX
			);

		FTextFormattingRules LEETCultureTextFormatting = FTextFormattingRules(false, FString(TEXT(",")), 1252, 37, 10000, 437);

		TArray<FString> AbbreviatedDayNames;

		AbbreviatedDayNames.Add(FString(TEXT("​SuN")));
		AbbreviatedDayNames.Add(FString(TEXT("​M0n")));
		AbbreviatedDayNames.Add(FString(TEXT("​Tu3")));
		AbbreviatedDayNames.Add(FString(TEXT("​W3D")));
		AbbreviatedDayNames.Add(FString(TEXT("​ThU")));
		AbbreviatedDayNames.Add(FString(TEXT("​Fr1")));
		AbbreviatedDayNames.Add(FString(TEXT("​Sa7")));

		TArray<FString> AbbreviatedMonthNames;

		AbbreviatedMonthNames.Add(FString(TEXT("​JaN")));
		AbbreviatedMonthNames.Add(FString(TEXT("​F3B")));
		AbbreviatedMonthNames.Add(FString(TEXT("​MaR")));
		AbbreviatedMonthNames.Add(FString(TEXT("​ApR")));
		AbbreviatedMonthNames.Add(FString(TEXT("​MaY")));
		AbbreviatedMonthNames.Add(FString(TEXT("​JuN")));
		AbbreviatedMonthNames.Add(FString(TEXT("​JuL")));
		AbbreviatedMonthNames.Add(FString(TEXT("​AuG")));
		AbbreviatedMonthNames.Add(FString(TEXT("​S3p")));
		AbbreviatedMonthNames.Add(FString(TEXT("​0ct")));
		AbbreviatedMonthNames.Add(FString(TEXT("​N0v")));
		AbbreviatedMonthNames.Add(FString(TEXT("​D3c")));

		TArray<FString> DayNames;
		DayNames.Add(FString(TEXT("SuNdAy")));
		DayNames.Add(FString(TEXT("M0nDaY")));
		DayNames.Add(FString(TEXT("Tu35daY")));
		DayNames.Add(FString(TEXT("W3Dn35daY")));
		DayNames.Add(FString(TEXT("ThUr5daY")));
		DayNames.Add(FString(TEXT("Fr1DaY")));
		DayNames.Add(FString(TEXT("Sa7uRdaY")));

		TArray<FString> MonthNames;
		MonthNames.Add(FString(TEXT("JaNuArY")));
		MonthNames.Add(FString(TEXT("F3bRuArY")));
		MonthNames.Add(FString(TEXT("MaRcH")));
		MonthNames.Add(FString(TEXT("ApR1L")));
		MonthNames.Add(FString(TEXT("MaY")));
		MonthNames.Add(FString(TEXT("Jun3")));
		MonthNames.Add(FString(TEXT("Ju1y")));
		MonthNames.Add(FString(TEXT("Augu57")));
		MonthNames.Add(FString(TEXT("53p73mb3r")));
		MonthNames.Add(FString(TEXT("0c70b3r")));
		MonthNames.Add(FString(TEXT("N0v3mb3r")));
		MonthNames.Add(FString(TEXT("D3c3mb3r")));

		TArray<FString> ShortestDayNames;
		ShortestDayNames.Add(FString(TEXT("5u")));
		ShortestDayNames.Add(FString(TEXT("M0")));
		ShortestDayNames.Add(FString(TEXT("7u")));
		ShortestDayNames.Add(FString(TEXT("W3")));
		ShortestDayNames.Add(FString(TEXT("7h")));
		ShortestDayNames.Add(FString(TEXT("8r")));
		ShortestDayNames.Add(FString(TEXT("5a")));

		FDateTimeFormattingRules LEETCultureDateFormatting = FDateTimeFormattingRules(
			  AbbreviatedDayNames		//const TArray<FString>& AbbreviatedDayNames
			, AbbreviatedMonthNames		//const TArray<FString>& AbbreviatedMonthNames
			, DayNames					//const TArray<FString>& DayNames
			, MonthNames				//const TArray<FString>& MonthNames
			, ShortestDayNames			//const TArray<FString>& ShortestDayNames
			, FString( TEXT("4M") )		//const FString AMDesignator
			, FString( TEXT("PM") )		//const FString PMDesignator
			, FString( TEXT(" / ") )		//const FString DateSeparator
			, FString( TEXT(" : ") )		//const FString TimeSeparator
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
			  LOCTEXT("LEETCultureDisplayName", "LEET Language (LEET Country)")		//const FText DisplayName
			, FString(TEXT("LEET Language (LEET Country)"))			//const FString EnglishName
			, 1033												//const int KeyboardLayoutId
			, 1033												//const int LCID
			, FString(TEXT("LEET"))									//const FString Name
			, FString(TEXT("LEET Language (LEET Country)"))	//const FString NativeName
			, FString(TEXT(""))								//const FString UnrealLegacyThreeLetterISOLanguageName
			, FString(TEXT(""))								//const FString ThreeLetterISOLanguageName
			, FString(TEXT(""))								//const FString TwoLetterISOLanguageName
			, LEETCultureNumberFormatting					//const FNumberFormattingRules NumberFormattingRule
			, LEETCultureTextFormatting						//const FTextFormattingRules TextFormattingRule
			, LEETCultureDateFormatting						//const FDateTimeFormattingRules DateTimeFormattingRule
			) );
#endif

		return Culture;
	}
};

#undef LOCTEXT_NAMESPACE
