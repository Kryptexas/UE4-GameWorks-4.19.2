// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SScreenShotItem.cpp: Implements the SScreenShotItem class.
=============================================================================*/

#include "ScreenShotComparisonPrivatePCH.h"

void SScreenShotItem::Construct( const FArguments& InArgs )
{
	ScreenShotData = InArgs._ScreenShotData;

	LoadBrush();
	// Create the screen shot data widget.
	ChildSlot
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 8.0f, 8.0f )
		[
			SNew( SBorder )
			.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
			[
				SNew( SVerticalBox )
				+SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Padding( 4.0f, 4.0f )
				[
					SNew( STextBlock ) .Text( ScreenShotData->GetName() )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Padding( 4.0f, 4.0f )
				[
					// The image to use. This needs to be a dynamic brush at some point.
					SNew( SImage )
					.Image( DynamicBrush.Get() )
				]
			]
		]
	]; 
}

void SScreenShotItem::LoadBrush()
{
	// The asset filename
	FString AssetFilename = ScreenShotData->GetAssetName();

	// Create the dynamic brush
	DynamicBrush = MakeShareable( new FSlateDynamicImageBrush( *AssetFilename, FVector2D(256,128) ) );
}
