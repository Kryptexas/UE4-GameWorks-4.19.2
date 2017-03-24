// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ClothPaintSettingsCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "STextBlock.h"
#include "DetailLayoutBuilder.h"
#include "ClothPaintSettings.h"
#include "ClothingAsset.h"
#include "SComboBox.h"

#define LOCTEXT_NAMESPACE "ClothPaintSettingsCustomization"

FClothPaintSettingsCustomization::FClothPaintSettingsCustomization(FClothPainter* InPainter) 
	: Painter(InPainter)
{

}

FClothPaintSettingsCustomization::~FClothPaintSettingsCustomization()
{
	
}

TSharedRef<IDetailCustomization> FClothPaintSettingsCustomization::MakeInstance(FClothPainter* InPainter)
{
	return MakeShareable(new FClothPaintSettingsCustomization(InPainter));
}

void FClothPaintSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
	
	UClothPainterSettings* PainterSettings = nullptr;
	for(TWeakObjectPtr<UObject>& WeakObj : CustomizedObjects)
	{
		if(UObject* CustomizedObject = WeakObj.Get())
		{
			if(UClothPainterSettings* Settings = Cast<UClothPainterSettings>(CustomizedObject))
			{
				PainterSettings = Settings;
			}
		}
	}

	const FName ClothCategory = "ClothPainting";
	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(ClothCategory);
	// Hide gradient/brush settings according to which tool is being used
	if (PainterSettings)
	{
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UClothPainterSettings, PaintTool))->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([&]() { DetailBuilder.ForceRefreshDetails(); }));

		if (PainterSettings->PaintTool == EClothPaintTool::Brush)
		{
			DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UClothPainterSettings, GradientStartValue));
			DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UClothPainterSettings, GradientEndValue));
			DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UClothPainterSettings, bUseRegularBrushForGradient));
		}
		else
		{
			DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UClothPainterSettings, PaintValue));
		}
	}
}

#undef LOCTEXT_NAMESPACE
