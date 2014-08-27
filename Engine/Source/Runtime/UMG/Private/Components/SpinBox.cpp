// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// USpinBox

USpinBox::USpinBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// HACK Special font initialization hack since there are no font assets yet for slate.
	Font = FSlateFontInfo(TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12);

	// Grab other defaults from slate arguments.
	SSpinBox<float>::FArguments Defaults;

	Value = Defaults._Value.Get();
	MinValue = Defaults._MinValue.Get().GetValue();
	MaxValue = Defaults._MaxValue.Get().GetValue();
	MinSliderValue = Defaults._MinSliderValue.Get().GetValue();
	MaxSliderValue = Defaults._MaxSliderValue.Get().GetValue();
	Delta = Defaults._Delta.Get();
	SliderExponent = Defaults._SliderExponent.Get();
	MinDesiredWidth = Defaults._MinDesiredWidth.Get();
	ClearKeyboardFocusOnCommit = Defaults._ClearKeyboardFocusOnCommit.Get();
	SelectAllTextOnCommit = Defaults._SelectAllTextOnCommit.Get();
}

void USpinBox::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MySpinBox.Reset();
}

TSharedRef<SWidget> USpinBox::RebuildWidget()
{
	FString FontPath = FPaths::GameContentDir() / Font.FontName.ToString();

	if ( !FPaths::FileExists(FontPath) )
	{
		FontPath = FPaths::EngineContentDir() / Font.FontName.ToString();
	}
	
	SSpinBox<float>::FArguments Defaults;
	
	const FSpinBoxStyle* StylePtr = (Style != nullptr) ? Style->GetStyle<FSpinBoxStyle>() : nullptr;
	if ( StylePtr == nullptr )
	{
		StylePtr = Defaults._Style;
	}
	
	MySpinBox = SNew(SSpinBox<float>)
	.Style(StylePtr)
	.Font(FSlateFontInfo(FontPath, Font.Size))
	.ClearKeyboardFocusOnCommit(ClearKeyboardFocusOnCommit)
	.SelectAllTextOnCommit(SelectAllTextOnCommit)
	.OnValueChanged(BIND_UOBJECT_DELEGATE(FOnFloatValueChanged, HandleOnValueChanged))
	.OnValueCommitted(BIND_UOBJECT_DELEGATE(FOnFloatValueCommitted, HandleOnValueCommitted))
	.OnBeginSliderMovement(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnBeginSliderMovement))
	.OnEndSliderMovement(BIND_UOBJECT_DELEGATE(FOnFloatValueChanged, HandleOnEndSliderMovement))
	;
	
	return BuildDesignTimeWidget( MySpinBox.ToSharedRef() );
}

void USpinBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute<float> ValueBinding = OPTIONAL_BINDING(float, Value);
	MySpinBox->SetValue(ValueBinding);
	
	MySpinBox->SetDelta(Delta);
	MySpinBox->SetSliderExponent(SliderExponent);
	MySpinBox->SetMinDesiredWidth(MinDesiredWidth);

	// Set optional values
	bOverride_MinValue ? SetMinValue(MinValue) : ClearMinValue();
	bOverride_MaxValue ? SetMaxValue(MaxValue) : ClearMaxValue();
	bOverride_MinSliderValue ? SetMinSliderValue(MinSliderValue) : ClearMinSliderValue();
	bOverride_MaxSliderValue ? SetMaxSliderValue(MaxSliderValue) : ClearMaxSliderValue();
}

float USpinBox::GetValue()
{
	if (MySpinBox.IsValid())
	{
		return MySpinBox->GetValue();
	}

	return Value;
}

void USpinBox::SetValue(float InValue)
{
	Value = InValue;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetValue(InValue);
	}
}

// MIN VALUE
float USpinBox::GetMinValue()
{
	float ReturnVal = TNumericLimits<float>::Lowest();

	if (MySpinBox.IsValid())
	{
		ReturnVal = MySpinBox->GetMinValue();
	}
	else if (bOverride_MinValue)
	{
		ReturnVal = MinValue;
	}

	return ReturnVal;
}

void USpinBox::SetMinValue(float InMinValue)
{
	bOverride_MinValue = true;
	MinValue = InMinValue;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMinValue(InMinValue);
	}
}

void USpinBox::ClearMinValue()
{
	bOverride_MinValue = false;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMinValue(TOptional<float>());
	}
}

// MAX VALUE
float USpinBox::GetMaxValue()
{
	float ReturnVal = TNumericLimits<float>::Max();

	if (MySpinBox.IsValid())
	{
		ReturnVal = MySpinBox->GetMaxValue();
	}
	else if (bOverride_MaxValue)
	{
		ReturnVal = MaxValue;
	}

	return ReturnVal;
}

void USpinBox::SetMaxValue(float InMaxValue)
{
	bOverride_MaxValue = true;
	MaxValue = InMaxValue;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMaxValue(InMaxValue);
	}
}
void USpinBox::ClearMaxValue()
{
	bOverride_MaxValue = false;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMaxValue(TOptional<float>());
	}
}

// MIN SLIDER VALUE
float USpinBox::GetMinSliderValue()
{
	float ReturnVal = TNumericLimits<float>::Min();

	if (MySpinBox.IsValid())
	{
		ReturnVal = MySpinBox->GetMinSliderValue();
	}
	else if (bOverride_MinSliderValue)
	{
		ReturnVal = MinSliderValue;
	}

	return ReturnVal;
}

void USpinBox::SetMinSliderValue(float InMinSliderValue)
{
	bOverride_MinSliderValue = true;
	MinSliderValue = InMinSliderValue;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMinSliderValue(InMinSliderValue);
	}
}

void USpinBox::ClearMinSliderValue()
{
	bOverride_MinSliderValue = false;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMinSliderValue(TOptional<float>());
	}
}

// MAX SLIDER VALUE
float USpinBox::GetMaxSliderValue()
{
	float ReturnVal = TNumericLimits<float>::Max();

	if (MySpinBox.IsValid())
	{
		ReturnVal = MySpinBox->GetMaxSliderValue();
	}
	else if (bOverride_MaxSliderValue)
	{
		ReturnVal = MaxSliderValue;
	}

	return ReturnVal;
}

void USpinBox::SetMaxSliderValue(float InMaxSliderValue)
{
	bOverride_MaxSliderValue = true;
	MaxSliderValue = InMaxSliderValue;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMaxSliderValue(InMaxSliderValue);
	}
}

void USpinBox::ClearMaxSliderValue()
{
	bOverride_MaxSliderValue = false;
	if (MySpinBox.IsValid())
	{
		MySpinBox->SetMaxSliderValue(TOptional<float>());
	}
}

// Event handlers
void USpinBox::HandleOnValueChanged(float InValue)
{
	OnValueChanged.Broadcast(InValue);
}

void USpinBox::HandleOnValueCommitted(float InValue, ETextCommit::Type CommitMethod)
{
	OnValueCommitted.Broadcast(InValue, CommitMethod);
}

void USpinBox::HandleOnBeginSliderMovement()
{
	OnBeginSliderMovement.Broadcast();
}

void USpinBox::HandleOnEndSliderMovement(float InValue)
{
	OnEndSliderMovement.Broadcast(InValue);
}

#if WITH_EDITOR

const FSlateBrush* USpinBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.SpinBox");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE