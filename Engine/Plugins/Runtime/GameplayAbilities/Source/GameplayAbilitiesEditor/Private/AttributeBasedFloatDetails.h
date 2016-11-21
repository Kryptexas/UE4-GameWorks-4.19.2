// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"

class IPropertyHandle;
class FDetailWidgetRow;
class IDetailChildrenBuilder;

/** Details customization for FAttributeBasedFloat */
class FAttributeBasedFloatDetails : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** Overridden to provide the property name or hide, if necessary */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
	/** Overridden to allow for possibly being hidden */
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:

	/** Called via delegate to determine visibility of the final channel property */
	EVisibility GetFinalChannelVisibility() const;

	/** Property handle to the AttributeCalculationType property; Used to determine visibility of final channel property */
	TSharedPtr<IPropertyHandle> AttributeCalculationTypePropertyHandle;
};