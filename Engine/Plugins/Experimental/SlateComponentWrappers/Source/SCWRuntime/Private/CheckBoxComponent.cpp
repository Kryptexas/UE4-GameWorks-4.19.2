// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// UCheckBoxComponent

UCheckBoxComponent::UCheckBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SCheckBox::FArguments CheckBoxDefaults;

	CheckBoxStyleName = NAME_None;
	bIsChecked = false;

	HorizontalAlignment = CheckBoxDefaults._HAlign;
	Padding = CheckBoxDefaults._Padding.Get();

	ForegroundColor = CheckBoxDefaults._ForegroundColor.Get();
	BorderBackgroundColor = CheckBoxDefaults._BorderBackgroundColor.Get();
}

TSharedRef<SWidget> UCheckBoxComponent::RebuildWidget()
{
	return SNew(SCheckBox)
// 		SLATE_ATTRIBUTE( ESlateCheckBoxType::Type, Type )
// 		SLATE_DEFAULT_SLOT( FArguments, Content )
 		.Style( FEditorStyle::Get(), CheckBoxStyleName )
// 		SLATE_ARGUMENT( const FCheckBoxStyle *, CheckBoxStyle )
 		.OnCheckStateChanged( BIND_UOBJECT_DELEGATE(FOnCheckStateChanged, SlateOnCheckStateChangedCallback) )
 		.IsChecked( BIND_UOBJECT_ATTRIBUTE(ESlateCheckBoxState::Type, GetCheckState) )
		.HAlign( HorizontalAlignment )
 		.Padding( Padding )
		.ForegroundColor( ForegroundColor )
		.BorderBackgroundColor( BorderBackgroundColor );
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
