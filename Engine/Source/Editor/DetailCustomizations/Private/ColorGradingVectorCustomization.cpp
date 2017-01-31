// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "ColorGradingVectorCustomization.h"
#include "IPropertyUtilities.h"
#include "SNumericEntryBox.h"
#include "SColorGradingPicker.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SBox.h"

FColorGradingVectorCustomization::FColorGradingVectorCustomization()
{
	CurrentData.Empty();
	CustomColorGradingBuilder = nullptr;
}

void FColorGradingVectorCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils, TArray<TWeakPtr<IPropertyHandle>> WeakChildArray)
{
	FColorGradingCustomBuilderDelegates ColorGradingPickerDelegates;
	ColorGradingPickerDelegates.OnGetColorGradingData = FOnGetColorGradingData::CreateSP(this, &FColorGradingVectorCustomization::OnGetData);
	ColorGradingPickerDelegates.OnColorGradingChanged = FOnColorGradingChanged::CreateSP(this, &FColorGradingVectorCustomization::OnVector4DataChanged);
	TWeakPtr<IPropertyHandle> PropertyHandleWeak = StructPropertyHandle;
	CustomColorGradingBuilder = MakeShareable(new FColorGradingCustomBuilder(ColorGradingPickerDelegates, PropertyHandleWeak, WeakChildArray));
	// Add the individual properties as children as well so the vector can be expanded for more room
	StructBuilder.AddChildCustomBuilder(CustomColorGradingBuilder.ToSharedRef());
}

void FColorGradingVectorCustomization::OnGetData(TArray<FVector4*> &OutColorGradingData, TWeakPtr<IPropertyHandle> StructPropertyHandle)
{
	//We need a value array to kept the FVector4 pointer alive during the color grading Picker operation
	TArray<FVector4> PropertyValues;
	CurrentData.Empty();
	TArray<FString> StrPropertyValues;
	if (StructPropertyHandle.IsValid() && StructPropertyHandle.Pin()->GetPerObjectValues(StrPropertyValues) == FPropertyAccess::Success)
	{
		// Loop through each object and scale based on the new ratio for each object individually
		PropertyValues.AddZeroed(StrPropertyValues.Num());
		for (int32 PropertyValueIndex = 0; PropertyValueIndex < StrPropertyValues.Num(); ++PropertyValueIndex)
		{
			PropertyValues[PropertyValueIndex].InitFromString(StrPropertyValues[PropertyValueIndex]);
			FVector4 *NewVector4 = new FVector4(PropertyValues[PropertyValueIndex]);
			CurrentData.Add(NewVector4);
			OutColorGradingData.Add(NewVector4);
		}
	}
}

void FColorGradingVectorCustomization::OnVector4DataChanged(FVector4 ColorGradingData, TWeakPtr<IPropertyHandle> StructPropertyHandle)
{
	TArray<FString> ObjectChildValues;
	if (StructPropertyHandle.IsValid() && StructPropertyHandle.Pin()->GetPerObjectValues (ObjectChildValues) == FPropertyAccess::Success)
	{
		for (int32 PropertyValueIndex = 0; PropertyValueIndex < ObjectChildValues.Num(); ++PropertyValueIndex)
		{
			ObjectChildValues[PropertyValueIndex] = FString::Printf(TEXT("(X=%3.3f,Y=%3.3f,Z=%3.3f,W=%3.3f)"), ColorGradingData.X, ColorGradingData.Y, ColorGradingData.Z, ColorGradingData.W);
		}

		StructPropertyHandle.Pin()->SetPerObjectValues(ObjectChildValues);
	}
}


//////////////////////////////////////////////////////////////////////////
// Color Gradient custom builder implementation

FColorGradingCustomBuilder::FColorGradingCustomBuilder(FColorGradingCustomBuilderDelegates& ColorGradingCustomBuilderDelegates, TWeakPtr<IPropertyHandle> InColorGradingPropertyHandle, TArray< TWeakPtr<IPropertyHandle> >& InSortedChildArray)
{
	ColorGradingDelegates = ColorGradingCustomBuilderDelegates;
	ColorGradingPropertyHandle = InColorGradingPropertyHandle;
	for (TWeakPtr<IPropertyHandle> ChildProperty : InSortedChildArray)
	{
		SortedChildArray.Add(ChildProperty);
	}
}

