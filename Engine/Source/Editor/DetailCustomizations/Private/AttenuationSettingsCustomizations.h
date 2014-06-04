// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAttenuationSettingsCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization instance */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

protected:

	EVisibility IsSphereSelected() const;
	EVisibility IsBoxSelected() const;
	EVisibility IsCapsuleSelected() const;
	EVisibility IsConeSelected() const;
	EVisibility IsNaturalSoundSelected() const;

	TSharedPtr< IPropertyHandle > AttenuationShapeHandle;
	TSharedPtr< IPropertyHandle > DistanceAlgorithmHandle;
};
