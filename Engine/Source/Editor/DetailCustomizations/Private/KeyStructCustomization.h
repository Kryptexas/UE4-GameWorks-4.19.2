// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	KeyStructCustomization.h: Declares the FGuidCustomization class.
=============================================================================*/

#pragma once

/**
 * Implements a details panel customization for FKey structures.
 */
class FKeyStructCustomization
	: public IStructCustomization
{
public:

	// Begin IStructCustomization interface

	virtual void CustomizeStructHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE {};

	// End IStructCustomization interface

	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for Keys.
	 */
	static TSharedRef<IStructCustomization> MakeInstance( );

private:

	TSharedRef<SWidget> OnGenerateComboWidget(TSharedPtr<FKey> Key);
	void OnSelectionChanged(TSharedPtr<FKey> SelectedItem, ESelectInfo::Type SelectInfo);

	// Holds a handle to the property being edited.
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<STextBlock> TextBlock;

	TArray< TSharedPtr<FKey> > InputKeys;
};
