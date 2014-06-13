// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#include "TextHistory.h"

///////////////////////////////////////
// FTextHistory

/** Base class for all FText history types */
bool FTextHistory::IsOutOfDate(int32 InRevision)
{
	return InRevision < FTextLocalizationManager::Get().GetHeadCultureRevision();
}

TSharedPtr< FString > FTextHistory::GetSourceString()
{
	return NULL;
}

///////////////////////////////////////
// FTextHistory_NamedFormat

FTextHistory_Base::FTextHistory_Base(FString InSourceString)
	: SourceString(new FString( MoveTemp( InSourceString ) ))
{

}

FTextHistory_Base::FTextHistory_Base(TSharedPtr< FString > InSourceString)
	: SourceString(InSourceString)
{

}

FText FTextHistory_Base::ToText() const
{
	// This should never be called
	check(0);

	return FText::GetEmpty();
}

TSharedPtr< FString > FTextHistory_Base::GetSourceString()
{
	return SourceString;
}

void FTextHistory_Base::Serialize( FArchive& Ar )
{
	// If I serialize out the Namespace and Key HERE, then we can load it up.
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::Base;
		Ar << HistoryType;
	}
}

void FTextHistory_Base::SerializeForDisplayString(FArchive& Ar, TSharedRef<FString>& InOutDisplayString)
{
	if(Ar.IsLoading())
	{
		FString Namespace;
		FString Key;
		FString SourceStringRaw;

		Ar << Namespace;
		Ar << Key;
		Ar << SourceStringRaw;

		// If there was a SourceString, store it in the member variable
		if(!SourceStringRaw.IsEmpty())
		{
			SourceString = MakeShareable(new FString(SourceStringRaw));
		}

		// Using the deserialized namespace and key, find the DisplayString.
		InOutDisplayString = FTextLocalizationManager::Get().GetString(Namespace, Key, SourceString.Get());
	}
	else if(Ar.IsSaving())
	{
		TSharedPtr< FString > Namespace;
		TSharedPtr< FString > Key;

		FTextLocalizationManager::Get().FindKeyNamespaceFromDisplayString(InOutDisplayString, Namespace, Key);

		// Serialize the Namespace, or an empty string if there is none
		if( Namespace.IsValid() )
		{
			Ar << *(Namespace);
		}
		else
		{
			FString Empty;
			Ar << Empty;
		}

		// Serialize the Key, if there is not one and this is in the editor, generate a new key.
		if( Key.IsValid() )
		{
			Ar << *(Key);
		}
		else
		{
			if(GIsEditor)
			{
				FString SerializedKey = FGuid::NewGuid().ToString();
				Ar << SerializedKey;
			}
			else
			{
				FString Empty;
				Ar << Empty;
			}
		}

		// Serialize the SourceString, or an empty string if there is none
		if( SourceString.IsValid() )
		{
			Ar << *(SourceString);
		}
		else
		{
			FString Empty;
			Ar << Empty;
		}
	}
}

///////////////////////////////////////
// FTextHistory_NamedFormat

FTextHistory_NamedFormat::FTextHistory_NamedFormat(const FText& InSourceText, const FFormatNamedArguments& InArguments)
	: SourceText(InSourceText)
	, Arguments(InArguments)
{

}

FText FTextHistory_NamedFormat::ToText() const
{
	return FText::FormatInternal(SourceText, Arguments, true);
}

void FTextHistory_NamedFormat::Serialize( FArchive& Ar )
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::NamedFormat;
		Ar << HistoryType;
	}

	Ar << SourceText;
	Ar << Arguments;
}

///////////////////////////////////////
// FTextHistory_OrderedFormat

FTextHistory_OrderedFormat::FTextHistory_OrderedFormat(const FText& InSourceText, const FFormatOrderedArguments& InArguments)
	: SourceText(InSourceText)
	, Arguments(InArguments)
{

}

FText FTextHistory_OrderedFormat::ToText() const
{
	return FText::FormatInternal(SourceText, Arguments, true);
}

void FTextHistory_OrderedFormat::Serialize( FArchive& Ar )
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::OrderedFormat;
		Ar << HistoryType;
	}

	Ar << SourceText;
	Ar << Arguments;
}

///////////////////////////////////////
// FTextHistory_ArgumentDataFormat

FTextHistory_ArgumentDataFormat::FTextHistory_ArgumentDataFormat(const FText& InSourceText, const TArray< struct FFormatArgumentData >& InArguments)
	: SourceText(InSourceText)
	, Arguments(InArguments)
{

}

FText FTextHistory_ArgumentDataFormat::ToText() const
{
	return FText::FormatInternal(SourceText, Arguments, true);
}

void FTextHistory_ArgumentDataFormat::Serialize( FArchive& Ar )
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::ArgumentFormat;
		Ar << HistoryType;
	}

	Ar << SourceText;
	Ar << Arguments;
}

///////////////////////////////////////
// FTextHistory_FormatNumber

FTextHistory_FormatNumber::FTextHistory_FormatNumber()
	: FormatOptions(nullptr)
{
}

