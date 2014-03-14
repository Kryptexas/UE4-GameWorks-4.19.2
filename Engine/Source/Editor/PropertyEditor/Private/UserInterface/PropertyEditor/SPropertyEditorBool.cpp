// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PropertyEditorPrivatePCH.h"
#include "SPropertyEditorBool.h"
#include "PropertyHandle.h"
#include "PropertyEditor.h"

void SPropertyEditorBool::Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;

	CheckBox = SNew( SCheckBox )
		.OnCheckStateChanged( this, &SPropertyEditorBool::OnCheckStateChanged )
		.IsChecked( this, &SPropertyEditorBool::OnGetCheckState )
		.ForegroundColor( FEditorStyle::GetSlateColor("DefaultForeground") )
		.Padding(0.0f);

	ChildSlot
	[	
		CheckBox.ToSharedRef()
	];

	SetEnabled( TAttribute<bool>( this, &SPropertyEditorBool::CanEdit ) );
}

void SPropertyEditorBool::GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth )
{
	// No desired width
	OutMinDesiredWidth = 0;
	OutMaxDesiredWidth = 0;
}

bool SPropertyEditorBool::Supports( const TSharedRef< class FPropertyEditor >& PropertyEditor )
{
	return PropertyEditor->PropertyIsA( UBoolProperty::StaticClass() );
}

bool SPropertyEditorBool::SupportsKeyboardFocus() const
{
	return CheckBox->SupportsKeyboardFocus();
}

bool SPropertyEditorBool::HasKeyboardFocus() const
{
	return CheckBox->HasKeyboardFocus();
}

FReply SPropertyEditorBool::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	// Forward keyboard focus to our editable text widget
	return FReply::Handled().SetKeyboardFocus( CheckBox.ToSharedRef(), InKeyboardFocusEvent.GetCause() );
}

ESlateCheckBoxState::Type SPropertyEditorBool::OnGetCheckState() const
{
	ESlateCheckBoxState::Type ReturnState = ESlateCheckBoxState::Undetermined;

	bool Value;
	const TSharedRef< IPropertyHandle > PropertyHandle = PropertyEditor->GetPropertyHandle();
	if( PropertyHandle->GetValue( Value ) == FPropertyAccess::Success )
	{
		if( Value == true )
		{
			ReturnState = ESlateCheckBoxState::Checked;
		}
		else if( Value == false )
		{
			ReturnState = ESlateCheckBoxState::Unchecked;
		}
	}

	return ReturnState;
}

void SPropertyEditorBool::OnCheckStateChanged( ESlateCheckBoxState::Type InNewState )
{
	const TSharedRef< IPropertyHandle > PropertyHandle = PropertyEditor->GetPropertyHandle();
	if( InNewState == ESlateCheckBoxState::Checked || InNewState == ESlateCheckBoxState::Undetermined )
	{
		PropertyHandle->SetValue( true );
	}
	else
	{
		PropertyHandle->SetValue( false );
	}
}

FReply SPropertyEditorBool::OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	if ( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		//toggle the our checkbox
		CheckBox->ToggleCheckedState();

		// Set focus to this object, but don't capture the mouse
		return FReply::Handled().SetKeyboardFocus( AsShared(), EKeyboardFocusCause::Mouse );
	}

	return FReply::Unhandled();
}

bool SPropertyEditorBool::CanEdit() const
{
	return PropertyEditor.IsValid() ? !PropertyEditor->IsEditConst() : true;
}
