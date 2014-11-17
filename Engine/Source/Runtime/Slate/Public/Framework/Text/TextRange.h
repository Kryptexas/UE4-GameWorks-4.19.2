// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

struct SLATE_API FTextRange
{
	FTextRange()
		: BeginIndex(INDEX_NONE)
		, EndIndex(INDEX_NONE)
	{

	}

	FTextRange( int32 InBeginIndex, int32 InEndIndex )
		: BeginIndex( InBeginIndex )
		, EndIndex( InEndIndex )
	{

	}

	int32 Len() const { return EndIndex - BeginIndex; }
	bool IsEmpty() const { return (EndIndex - BeginIndex) <= 0; }

	int32 BeginIndex;
	int32 EndIndex;
};