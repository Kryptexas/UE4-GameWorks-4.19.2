// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#if UE_ENABLE_ICU
#include "ICUText.h"
#else
#include "LegacyText.h"
#endif

//DEFINE_LOG_CATEGORY(LogText);

//DEFINE_STAT(STAT_TextFormat);

#define LOCTEXT_NAMESPACE "Core.Text"

const FString* FTextInspector::GetNamespace(const FText& Text)
{
	return Text.Namespace.Get();
}

const FString* FTextInspector::GetKey(const FText& Text)
{
	return Text.Key.Get();
}

const FString* FTextInspector::GetSourceString(const FText& Text)
{
	return Text.SourceString.Get();
}

const FString& FTextInspector::GetDisplayString(const FText& Text)
{
	return Text.DisplayString.Get();
}

int32 FTextInspector::GetFlags(const FText& Text)
{
	return Text.Flags;
}

FFormatArgumentValue::FFormatArgumentValue(const int Value)
	: Type(EFormatArgumentType::Int)
{
	IntValue = Value;
}

FFormatArgumentValue::FFormatArgumentValue(const unsigned int Value)
	: Type(EFormatArgumentType::UInt)
{
	UIntValue = Value;
}

FFormatArgumentValue::FFormatArgumentValue(const float Value)
	: Type(EFormatArgumentType::Float)
{
	FloatValue = Value;
}

FFormatArgumentValue::FFormatArgumentValue(const double Value)
	: Type(EFormatArgumentType::Double)
{
	DoubleValue = Value;
}

FFormatArgumentValue::FFormatArgumentValue(const FText& Value)
	: Type(EFormatArgumentType::Text)
{
	TextValue = new FText(Value);
}

FFormatArgumentValue::FFormatArgumentValue(const FFormatArgumentValue& Source)
	: Type(Source.Type)
{
	switch(Type)
	{
	case EFormatArgumentType::Int:
		{
			IntValue = Source.IntValue;
		}
		break;
	case EFormatArgumentType::UInt:
		{
			UIntValue = Source.UIntValue;
		}
		break;
	case EFormatArgumentType::Float:
		{
			FloatValue = Source.FloatValue;
		}
	case EFormatArgumentType::Double:
		{
			DoubleValue = Source.DoubleValue;
		}
		break;
	case EFormatArgumentType::Text:
		{
			TextValue = new FText(*Source.TextValue);
		}
		break;
	}
}

FFormatArgumentValue::~FFormatArgumentValue()
{
	if(Type == EFormatArgumentType::Text)
	{
		delete TextValue;
	}
}

// These default values have been duplicated to the KismetTextLibrary functions for Blueprints. Please replicate any changes there!
FNumberFormattingOptions::FNumberFormattingOptions()
	:   UseGrouping(true),
		RoundingMode(ERoundingMode::HalfToEven),
		MinimumIntegralDigits(1),
		MaximumIntegralDigits(DBL_MAX_10_EXP + DBL_DIG + 1),
		MinimumFractionalDigits(0),
		MaximumFractionalDigits(3)
{

}

bool FText::bEnableErrorCheckingResults = ENABLE_TEXT_ERROR_CHECKING_RESULTS;
bool FText::bSuppressWarnings = false;

