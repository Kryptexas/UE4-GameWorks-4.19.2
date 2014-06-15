// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UCheckBox

UCheckBox::UCheckBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SCheckBox::FArguments CheckBoxDefaults;

	bIsChecked = false;

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
		.IsChecked( BIND_UOBJECT_ATTRIBUTE(ESlateCheckBoxState::Type, GetCheckState) )
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
	return MyCheckbox->IsPressed();
}

bool UCheckBox::IsChecked() const
{
	return MyCheckbox->IsChecked();
}

ESlateCheckBoxState::Type UCheckBox::GetCheckState() const
{
	return bIsChecked ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
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
		bIsChecked = bWantsToBeChecked;
	}
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
