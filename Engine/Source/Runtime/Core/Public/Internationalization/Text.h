// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SharedPointer.h"

#define ENABLE_TEXT_ERROR_CHECKING_RESULTS (UE_BUILD_DEBUG | UE_BUILD_DEVELOPMENT | UE_BUILD_TEST )

//DECLARE_LOG_CATEGORY_EXTERN(LogText, Log, All);

//DECLARE_CYCLE_STAT_EXTERN( TEXT("Format Text"), STAT_TextFormat, STATGROUP_Text, );

namespace ETextFlag
{
	enum Type
	{
		Transient = (1<<0),
		CultureInvariant = (1<<1),
		ConvertedProperty = (1<<2)
	};
}

namespace ETextComparisonLevel
{
	enum Type
	{
		Default,	// Locale-specific Default
		Primary,	// Base
		Secondary,	// Accent
		Tertiary,	// Case
		Quaternary,	// Punctuation
		Quinary		// Identical
	};
}

namespace EDateTimeStyle
{
	enum Type
	{
		Default,
		Short,
		Medium,
		Long,
		Full
		// Add new enum types at the end only! They are serialized by index.
	};
}

namespace EFormatArgumentType
{
	enum Type
	{
		Int,
		UInt,
		Float,
		Double,
		Text
		// Add new enum types at the end only! They are serialized by index.
	};
}

struct FFormatArgumentValue;

typedef TMap<FString, FFormatArgumentValue> FFormatNamedArguments;
typedef TArray<FFormatArgumentValue> FFormatOrderedArguments;

/** Redeclared in KismetTextLibrary for meta-data extraction purposes, be sure to update there as well */
enum ERoundingMode
{
	/** Rounds to the nearest place, equidistant ties go to the value which is closest to an even value: 1.5 becomes 2, 0.5 becomes 0 */
	HalfToEven,
	/** Rounds to nearest place, equidistant ties go to the value which is further from zero: -0.5 becomes -1.0, 0.5 becomes 1.0 */
	HalfFromZero,
	/** Rounds to nearest place, equidistant ties go to the value which is closer to zero: -0.5 becomes 0, 0.5 becomes 0. */
	HalfToZero,
	/** Rounds to the value which is further from zero, "larger" in absolute value: 0.1 becomes 1, -0.1 becomes -1 */
	FromZero,
	/** Rounds to the value which is closer to zero, "smaller" in absolute value: 0.1 becomes 0, -0.1 becomes 0 */
	ToZero,
	/** Rounds to the value which is more negative: 0.1 becomes 0, -0.1 becomes -1 */
	ToNegativeInfinity,
	/** Rounds to the value which is more positive: 0.1 becomes 1, -0.1 becomes 0 */
	ToPositiveInfinity,


	// Add new enum types at the end only! They are serialized by index.
};

struct CORE_API FNumberFormattingOptions
{
	FNumberFormattingOptions();

	bool UseGrouping;
	ERoundingMode RoundingMode;
	int32 MinimumIntegralDigits;
	int32 MaximumIntegralDigits;
	int32 MinimumFractionalDigits;
	int32 MaximumFractionalDigits;

	friend FArchive& operator<<(FArchive& Ar, FNumberFormattingOptions& Value);
};

class FCulture;

class CORE_API FText
{
public:

	static const FText& GetEmpty()
	{
		static const FText StaticEmptyText = FText();
		return StaticEmptyText;
	}

public:

	FText();
	FText( const FText& Source );

