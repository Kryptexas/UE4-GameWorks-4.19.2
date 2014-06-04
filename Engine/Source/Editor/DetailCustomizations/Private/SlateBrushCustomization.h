// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


class FSlateBrushStructCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;              

private:
	/**
	 * Get the Slate Brush tiling property row visibility
	 */
	EVisibility GetTilingPropertyVisibility() const;

	/**
	 *  Get the Slate Brush margin property row visibility
	 */
	EVisibility GetMarginPropertyVisibility() const;

	/** Slate Brush DrawAs property */
	TSharedPtr<IPropertyHandle> DrawAsProperty;
};

