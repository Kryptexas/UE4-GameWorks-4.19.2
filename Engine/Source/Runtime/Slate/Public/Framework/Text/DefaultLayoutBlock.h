// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API FDefaultLayoutBlock : public ILayoutBlock
{
public:

	static TSharedRef< FDefaultLayoutBlock > Create( const TSharedRef< IRun >& InRun, const FTextRange& InRange, FVector2D InSize, const TSharedPtr< IRunHighlighter >& InHighlighter )
	{
		return MakeShareable( new FDefaultLayoutBlock( InRun, InRange, InSize, InHighlighter ) );
	}

	virtual ~FDefaultLayoutBlock() {}

	virtual TSharedRef< IRun > GetRun() const OVERRIDE { return Run; }
	virtual FTextRange GetTextRange() const OVERRIDE { return Range; }
	virtual FVector2D GetSize() const OVERRIDE { return Size; }
	virtual TSharedPtr< IRunHighlighter > GetHighlighter() const OVERRIDE { return Highlighter; }

	virtual void SetLocationOffset( const FVector2D& InLocationOffset ) OVERRIDE { LocationOffset = InLocationOffset; }
	virtual FVector2D GetLocationOffset() const OVERRIDE { return LocationOffset; }

private:

	static TSharedRef< FDefaultLayoutBlock > Create( const FDefaultLayoutBlock& Block )
	{
		return MakeShareable( new FDefaultLayoutBlock( Block ) );
	}

	FDefaultLayoutBlock( const TSharedRef< IRun >& InRun, const FTextRange& InRange, FVector2D InSize, const TSharedPtr< IRunHighlighter >& InHighlighter )
		: Run( InRun )
		, Range( InRange )
		, Size( InSize )
		, LocationOffset( ForceInitToZero )
		, Highlighter( InHighlighter )
	{

	}

	FDefaultLayoutBlock( const FDefaultLayoutBlock& Block )
		: Run( Block.Run )
		, Range( Block.Range )
		, Size( Block.Size )
		, LocationOffset( ForceInitToZero )
		, Highlighter( Block.Highlighter )
	{

	}

private:

	TSharedRef< IRun > Run;

	FTextRange Range;
	FVector2D Size;
	FVector2D LocationOffset;
	TSharedPtr< IRunHighlighter > Highlighter;
};