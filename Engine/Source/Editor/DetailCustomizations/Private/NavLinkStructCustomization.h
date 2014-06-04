// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FNavLinkStructCustomization : public IPropertyTypeCustomization
{
public:
	// Begin IPropertyTypeCustomization interface
	virtual void CustomizeHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	// End IPropertyTypeCustomization interface

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
};
