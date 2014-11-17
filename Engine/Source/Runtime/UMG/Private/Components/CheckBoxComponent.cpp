// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UCheckBoxComponent

UCheckBoxComponent::UCheckBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SCheckBox::FArguments CheckBoxDefaults;

	bIsChecked = false;

	HorizontalAlignment = CheckBoxDefaults._HAlign;
	Padding = CheckBoxDefaults._Padding.Get();

	ForegroundColor = FLinearColor::White;
	BorderBackgroundColor = FLinearColor::White;
}

TSharedRef<SWidget> UCheckBoxComponent::RebuildWidget()
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

	return SNew(SCheckBox)
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
// 		SLATE_ATTRIBUTE( bool, ReadOnly )
// 		SLATE_ARGUMENT( bool, IsFocusable )
// 		SLATE_EVENT( FOnGetContent, OnGetMenuContent )
}

ESlateCheckBoxState::Type UCheckBoxComponent::GetCheckState() const
{
	return bIsChecked ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void UCheckBoxComponent::SlateOnCheckStateChangedCallback(ESlateCheckBoxState::Type NewState)
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
