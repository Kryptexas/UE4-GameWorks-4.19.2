// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API IRun
{
public:

	virtual FTextRange GetTextRange() const = 0;
	virtual void SetTextRange( const FTextRange& Value ) = 0;

	virtual int16 GetBaseLine( float Scale ) const = 0;
	virtual int16 GetMaxHeight( float Scale ) const = 0;

	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale) const = 0;
	virtual int8 GetKerning(int32 CurrentIndex, float Scale) const = 0;

	virtual TSharedRef< class ILayoutBlock > CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< class IRunHighlighter >& Highlighter ) = 0;

	virtual int32 GetTextIndexAt( const TSharedRef< ILayoutBlock >& Block, const FVector2D& Location, float Scale ) const = 0;
	
	virtual FVector2D GetLocationAt(const TSharedRef< ILayoutBlock >& Block, int32 Offset, float Scale) const = 0;

	virtual void BeginLayout() = 0;
	virtual void EndLayout() = 0;

	virtual void Move(const TSharedRef<FString>& NewText, const FTextRange& NewRange) = 0;
	virtual TSharedRef<IRun> Clone() const = 0;

	virtual void AppendText(FString& Text) const = 0;
	virtual void AppendText(FString& Text, const FTextRange& Range) const = 0;

};
