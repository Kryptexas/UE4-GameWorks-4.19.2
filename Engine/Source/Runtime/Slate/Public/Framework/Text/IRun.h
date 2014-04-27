// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SLATE_API IRun
{
public:

	virtual FTextRange GetTextRange() const = 0;
	virtual int16 GetBaseLine( float Scale ) const = 0;
	virtual int16 GetMaxHeight( float Scale ) const = 0;

	virtual FVector2D Measure( int32 StartIndex, int32 EndIndex, float Scale) const = 0;
	virtual uint8 GetKerning( int32 CurrentIndex, float Scale ) const = 0;

	virtual TSharedRef< class ILayoutBlock > CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< class IRunHighlighter >& Highlighter ) = 0;

	virtual void BeginLayout() = 0;
	virtual void EndLayout() = 0;
};
