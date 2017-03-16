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
#include "Vector4StructCustomization.h"

FColorGradingVectorCustomization::FColorGradingVectorCustomization()
{
	CurrentData.Empty();
	CustomColorGradingBuilder = nullptr;
}

void FColorGradingVectorCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils, const TArray<TWeakPtr<IPropertyHandle>>& WeakChildArray, 
														 TSharedRef<FVector4StructCustomization> InVector4Customization)
{
	FColorGradingCustomBuilderDelegates ColorGradingPickerDelegates;
	ColorGradingPickerDelegates.OnGetColorGradingData = FOnGetColorGradingData::CreateSP(this, &FColorGradingVectorCustomization::OnGetData);
	ColorGradingPickerDelegates.OnColorGradingChanged = FOnColorGradingChanged::CreateSP(this, &FColorGradingVectorCustomization::OnVector4DataChanged);
	TWeakPtr<IPropertyHandle> PropertyHandleWeak = StructPropertyHandle;
	CustomColorGradingBuilder = MakeShareable(new FColorGradingCustomBuilder(ColorGradingPickerDelegates, PropertyHandleWeak, WeakChildArray, InVector4Customization));
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

void FColorGradingVectorCustomization::OnVector4DataChanged(FVector4 ColorGradingData, TWeakPtr<IPropertyHandle> StructPropertyHandle, bool ShouldCommitValueChanges)
{
	if (StructPropertyHandle.IsValid())
	{
		StructPropertyHandle.Pin()->SetValue(ColorGradingData, ShouldCommitValueChanges ? EPropertyValueSetFlags::DefaultFlags : EPropertyValueSetFlags::InteractiveChange);
	}
}

//////////////////////////////////////////////////////////////////////////
// Color Gradient custom builder implementation

FColorGradingCustomBuilder::FColorGradingCustomBuilder(FColorGradingCustomBuilderDelegates& ColorGradingCustomBuilderDelegates, TWeakPtr<IPropertyHandle> InColorGradingPropertyHandle, const TArray<TWeakPtr<IPropertyHandle>>& InSortedChildArray, 
													   TSharedRef<FVector4StructCustomization> InVector4Customization)
	: ColorGradingDelegates(ColorGradingCustomBuilderDelegates)
	, ColorGradingPropertyHandle(InColorGradingPropertyHandle)
	, Vector4Customization(InVector4Customization)
{
	SortedChildArray.Append(InSortedChildArray);
}

FColorGradingCustomBuilder::~FColorGradingCustomBuilder()
{
	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().RemoveAll(this);
	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().RemoveAll(this);
	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().RemoveAll(ColorGradingPickerWidget.Pin().Get());
	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().RemoveAll(ColorGradingPickerWidget.Pin().Get());

	if (ColorGradingPickerWidget.IsValid())
	{
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().RemoveAll(this);
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().RemoveAll(this);
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().RemoveAll(ColorGradingPickerWidget.Pin().Get());
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().RemoveAll(ColorGradingPickerWidget.Pin().Get());
	}

	OnNumericEntryBoxDynamicSliderMaxValueChanged.RemoveAll(&Vector4Customization.Get());
	OnNumericEntryBoxDynamicSliderMinValueChanged.RemoveAll(&Vector4Customization.Get());
	OnNumericEntryBoxDynamicSliderMaxValueChanged.RemoveAll(ColorGradingPickerWidget.Pin().Get());
	OnNumericEntryBoxDynamicSliderMinValueChanged.RemoveAll(ColorGradingPickerWidget.Pin().Get());
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
	const FString& ClampMinString = Property->GetMetaData(TEXT("ClampMin"));
	const FString& ClampMaxString = Property->GetMetaData(TEXT("ClampMax"));
	const FString& DeltaString = Property->GetMetaData(TEXT("Delta"));
	const FString& ShiftMouseMovePixelPerDeltaString = Property->GetMetaData(TEXT("ShiftMouseMovePixelPerDelta"));
	const FString& SupportDynamicSliderMaxValueString = Property->GetMetaData(TEXT("SupportDynamicSliderMaxValue"));
	const FString& SupportDynamicSliderMinValueString = Property->GetMetaData(TEXT("SupportDynamicSliderMinValue"));
	const FString& ColorGradingModeString = Property->GetMetaData(TEXT("ColorGradingMode"));

	float ClampMin = TNumericLimits<float>::Lowest();
	float ClampMax = TNumericLimits<float>::Max();

	if (!ClampMinString.IsEmpty())
	{
		TTypeFromString<float>::FromString(ClampMin, *ClampMinString);
	}

	if (!ClampMaxString.IsEmpty())
	{
		TTypeFromString<float>::FromString(ClampMax, *ClampMaxString);
	}

	float UIMin = TNumericLimits<float>::Lowest();
	float UIMax = TNumericLimits<float>::Max();
	TTypeFromString<float>::FromString(UIMin, *MetaUIMinString);
	TTypeFromString<float>::FromString(UIMax, *MetaUIMaxString);

	TOptional<float> MinValue = ClampMinString.Len() ? ClampMin : TOptional<float>();
	TOptional<float> MaxValue = ClampMaxString.Len() ? ClampMax : TOptional<float>();
	TOptional<float> SliderMinValue = (MetaUIMinString.Len()) ? FMath::Max(UIMin, ClampMin) : TOptional<float>();
	TOptional<float> SliderMaxValue = (MetaUIMaxString.Len()) ? FMath::Min(UIMax, ClampMax) : TOptional<float>();

	float Delta = 0.0f;
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

	bool SupportDynamicSliderMaxValue = SupportDynamicSliderMaxValueString.Len() > 0 && SupportDynamicSliderMaxValueString.ToBool();
	bool SupportDynamicSliderMinValue = SupportDynamicSliderMinValueString.Len() > 0 && SupportDynamicSliderMinValueString.ToBool();

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
				SAssignNew(ColorGradingPickerWidget, SColorGradingPicker)
					.ValueMin(MinValue)
					.ValueMax(MaxValue)
					.SliderValueMin(SliderMinValue)
					.SliderValueMax(SliderMaxValue)
					.MainDelta(Delta)
					.SupportDynamicSliderMaxValue(SupportDynamicSliderMaxValue)
					.SupportDynamicSliderMinValue(SupportDynamicSliderMinValue)
					.MainShiftMouseMovePixelPerDelta(ShiftMouseMovePixelPerDelta)
					.ColorGradingModes(ColorGradingMode)
					.OnColorCommitted(this, &FColorGradingCustomBuilder::OnColorGradingPickerChanged)
					.OnQueryCurrentColor(this, &FColorGradingCustomBuilder::GetCurrentColorGradingValue)
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


		TSharedPtr<SNumericEntryBox<float>> NumericEntry = SNew(SNumericEntryBox<float>)
			.EditableTextBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("DarkEditableTextBox"))
			.Value(this, &FColorGradingCustomBuilder::OnSliderGetValue, ChildProperty)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.OnValueChanged(this, &FColorGradingCustomBuilder::OnSliderValueChanged, ChildProperty, false)
			.OnEndSliderMovement(this, &FColorGradingCustomBuilder::OnSliderValueChanged, ChildProperty, true)
			.LabelVAlign(VAlign_Center)
			// Only allow spin on handles with one object.  Otherwise it is not clear what value to spin
			.AllowSpin(ChildProperty.Pin()->GetNumOuterObjects() == 1)
			.UseDarkStyle(true)
			.ColorIndex(ColorIndex)
			.ShiftMouseMovePixelPerDelta(ShiftMouseMovePixelPerDelta)
			.SupportDynamicSliderMaxValue(SupportDynamicSliderMaxValue)
			.SupportDynamicSliderMinValue(SupportDynamicSliderMinValue)
			.OnDynamicSliderMaxValueChanged(this, &FColorGradingCustomBuilder::OnDynamicSliderMaxValueChanged)
			.OnDynamicSliderMinValueChanged(this, &FColorGradingCustomBuilder::OnDynamicSliderMinValueChanged)
			.MinValue(MinValue)
			.MaxValue(MaxValue)
			.MinSliderValue(SliderMinValue)
			.MaxSliderValue(SliderMaxValue)
			.SliderExponent(3.0f)
			.SliderExponentNeutralValue(UIMin + (UIMax - UIMin) / 2.0f)
			.Delta(Delta)
			.Label()
			[
				LabelWidget.ToSharedRef()
			];

		NumericEntryBoxWidgetList.Add(NumericEntry);

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

	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().AddSP(this, &FColorGradingCustomBuilder::OnDynamicSliderMaxValueChanged);
	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().AddSP(this, &FColorGradingCustomBuilder::OnDynamicSliderMinValueChanged);
	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().AddSP(ColorGradingPickerWidget.Pin().Get(), &SColorGradingPicker::OnDynamicSliderMaxValueChanged);
	Vector4Customization->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().AddSP(ColorGradingPickerWidget.Pin().Get(), &SColorGradingPicker::OnDynamicSliderMinValueChanged);

	if (ColorGradingPickerWidget.IsValid())
	{
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().AddSP(Vector4Customization, &FMathStructCustomization::OnDynamicSliderMaxValueChanged);
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().AddSP(Vector4Customization, &FMathStructCustomization::OnDynamicSliderMinValueChanged);
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMaxValueChangedDelegate().AddSP(this, &FColorGradingCustomBuilder::OnDynamicSliderMaxValueChanged);
		ColorGradingPickerWidget.Pin()->GetOnNumericEntryBoxDynamicSliderMinValueChangedDelegate().AddSP(this, &FColorGradingCustomBuilder::OnDynamicSliderMinValueChanged);
	}

	OnNumericEntryBoxDynamicSliderMaxValueChanged.AddSP(Vector4Customization, &FMathStructCustomization::OnDynamicSliderMaxValueChanged);
	OnNumericEntryBoxDynamicSliderMinValueChanged.AddSP(Vector4Customization, &FMathStructCustomization::OnDynamicSliderMinValueChanged);
	OnNumericEntryBoxDynamicSliderMaxValueChanged.AddSP(ColorGradingPickerWidget.Pin().Get(), &SColorGradingPicker::OnDynamicSliderMaxValueChanged);
	OnNumericEntryBoxDynamicSliderMinValueChanged.AddSP(ColorGradingPickerWidget.Pin().Get(), &SColorGradingPicker::OnDynamicSliderMinValueChanged);

	// Find the highest current value and propagate it to all others so they all matches
	float BestMaxSliderValue = 0.0f;
	float BestMinSliderValue = 0.0f;

	for (TWeakPtr<SWidget>& Widget : NumericEntryBoxWidgetList)
	{
		TSharedPtr<SNumericEntryBox<float>> NumericBox = StaticCastSharedPtr<SNumericEntryBox<float>>(Widget.Pin());

		if (NumericBox.IsValid())
		{
			TSharedPtr<SSpinBox<float>> SpinBox = StaticCastSharedPtr<SSpinBox<float>>(NumericBox->GetSpinBox());

			if (SpinBox.IsValid())
			{
				if (SpinBox->GetMaxSliderValue() > BestMaxSliderValue)
				{
					BestMaxSliderValue = SpinBox->GetMaxSliderValue();
				}

				if (SpinBox->GetMinSliderValue() < BestMinSliderValue)
				{
					BestMinSliderValue = SpinBox->GetMinSliderValue();
				}
			}
		}
	}

	OnDynamicSliderMaxValueChanged(BestMaxSliderValue, nullptr, true, true);
	OnDynamicSliderMinValueChanged(BestMinSliderValue, nullptr, true, true);
}

