// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSlateColorCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;              

private:

	/**
	 * Called when the value is changed in the property editor
	 */
	virtual void OnValueChanged();

private:

	TSharedPtr<IPropertyHandle> ColorRuleHandle;
	TSharedPtr<IPropertyHandle> SpecifiedColorHandle;
};

