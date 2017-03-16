// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "Vector4StructCustomization.h"
#include "IPropertyUtilities.h"
#include "SNumericEntryBox.h"
#include "SColorGradingPicker.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SBox.h"

TSharedRef<IPropertyTypeCustomization> FVector4StructCustomization::MakeInstance()
{
	return MakeShareable(new FVector4StructCustomization);
}

FVector4StructCustomization::FVector4StructCustomization()
	: FMathStructCustomization()
	, ColorGradingVectorCustomization(nullptr)
{
}

FVector4StructCustomization::~FVector4StructCustomization()
{
	//Release all resources
	ColorGradingVectorCustomization = nullptr;
}

void FVector4StructCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	UProperty* Property = StructPropertyHandle->GetProperty();
	if (Property)
	{
		const FString& ColorGradingModeString = Property->GetMetaData(TEXT("ColorGradingMode"));
		if (!ColorGradingModeString.IsEmpty())
		{
			//Create our color grading customization shared pointer
			if (!ColorGradingVectorCustomization.IsValid())
			{
				ColorGradingVectorCustomization = MakeShareable(new FColorGradingVectorCustomization);
			}

			//Customize the childrens
			TArray<TWeakPtr<IPropertyHandle>> WeakChildArray;
			WeakChildArray.Append(SortedChildHandles);

			ColorGradingVectorCustomization->CustomizeChildren(StructPropertyHandle, StructBuilder, StructCustomizationUtils, WeakChildArray, StaticCastSharedRef<FVector4StructCustomization>(AsShared()));
			
			// We handle the customize Children so just return here
			return;
		}
	}

	//Use the base class customize children
	FMathStructCustomization::CustomizeChildren(StructPropertyHandle, StructBuilder, StructCustomizationUtils);
}

void FVector4StructCustomization::MakeHeaderRow(TSharedRef<class IPropertyHandle>& StructPropertyHandle, FDetailWidgetRow& Row)
{
	//Use the base class header row
	FMathStructCustomization::MakeHeaderRow(StructPropertyHandle, Row);

	//Modify the name content if the vector is representing some color grading.
	//We are not showing the color preview in the case of an additive color grading vector
	//like the offset since additive value as no color meaning
	UProperty* Property = StructPropertyHandle->GetProperty();
	if (Property)
	{
		const FString& ColorGradingModeString = Property->GetMetaData(TEXT("ColorGradingMode"));
		bool IsAdditiveMode = (ColorGradingModeString.Compare(TEXT("offset")) == 0);
		if (ColorGradingModeString.Len() > 0 && !IsAdditiveMode)
		{
			//Get a weak pointer of the Vector4 property handle to retrieve the value for the color box preview
			TWeakPtr<IPropertyHandle> StructWeakHandlePtr = StructPropertyHandle;

			//Get the original name content
			TSharedRef<SWidget> OriginalNameContent = Row.NameWidget.Widget;

			//Put back the original content on the left side of the NameContent box
			TSharedPtr<SHorizontalBox> NameContentHorizontalBox;
			SAssignNew(NameContentHorizontalBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				[
					OriginalNameContent
				];

			//Add the color box preview on the right side of the name content box
			NameContentHorizontalBox->AddSlot()
				.HAlign(HAlign_Right)
				.FillWidth(1.0)
				[
					SNew(SBox)
					.HAlign(HAlign_Right)
				.Padding(FMargin(0.0f, 0.0f, 5.0f, 0.0f))
				[
					SNew(SColorBlock)
					.Color(this, &FVector4StructCustomization::OnGetHeaderColorBlock, StructWeakHandlePtr)
					.ShowBackgroundForAlpha(false)
					.IgnoreAlpha(true)
					.Size(FVector2D(35.0f, 12.0f))
					]
				];

			//Override the old NameContent with the new NameContent box
			Row.NameContent()
				[
					NameContentHorizontalBox.ToSharedRef()
				];
		}
	}
}

FLinearColor FVector4StructCustomization::OnGetHeaderColorBlock(TWeakPtr<IPropertyHandle> WeakHandlePtr) const
{
	FLinearColor ColorValue(0.0f, 0.0f, 0.0f);
	FVector4 VectorValue;
	if (WeakHandlePtr.Pin()->GetValue(VectorValue) == FPropertyAccess::Success)
	{
		ColorValue.R = VectorValue.X * VectorValue.W;
		ColorValue.G = VectorValue.Y * VectorValue.W;
		ColorValue.B = VectorValue.Z * VectorValue.W;
	}
	return ColorValue;
}
