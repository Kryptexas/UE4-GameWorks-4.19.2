// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

struct SLATE_API FTextHighlight
{
	FTextHighlight( int32 InLineIndex, const FTextRange& InRange, const TSharedRef< class IRunHighlighter >& InHighlighter )
		: LineIndex( InLineIndex )
		, Range( InRange )
		, Highlighter( InHighlighter )
	{

	}

	int32 LineIndex;
	FTextRange Range;
	TSharedRef< IRunHighlighter > Highlighter;
};