//Some error text formats
const FText FText::UnusedArgumentsError = LOCTEXT("Error_UnusedArguments", "ERR: The following arguments were unused ({0}).");
const FText FText::CommentStartError = LOCTEXT("Error_CommentDoesntStartWithQMark", "ERR: The Comment for Arg Block {0} does not start with a '?'.");
const FText FText::TooFewArgsErrorFormat = LOCTEXT("Error_TooFewArgs", "ERR: There are too few arguments. Arg {0} is used in block {1} when {2} is the maximum arg index.");
const FText FText::VeryLargeArgumentNumberErrorText = LOCTEXT("Error_TooManyArgDigitsInBlock", "ERR: Arg Block {0} has a very large argument number. This is unlikely so it is possibley some other parsing error.");
const FText FText::NoArgIndexError = LOCTEXT("Error_NoDigitsAtStartOfBlock", "ERR: Arg Block {0} does not start with the index number of the argument that it references. An argument block must start with a positive integer index to the argument its referencing. 0...max.");
const FText FText::NoClosingBraceError = LOCTEXT("Error_NoClosingBrace", "ERR: Arg Block {0} does not have a closing brace.");
const FText FText::OpenBraceInBlock = LOCTEXT("Error_OpenBraceInBlock", "ERR: Arg Block {0} has an open brace inside it. Braces are not allowed in argument blocks.");
const FText FText::UnEscapedCloseBraceOutsideOfArgumentBlock = LOCTEXT("Error_UnEscapedCloseBraceOutsideOfArgumentBlock", "ERR: There is an un-escaped }} after Arg Block {0}.");
const FText FText::SerializationFailureError = LOCTEXT("Error_SerializationFailure", "ERR: Transient text cannot be serialized \"{0}\".");

FText::FText()
	:
	SourceString( NULL ) ,
	DisplayString( new FString() ) ,
	Flags(0)
{
}

FText::FText( const FText& Source )
	:
	SourceString( Source.SourceString )
	, DisplayString( Source.DisplayString )
	, Namespace( Source.Namespace )
	, Key( Source.Key )
	, Flags( Source.Flags )
{

}

FText::FText( FString InSourceString )
	:
	SourceString( new FString( MoveTemp( InSourceString ) ) ) ,
	DisplayString( SourceString.ToSharedRef() ) ,
	Flags(0)
{
#if WITH_EDITOR
	Key = MakeShareable( new FString(FGuid::NewGuid().ToString()) );
#endif
}

FText::FText( FString InSourceString, FString InNamespace, FString InKey, int32 InFlags )
	: SourceString( new FString( MoveTemp( InSourceString ) ) )
	, DisplayString( FTextLocalizationManager::Get().GetString(InNamespace, InKey, SourceString.Get()) )
	, Namespace( new FString( MoveTemp( InNamespace) ) )
	, Key( new FString( MoveTemp( InKey ) ) )
	, Flags(InFlags)
{
}

FText FText::TrimPreceding( const FText& InText )
{
	FString TrimmedString = InText.ToString();
	TrimmedString.Trim();

	FText NewText = FText( MoveTemp( TrimmedString ) );

#if !WITH_EDITOR
	if( (InText.Flags & (1 << ETextFlag::CultureInvariant)) != 0 )
	{
		NewText.Flags = NewText.Flags | ETextFlag::Transient;
	}
	else
	{
		NewText.Flags = NewText.Flags | ETextFlag::CultureInvariant;
	}
#endif

	return NewText;
}

FText FText::TrimTrailing( const FText& InText )
{
	FString TrimmedString = InText.ToString();
	TrimmedString.TrimTrailing();

	FText NewText = FText( MoveTemp ( TrimmedString ) );

#if !WITH_EDITOR
	if( (InText.Flags & (1 << ETextFlag::CultureInvariant)) != 0 )
	{
		NewText.Flags = NewText.Flags & ETextFlag::Transient;
	}
	else
	{
		NewText.Flags = NewText.Flags & ETextFlag::CultureInvariant;
	}
#endif

	return NewText;
}

FText FText::TrimPrecedingAndTrailing( const FText& InText )
{
	FString TrimmedString = InText.ToString();
	TrimmedString.Trim().TrimTrailing();

	FText NewText = FText( MoveTemp( TrimmedString ) );

#if !WITH_EDITOR
	if( (InText.Flags & (1 << ETextFlag::CultureInvariant)) != 0 )
	{
		NewText.Flags = NewText.Flags | ETextFlag::Transient;
	}
	else
	{
		NewText.Flags = NewText.Flags | ETextFlag::CultureInvariant;
	}
#endif

	return NewText;
}

