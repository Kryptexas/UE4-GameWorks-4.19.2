// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"

class IPropertyHandle;
class FDetailWidgetRow;
class IDetailChildrenBuilder;

class FGameplayModifierInfoCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	
	void OnModifierOpChanged(TSharedRef<IPropertyHandle> StructPropertyHandle);

	EVisibility GetPropertyVisibility(FName PropName) const;

	void UpdateHiddenProperties(const TSharedRef<IPropertyHandle>& StructPropertyHandle);

private:
	TSet<FName> HiddenProperties;
};