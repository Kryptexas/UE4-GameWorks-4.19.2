// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SNullWidget.cpp: Implements the SNullWidget class.
=============================================================================*/
 
#include "SlateCorePrivatePCH.h"


class SLATECORE_API SNullWidgetContent
	: public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SNullWidgetContent)
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		SWidget::Construct( 
			InArgs._ToolTipText,
			InArgs._ToolTip,
			InArgs._Cursor, 
			InArgs._IsEnabled,
			InArgs._Visibility,
			InArgs._Tag
		);
	}

public:

	// SWidget interface

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override
	{
		return LayerId;
	}

	virtual FVector2D ComputeDesiredSize( ) const override
	{
		return FVector2D(0.0f, 0.0f);
	}

	// End of SWidget interface
};


SLATECORE_API TSharedRef<SWidget> SNullWidget::NullWidget = SNew(SNullWidgetContent).Visibility(EVisibility::Hidden);
