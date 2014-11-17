// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Implements a details panel customization for FKey structures.
 */
class FKeyStructCustomization
	: public IPropertyTypeCustomization
{
public:

	// IPropertyTypeCustomization interface

	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override { };

public:

	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for Keys.
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance( );

private:

	TSharedRef<SWidget> OnGenerateComboWidget(TSharedPtr<FKey> Key);
	void OnSelectionChanged(TSharedPtr<FKey> SelectedItem, ESelectInfo::Type SelectInfo);

	// Holds a handle to the property being edited.
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<STextBlock> TextBlock;

	TArray<TSharedPtr<FKey>> InputKeys;
};