FText FText::Format(const FText& Fmt,const FText& v1)
{
	FFormatOrderedArguments Arguments;
	Arguments.Add(v1);
	return FText::Format(Fmt, Arguments);
}

FText FText::Format(const FText& Fmt,const FText& v1,const FText& v2)
{
	FFormatOrderedArguments Arguments;
	Arguments.Add(v1);
	Arguments.Add(v2);
	return FText::Format(Fmt, Arguments);
}

FText FText::Format(const FText& Fmt,const FText& v1,const FText& v2,const FText& v3)
{
	FFormatOrderedArguments Arguments;
	Arguments.Add(v1);
	Arguments.Add(v2);
	Arguments.Add(v3);
	return FText::Format(Fmt, Arguments);
}

FText FText::Format(const FText& Fmt,const FText& v1,const FText& v2,const FText& v3,const FText& v4)
{
	FFormatOrderedArguments Arguments;
	Arguments.Add(v1);
	Arguments.Add(v2);
	Arguments.Add(v3);
	Arguments.Add(v4);
	return FText::Format(Fmt, Arguments);
}

/**
* Generate an FText that represents the passed number in the passed culture
*/

#define DEF_ASNUMBER_CAST(T1, T2) FText FText::AsNumber(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture) { return FText::AsNumberTemplate<T1, T2>(Val, Options, TargetCulture); }
#define DEF_ASNUMBER(T) DEF_ASNUMBER_CAST(T, T)
DEF_ASNUMBER(float)
DEF_ASNUMBER(double)
DEF_ASNUMBER(int8)
DEF_ASNUMBER(int16)
DEF_ASNUMBER(int32)
DEF_ASNUMBER(int64)
DEF_ASNUMBER(uint8)
DEF_ASNUMBER(uint16)
DEF_ASNUMBER_CAST(uint32, int64)
DEF_ASNUMBER_CAST(uint64, int64)
#undef DEF_ASNUMBER

/**
* Generate an FText that represents the passed number as currency in the current culture
*/

#define DEF_ASCURRENCY_CAST(T1, T2) FText FText::AsCurrency(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture) { return FText::AsCurrencyTemplate<T1, T2>(Val, Options, TargetCulture); }
#define DEF_ASCURRENCY(T) DEF_ASCURRENCY_CAST(T, T)
DEF_ASCURRENCY(float)
	DEF_ASCURRENCY(double)
	DEF_ASCURRENCY(int8)
	DEF_ASCURRENCY(int16)
	DEF_ASCURRENCY(int32)
	DEF_ASCURRENCY(int64)
	DEF_ASCURRENCY(uint8)
	DEF_ASCURRENCY(uint16)
	DEF_ASCURRENCY_CAST(uint32, int64)
	DEF_ASCURRENCY_CAST(uint64, int64)
#undef DEF_ASCURRENCY

/**
* Generate an FText that represents the passed number as a percentage in the current culture
*/

#define DEF_ASPERCENT_CAST(T1, T2) FText FText::AsPercent(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture) { return FText::AsPercentTemplate<T1, T2>(Val, Options, TargetCulture); }
#define DEF_ASPERCENT(T) DEF_ASPERCENT_CAST(T, T)
DEF_ASPERCENT(double)
DEF_ASPERCENT(float)
#undef DEF_ASPERCENT

bool FText::FindText( const FString& Namespace, const FString& Key, FText& OutText )
{
	TSharedPtr< FString > FoundString = FTextLocalizationManager::Get().FindString( Namespace, Key );

	if ( FoundString.IsValid() )
	{
		OutText = FText( FString(), Namespace, Key );
	}

	return FoundString.IsValid();
}