FTextHistory_FormatNumber::FTextHistory_FormatNumber(const FFormatArgumentValue& InSourceValue, const FNumberFormattingOptions* const InFormatOptions, const TSharedPtr<FCulture> InTargetCulture)
	: SourceValue(InSourceValue)
	, FormatOptions(nullptr)
	, TargetCulture(InTargetCulture)
{
	if(InFormatOptions)
	{
		FormatOptions = new FNumberFormattingOptions;
		(*FormatOptions) = *InFormatOptions;
	}
}

FTextHistory_FormatNumber::~FTextHistory_FormatNumber()
{
	delete FormatOptions;
}

void FTextHistory_FormatNumber::Serialize(FArchive& Ar)
{
	Ar << SourceValue;

	bool bHasFormatOptions = FormatOptions != nullptr;
	Ar << bHasFormatOptions;

	if(bHasFormatOptions)
	{
		if(Ar.IsLoading())
		{
			FormatOptions = new FNumberFormattingOptions;
		}
		CA_SUPPRESS(6011)
		Ar << *FormatOptions;
	}

	if(Ar.IsSaving())
	{
		FString CultureName = TargetCulture.IsValid()? TargetCulture->GetName() : FString();
		Ar << CultureName;
	}
	else if(Ar.IsLoading())
	{
		FString CultureName;
		Ar << CultureName;

		if(!CultureName.IsEmpty())
		{
			TargetCulture = FInternationalization::Get().GetCulture(CultureName);
		}
	}
}

///////////////////////////////////////
// FTextHistory_AsNumber

FTextHistory_AsNumber::FTextHistory_AsNumber(const FFormatArgumentValue& InSourceValue, const FNumberFormattingOptions* const InFormatOptions, const TSharedPtr<FCulture> InTargetCulture)
	: FTextHistory_FormatNumber(InSourceValue, InFormatOptions, InTargetCulture)
{
}

FText FTextHistory_AsNumber::ToText() const
{
	switch(SourceValue.Type)
	{
	case EFormatArgumentType::UInt:
		{
			return FText::AsNumber(SourceValue.UIntValue, FormatOptions, TargetCulture);
		}
	case EFormatArgumentType::Int:
		{
			return FText::AsNumber(SourceValue.IntValue, FormatOptions, TargetCulture);
		}
	case EFormatArgumentType::Float:
		{
			return FText::AsNumber(SourceValue.FloatValue, FormatOptions, TargetCulture);
		}
	case EFormatArgumentType::Double:
		{
			return FText::AsNumber(SourceValue.DoubleValue, FormatOptions, TargetCulture);
		}
	default:
		{
			// Should never reach this point
			check(0);
		}
	}
	return FText();
}

void FTextHistory_AsNumber::Serialize( FArchive& Ar )
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::AsNumber;
		Ar << HistoryType;
	}

	FTextHistory_FormatNumber::Serialize(Ar);
}

///////////////////////////////////////
// FTextHistory_AsPercent

FTextHistory_AsPercent::FTextHistory_AsPercent(const FFormatArgumentValue& InSourceValue, const FNumberFormattingOptions* const InFormatOptions, const TSharedPtr<FCulture> InTargetCulture)
	: FTextHistory_FormatNumber(InSourceValue, InFormatOptions, InTargetCulture)
{
}

FText FTextHistory_AsPercent::ToText() const
{
	switch(SourceValue.Type)
	{
	case EFormatArgumentType::Float:
		{
			return FText::AsPercent(SourceValue.FloatValue, FormatOptions, TargetCulture);
		}
	case EFormatArgumentType::Double:
		{
			return FText::AsPercent(SourceValue.DoubleValue, FormatOptions, TargetCulture);
		}
	default:
		{
			// Should never reach this point
			check(0);
		}
	}
	return FText();
}

void FTextHistory_AsPercent::Serialize( FArchive& Ar )
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::AsPercent;
		Ar << HistoryType;
	}

	FTextHistory_FormatNumber::Serialize(Ar);
}

///////////////////////////////////////
// FTextHistory_AsCurrency

FTextHistory_AsCurrency::FTextHistory_AsCurrency(const FFormatArgumentValue& InSourceValue, const FNumberFormattingOptions* const InFormatOptions, const TSharedPtr<FCulture> InTargetCulture)
	: FTextHistory_FormatNumber(InSourceValue, InFormatOptions, InTargetCulture)
{
}

FText FTextHistory_AsCurrency::ToText() const
{
	switch(SourceValue.Type)
	{
	case EFormatArgumentType::UInt:
		{
			return FText::AsCurrency(SourceValue.UIntValue, FormatOptions, TargetCulture);
		}
	case EFormatArgumentType::Int:
		{
			return FText::AsCurrency(SourceValue.IntValue, FormatOptions, TargetCulture);
		}
	case EFormatArgumentType::Float:
		{
			return FText::AsCurrency(SourceValue.FloatValue, FormatOptions, TargetCulture);
		}
	case EFormatArgumentType::Double:
		{
			return FText::AsCurrency(SourceValue.DoubleValue, FormatOptions, TargetCulture);
		}
	default:
		{
			// Should never reach this point
			check(0);
		}
	}
	return FText();
}

