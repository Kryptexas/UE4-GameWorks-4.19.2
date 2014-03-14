// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

class FEnvDirectionCustomization : public IStructCustomization
{
public:

	// Begin IStructCustomization interface
	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	// End IStructCustomization interface

	static TSharedRef<IStructCustomization> MakeInstance( );

protected:

	TSharedPtr<IPropertyHandle> ModeProp;
	bool bIsRotation;

	FString GetShortDescription() const;
	EVisibility GetTwoPointsVisibility() const;
	EVisibility GetRotationVisibility() const;
	void OnModeChanged();
};
