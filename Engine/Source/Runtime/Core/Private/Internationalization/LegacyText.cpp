// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"

#if !UE_ENABLE_ICU
#include "Text.h"

bool FText::IsWhitespace( const TCHAR Char )
{
	return FChar::IsWhitespace(Char);
}

FText FText::AsDate( const FDateTime& DateTime, const EDateTimeStyle::Type DateStyle, const FString& TimeZone, const FCulturePtr& TargetCulture )
{
	checkf(FInternationalization::Get().IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	return FText::FromString( DateTime.ToString( TEXT("%Y.%m.%d") ) );
}

FText FText::AsTime( const FDateTime& DateTime, const EDateTimeStyle::Type TimeStyle, const FString& TimeZone, const FCulturePtr& TargetCulture )
{
	checkf(FInternationalization::Get().IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	return FText::FromString( DateTime.ToString( TEXT("%H.%M.%S") ) );
}

FText FText::AsTimespan( const FTimespan& Timespan, const FCulturePtr& TargetCulture)
{
	checkf(FInternationalization::Get().IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	FDateTime DateTime(Timespan.GetTicks());
	return FText::FromString( DateTime.ToString( TEXT("%H.%M.%S") ) );
}

FText FText::AsDateTime( const FDateTime& DateTime, const EDateTimeStyle::Type DateStyle, const EDateTimeStyle::Type TimeStyle, const FString& TimeZone, const FCulturePtr& TargetCulture )
{
	checkf(FInternationalization::Get().IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	return FText::FromString(DateTime.ToString(TEXT("%Y.%m.%d-%H.%M.%S")));
}

FText FText::AsMemory( SIZE_T NumBytes, const FNumberFormattingOptions* const Options, const FCulturePtr& TargetCulture )
{
	checkf(FInternationalization::Get().IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
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

	const double MemorySizeAsDouble = (double)NumBytes / 1024.0;
	Args.Add( TEXT("Number"), FText::AsNumber( MemorySizeAsDouble, Options, TargetCulture) );
	Args.Add( TEXT("Unit"), FText::FromString( FString( 1, &Prefixes[Prefix] ) + TEXT("B") ) );
	return FText::Format( NSLOCTEXT("Internationalization", "ComputerMemoryFormatting", "{Number} {Unit}"), Args);
}

int32 FText::CompareTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel ) const
{
	return FCString::Strcmp( *DisplayString.Get(), *Other.DisplayString.Get() );
}

int32 FText::CompareToCaseIgnored( const FText& Other ) const
{
	return FCString::Stricmp( *DisplayString.Get(), *Other.DisplayString.Get() );
}

bool FText::EqualTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel ) const
{
	return FCString::Strcmp( *DisplayString.Get(), *Other.DisplayString.Get() ) == 0;
}

bool FText::EqualToCaseIgnored( const FText& Other ) const
{
	return CompareToCaseIgnored( Other ) == 0;
}

FText::FSortPredicate::FSortPredicate(const ETextComparisonLevel::Type ComparisonLevel)
{

}

bool FText::FSortPredicate::operator()(const FText& A, const FText& B) const
{
	return A.ToString() < B.ToString();
}

bool FText::IsLetter( const TCHAR Char )
{
	return (Char>='A' && Char<='Z') || (Char>='a' && Char<='z');
}

namespace TextBiDi
{

namespace Internal
{

class FLegacyTextBiDi : public ITextBiDi
{
public:
	virtual ETextDirection ComputeTextDirection(const FText& InText) override
	{
		return FLegacyTextBiDi::ComputeTextDirection(InText.ToString());
	}

	virtual ETextDirection ComputeTextDirection(const FString& InString) override
	{
		return ETextDirection::LeftToRight;
	}

	virtual ETextDirection ComputeTextDirection(const FText& InText, TArray<FTextDirectionInfo>& OutTextDirectionInfo) override
	{
		return FLegacyTextBiDi::ComputeTextDirection(InText.ToString(), OutTextDirectionInfo);
	}

	virtual ETextDirection ComputeTextDirection(const FString& InString, TArray<FTextDirectionInfo>& OutTextDirectionInfo) override
	{
		OutTextDirectionInfo.Reset();

		if (!InString.IsEmpty())
		{
			FTextDirectionInfo TextDirectionInfo;
			TextDirectionInfo.StartIndex = 0;
			TextDirectionInfo.Length = InString.Len();
			TextDirectionInfo.TextDirection = ETextDirection::LeftToRight;
			OutTextDirectionInfo.Add(MoveTemp(TextDirectionInfo));
		}

		return ETextDirection::LeftToRight;
	}
};

} // namespace Internal

TUniquePtr<ITextBiDi> CreateTextBiDi()
{
	return MakeUnique<Internal::FLegacyTextBiDi>();
}

ETextDirection ComputeTextDirection(const FText& InText)
{
	return ComputeTextDirection(InText.ToString());
}

ETextDirection ComputeTextDirection(const FString& InString)
{
	return ETextDirection::LeftToRight;
}

ETextDirection ComputeTextDirection(const FText& InText, TArray<FTextDirectionInfo>& OutTextDirectionInfo)
{
	return ComputeTextDirection(InText.ToString(), OutTextDirectionInfo);
}

ETextDirection ComputeTextDirection(const FString& InString, TArray<FTextDirectionInfo>& OutTextDirectionInfo)
{
	OutTextDirectionInfo.Reset();

	if (!InString.IsEmpty())
	{
		FTextDirectionInfo TextDirectionInfo;
		TextDirectionInfo.StartIndex = 0;
		TextDirectionInfo.Length = InString.Len();
		TextDirectionInfo.TextDirection = ETextDirection::LeftToRight;
		OutTextDirectionInfo.Add(MoveTemp(TextDirectionInfo));
	}

	return ETextDirection::LeftToRight;
}

} // namespace TextBiDi

#endif
