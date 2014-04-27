// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FWidgetLayoutBlock : public ILayoutBlock
{
public:

	static TSharedRef< FWidgetLayoutBlock > Create( const TSharedRef< IRun >& InRun, const TSharedRef< SWidget >& InWidget, const FTextRange& InRange, FVector2D InSize, const TSharedPtr< IRunHighlighter >& InHighlighter )
	{
		return MakeShareable( new FWidgetLayoutBlock( InRun, InWidget, InRange, InSize, InHighlighter ) );
	}

	virtual ~FWidgetLayoutBlock() {}

	virtual TSharedRef< IRun > GetRun() const OVERRIDE { return Run; }

	virtual FTextRange GetTextRange() const OVERRIDE { return Range; }

	virtual FVector2D GetSize() const OVERRIDE { return Size; }

	virtual TSharedPtr< IRunHighlighter > GetHighlighter() const OVERRIDE { return Highlighter; }

	TSharedRef< SWidget > GetWidget() const { return Widget; }

	virtual void SetLocationOffset( const FVector2D& InLocationOffset ) OVERRIDE { LocationOffset = InLocationOffset; }
	virtual FVector2D GetLocationOffset() const OVERRIDE { return LocationOffset; }

private:

	static TSharedRef< FWidgetLayoutBlock > Create( const FWidgetLayoutBlock& Block )
	{
		return MakeShareable( new FWidgetLayoutBlock( Block ) );
	}

	FWidgetLayoutBlock( const TSharedRef< IRun >& InRun, const TSharedRef< SWidget >& InWidget, const FTextRange& InRange, FVector2D InSize, const TSharedPtr< IRunHighlighter >& InHighlighter )
		: Run( InRun )
		, Widget( InWidget )
		, Range( InRange )
		, Size( InSize )
		, LocationOffset( ForceInitToZero )
		, Highlighter( InHighlighter )
	{

	}

	FWidgetLayoutBlock( const FWidgetLayoutBlock& Block )
		: Run( Block.Run )
		, Widget( Block.Widget )
		, Range( Block.Range )
		, Size( Block.Size )
		, LocationOffset( ForceInitToZero )
		, Highlighter( Block.Highlighter )
	{

	}

private:

	TSharedRef< IRun > Run;
	TSharedRef< SWidget > Widget;

	FTextRange Range;
	FVector2D Size;
	FVector2D LocationOffset;
	TSharedPtr< IRunHighlighter > Highlighter;
};