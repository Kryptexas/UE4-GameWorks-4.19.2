// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#if UE_ENABLE_ICU
#include "ICUUtilities.h"
#include "ICUCulture.h"

namespace
{
	TSharedRef<const icu::Collator> CreateCollator( const icu::Locale& ICULocale )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( icu::Collator::createInstance( ICULocale, ICUStatus ) );
	}

	TSharedRef<const icu::DecimalFormat> CreateDecimalFormat( const icu::Locale& ICULocale )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( static_cast<icu::DecimalFormat*>(icu::NumberFormat::createInstance( ICULocale, ICUStatus )) );
	}

	TSharedRef<const icu::DecimalFormat> CreateCurrencyFormat( const icu::Locale& ICULocale )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( static_cast<icu::DecimalFormat*>(icu::NumberFormat::createCurrencyInstance( ICULocale, ICUStatus )) );
	}

	TSharedRef<const icu::DecimalFormat> CreatePercentFormat( const icu::Locale& ICULocale )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( static_cast<icu::DecimalFormat*>(icu::NumberFormat::createPercentInstance( ICULocale, ICUStatus )) );
	}

	TSharedRef<const icu::DateFormat> CreateDateFormat( const icu::Locale& ICULocale )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( icu::DateFormat::createDateInstance( icu::DateFormat::EStyle::kDefault, ICULocale ) );
	}

	TSharedRef<const icu::DateFormat> CreateTimeFormat( const icu::Locale& ICULocale )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( icu::DateFormat::createTimeInstance( icu::DateFormat::EStyle::kDefault, ICULocale ) );
	}

	TSharedRef<const icu::DateFormat> CreateDateTimeFormat( const icu::Locale& ICULocale )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( icu::DateFormat::createDateTimeInstance( icu::DateFormat::EStyle::kDefault, icu::DateFormat::EStyle::kDefault, ICULocale ) );
	}
}

FCulture::FICUCultureImplementation::FICUCultureImplementation(const FString& LocaleName)
	: ICULocale( TCHAR_TO_ANSI( *LocaleName ) )
	, ICUCollator( CreateCollator( ICULocale ) )
	, ICUDecimalFormat( CreateDecimalFormat( ICULocale ) )
	, ICUCurrencyFormat( CreateCurrencyFormat( ICULocale ) )
	, ICUPercentFormat( CreatePercentFormat( ICULocale ) )
	, ICUDateFormat ( CreateDateFormat( ICULocale ) )
	, ICUTimeFormat ( CreateTimeFormat( ICULocale ) )
	, ICUDateTimeFormat ( CreateDateTimeFormat( ICULocale ) )
{

}

FString FCulture::FICUCultureImplementation::GetDisplayName() const
{
	icu::UnicodeString ICUResult;
	ICULocale.getDisplayName(ICUResult);
	FString Result;
	ICUUtilities::Convert(ICUResult, Result);
	return Result;
}

FString FCulture::FICUCultureImplementation::GetEnglishName() const
{
	icu::UnicodeString ICUResult;
	ICULocale.getDisplayName(icu::Locale("en"), ICUResult);
	FString Result;
	ICUUtilities::Convert(ICUResult, Result);
	return Result;
}

int FCulture::FICUCultureImplementation::GetKeyboardLayoutId() const
{
	return 0;
}

int FCulture::FICUCultureImplementation::GetLCID() const
{
	return ICULocale.getLCID();
}

FString FCulture::FICUCultureImplementation::GetName() const
{
	return ICULocale.getName();
}

FString FCulture::FICUCultureImplementation::GetNativeName() const
{
	icu::UnicodeString ICUResult;
	ICULocale.getDisplayName(ICULocale, ICUResult);
	FString Result;
	ICUUtilities::Convert(ICUResult, Result);
	return Result;
}

FString FCulture::FICUCultureImplementation::GetNativeLanguage() const
{
	icu::UnicodeString ICUNativeLanguage;
	ICULocale.getDisplayLanguage(ICULocale, ICUNativeLanguage);
	FString NativeLanguage;
	ICUUtilities::Convert(ICUNativeLanguage, NativeLanguage);

	icu::UnicodeString ICUNativeScript;
	ICULocale.getDisplayScript(ICULocale, ICUNativeScript);
	FString NativeScript;
	ICUUtilities::Convert(ICUNativeScript, NativeScript);

	if ( !NativeScript.IsEmpty() )
	{
		return NativeLanguage + TEXT(" (") + NativeScript + TEXT(")");
	}
	return NativeLanguage;
}

FString FCulture::FICUCultureImplementation::GetNativeRegion() const
{
	icu::UnicodeString ICUNativeCountry;
	ICULocale.getDisplayCountry(ICULocale, ICUNativeCountry);
	FString NativeCountry;
	ICUUtilities::Convert(ICUNativeCountry, NativeCountry);

	icu::UnicodeString ICUNativeVariant;
	ICULocale.getDisplayVariant(ICULocale, ICUNativeVariant);
	FString NativeVariant;
	ICUUtilities::Convert(ICUNativeVariant, NativeVariant);

	if ( !NativeVariant.IsEmpty() )
	{
		return NativeCountry + TEXT(", ") + NativeVariant;
	}
	return NativeCountry;
}

