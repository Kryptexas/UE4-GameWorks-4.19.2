// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GuidStructCustomization.h: Declares the FGuidCustomization class.
=============================================================================*/

#pragma once


namespace EPropertyEditorGuidActions
{
	/**
	 * Enumerates quick-set action types.
	 */
	enum Type
	{
		// Generate a new GUID.
		Generate,

		// Set a null GUID.
		Invalidate
	};
}


/**
 * Implements a details panel customization for  FGuid structures.
 */
class FGuidStructCustomization
	: public IStructCustomization
{
public:

	// Begin IStructCustomization interface

	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

	// End IStructCustomization interface


public:

	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for Guids.
	 */
	static TSharedRef<IStructCustomization> MakeInstance( );


protected:

	/**
	 * Sets the property's value.
	 *
	 * @param Guid - The value to set.
	 */
	void SetGuidValue( const FGuid& Guid );


private:

	// Callback for clicking an item in the quick-set menu.
	void HandleGuidActionClicked( EPropertyEditorGuidActions::Type Action );

	// Handles getting the text color of the editable text box.
	FSlateColor HandleTextBoxForegroundColor( ) const;

	// Handles getting the text to be displayed in the editable text box.
	FText HandleTextBoxText( ) const;

	// Handles changing the value in the editable text box.
	void HandleTextBoxTextChanged( const FText& NewText );

	// Handles committing the text in the editable text box.
	void HandleTextBoxTextCommited( const FText& NewText, ETextCommit::Type CommitInfo );


private:

	// Holds a flag indicating whether the current input is a valid GUID.
	bool InputValid;

	// Holds a handle to the property being edited.
	TSharedPtr<IPropertyHandle> PropertyHandle;

	// Holds the text box for editing the Guid.
	TSharedPtr<SEditableTextBox> TextBox;
};
