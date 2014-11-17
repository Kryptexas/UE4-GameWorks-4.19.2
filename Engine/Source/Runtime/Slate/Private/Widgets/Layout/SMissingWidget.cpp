// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMissingWidget.h: Declares the SMissingWidget class.
=============================================================================*/

#include "SlatePrivatePCH.h"


#define LOCTEXT_NAMESPACE "SMissingWidget"


TSharedRef<class SWidget> SMissingWidget::MakeMissingWidget( )
{
	return SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("MissingWidget", "Missing Widget"))
				.TextStyle(FCoreStyle::Get(), "EmbossedText")
		];
}


#undef LOCTEXT_NAMESPACE
