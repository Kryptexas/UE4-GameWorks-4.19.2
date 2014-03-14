// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#if UE_ENABLE_ICU
#include "Text.h"
#include "ICUCulture.h"
#include <unicode/coll.h>
#include <unicode/sortkey.h>
#include <unicode/numfmt.h>
#include <unicode/msgfmt.h>
#include "ICUUtilities.h"

FText FText::AsDate(const FDateTime::FDate& Date, const EDateTimeStyle::Type DateStyle, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	TSharedPtr<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture : FInternationalization::GetCurrentCulture();

	FDateTime DateTime(Date.Year, Date.Month, Date.Day);
	int32 UNIXTimestamp = DateTime.ToUnixTimestamp();
	UDate ICUDate = static_cast<double>(UNIXTimestamp) * U_MILLIS_PER_SECOND;

	UErrorCode ICUStatus = U_ZERO_ERROR;
	const TSharedRef<const icu::DateFormat> ICUDateFormat( Culture->Implementation->GetDateFormatter(DateStyle) );
	icu::UnicodeString FormattedString;
	ICUDateFormat->format(ICUDate, FormattedString);

	FString NativeString;
	ICUUtilities::Convert(FormattedString, NativeString);

	return FText::CreateChronologicalText(NativeString);
}

FText FText::AsTime(const FDateTime::FTime& Time, const EDateTimeStyle::Type TimeStyle, const FString& TimeZone, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	TSharedPtr<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture : FInternationalization::GetCurrentCulture();

	FDateTime DateTime(1970, 1, 1, Time.Hour, Time.Minute, Time.Second, Time.Millisecond);
	int32 UNIXTimestamp = DateTime.ToUnixTimestamp();
	UDate ICUDate = static_cast<double>(UNIXTimestamp) * U_MILLIS_PER_SECOND;

	UErrorCode ICUStatus = U_ZERO_ERROR;
	const TSharedRef<const icu::DateFormat> ICUDateFormat( Culture->Implementation->GetTimeFormatter(TimeStyle, TimeZone) );
	icu::UnicodeString FormattedString;
	ICUDateFormat->format(ICUDate, FormattedString);

	FString NativeString;
	ICUUtilities::Convert(FormattedString, NativeString);

	return FText::CreateChronologicalText(NativeString);
}

FText FText::AsTime(const FTimespan& Time, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	TSharedPtr<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture : FInternationalization::GetCurrentCulture();

	FText TimespanFormatPattern = NSLOCTEXT("Timespan", "FormatPattern", "{Hours}:{Minutes}:{Seconds}");

	double TotalHours = Time.GetTotalHours();
	int32 Hours = static_cast<int32>(TotalHours);
	int32 Minutes = Time.GetMinutes();
	int32 Seconds = Time.GetSeconds();

	FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.MinimumIntegralDigits = 2;
	NumberFormattingOptions.MaximumIntegralDigits = 2;

	FFormatNamedArguments TimeArguments;
	TimeArguments.Add(TEXT("Hours"), Hours);
	TimeArguments.Add(TEXT("Minutes"), FText::AsNumber(Minutes, &(NumberFormattingOptions), Culture));
	TimeArguments.Add(TEXT("Seconds"), FText::AsNumber(Seconds, &(NumberFormattingOptions), Culture));
	return FText::Format(TimespanFormatPattern, TimeArguments);
}

FText FText::AsDateTime(const FDateTime& DateTime, const EDateTimeStyle::Type DateStyle, const EDateTimeStyle::Type TimeStyle, const FString& TimeZone, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	TSharedPtr<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture : FInternationalization::GetCurrentCulture();

	int32 UNIXTimestamp = DateTime.ToUnixTimestamp();
	UDate ICUDate = static_cast<double>(UNIXTimestamp) * U_MILLIS_PER_SECOND;

	UErrorCode ICUStatus = U_ZERO_ERROR;
	const TSharedRef<const icu::DateFormat> ICUDateFormat( Culture->Implementation->GetDateTimeFormatter(DateStyle, TimeStyle, TimeZone) );
	icu::UnicodeString FormattedString;
	ICUDateFormat->format(ICUDate, FormattedString);

	FString NativeString;
	ICUUtilities::Convert(FormattedString, NativeString);

	return FText::CreateChronologicalText(NativeString);
}

