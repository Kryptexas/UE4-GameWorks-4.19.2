// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"
#include "LandscapeEdMode.h"
#include "LandscapeEditorObject.h"
#include "LandscapeEditorDetails.h"
#include "LandscapeEditorDetailCustomizations.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "PropertyHandle.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "LandscapeEditor.Tools"

TSharedRef<IDetailCustomization> FLandscapeEditorDetailCustomization_MiscTools::MakeInstance()
{
	return MakeShareable(new FLandscapeEditorDetailCustomization_MiscTools);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FLandscapeEditorDetailCustomization_MiscTools::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& ToolsCategory = DetailBuilder.EditCategory("Tool Settings");

	if (IsBrushSetActive("BrushSet_Component"))
	{
		ToolsCategory.AddCustomRow("Clear Component Selection")
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FLandscapeEditorDetailCustomization_MiscTools::GetClearComponentSelectionVisibility)))
		[
			SNew(SButton)
			.Text(LOCTEXT("Component.ClearSelection", "Clear Component Selection"))
			.HAlign(HAlign_Center)
			.OnClicked_Static(&FLandscapeEditorDetailCustomization_MiscTools::OnClearComponentSelectionButtonClicked)
		];
	}

	//if (IsToolActive("ToolSet_Mask"))
	{
		ToolsCategory.AddCustomRow("Clear Region Selection")
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FLandscapeEditorDetailCustomization_MiscTools::GetClearRegionSelectionVisibility)))
		[
			SNew(SButton)
			.Text(LOCTEXT("Mask.ClearSelection", "Clear Region Selection"))
			.HAlign(HAlign_Center)
			.OnClicked_Static(&FLandscapeEditorDetailCustomization_MiscTools::OnClearRegionSelectionButtonClicked)
		];
	}

	if (IsToolActive("ToolSet_Splines"))
	{
		ToolsCategory.AddCustomRow("Apply Splines")
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(0, 6, 0, 0)
			[
				SNew(STextBlock)
				.Font(DetailBuilder.GetDetailFont())
				.ShadowOffset(FVector2D::UnitVector)
				.Text(LOCTEXT("Spline.ApplySplines", "Deform Landscape to Splines:"))
			]
		];
		ToolsCategory.AddCustomRow("Apply Splines")
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				.ToolTipText(LOCTEXT("Spline.ApplySplines.All.Tooltip", "Deforms and paints the landscape to fit all the landscape spline segments and controlpoints."))
				.Text(LOCTEXT("Spline.ApplySplines.All", "All Splines"))
				.HAlign(HAlign_Center)
				.OnClicked_Static(&FLandscapeEditorDetailCustomization_MiscTools::OnApplyAllSplinesButtonClicked)
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				.ToolTipText(LOCTEXT("Spline.ApplySplines.Tooltip", "Deforms and paints the landscape to fit only the selected landscape spline segments and controlpoints."))
				.Text(LOCTEXT("Spline.ApplySplines.Selected", "Only Selected"))
				.HAlign(HAlign_Center)
				.OnClicked_Static(&FLandscapeEditorDetailCustomization_MiscTools::OnApplySelectedSplinesButtonClicked)
			]
		];
	}


	if (IsToolActive("ToolSet_Ramp"))
	{
		ToolsCategory.AddCustomRow("Ramp")
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(0, 6, 0, 0)
			[
				SNew(STextBlock)
				.Font(DetailBuilder.GetDetailFont())
				.ShadowOffset(FVector2D::UnitVector)
				.Text(LOCTEXT("Ramp.Hint", "Ctrl+Click to add ramp points, then press \"Add Ramp\"."))
			]
		];
		ToolsCategory.AddCustomRow("Apply Ramp")
		[
			SNew(SBox)
			.Padding(FMargin(0, 0, 12, 0)) // Line up with the other properties due to having no reset to default button
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0, 0, 3, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Ramp.Reset", "Reset"))
					.HAlign(HAlign_Center)
					.OnClicked_Static(&FLandscapeEditorDetailCustomization_MiscTools::OnResetRampButtonClicked)
				]
				+ SHorizontalBox::Slot()
				.Padding(3, 0, 0, 0)
				[
					SNew(SButton)
					.IsEnabled_Static(&FLandscapeEditorDetailCustomization_MiscTools::GetApplyRampButtonIsEnabled)
					.Text(LOCTEXT("Ramp.Apply", "Add Ramp"))
					.HAlign(HAlign_Center)
					.OnClicked_Static(&FLandscapeEditorDetailCustomization_MiscTools::OnApplyRampButtonClicked)
				]
			]
		];
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

