// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "SlateTextLayoutMarshaller.h"

#if WITH_FANCY_TEXT

void FSlateTextLayoutMarshaller::SetDefaultTextStyle(FTextBlockStyle InDefaultTextStyle)
{
	DefaultTextStyle = MoveTemp(InDefaultTextStyle);
	MakeDirty();
}

const FTextBlockStyle& FSlateTextLayoutMarshaller::GetDefaultTextStyle() const
{
	return DefaultTextStyle;
}

FSlateTextLayoutMarshaller::FSlateTextLayoutMarshaller(FTextBlockStyle InDefaultTextStyle)
	: DefaultTextStyle(MoveTemp(InDefaultTextStyle))
{
}

#endif //WITH_FANCY_TEXT
