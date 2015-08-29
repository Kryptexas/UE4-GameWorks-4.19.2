// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SAutoTextScroller : public SUserWidget
{
public:
	SLATE_USER_ARGS(SAutoTextScroller)
	{ }
	SLATE_ARGUMENT(const FFriendsFontStyle*, FontStyle)
	SLATE_ATTRIBUTE(FText, Text)
	SLATE_END_ARGS()

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	virtual void Construct( const FArguments& InArgs ) = 0;
};
