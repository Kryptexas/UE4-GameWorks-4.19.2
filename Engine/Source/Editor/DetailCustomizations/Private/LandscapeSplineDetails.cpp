// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "LandscapeSplineDetails.h"
#include "LandscapeEdMode.h"

#define LOCTEXT_NAMESPACE "LandscapeSplineDetails"


TSharedRef<IDetailCustomization> FLandscapeSplineDetails::MakeInstance()
{
	return MakeShareable( new FLandscapeSplineDetails );
}

void FLandscapeSplineDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	IDetailCategoryBuilder& LandscapeSplineCategory = DetailBuilder.EditCategory( "LandscapeSpline", TEXT(""), ECategoryPriority::Transform );

	LandscapeSplineCategory.AddCustomRow( TEXT("") )
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(0, 0, 2, 0)
		.VAlign(VAlign_Center)
		.FillWidth(1)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectAll", "Select all connected:").ToString())
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SButton)
			.Text(LOCTEXT("ControlPoints", "Control Points").ToString())
			.HAlign(HAlign_Center)
			.OnClicked(this, &FLandscapeSplineDetails::OnSelectConnectedControlPointsButtonClicked)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SButton)
			.Text(LOCTEXT("Segments", "Segments").ToString())
			.HAlign(HAlign_Center)
			.OnClicked(this, &FLandscapeSplineDetails::OnSelectConnectedSegmentsButtonClicked)
		]
	];
}

class FEdModeLandscape* FLandscapeSplineDetails::GetEditorMode() const
{
	return (FEdModeLandscape*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);
}

FReply FLandscapeSplineDetails::OnSelectConnectedControlPointsButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		LandscapeEdMode->SelectAllConnectedSplineControlPoints();
	}

	return FReply::Handled();
}

FReply FLandscapeSplineDetails::OnSelectConnectedSegmentsButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		LandscapeEdMode->SelectAllConnectedSplineSegments();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
