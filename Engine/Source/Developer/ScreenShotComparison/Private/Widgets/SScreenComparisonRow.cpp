// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ScreenShotComparisonPrivatePCH.h"

#include "SScreenComparisonRow.h"

#define LOCTEXT_NAMESPACE "SScreenShotBrowser"

void SScreenComparisonRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	STableRow<TSharedPtr<FImageComparisonResult> >::ConstructInternal(
		STableRow::FArguments()
			.ShowSelection(true),
		InOwnerTableView
	);

	ComparisonResult = InArgs._ComparisonResult;

	CachedActualImageSize = FIntPoint::NoneValue;

	FString ApprovedFile = InArgs._ComparisonDirectory / InArgs._Comparisons->ApprovedPath / ComparisonResult->ApprovedFile;
	FString IncomingFile = InArgs._ComparisonDirectory / InArgs._Comparisons->IncomingPath / ComparisonResult->IncomingFile;
	FString DeltaFile = InArgs._ComparisonDirectory / InArgs._Comparisons->DeltaPath / ComparisonResult->ComparisonFile;

	ApprovedBrush = MakeShareable(new FSlateDynamicImageBrush(*ApprovedFile, FVector2D(320, 180)));
	UnapprovedBrush = MakeShareable(new FSlateDynamicImageBrush(*IncomingFile, FVector2D(320, 180)));
	ComparisonBrush = MakeShareable(new FSlateDynamicImageBrush(*DeltaFile, FVector2D(320, 180)));

	// Create the screen shot data widget.
	ChildSlot
	[
		SNew( SHorizontalBox )

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding( 8.0f, 8.0f )
		[
			SNew( SImage )
			.Image(ApprovedBrush.Get() )
			//.OnMouseButtonDown(this,&SScreenShotItem::OnImageClicked)
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(8.0f, 8.0f)
		[
			SNew(SImage)
			.Image(ComparisonBrush.Get())
			//.OnMouseButtonDown(this,&SScreenShotItem::OnImageClicked)
		]
		
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding( 8.0f, 8.0f )
		[
			SNew( SImage )
			.Image(UnapprovedBrush.Get() )
			//.OnMouseButtonDown(this,&SScreenShotItem::OnImageClicked)
		]
	]; 
}

#undef LOCTEXT_NAMESPACE