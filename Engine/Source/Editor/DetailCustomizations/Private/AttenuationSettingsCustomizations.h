// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class FBaseAttenuationSettingsCustomization : public IPropertyTypeCustomization
{
public:
	/** IPropertyTypeCustomization instance */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	virtual void CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;

protected:

	EVisibility IsSphereSelected() const;
	EVisibility IsBoxSelected() const;
	EVisibility IsCapsuleSelected() const;
	EVisibility IsConeSelected() const;
	EVisibility IsNaturalSoundSelected() const;
	EVisibility IsCustomCurveSelected() const;

	TSharedPtr< IPropertyHandle > AttenuationShapeHandle;
	TSharedPtr< IPropertyHandle > DistanceAlgorithmHandle;
};

class FSoundAttenuationSettingsCustomization : public FBaseAttenuationSettingsCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;

protected:

	TSharedPtr< IPropertyHandle > bIsSpatializedHandle;
	TSharedPtr< IPropertyHandle > bIsFocusedHandle;
	TSharedPtr< IPropertyHandle > bIsOcclusionEnabledHandle;

	bool IsFocusedEnabled() const;
	TAttribute<bool> GetIsFocusEnabledAttribute() const;

	bool IsOcclusionEnabled() const;
	TAttribute<bool> GetIsOcclusionEnabledAttribute() const;

	TAttribute<bool> IsFocusEnabledAttribute;
};

class FForceFeedbackAttenuationSettingsCustomization : public FBaseAttenuationSettingsCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
};