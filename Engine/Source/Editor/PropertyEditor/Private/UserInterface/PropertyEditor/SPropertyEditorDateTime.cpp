// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PropertyEditorPrivatePCH.h"
#include "SPropertyEditorDateTime.h"
#include "PropertyNode.h"
#include "ObjectPropertyNode.h"
#include "PropertyEditor.h"
#include "PropertyEditorHelpers.h"


void SPropertyEditorDateTime::Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;

	ChildSlot
	[
		SAssignNew( PrimaryWidget, SEditableTextBox )
		.Text( InPropertyEditor, &FPropertyEditor::GetValueAsText )
		.Font( InArgs._Font )
		.IsReadOnly(InPropertyEditor->IsEditConst())
	];

	if( InPropertyEditor->PropertyIsA( UObjectPropertyBase::StaticClass() ) )
	{
		// Object properties should display their entire text in a tooltip
		PrimaryWidget->SetToolTipText( TAttribute<FString>( InPropertyEditor, &FPropertyEditor::GetValueAsString ) );
	}
}

bool SPropertyEditorDateTime::Supports( const TSharedRef< FPropertyEditor >& InPropertyEditor )
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	const UProperty* Property = InPropertyEditor->GetProperty();

	if (Property->IsA(UStructProperty::StaticClass()))
	{
		const UStructProperty* StructProp = Cast<const UStructProperty>(Property);
		extern UScriptStruct* Z_Construct_UScriptStruct_UObject_FDateTime();	// It'd be really nice if StaticStruct() worked on types declared in Object.h
		if (Z_Construct_UScriptStruct_UObject_FDateTime() == StructProp->Struct)
		{
			return true;
		}
	}

	return false;
}