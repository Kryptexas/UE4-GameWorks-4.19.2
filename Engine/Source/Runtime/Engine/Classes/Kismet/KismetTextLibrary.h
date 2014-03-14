// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "KismetTextLibrary.generated.h"

#if !CPP
/** This code is only used for meta-data extraction and the main declaration is with FText, be sure to update there as well */

/** Provides rounding modes for converting numbers into localized text */
UENUM(BlueprintType)
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
};
#endif

UCLASS(MinimalAPI)
class UKismetTextLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** Converts localizable text to the string */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "ToString (text)", CompactNodeTitle = "->"), Category="Utilities|String")
	static FString Conv_TextToString(const FText& InText);

	/** Converts string to localizable text */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "ToText (string)", CompactNodeTitle = "->"), Category="Utilities|Text")
	static FText Conv_StringToText(const FString& InString);

	/** Converts string to localizable text */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "ToText (name)", CompactNodeTitle = "->"), Category="Utilities|Text")
	static FText Conv_NameToText(FName InName);

	/* Returns true if text is empty. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static bool TextIsEmpty(const FText& InText);

	/* Returns true if text is transient. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static bool TextIsTransient(const FText& InText);

	/* Returns true if text is culture invariant. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static bool TextIsCultureInvariant(const FText& InText);

	/* Removes whitespace characters from the front of the text. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static FText TextTrimPreceding(const FText& InText);

	/* Removes trailing whitespace characters. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static FText TextTrimTrailing(const FText& InText);

	/* Removes whitespace characters from the front and end of the text. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static FText TextTrimPrecedingAndTrailing(const FText& InText);

	/* Returns an empty piece of text. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static FText GetEmptyText();

	/* Attempts to find existing Text using the representation found in the loc tables for the specified namespace and key. */
	UFUNCTION(BlueprintPure, Category="Utilities|Text")
	static bool FindTextInLocalizationTable(const FString& Namespace, const FString& Key, FText& OutText);

	/* Returns true if A and B are linguistically equal (A == B). */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "Equal (text)", CompactNodeTitle = "=="), Category="Utilities|Text")
	static bool EqualEqual_TextText(const FText& A, const FText& B);

	/* Returns true if A and B are linguistically equal (A == B), ignoring case. */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "Equal, Case Insensitive (text)", CompactNodeTitle = "=="), Category="Utilities|Text")
	static bool EqualEqual_IgnoreCase_TextText(const FText& A, const FText& B);
				
	/* Returns true if A and B are linguistically not equal (A != B). */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "NotEqual (text)", CompactNodeTitle = "!="), Category="Utilities|Text")
	static bool NotEqual_TextText(const FText& A, const FText& B);

	/* Returns true if A and B are linguistically not equal (A != B), ignoring case. */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "NotEqual, Case Insensitive (text)", CompactNodeTitle = "!="), Category="Utilities|Text")
	static bool NotEqual_IgnoreCase_TextText(const FText& A, const FText& B);

	// Default values are duplicated from FNumberFormattingOptions and should be replicated in all functions and in the struct when changed!
	/* Converts a passed in integer to a text based on formatting options */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "ToText (int)", AdvancedDisplay = "1"), Category="Utilities|Text")
	static FText Conv_IntToText(int32 Value, TEnumAsByte<ERoundingMode> RoundingMode, bool bUseGrouping = true, int32 MinimumIntegralDigits = 1, int32 MaximumIntegralDigits = 324, int32 MinimumFractionalDigits = 0, int32 MaximumFractionalDigits = 3);

	// Default values are duplicated from FNumberFormattingOptions and should be replicated in all functions and in the struct when changed!
	/* Converts a passed in float to a text based on formatting options */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "ToText (float)", AdvancedDisplay = "1"), Category="Utilities|Text")
	static FText Conv_FloatToText(float Value, TEnumAsByte<ERoundingMode> RoundingMode, bool bUseGrouping = true, int32 MinimumIntegralDigits = 1, int32 MaximumIntegralDigits = 324, int32 MinimumFractionalDigits = 0, int32 MaximumFractionalDigits = 3);

	// Default values are duplicated from FNumberFormattingOptions and should be replicated in all functions and in the struct when changed!
	/* Converts a passed in integer to a text formatted as a currency */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "AsCurrency (int)", AdvancedDisplay = "1"), Category="Utilities|Text")
	static FText AsCurrency_Integer(int32 Value, TEnumAsByte<ERoundingMode> RoundingMode, bool bUseGrouping = true, int32 MinimumIntegralDigits = 1, int32 MaximumIntegralDigits = 324, int32 MinimumFractionalDigits = 0, int32 MaximumFractionalDigits = 3);

	// Default values are duplicated from FNumberFormattingOptions and should be replicated in all functions and in the struct when changed!
	/* Converts a passed in float to a text formatted as a currency */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "AsCurrency (float)", AdvancedDisplay = "1"), Category="Utilities|Text")
	static FText AsCurrency_Float(float Value, TEnumAsByte<ERoundingMode> RoundingMode, bool bUseGrouping = true, int32 MinimumIntegralDigits = 1, int32 MaximumIntegralDigits = 324, int32 MinimumFractionalDigits = 0, int32 MaximumFractionalDigits = 3);

	// Default values are duplicated from FNumberFormattingOptions and should be replicated in all functions and in the struct when changed!
	/* Converts a passed in float to a text, formatted as a percent */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "AsPercent", AdvancedDisplay = "1"), Category="Utilities|Text")
	static FText AsPercent_Float(float Value, TEnumAsByte<ERoundingMode> RoundingMode, bool bUseGrouping = true, int32 MinimumIntegralDigits = 1, int32 MaximumIntegralDigits = 324, int32 MinimumFractionalDigits = 0, int32 MaximumFractionalDigits = 3);
};