FString FCulture::FICUCultureImplementation::GetUnrealLegacyThreeLetterISOLanguageName() const
{
	FString Result( ICULocale.getISO3Language() );

	// Legacy Overrides:
	     if(Result == TEXT("eng")) { Result = TEXT("INT"); }
	else if(Result == TEXT("jpn")) { Result = TEXT("JPN"); }
	else if(Result == TEXT("kor")) { Result = TEXT("KOR"); }

	return Result;
}

FString FCulture::FICUCultureImplementation::GetThreeLetterISOLanguageName() const
{
	return ICULocale.getISO3Language();
}

FString FCulture::FICUCultureImplementation::GetTwoLetterISOLanguageName() const
{
	return ICULocale.getLanguage();
}

FString FCulture::FICUCultureImplementation::GetCountry() const
{
	return ICULocale.getCountry();
}

FString FCulture::FICUCultureImplementation::GetVariant() const
{
	return ICULocale.getVariant();
}

TSharedRef<const icu::Collator> FCulture::FICUCultureImplementation::GetCollator(const ETextComparisonLevel::Type ComparisonLevel) const
{
	UErrorCode ICUStatus = U_ZERO_ERROR;
	const bool bIsDefault = (ComparisonLevel == ETextComparisonLevel::Default);
	const TSharedRef<const icu::Collator> DefaultCollator( ICUCollator );
	if(bIsDefault)
	{
		return DefaultCollator;
	}
	else
	{
		const TSharedRef<icu::Collator> Collator( DefaultCollator->clone() );
		Collator->setAttribute(UColAttribute::UCOL_STRENGTH, UEToICU(ComparisonLevel), ICUStatus);
		return Collator;
	}
}

TSharedRef<const icu::DecimalFormat> FCulture::FICUCultureImplementation::GetDecimalFormatter(const FNumberFormattingOptions* const Options) const
{
	const bool bIsDefault = Options == NULL;
	const TSharedRef<const icu::DecimalFormat> DefaultFormatter( ICUDecimalFormat );
	if(bIsDefault)
	{
		return DefaultFormatter;
	}
	else
	{
		const TSharedRef<icu::DecimalFormat> Formatter( static_cast<icu::DecimalFormat*>(DefaultFormatter->clone()) );
		if(Options)
		{
			Formatter->setGroupingUsed(Options->UseGrouping);
			Formatter->setRoundingMode(UEToICU(Options->RoundingMode));
			Formatter->setMinimumIntegerDigits(Options->MinimumIntegralDigits);
			Formatter->setMaximumIntegerDigits(Options->MaximumIntegralDigits);
			Formatter->setMinimumFractionDigits(Options->MinimumFractionalDigits);
			Formatter->setMaximumFractionDigits(Options->MaximumFractionalDigits);
		}
		return Formatter;
	}
}

TSharedRef<const icu::DecimalFormat> FCulture::FICUCultureImplementation::GetCurrencyFormatter(const FNumberFormattingOptions* const Options) const
{
	const bool bIsDefault = Options == NULL;
	const TSharedRef<const icu::DecimalFormat> DefaultFormatter( ICUCurrencyFormat );
	if(bIsDefault)
	{
		return DefaultFormatter;
	}
	else
	{
		const TSharedRef<icu::DecimalFormat> Formatter( static_cast<icu::DecimalFormat*>(DefaultFormatter->clone()) );
		if(Options)
		{
			Formatter->setGroupingUsed(Options->UseGrouping);
			Formatter->setRoundingMode(UEToICU(Options->RoundingMode));
			Formatter->setMinimumIntegerDigits(Options->MinimumIntegralDigits);
			Formatter->setMaximumIntegerDigits(Options->MaximumIntegralDigits);
			Formatter->setMinimumFractionDigits(Options->MinimumFractionalDigits);
			Formatter->setMaximumFractionDigits(Options->MaximumFractionalDigits);
		}
		return Formatter;
	}
}

TSharedRef<const icu::DecimalFormat> FCulture::FICUCultureImplementation::GetPercentFormatter(const FNumberFormattingOptions* const Options) const
{
	const bool bIsDefault = Options == NULL;
	const TSharedRef<const icu::DecimalFormat> DefaultFormatter( ICUPercentFormat );
	if(bIsDefault)
	{
		return DefaultFormatter;
	}
	else
	{
		const TSharedRef<icu::DecimalFormat> Formatter( static_cast<icu::DecimalFormat*>(DefaultFormatter->clone()) );
		if(Options)
		{
			Formatter->setGroupingUsed(Options->UseGrouping);
			Formatter->setRoundingMode(UEToICU(Options->RoundingMode));
			Formatter->setMinimumIntegerDigits(Options->MinimumIntegralDigits);
			Formatter->setMaximumIntegerDigits(Options->MaximumIntegralDigits);
			Formatter->setMinimumFractionDigits(Options->MinimumFractionalDigits);
			Formatter->setMaximumFractionDigits(Options->MaximumFractionalDigits);
		}
		return Formatter;
	}
}