CORE_API FArchive& operator<<( FArchive& Ar, FText& Value )
{
	Ar.ThisRequiresLocalizationGather();

	//When duplicating, the CDO is used as the template, then values for the instance are assigned.
	//If we don't duplicate the string, the CDO and the instance are both pointing at the same thing.
	//This would result in all subsequently duplicated objects stamping over formerly duplicated ones.
	if(Ar.IsLoading())
	{
		Value.SourceString = MakeShareable( new FString() );
		Ar << *(Value.SourceString);
		if( Value.SourceString->IsEmpty() )
		{
			Value.SourceString.Reset();
		}
	}
	else if( Ar.IsSaving() )
	{
		if( Value.SourceString.IsValid() )
		{
			Ar << *(Value.SourceString);
		}
		else
		{
			FString EmptySourceString;
			Ar << EmptySourceString;
		}
	}

	if( Ar.UE4Ver() >= VER_UE4_ADDED_NAMESPACE_AND_KEY_DATA_TO_FTEXT )
	{
		if(Ar.IsLoading())
		{
			Value.Namespace = MakeShareable( new FString() );
			Ar << *Value.Namespace;
			if( Value.Namespace->IsEmpty() )
			{
				Value.Namespace.Reset();
			}

			Value.Key = MakeShareable( new FString() );
			Ar << *Value.Key;
			if( Value.Key->IsEmpty() )
			{
				Value.Key.Reset();
			}
		}
		else if(Ar.IsSaving())
		{
			if( Value.Namespace.IsValid() )
			{
				Ar << *(Value.Namespace);
			}
			else
			{
				FString Empty;
				Ar << Empty;
			}

			if( Value.Key.IsValid() )
			{
				Ar << *(Value.Key);
			}
			else
			{
				FString Empty;
				Ar << Empty;
			}
		}

		if(Ar.IsPersistent())
		{
			Value.Flags &= ~(ETextFlag::ConvertedProperty); // Remove conversion flag before saving.
		}
		Ar << Value.Flags;
	}

	if(Ar.IsLoading())
	{
		const FString& Namespace = Value.Namespace.Get() ? *(Value.Namespace) : TEXT("");
		const FString& Key = Value.Key.Get() ? *(Value.Key) : TEXT("");
		Value.DisplayString = FTextLocalizationManager::Get().GetString(Namespace, Key, Value.SourceString.Get());
	}

	// Validate
	//if( Ar.IsLoading() && Ar.IsPersistent() && !Value.Key.IsValid() )
	//{
	//	UE_LOG( LogText, Error, TEXT("Loaded an FText from a persistent archive but lacking a key (Namespace:%s, Source:%s)."), Value.Namespace.Get() ? **Value.Namespace : TEXT(""), Value.SourceString.Get() ? **Value.SourceString : TEXT("") );
	//}

	return Ar;
}

#if WITH_EDITOR
FText FText::ChangeKey( FString Namespace, FString Key, const FText& Text )
{
	return FText( *Text.SourceString, MoveTemp( Namespace ), MoveTemp( Key ) );
}
#endif

FText FText::CreateNumericalText(FString InSourceString)
{
	FText NewText = FText( MoveTemp( InSourceString ) );
#if !WITH_EDITOR
	NewText.Flags |= ETextFlag::Transient;
#endif
	return NewText;
}

FText FText::CreateChronologicalText(FString InSourceString)
{
	FText NewText = FText( MoveTemp( InSourceString ) );
#if !WITH_EDITOR
	NewText.Flags |= ETextFlag::Transient;
#endif
	return NewText;
}

FText FText::FromName( const FName& Val) 
{
	FText NewText = FText( Val.ToString() );

#if !WITH_EDITOR
	NewText.Flags |= ETextFlag::CultureInvariant;
#endif

	return NewText; 
}

FText FText::FromString( FString String )
{
	FText NewText = FText( MoveTemp(String) );

#if !WITH_EDITOR
	NewText.Flags |= ETextFlag::CultureInvariant;
#endif

	return NewText;
}


const FString& FText::ToString() const
{
	return DisplayString.Get();
}



#undef LOCTEXT_NAMESPACE
