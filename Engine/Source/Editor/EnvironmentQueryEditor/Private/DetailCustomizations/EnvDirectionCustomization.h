// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

class FEnvDirectionCustomization : public IPropertyTypeCustomization
{
public:

	// Begin IPropertyTypeCustomization interface
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	// End IPropertyTypeCustomization interface

	static TSharedRef<IPropertyTypeCustomization> MakeInstance( );

protected:

	TSharedPtr<IPropertyHandle> ModeProp;
	bool bIsRotation;

	FString GetShortDescription() const;
	EVisibility GetTwoPointsVisibility() const;
	EVisibility GetRotationVisibility() const;
	void OnModeChanged();
};