FText FText::AsMemory(SIZE_T NumBytes, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	FFormatNamedArguments Args;

	if (NumBytes < 1024)
	{
		Args.Add( TEXT("Number"), FText::AsNumber( (uint64)NumBytes, Options, TargetCulture) );
		Args.Add( TEXT("Unit"), FText::FromString( FString( TEXT("B") ) ) );
		return FText::Format( NSLOCTEXT("Internationalization", "ComputerMemoryFormatting", "{Number} {Unit}"), Args );
	}

	static const TCHAR* Prefixes = TEXT("kMGTPEZY");
	int32 Prefix = 0;

	for (; NumBytes > 1024 * 1024; NumBytes >>= 10)
	{
		++Prefix;
	}

	Args.Add( TEXT("Number"), FText::AsNumber( (uint64)(NumBytes / 1024), Options, TargetCulture) );
	Args.Add( TEXT("Unit"), FText::FromString( FString( 1, &Prefixes[Prefix] ) + TEXT("B") ) );
	return FText::Format( NSLOCTEXT("Internationalization", "ComputerMemoryFormatting", "{Number} {Unit}"), Args);
}

int32 FText::CompareTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel ) const
{
	UErrorCode ICUStatus = U_ZERO_ERROR;

	const TSharedRef<const icu::Collator> Collator( FInternationalization::GetCurrentCulture()->Implementation->GetCollator(ComparisonLevel) );

	icu::UnicodeString A;
	ICUUtilities::Convert(DisplayString.Get(), A);
	icu::UnicodeString B;
	ICUUtilities::Convert(Other.DisplayString.Get(), B);
	UCollationResult Result = Collator->compare(A, B, ICUStatus);

	return Result;
}

int32 FText::CompareToCaseIgnored( const FText& Other ) const
{
	return CompareTo(Other, ETextComparisonLevel::Secondary);
}

bool FText::EqualTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel ) const
{
	UErrorCode ICUStatus = U_ZERO_ERROR;

	const TSharedRef<const icu::Collator> Collator( FInternationalization::GetCurrentCulture()->Implementation->GetCollator(ComparisonLevel) );

	icu::UnicodeString A;
	ICUUtilities::Convert(DisplayString.Get(), A);
	icu::UnicodeString B;
	ICUUtilities::Convert(Other.DisplayString.Get(), B);
	UCollationResult Result = Collator->compare(A, B, ICUStatus);

	return Result == 0;
}

bool FText::EqualToCaseIgnored( const FText& Other ) const
{
	return EqualTo(Other, ETextComparisonLevel::Secondary);
}

class FText::FSortPredicate::FSortPredicateImplementation
{
private:
	icu::CollationKey GetCollationKeyFromText(const FText& Text)
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		icu::CollationKey CollationKey;
		const FString& String = Text.ToString();
		const UChar* const Characters = reinterpret_cast<const UChar* const>(*String);
		int32_t Length = String.Len();
		return ICUCollator->getCollationKey(Characters, Length, CollationKey, ICUStatus);
	}

public:
	FSortPredicateImplementation(const ETextComparisonLevel::Type ComparisonLevel)
		: ComparisonLevel(ComparisonLevel), ICUCollator( FInternationalization::GetCurrentCulture()->Implementation->GetCollator(ComparisonLevel) )
	{
	}

	bool Compare(const FText& A, const FText& B)
	{
		const icu::CollationKey ACollationKey = GetCollationKeyFromText(A);
		const icu::CollationKey BCollationKey = GetCollationKeyFromText(B);
		return (ACollationKey.compareTo(BCollationKey) == icu::Collator::EComparisonResult::GREATER ? false : true);
	}

private:
	const ETextComparisonLevel::Type ComparisonLevel;
	const TSharedRef<const icu::Collator> ICUCollator;
};

FText::FSortPredicate::FSortPredicate(const ETextComparisonLevel::Type ComparisonLevel)
	: Implementation( new FSortPredicateImplementation(ComparisonLevel) )
{

}

bool FText::FSortPredicate::operator()(const FText& A, const FText& B) const
{
	return Implementation->Compare(A, B);
}

void FText::GetFormatPatternParameters(const FText& Pattern, TArray<FString>& ParameterNames)
{
	UErrorCode ICUStatus = U_ZERO_ERROR;

	const FString& NativePatternString = Pattern.ToString();

	icu::UnicodeString ICUPatternString;
	ICUUtilities::Convert(NativePatternString, ICUPatternString);

	UParseError ICUParseError;
	icu::MessagePattern ICUMessagePattern(ICUPatternString, &(ICUParseError), ICUStatus);

	const int32_t PartCount = ICUMessagePattern.countParts();
	for(int32_t i = 0; i < PartCount; ++i)
	{
		UMessagePatternPartType PartType = ICUMessagePattern.getPartType(i);
		if(PartType == UMSGPAT_PART_TYPE_ARG_NAME || PartType == UMSGPAT_PART_TYPE_ARG_NUMBER)
		{
			icu::MessagePattern::Part Part = ICUMessagePattern.getPart(i);

			/** The following does a case-sensitive "TArray::AddUnique" by first checking to see if
			the parameter is in the ParameterNames list (using a case-sensitive comparison) followed
			by adding it to the ParameterNames */

			bool bIsCaseSensitiveUnique = true;
			FString CurrentArgument;
			ICUUtilities::Convert(ICUMessagePattern.getSubstring(Part), CurrentArgument);

			for(auto It = ParameterNames.CreateConstIterator(); It; ++It)
			{
				if(It->Equals(CurrentArgument))
				{
					bIsCaseSensitiveUnique = false;
				}
			}

			if(bIsCaseSensitiveUnique)
			{
				ParameterNames.Add( CurrentArgument );
			}
		}
	}
}

