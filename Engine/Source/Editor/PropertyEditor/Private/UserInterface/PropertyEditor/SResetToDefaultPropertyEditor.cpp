// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "UserInterface/PropertyEditor/SResetToDefaultPropertyEditor.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "ResetToDefaultPropertyEditor"

void SResetToDefaultPropertyEditor::Construct( const FArguments& InArgs, const TSharedPtr<IPropertyHandle>& InPropertyHandle )
{
	PropertyHandle = InPropertyHandle;
	NonVisibleState = InArgs._NonVisibleState;
	bValueDiffersFromDefault = false;
	OptionalCustomResetToDefault = InArgs._CustomResetToDefault;

	InPropertyHandle->MarkResetToDefaultCustomized();

	// Indicator for a value that differs from default. Also offers the option to reset to default.
	ChildSlot
	[
		SNew(SButton)
		.IsFocusable(false)
		.ToolTipText(this, &SResetToDefaultPropertyEditor::GetResetToolTip)
		.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
		.ContentPadding(0) 
		.Visibility( this, &SResetToDefaultPropertyEditor::GetDiffersFromDefaultAsVisibility )
		.OnClicked( this, &SResetToDefaultPropertyEditor::OnResetClicked )
		.Content()
		[
			SNew(SImage)
			.Image( FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault") )
		]
	];

	UpdateDiffersFromDefaultState();
}

void SResetToDefaultPropertyEditor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	UpdateDiffersFromDefaultState();
}

FText SResetToDefaultPropertyEditor::GetResetToolTip() const
{
	FString Tooltip;
	Tooltip = NSLOCTEXT("PropertyEditor", "ResetToDefaultToolTip", "Reset to Default").ToString();

	if( PropertyHandle.IsValid() && !PropertyHandle->IsEditConst() && PropertyHandle->DiffersFromDefault() )
	{
		FString DefaultLabel = PropertyHandle->GetResetToDefaultLabel().ToString();

		if (DefaultLabel.Len() > 0)
		{
			Tooltip += "\n";
			Tooltip += DefaultLabel;
		}
	}

	return FText::FromString(Tooltip);
}

FReply SResetToDefaultPropertyEditor::OnResetClicked()
{
	if (PropertyHandle.IsValid())
	{
		if (OptionalCustomResetToDefault.IsSet())
		{
			PropertyHandle->ExecuteCustomResetToDefault(OptionalCustomResetToDefault.GetValue());
		}
		else
		{
			PropertyHandle->ResetToDefault();
		}
	}
	return FReply::Handled();
}

void SResetToDefaultPropertyEditor::UpdateDiffersFromDefaultState()
{
	if (PropertyHandle.IsValid())
	{
		if (OptionalCustomResetToDefault.IsSet())
		{
			bValueDiffersFromDefault = OptionalCustomResetToDefault.GetValue().IsResetToDefaultVisible(PropertyHandle.ToSharedRef());
		}
		else
		{
			bValueDiffersFromDefault = PropertyHandle->CanResetToDefault();
		}
	}
}

EVisibility SResetToDefaultPropertyEditor::GetDiffersFromDefaultAsVisibility() const
{
	return bValueDiffersFromDefault ? EVisibility::Visible : NonVisibleState;
}

#undef LOCTEXT_NAMESPACE