void FTextHistory_AsCurrency::Serialize( FArchive& Ar )
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::AsCurrency;
		Ar << HistoryType;
	}

	FTextHistory_FormatNumber::Serialize(Ar);
}

///////////////////////////////////////
// FTextHistory_AsDate

FTextHistory_AsDate::FTextHistory_AsDate(const FDateTime& InSourceDateTime, const EDateTimeStyle::Type InDateStyle, const TSharedPtr<FCulture> InTargetCulture)
	: SourceDateTime(InSourceDateTime)
	, DateStyle(InDateStyle)
	, TargetCulture(InTargetCulture)
{
}

void FTextHistory_AsDate::Serialize(FArchive& Ar)
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::AsDate;
		Ar << HistoryType;
	}

	Ar << SourceDateTime;

	int8 DateStyleInt8 = (int8)DateStyle;
	Ar << DateStyleInt8;
	DateStyle = (EDateTimeStyle::Type)DateStyleInt8;

	if(Ar.IsSaving())
	{
		FString CultureName = TargetCulture.IsValid()? TargetCulture->GetName() : FString();
		Ar << CultureName;
	}
	else if(Ar.IsLoading())
	{
		FString CultureName;
		Ar << CultureName;

		if(!CultureName.IsEmpty())
		{
			TargetCulture = FInternationalization::Get().GetCulture(CultureName);
		}
	}
}

FText FTextHistory_AsDate::ToText() const
{
	return FText::AsDate(SourceDateTime, DateStyle, TargetCulture);
}

///////////////////////////////////////
// FTextHistory_AsTime

FTextHistory_AsTime::FTextHistory_AsTime(const FDateTime& InSourceDateTime, const EDateTimeStyle::Type InTimeStyle, const FString& InTimeZone, const TSharedPtr<FCulture> InTargetCulture)
	: SourceDateTime(InSourceDateTime)
	, TimeStyle(InTimeStyle)
	, TimeZone(InTimeZone)
	, TargetCulture(InTargetCulture)
{
}

void FTextHistory_AsTime::Serialize(FArchive& Ar)
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::AsTime;
		Ar << HistoryType;
	}

	Ar << SourceDateTime;

	int8 TimeStyleInt8 = (int8)TimeStyle;
	Ar << TimeStyleInt8;
	TimeStyle = (EDateTimeStyle::Type)TimeStyleInt8;

	Ar << TimeZone;

	if(Ar.IsSaving())
	{
		FString CultureName = TargetCulture.IsValid()? TargetCulture->GetName() : FString();
		Ar << CultureName;
	}
	else if(Ar.IsLoading())
	{
		FString CultureName;
		Ar << CultureName;

		if(!CultureName.IsEmpty())
		{
			TargetCulture = FInternationalization::Get().GetCulture(CultureName);
		}
	}
}

FText FTextHistory_AsTime::ToText() const
{
	return FText::AsTime(SourceDateTime, TimeStyle, TimeZone, TargetCulture);
}

///////////////////////////////////////
// FTextHistory_AsDateTime

FTextHistory_AsDateTime::FTextHistory_AsDateTime(const FDateTime& InSourceDateTime, const EDateTimeStyle::Type InDateStyle, const EDateTimeStyle::Type InTimeStyle, const FString& InTimeZone, const TSharedPtr<FCulture> InTargetCulture)
	: SourceDateTime(InSourceDateTime)
	, DateStyle(InDateStyle)
	, TimeStyle(InTimeStyle)
	, TimeZone(InTimeZone)
	, TargetCulture(InTargetCulture)
{
}

void FTextHistory_AsDateTime::Serialize(FArchive& Ar)
{
	if(Ar.IsSaving())
	{
		int8 HistoryType = (int8)ETextHistoryType::AsDateTime;
		Ar << HistoryType;
	}

	Ar << SourceDateTime;

	int8 DateStyleInt8 = (int8)DateStyle;
	Ar << DateStyleInt8;
	DateStyle = (EDateTimeStyle::Type)DateStyleInt8;

	int8 TimeStyleInt8 = (int8)TimeStyle;
	Ar << TimeStyleInt8;
	TimeStyle = (EDateTimeStyle::Type)TimeStyleInt8;

	Ar << TimeZone;

	if(Ar.IsSaving())
	{
		FString CultureName = TargetCulture.IsValid()? TargetCulture->GetName() : FString();
		Ar << CultureName;
	}
	else if(Ar.IsLoading())
	{
		FString CultureName;
		Ar << CultureName;

		if(!CultureName.IsEmpty())
		{
			TargetCulture = FInternationalization::Get().GetCulture(CultureName);
		}
	}
}

FText FTextHistory_AsDateTime::ToText() const
{
	return FText::AsDateTime(SourceDateTime, DateStyle, TimeStyle, TimeZone, TargetCulture);
}