EVisibility FLandscapeEditorDetailCustomization_MiscTools::GetClearComponentSelectionVisibility()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		const FName CurrentToolSetName = LandscapeEdMode->CurrentToolSet->GetToolSetName();
		if (CurrentToolSetName == FName("ToolSet_Select"))
		{
			return EVisibility::Visible;
		}
		else if (LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid() && LandscapeEdMode->CurrentToolTarget.LandscapeInfo->GetSelectedComponents().Num() > 0)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

FReply FLandscapeEditorDetailCustomization_MiscTools::OnClearComponentSelectionButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		ULandscapeInfo* LandscapeInfo = LandscapeEdMode->CurrentToolTarget.LandscapeInfo.Get();
		if (LandscapeInfo)
		{
			FScopedTransaction Transaction(LOCTEXT("Component.Undo_ClearSelected", "Clearing Selection"));
			LandscapeInfo->Modify();
			LandscapeInfo->ClearSelectedRegion(true);
		}
	}

	return FReply::Handled();
}

EVisibility FLandscapeEditorDetailCustomization_MiscTools::GetClearRegionSelectionVisibility()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolSet)
	{
		const FName CurrentToolSetName = LandscapeEdMode->CurrentToolSet->GetToolSetName();
		if (CurrentToolSetName == FName("ToolSet_Mask"))
		{
			return EVisibility::Visible;
		}
		else if (LandscapeEdMode->CurrentToolSet->GetTool() && LandscapeEdMode->CurrentToolSet->GetTool()->SupportsMask() &&
			LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid() && LandscapeEdMode->CurrentToolTarget.LandscapeInfo->SelectedRegion.Num() > 0)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

FReply FLandscapeEditorDetailCustomization_MiscTools::OnClearRegionSelectionButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode)
	{
		ULandscapeInfo* LandscapeInfo = LandscapeEdMode->CurrentToolTarget.LandscapeInfo.Get();
		if( LandscapeInfo )
		{
			LandscapeInfo->ClearSelectedRegion(false);
		}
	}

	return FReply::Handled();
}

FReply FLandscapeEditorDetailCustomization_MiscTools::OnApplyAllSplinesButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		LandscapeEdMode->CurrentToolTarget.LandscapeInfo->ApplySplines(false);
	}

	return FReply::Handled();
}

FReply FLandscapeEditorDetailCustomization_MiscTools::OnApplySelectedSplinesButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode && LandscapeEdMode->CurrentToolTarget.LandscapeInfo.IsValid())
	{
		LandscapeEdMode->CurrentToolTarget.LandscapeInfo->ApplySplines(true);
	}

	return FReply::Handled();
}

FReply FLandscapeEditorDetailCustomization_MiscTools::OnApplyRampButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && IsToolActive(FName("ToolSet_Ramp")))
	{
		LandscapeEdMode->ApplyRampTool();
	}

	return FReply::Handled();
}

bool FLandscapeEditorDetailCustomization_MiscTools::GetApplyRampButtonIsEnabled()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && IsToolActive(FName("ToolSet_Ramp")))
	{
		return LandscapeEdMode->CanApplyRampTool();
	}

	return false;
}

FReply FLandscapeEditorDetailCustomization_MiscTools::OnResetRampButtonClicked()
{
	FEdModeLandscape* LandscapeEdMode = GetEditorMode();
	if (LandscapeEdMode != NULL && IsToolActive(FName("ToolSet_Ramp")))
	{
		LandscapeEdMode->ResetRampTool();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