void FColorGradingCustomBuilder::OnDynamicSliderMaxValueChanged(float NewMaxSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfHigher)
{
	for (TWeakPtr<SWidget>& Widget : NumericEntryBoxWidgetList)
	{
		TSharedPtr<SNumericEntryBox<float>> NumericBox = StaticCastSharedPtr<SNumericEntryBox<float>>(Widget.Pin());

		if (NumericBox.IsValid())
		{
			TSharedPtr<SSpinBox<float>> SpinBox = StaticCastSharedPtr<SSpinBox<float>>(NumericBox->GetSpinBox());

			if (SpinBox.IsValid())
			{
				if (SpinBox != InValueChangedSourceWidget)
				{
					if ((NewMaxSliderValue > SpinBox->GetMaxSliderValue() && UpdateOnlyIfHigher) || !UpdateOnlyIfHigher)
					{
						SpinBox->SetMaxSliderValue(NewMaxSliderValue);
					}
				}
			}
		}
	}

	if (IsOriginator)
	{
		OnNumericEntryBoxDynamicSliderMaxValueChanged.Broadcast(NewMaxSliderValue, InValueChangedSourceWidget, false, UpdateOnlyIfHigher);
	}
}

void FColorGradingCustomBuilder::OnDynamicSliderMinValueChanged(float NewMinSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfLower)
{
	for (TWeakPtr<SWidget>& Widget : NumericEntryBoxWidgetList)
	{
		TSharedPtr<SNumericEntryBox<float>> NumericBox = StaticCastSharedPtr<SNumericEntryBox<float>>(Widget.Pin());

		if (NumericBox.IsValid())
		{
			TSharedPtr<SSpinBox<float>> SpinBox = StaticCastSharedPtr<SSpinBox<float>>(NumericBox->GetSpinBox());

			if (SpinBox.IsValid())
			{
				if (SpinBox != InValueChangedSourceWidget)
				{
					if ((NewMinSliderValue < SpinBox->GetMinSliderValue() && UpdateOnlyIfLower) || !UpdateOnlyIfLower)
					{
						SpinBox->SetMinSliderValue(NewMinSliderValue);
					}
				}
			}
		}
	}

	if (IsOriginator)
	{
		OnNumericEntryBoxDynamicSliderMinValueChanged.Broadcast(NewMinSliderValue, InValueChangedSourceWidget, false, UpdateOnlyIfLower);
	}
}

void FColorGradingCustomBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
}

void FColorGradingCustomBuilder::OnSliderValueChanged(float NewValue, TWeakPtr<IPropertyHandle> WeakHandlePtr, bool ShouldCommitValueChanges)
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
		ColorGradingDelegates.OnColorGradingChanged.Execute(ValueVector, ColorGradingPropertyHandle, ShouldCommitValueChanges);
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
	return TOptional<float>();
}

void FColorGradingCustomBuilder::OnColorGradingPickerChanged(FVector4 &NewValue, bool ShouldCommitValueChanges)
{
	if (!ColorGradingDelegates.OnColorGradingChanged.IsBound())
	{
		return;
	}
	ColorGradingDelegates.OnColorGradingChanged.Execute(NewValue, ColorGradingPropertyHandle, ShouldCommitValueChanges);
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
