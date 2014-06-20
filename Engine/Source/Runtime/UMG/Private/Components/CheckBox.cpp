// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UCheckBox

UCheckBox::UCheckBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SCheckBox::FArguments CheckBoxDefaults;

	CheckedState = ESlateCheckBoxState::Unchecked;

	HorizontalAlignment = CheckBoxDefaults._HAlign;
	Padding = CheckBoxDefaults._Padding.Get();

	ForegroundColor = FLinearColor::White;
	BorderBackgroundColor = FLinearColor::White;
}

TSharedRef<SWidget> UCheckBox::RebuildWidget()
{
	const FCheckBoxStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FCheckBoxStyle>() : NULL;
	if ( StylePtr == NULL )
	{
		SCheckBox::FArguments Defaults;
		StylePtr = Defaults._Style;
	}

	TOptional<FSlateSound> OptionalCheckedSound;
	if ( CheckedSound.GetResourceObject() )
	{
		OptionalCheckedSound = CheckedSound;
	}

	TOptional<FSlateSound> OptionalUncheckedSound;
	if ( UncheckedSound.GetResourceObject() )
	{
		OptionalUncheckedSound = UncheckedSound;
	}

	TOptional<FSlateSound> OptionalHoveredSound;
	if ( HoveredSound.GetResourceObject() )
	{
		OptionalHoveredSound = HoveredSound;
	}

	MyCheckbox = SNew(SCheckBox)
		.Style(StylePtr)
		.OnCheckStateChanged( BIND_UOBJECT_DELEGATE(FOnCheckStateChanged, SlateOnCheckStateChangedCallback) )
		.HAlign( HorizontalAlignment )
		.Padding( Padding )
		.ForegroundColor( ForegroundColor )
		.BorderBackgroundColor( BorderBackgroundColor )
		.CheckedSoundOverride(OptionalCheckedSound)
		.UncheckedSoundOverride(OptionalUncheckedSound)
		.HoveredSoundOverride(OptionalHoveredSound)
		;
	
	return MyCheckbox.ToSharedRef();
}

void UCheckBox::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	MyCheckbox->SetIsChecked( OPTIONAL_BINDING(ESlateCheckBoxState::Type, CheckedState) );
	
	MyCheckbox->SetUncheckedImage(UncheckedImage ? &UncheckedImage->Brush : nullptr);
	MyCheckbox->SetUncheckedHoveredImage(UncheckedHoveredImage ? &UncheckedHoveredImage->Brush : nullptr);
	MyCheckbox->SetUncheckedPressedImage(UncheckedPressedImage ? &UncheckedPressedImage->Brush : nullptr);
	
	MyCheckbox->SetCheckedImage(CheckedImage ? &CheckedImage->Brush : nullptr);
	MyCheckbox->SetCheckedHoveredImage(CheckedHoveredImage ? &CheckedHoveredImage->Brush : nullptr);
	MyCheckbox->SetCheckedPressedImage(CheckedPressedImage ? &CheckedPressedImage->Brush : nullptr);
	
	MyCheckbox->SetUndeterminedImage(UndeterminedImage ? &UndeterminedImage->Brush : nullptr);
	MyCheckbox->SetUndeterminedHoveredImage(UndeterminedHoveredImage ? &UndeterminedHoveredImage->Brush : nullptr);
	MyCheckbox->SetUndeterminedPressedImage(UndeterminedPressedImage ? &UndeterminedPressedImage->Brush : nullptr);
}

bool UCheckBox::IsPressed() const
{
	if ( MyCheckbox.IsValid() )
	{
		return MyCheckbox->IsPressed();
	}

	return false;
}

bool UCheckBox::IsChecked() const
{
	if ( MyCheckbox.IsValid() )
	{
		return MyCheckbox->IsChecked();
	}

	return ( CheckedState == ESlateCheckBoxState::Checked );
}

ESlateCheckBoxState::Type UCheckBox::GetCheckedState() const
{
	if ( MyCheckbox.IsValid() )
	{
		return MyCheckbox->GetCheckedState();
	}

	return CheckedState;
}

void UCheckBox::SetIsChecked(bool InIsChecked)
{
	CheckedState = InIsChecked ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	if ( MyCheckbox.IsValid() )
	{
		MyCheckbox->SetIsChecked(OPTIONAL_BINDING(ESlateCheckBoxState::Type, CheckedState));
	}
}

void UCheckBox::SetCheckedState(ESlateCheckBoxState::Type InCheckedState)
{
	CheckedState = InCheckedState;
	if ( MyCheckbox.IsValid() )
	{
		MyCheckbox->SetIsChecked(OPTIONAL_BINDING(ESlateCheckBoxState::Type, CheckedState));
	}
}

void UCheckBox::SlateOnCheckStateChangedCallback(ESlateCheckBoxState::Type NewState)
{
	//@TODO: Choosing to treat Undetermined as Checked
	const bool bWantsToBeChecked = NewState != ESlateCheckBoxState::Unchecked;

	if (OnCheckStateChanged.IsBound())
	{
		OnCheckStateChanged.Broadcast(bWantsToBeChecked);
	}
	else
	{
		CheckedState = NewState;
	}
}

#if WITH_EDITOR

const FSlateBrush* UCheckBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.CheckBox");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
