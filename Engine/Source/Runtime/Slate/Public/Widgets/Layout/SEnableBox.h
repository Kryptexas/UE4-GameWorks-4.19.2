// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A SEnableBox contains a widget that is lied to about whether the parent hierarchy is enabled or not, being told the parent is always enabled
 */
class SEnableBox : public SBox
{
public:
	SLATE_BEGIN_ARGS(SEnableBox) {}
		/** The widget content to be presented as if the parent were enabled */
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SBox::Construct(SBox::FArguments().Content()[InArgs._Content.Widget]);
	}

	// SWidget interface
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE
	{
		return SBox::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, /*bParentEnabled=*/ true);
	}
	// End of SWidget interface
};