void FColorGradingCustomBuilder::Tick(float DeltaTime)
{
}

void FColorGradingCustomBuilder::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	if (!ColorGradingDelegates.OnGetColorGradingData.IsBound())
	{
		return;
	}
	//Query all meta data we need
	UProperty* Property = ColorGradingPropertyHandle.Pin()->GetProperty();
	const FString& MetaUIMinString = Property->GetMetaData(TEXT("UIMin"));
	const FString& MetaUIMaxString = Property->GetMetaData(TEXT("UIMax"));
	const FString& DeltaString = Property->GetMetaData(TEXT("Delta"));
	const FString& ShiftMouseMovePixelPerDeltaString = Property->GetMetaData(TEXT("ShiftMouseMovePixelPerDelta"));
	const FString& ColorGradingModeString = Property->GetMetaData(TEXT("ColorGradingMode"));
	float UIMin = TNumericLimits<float>::Lowest();
	float UIMax = TNumericLimits<float>::Max();
	TTypeFromString<float>::FromString(UIMin, *MetaUIMinString);
	TTypeFromString<float>::FromString(UIMax, *MetaUIMaxString);
	float Delta = float(0);
	if (DeltaString.Len())
	{
		TTypeFromString<float>::FromString(Delta, *DeltaString);
	}
	int32 ShiftMouseMovePixelPerDelta = 1;
	if (ShiftMouseMovePixelPerDeltaString.Len())
	{
		TTypeFromString<int32>::FromString(ShiftMouseMovePixelPerDelta, *ShiftMouseMovePixelPerDeltaString);
		//The value should be greater or equal to 1
		// 1 is neutral since it is a multiplier of the mouse drag pixel
		if (ShiftMouseMovePixelPerDelta < 1)
		{
			ShiftMouseMovePixelPerDelta = 1;
		}
	}

	EColorGradingModes ColorGradingMode = EColorGradingModes::Contrast;
	if (ColorGradingModeString.Len())
	{
		if (ColorGradingModeString.Compare(TEXT("saturation")) == 0)
		{
			ColorGradingMode = EColorGradingModes::Saturation;
		}
		else if (ColorGradingModeString.Compare(TEXT("contrast")) == 0)
		{
			ColorGradingMode = EColorGradingModes::Contrast;
		}
		else if (ColorGradingModeString.Compare(TEXT("gamma")) == 0)
		{
			ColorGradingMode = EColorGradingModes::Gamma;
		}
		else if (ColorGradingModeString.Compare(TEXT("gain")) == 0)
		{
			ColorGradingMode = EColorGradingModes::Gain;
		}
		else if (ColorGradingModeString.Compare(TEXT("offset")) == 0)
		{
			ColorGradingMode = EColorGradingModes::Offset;
		}
	}

	NodeRow.NameContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.WidthOverride(125.0f)
			//.HeightOverride(150.0f)
			.MinDesiredWidth(125.0f)
			.MaxDesiredWidth(125.0f)
			.Padding(FMargin(2.0f, 2.0f, 2.0f, 2.0f))
			[
				SNew(SColorGradingPicker)
					.ValuesMin(UIMin)
					.ValuesMax(UIMax)
					.MainDelta(Delta)
					.MainShiftMouseMovePixelPerDelta(ShiftMouseMovePixelPerDelta)
					.ColorGradingModes(ColorGradingMode)
					.OnColorCommitted(FOnVector4ValueChanged::CreateSP(this, &FColorGradingCustomBuilder::OnColorGradingPickerChanged))
					.OnQueryCurrentColor(FOnGetCurrentVector4Value::CreateSP(this, &FColorGradingCustomBuilder::GetCurrentColorGradingValue))
			]
		];
	
	TSharedPtr<SVerticalBox> VerticalBox = SNew(SVerticalBox);
	VerticalBox->AddSlot()
		.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
		.FillHeight(1.0)
		[
			SNew(SBox)
		];
	int32 ColorIndex = 0;
	FText LabelText[4];
	LabelText[0] = NSLOCTEXT("ColorGradingVectorCustomizationNS", "RedChannelSmallName", "R");
	LabelText[1] = NSLOCTEXT("ColorGradingVectorCustomizationNS", "GreenChannelSmallName", "G");
	LabelText[2] = NSLOCTEXT("ColorGradingVectorCustomizationNS", "BlueChannelSmallName", "B");
	LabelText[3] = NSLOCTEXT("ColorGradingVectorCustomizationNS", "LuminanceChannelSmallName", "Y");
	for (TWeakPtr<IPropertyHandle> ChildProperty : SortedChildArray)
	{
		FText LabelChannelText = ChildProperty.Pin()->GetPropertyDisplayName();
		if(ColorIndex >= 0 && ColorIndex < 4)
		{
			LabelChannelText = LabelText[ColorIndex];
		}
		TSharedPtr<SWidget> LabelWidget = SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(LabelChannelText);


		TSharedPtr<SNumericEntryBox<float>> NumericEntry;
		SAssignNew(NumericEntry, SNumericEntryBox<float>)
			.EditableTextBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("DarkEditableTextBox"))
			.Value(this, &FColorGradingCustomBuilder::OnSliderGetValue, ChildProperty)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.OnValueChanged(this, &FColorGradingCustomBuilder::OnSliderValueChanged, ChildProperty)
			.LabelVAlign(VAlign_Center)
			// Only allow spin on handles with one object.  Otherwise it is not clear what value to spin
			.AllowSpin(ChildProperty.Pin()->GetNumOuterObjects() == 1)
			.UseDarkStyle(true)
			.ColorIndex(ColorIndex)
			.ShiftMouseMovePixelPerDelta(ShiftMouseMovePixelPerDelta)
			.MinValue(UIMin)
			.MaxValue(UIMax)
			.MinSliderValue(UIMin)
			.MaxSliderValue(UIMax)
			.SliderExponent(3.0f)
			.SliderExponentNeutralValue(UIMin + (UIMax - UIMin) / 2.0f)
			.Delta(Delta)
			.Label()
			[
				LabelWidget.ToSharedRef()
			];

		VerticalBox->AddSlot()
			.Padding(FMargin(3.0f, 3.0f, 3.0f, 3.0f))
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				NumericEntry.ToSharedRef()
			];
		ColorIndex++;
	}
	//Add a filler to take all the rest of the space
	VerticalBox->AddSlot()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
		.FillHeight(1.0)
		[
			SNew(SBox)
		];
	
	NodeRow.ValueContent()
		.HAlign(HAlign_Fill)
	[
		VerticalBox.ToSharedRef()
	];
}

void FColorGradingCustomBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
}

void FColorGradingCustomBuilder::OnSliderValueChanged(float NewValue, TWeakPtr<IPropertyHandle> WeakHandlePtr)
{
	if (!ColorGradingDelegates.OnColorGradingChanged.IsBound())
	{
		return;
	}
	int32 ChildIndex = 0;
	if (SortedChildArray.Find(WeakHandlePtr, ChildIndex))
	{
		FVector4 ValueVector;
		GetCurrentColorGradingValue(ValueVector);
		ValueVector[ChildIndex] = NewValue;
		ColorGradingDelegates.OnColorGradingChanged.Execute(ValueVector, ColorGradingPropertyHandle);
	}
}

TOptional<float> FColorGradingCustomBuilder::OnSliderGetValue(TWeakPtr<IPropertyHandle> WeakHandlePtr) const
{
	float CurrentValue = 0.0f;
	if (WeakHandlePtr.Pin()->GetValue(CurrentValue) == FPropertyAccess::Success)
	{
		return CurrentValue;
	}

	// Value couldn't be accessed.  Return an unset value
	return 0.0f;
}

void FColorGradingCustomBuilder::OnColorGradingPickerChanged(FVector4 &NewValue)
{
	if (!ColorGradingDelegates.OnColorGradingChanged.IsBound())
	{
		return;
	}
	ColorGradingDelegates.OnColorGradingChanged.Execute(NewValue, ColorGradingPropertyHandle);
}

void FColorGradingCustomBuilder::GetCurrentColorGradingValue(FVector4 &OutCurrentValue)
{
	if (!ColorGradingDelegates.OnGetColorGradingData.IsBound())
	{
		OutCurrentValue = FVector4(0.0f);
		return;
	}
	TArray<FVector4*> ColorGradingArrayPtr;
	ColorGradingDelegates.OnGetColorGradingData.Execute(ColorGradingArrayPtr, ColorGradingPropertyHandle);

	OutCurrentValue = ColorGradingArrayPtr.Num() > 0 ? *ColorGradingArrayPtr[0] : FVector4(0.0f);
}
