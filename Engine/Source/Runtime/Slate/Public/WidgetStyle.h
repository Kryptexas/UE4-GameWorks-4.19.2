// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** Contains info about those aspects of widget appearance that should be propagated hierarchically. */
class SLATE_API FWidgetStyle
{

static const float SubdueAmount;

public:
	/** Construct a default widget style */
	FWidgetStyle()
	: ColorAndOpacityTint(FLinearColor::White)
	, ForegroundColor(FLinearColor::White)
	, SubduedForeground( FLinearColor::White*SubdueAmount )
	{
	}


	/** Blend the current tint color with the one that is passed in */
	FWidgetStyle& BlendColorAndOpacityTint( const FLinearColor& InTint )
	{
		ColorAndOpacityTint *= InTint;
		return *this;
	}

	/** Set the current foreground color */
	FWidgetStyle& SetForegroundColor( const FLinearColor& InForeground )
	{
		ForegroundColor = InForeground;

		SubduedForeground = InForeground;
		SubduedForeground.A *= SubdueAmount;

		return *this;
	}

	/** Set the current foreground color. If the attribute passed in is not set, the foreground will remain unchanged. */
	FWidgetStyle& SetForegroundColor( const TAttribute< struct FSlateColor >& InForeground );

	public:
	/** @return the current tint color */
	const FLinearColor& GetColorAndOpacityTint() const
	{
		return ColorAndOpacityTint;
	}

	/** @return the current foreground color */
	const FLinearColor& GetForegroundColor() const
	{
		return ForegroundColor;
	}

	/** @return the current foreground color */
	const FLinearColor& GetSubduedForegroundColor() const
	{
		return SubduedForeground;
	}

	private:
	FLinearColor ColorAndOpacityTint;
	FLinearColor ForegroundColor;
	FLinearColor SubduedForeground;
};



