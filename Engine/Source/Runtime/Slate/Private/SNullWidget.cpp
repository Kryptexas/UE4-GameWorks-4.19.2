// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#include "Slate.h"

#define LOCTEXT_NAMESPACE "SNullWidget"

SLATE_API TSharedRef<SWidget> SNullWidget::NullWidget =
	SNew( SSpacer )
	.Visibility( EVisibility::Hidden );

TSharedRef<class SWidget> SMissingWidget::MakeMissingWidget()
{
	return 
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew( STextBlock )
			.Text(LOCTEXT("MissingWidget", "Missing Widget") )
			.TextStyle( FCoreStyle::Get(), "EmbossedText" )
		];
}

TSharedRef<class SWidget> SDocumentAreaWidget::MakeDocumentAreaWidget()
{
	return 
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew( STextBlock )
			.Text(LOCTEXT("DocumentArea", "Document Area") )
			.TextStyle( FCoreStyle::Get(), "EmbossedText" )
		];
}

#undef LOCTEXT_NAMESPACE