TSharedRef<const icu::DateFormat> FCulture::FICUCultureImplementation::GetDateFormatter(const EDateTimeStyle::Type DateStyle) const
{
	const bool bIsDefault = (DateStyle == EDateTimeStyle::Default);

	const TSharedRef<const icu::DateFormat> DefaultFormatter( ICUDateFormat );
	if(bIsDefault)
	{
		return DefaultFormatter;
	}
	else
	{
		const TSharedRef<icu::DateFormat> Formatter( icu::DateFormat::createDateInstance( UEToICU(DateStyle), ICULocale ) );
		const icu::TimeZone& TimeZone = *(icu::TimeZone::getGMT());
		Formatter->setTimeZone( TimeZone );
		return Formatter;
	}
}

TSharedRef<const icu::DateFormat> FCulture::FICUCultureImplementation::GetTimeFormatter(const EDateTimeStyle::Type TimeStyle, const FString& TimeZone) const
{
	icu::UnicodeString InputTimeZoneID;
	ICUUtilities::Convert(TimeZone, InputTimeZoneID, false);

	const TSharedRef<const icu::DateFormat> DefaultFormatter( ICUDateTimeFormat );

	bool bIsDefaultTimeZone = TimeZone.IsEmpty();
	if( !bIsDefaultTimeZone )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;

		icu::UnicodeString CanonicalInputTimeZoneID;
		icu::TimeZone::getCanonicalID(InputTimeZoneID, CanonicalInputTimeZoneID, ICUStatus);

		icu::UnicodeString DefaultTimeZoneID;
		DefaultFormatter->getTimeZone().getID(DefaultTimeZoneID);

		icu::UnicodeString CanonicalDefaultTimeZoneID;
		icu::TimeZone::getCanonicalID(DefaultTimeZoneID, CanonicalDefaultTimeZoneID, ICUStatus);

		bIsDefaultTimeZone = (CanonicalInputTimeZoneID == CanonicalDefaultTimeZoneID ? true : false);
	}

	const bool bIsDefault = 
		TimeStyle == EDateTimeStyle::Default &&
		bIsDefaultTimeZone;

	if(bIsDefault)
	{
		return DefaultFormatter;
	}
	else
	{
		const TSharedRef<icu::DateFormat> Formatter( icu::DateFormat::createTimeInstance( UEToICU(TimeStyle), ICULocale ) );
		Formatter->adoptTimeZone( bIsDefaultTimeZone ? icu::TimeZone::createDefault() :icu::TimeZone::createTimeZone(InputTimeZoneID) );
		return Formatter;
	}
}

TSharedRef<const icu::DateFormat> FCulture::FICUCultureImplementation::GetDateTimeFormatter(const EDateTimeStyle::Type DateStyle, const EDateTimeStyle::Type TimeStyle, const FString& TimeZone) const
{
	icu::UnicodeString InputTimeZoneID;
	ICUUtilities::Convert(TimeZone, InputTimeZoneID, false);

	const TSharedRef<const icu::DateFormat> DefaultFormatter( ICUDateTimeFormat );

	bool bIsDefaultTimeZone = TimeZone.IsEmpty();
	if( !bIsDefaultTimeZone )
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;

		icu::UnicodeString CanonicalInputTimeZoneID;
		icu::TimeZone::getCanonicalID(InputTimeZoneID, CanonicalInputTimeZoneID, ICUStatus);

		icu::UnicodeString DefaultTimeZoneID;
		DefaultFormatter->getTimeZone().getID(DefaultTimeZoneID);

		icu::UnicodeString CanonicalDefaultTimeZoneID;
		icu::TimeZone::getCanonicalID(DefaultTimeZoneID, CanonicalDefaultTimeZoneID, ICUStatus);

		bIsDefaultTimeZone = (CanonicalInputTimeZoneID == CanonicalDefaultTimeZoneID ? true : false);
	}

	const bool bIsDefault = 
		DateStyle == EDateTimeStyle::Default &&
		TimeStyle == EDateTimeStyle::Default &&
		bIsDefaultTimeZone;

	if(bIsDefault)
	{
		return DefaultFormatter;
	}
	else
	{
		const TSharedRef<icu::DateFormat> Formatter( icu::DateFormat::createDateTimeInstance( UEToICU(DateStyle), UEToICU(TimeStyle), ICULocale ) );
		Formatter->adoptTimeZone( bIsDefaultTimeZone ? icu::TimeZone::createDefault() :icu::TimeZone::createTimeZone(InputTimeZoneID) );
		return Formatter;
	}
}
#endif