FText FText::Format(const FText& Pattern, const FFormatNamedArguments& Arguments)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	//SCOPE_CYCLE_COUNTER( STAT_TextFormat );

	const bool bEnableErrorResults = ENABLE_TEXT_ERROR_CHECKING_RESULTS && bEnableErrorCheckingResults;

	TArray<icu::UnicodeString> ArgumentNames;
	ArgumentNames.Reserve(Arguments.Num());

	TArray<icu::Formattable> ArgumentValues;
	ArgumentValues.Reserve(Arguments.Num());

	for(auto It = Arguments.CreateConstIterator(); It; ++It)
	{
		const FString& NativeArgumentName = It.Key();
		icu::UnicodeString ICUArgumentName;
		ICUUtilities::Convert(NativeArgumentName, ICUArgumentName, false);
		ArgumentNames.Add( ICUArgumentName );

		const FFormatArgumentValue& ArgumentValue = It.Value();
		switch(ArgumentValue.Type)
		{
		case EFormatArgumentType::Int:
			{
				ArgumentValues.Add( icu::Formattable( static_cast<int64_t>(ArgumentValue.IntValue) ));
			}
			break;
		case EFormatArgumentType::UInt:
			{
				ArgumentValues.Add( icu::Formattable( static_cast<int64_t>(ArgumentValue.UIntValue) ));
			}
			break;
		case EFormatArgumentType::Float:
			{
				ArgumentValues.Add( icu::Formattable( ArgumentValue.FloatValue ));
			}
			break;
		case EFormatArgumentType::Double:
			{
				ArgumentValues.Add( icu::Formattable( ArgumentValue.DoubleValue ));
			}
			break;
		case EFormatArgumentType::Text:
			{
				FString NativeStringValue = ArgumentValue.TextValue->ToString();
				icu::UnicodeString ICUStringValue;
				ICUUtilities::Convert(NativeStringValue, ICUStringValue, false);
				ArgumentValues.Add( icu::Formattable( ICUStringValue ) );
			}
			break;
		}
	}

	const FString& NativePatternString = Pattern.ToString();
	icu::UnicodeString ICUPatternString;
	ICUUtilities::Convert(NativePatternString, ICUPatternString);

	UErrorCode ICUStatus = U_ZERO_ERROR;
	UParseError ICUParseError;
	icu::MessageFormat ICUMessageFormatter(ICUPatternString, icu::Locale::getDefault(), ICUParseError, ICUStatus);
	if(U_FAILURE(ICUStatus))
	{
		return bEnableErrorResults ? FText::FromString(u_errorName(ICUStatus)) : FText();
	}

	icu::UnicodeString ICUResultString;
	ICUMessageFormatter.format(ArgumentNames.GetData(), ArgumentValues.GetData(), Arguments.Num(), ICUResultString, ICUStatus);
	if(U_FAILURE(ICUStatus))
	{
		return bEnableErrorResults ? FText::FromString(u_errorName(ICUStatus)) : FText();
	}

	FString NativeResultString;
	ICUUtilities::Convert(ICUResultString, NativeResultString);

	return FText(NativeResultString);
}

