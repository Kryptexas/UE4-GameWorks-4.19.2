// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateColor.h: Declares the FSlateColor structure.
=============================================================================*/

#pragma once

#include "SlateColor.generated.h"


/**
 * Enumerates types of color values that can be held by Slate color.
 *
 * Should we use the specified color? If not, then which color from the style should we use.
 */
UENUM()
namespace ESlateColorStylingMode
{
	enum Type
	{
		/** Color value is stored in this Slate color. */
		UseColor_Specified,

		/** Color value is stored in the linked color. */
		UseColor_Specified_Link,

		/** Use the widget's foreground color. */
		UseColor_Foreground,

		/** Use the widget's subdued color. */
		UseColor_Foreground_Subdued
	};
}


/**
 * A Slate color can be a directly specified value, or the color can be pulled from a WidgetStyle.
 */
USTRUCT()
struct FSlateColor
{
	GENERATED_USTRUCT_BODY()

public:

	/**
	 * Default constructor.
	 *
	 * Uninitialized Slate colors are Fuchsia by default.
	 */
	FSlateColor()
		: SpecifiedColor(FLinearColor(1.0f, 0.0f, 1.0f))
		, ColorUseRule(ESlateColorStylingMode::UseColor_Specified)
	{ }

	/**
	 * Creates a new Slate color with the specified values.
	 *
	 * @param InColor The color value to assign.
	 */
	FSlateColor( const FLinearColor& InColor )
		: SpecifiedColor(InColor)
		, ColorUseRule(ESlateColorStylingMode::UseColor_Specified)
	{ }

	/**
	 * Creates a new Slate color that is linked to the given values.
	 *
	 * @param InColor The color value to assign.
	 */
	FSlateColor( const TSharedRef< FLinearColor >& InColor )
		: SpecifiedColor(FLinearColor(1.0f, 0.0f, 1.0f))
		, ColorUseRule(ESlateColorStylingMode::UseColor_Specified_Link)
		, LinkedSpecifiedColor(InColor)
	{ }

public:

	/**
	 * Gets the color value represented by this Slate color.
	 *
	 * @param InWidgetStyle The widget style to use when this color represents a foreground or subdued color.
	 *
	 * @return The color value.
	 *
	 * @see GetSpecifiedColor
	 */
	const FLinearColor& GetColor( const FWidgetStyle& InWidgetStyle ) const
	{
		switch(ColorUseRule)
		{
		default:
		case ESlateColorStylingMode::UseColor_Foreground:
			return InWidgetStyle.GetForegroundColor();
			break;

		case ESlateColorStylingMode::UseColor_Specified:
			return SpecifiedColor;
			break;

		case ESlateColorStylingMode::UseColor_Specified_Link:
			return *LinkedSpecifiedColor;
			break;

		case ESlateColorStylingMode::UseColor_Foreground_Subdued:
			return InWidgetStyle.GetSubduedForegroundColor();
			break;
		};
	}

	/**
	 * Gets the specified color value.
	 *
	 * @return The specified color value.
	 *
	 * @see GetColor
	 * @see IsColorSpecified
	 */
	FLinearColor GetSpecifiedColor( ) const
	{
		if (LinkedSpecifiedColor.IsValid())
		{
			return *LinkedSpecifiedColor;
		}

		return SpecifiedColor;
	}

	/**
	 * Checks whether the values for this color have been specified.
	 *
	 * @return true if specified, false otherwise.
	 *
	 * @see GetSpecifiedColor
	 */
	bool IsColorSpecified( ) const
	{
		return (ColorUseRule == ESlateColorStylingMode::UseColor_Specified) || (ColorUseRule == ESlateColorStylingMode::UseColor_Specified_Link);
	}

public:

	/**
	 * @returns an FSlateColor that is the widget's foreground. */
	static FSlateColor UseForeground( )
	{
		return FSlateColor( ESlateColorStylingMode::UseColor_Foreground );
	}

	/** @returns an FSlateColor that is the subdued version of the widget's foreground. */
	static FSlateColor UseSubduedForeground( )
	{
		return FSlateColor( ESlateColorStylingMode::UseColor_Foreground_Subdued );
	}

protected:
	
	// Private constructor to prevent construction of invalid FSlateColors
	FSlateColor( ESlateColorStylingMode::Type InColorUseRule )
		: ColorUseRule(InColorUseRule)
		, LinkedSpecifiedColor()
	{ }

protected:

	// The current specified color; only meaningful when ColorToUse == UseColor_Specified.
	UPROPERTY(EditAnywhere, Category=Color)
	FLinearColor SpecifiedColor;

	// The rule for which color to pick.
	UPROPERTY(EditAnywhere, Category=Color)
	TEnumAsByte<ESlateColorStylingMode::Type> ColorUseRule;

private:

	// A shared pointer to the linked color value, if any.
	TSharedPtr<FLinearColor> LinkedSpecifiedColor;
};