	/**
	 * Generate an FText that represents the passed number in the current culture
	 */
	static FText AsNumber(float Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(double Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(int8 Val,		const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(int16 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(int32 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(int64 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(uint8 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(uint16 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(uint32 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(uint64 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsNumber(long Val,		const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);

	/**
	 * Generate an FText that represents the passed number as currency in the current culture
	 */
	static FText AsCurrency(float Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(double Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(int8 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(int16 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(int32 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(int64 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(uint8 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(uint16 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(uint32 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(uint64 Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsCurrency(long Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);

	/**
	 * Generate an FText that represents the passed number as a percentage in the current culture
	 */
	static FText AsPercent(float Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsPercent(double Val,	const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);

	/**
	 * Generate an FText that represents the passed number as a date and/or time in the current culture
	 */
	static FText AsDate(const FDateTime::FDate& Date, const EDateTimeStyle::Type DateStyle = EDateTimeStyle::Default, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsTime(const FDateTime::FTime& Time, const EDateTimeStyle::Type TimeStyle = EDateTimeStyle::Default, const FString& TimeZone = TEXT(""), const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsTime(const FTimespan& Time, const TSharedPtr<FCulture>& TargetCulture = NULL);
	static FText AsDateTime(const FDateTime& DateTime, const EDateTimeStyle::Type DateStyle = EDateTimeStyle::Default, const EDateTimeStyle::Type TimeStyle = EDateTimeStyle::Default, const FString& TimeZone = TEXT(""), const TSharedPtr<FCulture>& TargetCulture = NULL);

	/**
	 * Generate an FText that represents the passed number as a memory size in the current culture
	 */
	static FText AsMemory(SIZE_T NumBytes, const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);

	/**
	 * Attempts to find an existing FText using the representation found in the loc tables for the specified namespace and key
	 * @return true if OutText was properly set; otherwise false and OutText will be untouched
	 */
	static bool FindText( const FString& Namespace, const FString& Key, FText& OutText, const FString* const SourceString = nullptr );

	/**
	 * Generate an FText representing the pass name
	 */
	static FText FromName( const FName& Val);
	
	/**
	 * Generate an FText representing the passed in string
	 */
	static FText FromString( FString String );

	const FString& ToString() const;

	bool IsNumeric() const
	{
		return DisplayString.Get().IsNumeric();
	}

	int32 CompareTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel = ETextComparisonLevel::Default ) const;
	int32 CompareToCaseIgnored( const FText& Other ) const;

	bool EqualTo( const FText& Other, const ETextComparisonLevel::Type ComparisonLevel = ETextComparisonLevel::Default ) const;
	bool EqualToCaseIgnored( const FText& Other ) const;

	class CORE_API FSortPredicate
	{
	public:
		FSortPredicate(const ETextComparisonLevel::Type ComparisonLevel = ETextComparisonLevel::Default);

		bool operator()(const FText& A, const FText& B) const;

	private:
#if UE_ENABLE_ICU
		class FSortPredicateImplementation;
		TSharedRef<FSortPredicateImplementation> Implementation;
#endif
	};

	bool IsEmpty() const
	{
		return DisplayString.Get().IsEmpty();
	}

	/**
	 * Removes whitespace characters from the front of the string.
	 */
	static FText TrimPreceding( const FText& );

	/**
	 * Removes trailing whitespace characters
	 */
	static FText TrimTrailing( const FText& );

	/**
	 * Does both of the above without needing to create an additional FText in the interim.
	 */
	static FText TrimPrecedingAndTrailing( const FText& );

	static void GetFormatPatternParameters(const FText& Pattern, TArray<FString>& ParameterNames);

	static FText Format(const FText& Pattern, const FFormatNamedArguments& Arguments);
	static FText Format(const FText& Pattern, const FFormatOrderedArguments& Arguments);
	static FText Format(const FText& Pattern, const TArray< struct FFormatArgumentData > InArguments);

	static FText Format(const FText& Fmt,const FText& v1);
	static FText Format(const FText& Fmt,const FText& v1,const FText& v2);
	static FText Format(const FText& Fmt,const FText& v1,const FText& v2,const FText& v3);
	static FText Format(const FText& Fmt,const FText& v1,const FText& v2,const FText& v3,const FText& v4);

	static void SetEnableErrorCheckingResults(bool bEnable){bEnableErrorCheckingResults=bEnable;}
	static bool GetEnableErrorCheckingResults(){return bEnableErrorCheckingResults;}

	static void SetSuppressWarnings(bool bSuppress){ bSuppressWarnings = bSuppress; }
	static bool GetSuppressWarnings(){ return bSuppressWarnings; }

	bool IsTransient() const { return (Flags & ETextFlag::Transient) != 0; }
	bool IsCultureInvariant() const { return (Flags & ETextFlag::CultureInvariant) != 0; }

private:

	explicit FText( FString InSourceString );

	explicit FText( FString InSourceString, FString InNamespace, FString InKey, int32 InFlags=0 );

	friend CORE_API FArchive& operator<<( FArchive& Ar, FText& Value );

#if WITH_EDITOR
	/**
	 * Constructs a new FText with the SourceString of the specified text but with the specified namespace and key
	 */
	static FText ChangeKey( FString Namespace, FString Key, const FText& Text );
#endif

	/**
	 * Generate an FText for a string formatted numerically.
	 */
	static FText CreateNumericalText(FString InSourceString);

	/**
	 * Generate an FText for a string formatted from a date/time.
	 */
	static FText CreateChronologicalText(FString InSourceString);

	/** Returns the source string of the FText */
	TSharedPtr<FString> GetSourceString() const;

	/** Rebuilds the FText under the current culture if needed */
	void Rebuild() const;

	static FText FormatInternal(const FText& Pattern, const FFormatNamedArguments& Arguments, bool bInRebuildText);
	static FText FormatInternal(const FText& Pattern, const FFormatOrderedArguments& Arguments, bool bInRebuildText);
	static FText FormatInternal(const FText& Pattern, const TArray< struct FFormatArgumentData > InArguments, bool bInRebuildText);

private:
	template<typename T1, typename T2>
	static FText AsNumberTemplate(T1 Val, const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	template<typename T1, typename T2>
	static FText AsCurrencyTemplate(T1 Val, const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);
	template<typename T1, typename T2>
	static FText AsPercentTemplate(T1 Val, const FNumberFormattingOptions* const Options = NULL, const TSharedPtr<FCulture>& TargetCulture = NULL);

private:
	/** The visible display string for this FText */
	TSharedRef<FString> DisplayString;

	/** The FText's history, to allow it to rebuild under a new culture */
	TSharedPtr<class FTextHistory> History;

	/** Flags with various information on what sort of FText this is */
	int32 Flags;

	/** Revision index of this FText, rebuilds when it is out of sync with the FTextLocalizationManager */
	int32 Revision;

	static bool bEnableErrorCheckingResults;
	static bool bSuppressWarnings;

public:
	//Some error text formats
	static const FText UnusedArgumentsError;
	static const FText CommentStartError;
	static const FText TooFewArgsErrorFormat;
	static const FText VeryLargeArgumentNumberErrorText;
	static const FText NoArgIndexError;
	static const FText NoClosingBraceError;
	static const FText OpenBraceInBlock;
	static const FText UnEscapedCloseBraceOutsideOfArgumentBlock;
	static const FText SerializationFailureError;

	friend class FTextInspector;
	friend class FInternationalization;
	friend class UStruct;
	friend class UGatherTextFromAssetsCommandlet;
	friend class UEdGraphSchema;
	friend class UEdGraphSchema_K2;
	friend class FTextHistory_NamedFormat;
	friend class FTextHistory_ArgumentDataFormat;
	friend class FTextHistory_OrderedFormat;

#if !UE_ENABLE_ICU
	friend class FLegacyTextHelper;
#endif
};

class CORE_API FTextInspector
{
private:
	FTextInspector() {}
	~FTextInspector() {}

public:
	static const FString* GetNamespace(const FText& Text);
	static const FString* GetKey(const FText& Text);
	static const FString* GetSourceString(const FText& Text);
	static const FString& GetDisplayString(const FText& Text);
	static int32 GetFlags(const FText& Text);
};

struct CORE_API FFormatArgumentValue
{
	EFormatArgumentType::Type Type;
	union
	{
		int64 IntValue;
		uint64 UIntValue;
		float FloatValue;
		double DoubleValue;
		FText* TextValue;
	};

	FFormatArgumentValue();

	FFormatArgumentValue(const int Value);
	FFormatArgumentValue(const unsigned int Value);
	FFormatArgumentValue(const int64 Value);
	FFormatArgumentValue(const uint64 Value);
	FFormatArgumentValue(const float Value);
	FFormatArgumentValue(const double Value);
	FFormatArgumentValue(const FText& Value);
	FFormatArgumentValue(const FFormatArgumentValue& Source);
	~FFormatArgumentValue();

	friend FArchive& operator<<( FArchive& Ar, FFormatArgumentValue& Value );
};

struct FFormatArgumentData
{
	FText ArgumentName;
	FText ArgumentValue;

	friend FArchive& operator<<( FArchive& Ar, FFormatArgumentData& Value );
};

class CORE_API FTextBuilder
{
public:

	FTextBuilder()
		: IndentCount(0)
	{

	}

	void Indent()
	{
		++IndentCount;
	}

	void Unindent()
	{
		--IndentCount;
	}

	void AppendLine()
	{
		if (!Report.IsEmpty())
		{
			Report += LINE_TERMINATOR;
		}

		for (int32 Index = 0; Index < IndentCount; Index++)
		{
			Report += TEXT("    ");
		}
	}

	void AppendLine(const FText& Text)
	{
		AppendLine();
		Report += Text.ToString();
	}

	void AppendLine(const FString& String)
	{
		AppendLine();
		Report += String;
	}

	void AppendLine(const FName& Name)
	{
		AppendLine();
		Report += Name.ToString();
	}

	void AppendLineFormat(const FText& Pattern, const FFormatNamedArguments& Arguments)
	{
		AppendLine(FText::Format(Pattern, Arguments));
	}

	void AppendLineFormat(const FText& Pattern, const FFormatOrderedArguments& Arguments)
	{
		AppendLine(FText::Format(Pattern, Arguments));
	}

	void AppendLineFormat(const FText& Pattern, const TArray< struct FFormatArgumentData > InArguments)
	{
		AppendLine(FText::Format(Pattern, InArguments));
	}

	void AppendLineFormat(const FText& Fmt, const FText& v1)
	{
		AppendLine(FText::Format(Fmt, v1));
	}

	void AppendLineFormat(const FText& Fmt, const FText& v1, const FText& v2)
	{
		AppendLine(FText::Format(Fmt, v1, v2));
	}

	void AppendLineFormat(const FText& Fmt, const FText& v1, const FText& v2, const FText& v3)
	{
		AppendLine(FText::Format(Fmt, v1, v2, v3));
	}

	void AppendLineFormat(const FText& Fmt, const FText& v1, const FText& v2, const FText& v3, const FText& v4)
	{
		AppendLine(FText::Format(Fmt, v1, v2, v3, v4));
	}

	void Clear()
	{
		Report.Empty();
	}

	FText ToText() const
	{
		return FText::FromString(Report);
	}

private:
	FString Report;
	int32 IndentCount;
};

Expose_TNameOf(FText)