FText FText::Format(const FText& Pattern, const FFormatOrderedArguments& Arguments)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	//SCOPE_CYCLE_COUNTER( STAT_TextFormat );

	const bool bEnableErrorResults = ENABLE_TEXT_ERROR_CHECKING_RESULTS && bEnableErrorCheckingResults;
	
	TArray<icu::Formattable> ArgumentValues;
	ArgumentValues.Reserve(Arguments.Num());

	for(auto It = Arguments.CreateConstIterator(); It; ++It)
	{
		const FFormatArgumentValue& ArgumentValue = *It;
		switch(ArgumentValue.Type)
		{
		case EFormatArgumentType::Int:
			{
				ArgumentValues.Add( icu::Formattable( static_cast<int64_t>(ArgumentValue.IntValue) ));
			}
			break;
		case EFormatArgumentType::UInt:
			{
				ArgumentValues.Add( icu::Formattable( static_cast<int64_t>(ArgumentValue.UIntValue) ));
			}
			break;
		case EFormatArgumentType::Float:
			{
				ArgumentValues.Add( icu::Formattable( ArgumentValue.FloatValue ));
			}
			break;
		case EFormatArgumentType::Double:
			{
				ArgumentValues.Add( icu::Formattable( ArgumentValue.DoubleValue ));
			}
			break;
		case EFormatArgumentType::Text:
			{
				FString NativeStringValue = ArgumentValue.TextValue->ToString();
				icu::UnicodeString ICUStringValue;
				ICUUtilities::Convert(NativeStringValue, ICUStringValue, false);
				ArgumentValues.Add( icu::Formattable( ICUStringValue ) );
			}
			break;
		}
	}

	const FString& NativePatternString = Pattern.ToString();
	icu::UnicodeString ICUPatternString;
	ICUUtilities::Convert(NativePatternString, ICUPatternString);

	UErrorCode ICUStatus = U_ZERO_ERROR;
	UParseError ICUParseError;
	icu::MessageFormat ICUMessageFormatter(ICUPatternString, icu::Locale::getDefault(), ICUParseError, ICUStatus);
	if(U_FAILURE(ICUStatus))
	{
		return bEnableErrorResults ? FText::FromString(u_errorName(ICUStatus)) : FText();
	}

	icu::UnicodeString ICUResultString;
	ICUMessageFormatter.format(NULL, ArgumentValues.GetData(), Arguments.Num(), ICUResultString, ICUStatus);
	if(U_FAILURE(ICUStatus))
	{
		return bEnableErrorResults ? FText::FromString(u_errorName(ICUStatus)) : FText();
	}

	FString NativeResultString;
	ICUUtilities::Convert(ICUResultString, NativeResultString);

	return FText(NativeResultString);
}

FText FText::Format(const FText& Pattern, const TArray< FFormatArgumentData > InArguments)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	//SCOPE_CYCLE_COUNTER( STAT_TextFormat );

	const bool bEnableErrorResults = ENABLE_TEXT_ERROR_CHECKING_RESULTS && bEnableErrorCheckingResults;

	TArray<icu::UnicodeString> ArgumentNames;
	ArgumentNames.Reserve(InArguments.Num());

	TArray<icu::Formattable> ArgumentValues;
	ArgumentValues.Reserve(InArguments.Num());

	for(int32 x = 0; x < InArguments.Num(); ++x)
	{
		const FString& ArgumentName = InArguments[x].ArgumentName.ToString();
		icu::UnicodeString ICUArgumentName;
		ICUUtilities::Convert(ArgumentName, ICUArgumentName, false);
		ArgumentNames.Add( ICUArgumentName );

		const FFormatArgumentValue& ArgumentValue = InArguments[x].ArgumentValue;
		switch(ArgumentValue.Type)
		{
		case EFormatArgumentType::Int:
			{
				ArgumentValues.Add( icu::Formattable( static_cast<int64_t>(ArgumentValue.IntValue) ));
			}
			break;
		case EFormatArgumentType::UInt:
			{
				ArgumentValues.Add( icu::Formattable( static_cast<int64_t>(ArgumentValue.UIntValue) ));
			}
			break;
		case EFormatArgumentType::Float:
			{
				ArgumentValues.Add( icu::Formattable( ArgumentValue.FloatValue ));
			}
			break;
		case EFormatArgumentType::Double:
			{
				ArgumentValues.Add( icu::Formattable( ArgumentValue.DoubleValue ));
			}
			break;
		case EFormatArgumentType::Text:
			{
				FString StringValue = ArgumentValue.TextValue->ToString();
				icu::UnicodeString ICUStringValue;
				ICUUtilities::Convert(StringValue, ICUStringValue, false);
				ArgumentValues.Add( ICUStringValue );
			}
			break;
		}
	}

	const FString& PatternString = Pattern.ToString();
	icu::UnicodeString ICUPatternString;
	ICUUtilities::Convert(PatternString, ICUPatternString, false);

	UErrorCode ICUStatus = U_ZERO_ERROR;
	UParseError ICUParseError;
	icu::MessageFormat ICUMessageFormatter(ICUPatternString, icu::Locale::getDefault(), ICUParseError, ICUStatus);
	if(U_FAILURE(ICUStatus))
	{
		return bEnableErrorResults ? FText::FromString(u_errorName(ICUStatus)) : FText();
	}

	icu::UnicodeString Result;
	ICUMessageFormatter.format(ArgumentNames.GetData(), ArgumentValues.GetData(), InArguments.Num(), Result, ICUStatus);
	if(U_FAILURE(ICUStatus))
	{
		return bEnableErrorResults ? FText::FromString(u_errorName(ICUStatus)) : FText();
	}

	FString NativeResultString;
	ICUUtilities::Convert(Result, NativeResultString);

	return FText(NativeResultString);
}

#endif
