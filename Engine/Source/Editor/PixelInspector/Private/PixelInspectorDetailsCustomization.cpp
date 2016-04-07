// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PixelInspectorPrivatePCH.h"
#include "PixelInspectorDetailsCustomization.h"
#include "PixelInspectorView.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"

#define LOCTEXT_NAMESPACE "PixelInspector"


FPixelInspectorDetailsCustomization::FPixelInspectorDetailsCustomization()
{
	CachedDetailBuilder = nullptr;
}

TSharedRef<IDetailCustomization> FPixelInspectorDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FPixelInspectorDetailsCustomization);
}

TSharedRef<SHorizontalBox> FPixelInspectorDetailsCustomization::GetGridColorContext()
{
	TSharedRef<SHorizontalBox> HorizontalMainGrid = SNew(SHorizontalBox);
	for (int RowIndex = 0; RowIndex < FinalColorContextGridSize; ++RowIndex)
	{
		SBoxPanel::FSlot &HorizontalSlot = HorizontalMainGrid->AddSlot()
			.AutoWidth()
			.Padding(2.0f, 2.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center);

		TSharedRef<SVerticalBox> VerticalColumn = SNew(SVerticalBox);
		for (int ColumnIndex = 0; ColumnIndex < FinalColorContextGridSize; ++ColumnIndex)
		{
			VerticalColumn->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					CreateColorCell(PixelInspectorView->FinalColorContext[RowIndex + ColumnIndex*FinalColorContextGridSize])
				];
		}
		HorizontalSlot[VerticalColumn];
	}
	return HorizontalMainGrid;
}


TSharedRef<SColorBlock> FPixelInspectorDetailsCustomization::CreateColorCell(const FLinearColor &CellColor)
{
	return SNew(SColorBlock)
		.Color(CellColor)
		.ShowBackgroundForAlpha(false)
		.IgnoreAlpha(true)
		.Size(FVector2D(16.0f, 16.0f));
}

void FPixelInspectorDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CachedDetailBuilder = &DetailBuilder;
	TArray<TWeakObjectPtr<UObject>> EditingObjects;
	DetailBuilder.GetObjectsBeingCustomized(EditingObjects);
	check(EditingObjects.Num() == 1);

	PixelInspectorView = Cast<UPixelInspectorView>(EditingObjects[0].Get());

	IDetailCategoryBuilder& FinalColorCategory = DetailBuilder.EditCategory("FinalColor", FText::GetEmpty());
	if (PixelInspectorView != nullptr)
	{
		FDetailWidgetRow& MergeRow = FinalColorCategory.AddCustomRow(LOCTEXT("FinalColorContextArray", "Context Color"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ContextColorRowTitle", "Context Colors"))
		];

		MergeRow.ValueContent()
		[
			GetGridColorContext()
		];
	}
	

	// Show only the option that go with the shading model
	TSharedRef<IPropertyHandle> SubSurfaceColorProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, SubSurfaceColor));
	TSharedRef<IPropertyHandle> SubSurfaceProfileProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, SubsurfaceProfile));
	TSharedRef<IPropertyHandle> OpacityProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, Opacity));
	TSharedRef<IPropertyHandle> ClearCoatProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, ClearCoat));
	TSharedRef<IPropertyHandle> ClearCoatRoughnessProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, ClearCoatRoughness));
	TSharedRef<IPropertyHandle> WorldNormalProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, WorldNormal));
	TSharedRef<IPropertyHandle> BackLitProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, BackLit));
	TSharedRef<IPropertyHandle> ClothProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, Cloth));
	TSharedRef<IPropertyHandle> EyeTangentProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, EyeTangent));
	TSharedRef<IPropertyHandle> IrisMaskProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, IrisMask));
	TSharedRef<IPropertyHandle> IrisDistanceProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPixelInspectorView, IrisDistance));

	EMaterialShadingModel MaterialShadingModel = PixelInspectorView->MaterialShadingModel;
	switch (MaterialShadingModel)
	{
		case MSM_DefaultLit:
		case MSM_Unlit:
		{
			DetailBuilder.HideProperty(SubSurfaceColorProp);
			DetailBuilder.HideProperty(SubSurfaceProfileProp);
			DetailBuilder.HideProperty(OpacityProp);
			DetailBuilder.HideProperty(ClearCoatProp);
			DetailBuilder.HideProperty(ClearCoatRoughnessProp);
			DetailBuilder.HideProperty(WorldNormalProp);
			DetailBuilder.HideProperty(BackLitProp);
			DetailBuilder.HideProperty(ClothProp);
			DetailBuilder.HideProperty(EyeTangentProp);
			DetailBuilder.HideProperty(IrisMaskProp);
			DetailBuilder.HideProperty(IrisDistanceProp);
		}
		break;
		case MSM_Subsurface:
		case MSM_PreintegratedSkin:
		case MSM_TwoSidedFoliage:
		{
			DetailBuilder.HideProperty(SubSurfaceProfileProp);
			DetailBuilder.HideProperty(ClearCoatProp);
			DetailBuilder.HideProperty(ClearCoatRoughnessProp);
			DetailBuilder.HideProperty(WorldNormalProp);
			DetailBuilder.HideProperty(BackLitProp);
			DetailBuilder.HideProperty(ClothProp);
			DetailBuilder.HideProperty(EyeTangentProp);
			DetailBuilder.HideProperty(IrisMaskProp);
			DetailBuilder.HideProperty(IrisDistanceProp);
		}
		break;
		case MSM_SubsurfaceProfile:
		{
			DetailBuilder.HideProperty(SubSurfaceColorProp);
			DetailBuilder.HideProperty(ClearCoatProp);
			DetailBuilder.HideProperty(ClearCoatRoughnessProp);
			DetailBuilder.HideProperty(WorldNormalProp);
			DetailBuilder.HideProperty(BackLitProp);
			DetailBuilder.HideProperty(ClothProp);
			DetailBuilder.HideProperty(EyeTangentProp);
			DetailBuilder.HideProperty(IrisMaskProp);
			DetailBuilder.HideProperty(IrisDistanceProp);
		}
		break;
		case MSM_ClearCoat:
		{
			DetailBuilder.HideProperty(SubSurfaceColorProp);
			DetailBuilder.HideProperty(SubSurfaceProfileProp);
			DetailBuilder.HideProperty(OpacityProp);
			DetailBuilder.HideProperty(WorldNormalProp);
			DetailBuilder.HideProperty(BackLitProp);
			DetailBuilder.HideProperty(ClothProp);
			DetailBuilder.HideProperty(EyeTangentProp);
			DetailBuilder.HideProperty(IrisMaskProp);
			DetailBuilder.HideProperty(IrisDistanceProp);
		}
		break;
		case MSM_Hair:
		{
			DetailBuilder.HideProperty(SubSurfaceColorProp);
			DetailBuilder.HideProperty(SubSurfaceProfileProp);
			DetailBuilder.HideProperty(OpacityProp);
			DetailBuilder.HideProperty(ClearCoatProp);
			DetailBuilder.HideProperty(ClearCoatRoughnessProp);
			DetailBuilder.HideProperty(ClothProp);
			DetailBuilder.HideProperty(EyeTangentProp);
			DetailBuilder.HideProperty(IrisMaskProp);
			DetailBuilder.HideProperty(IrisDistanceProp);
		}
		break;
		case MSM_Cloth:
		{
			DetailBuilder.HideProperty(SubSurfaceProfileProp);
			DetailBuilder.HideProperty(OpacityProp);
			DetailBuilder.HideProperty(ClearCoatProp);
			DetailBuilder.HideProperty(ClearCoatRoughnessProp);
			DetailBuilder.HideProperty(WorldNormalProp);
			DetailBuilder.HideProperty(BackLitProp);
			DetailBuilder.HideProperty(EyeTangentProp);
			DetailBuilder.HideProperty(IrisMaskProp);
			DetailBuilder.HideProperty(IrisDistanceProp);
		}
		break;
		case MSM_Eye:
		{
			DetailBuilder.HideProperty(SubSurfaceColorProp);
			DetailBuilder.HideProperty(SubSurfaceProfileProp);
			DetailBuilder.HideProperty(OpacityProp);
			DetailBuilder.HideProperty(ClearCoatProp);
			DetailBuilder.HideProperty(ClearCoatRoughnessProp);
			DetailBuilder.HideProperty(WorldNormalProp);
			DetailBuilder.HideProperty(BackLitProp);
			DetailBuilder.HideProperty(ClothProp);
		}
		break;
	}
}

#undef LOCTEXT_NAMESPACE
