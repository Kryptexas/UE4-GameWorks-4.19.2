// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FNavLinkStructCustomization : public IStructCustomization
{
public:
	// Begin IStructCustomization interface
	virtual void CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	// End IStructCustomization interface

	static TSharedRef<IStructCustomization> MakeInstance();
};
