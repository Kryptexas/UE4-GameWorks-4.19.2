// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"

class IPropertyHandle;
class FDetailWidgetRow;
class IDetailChildrenBuilder;

/** Details customization for FGameplayEffectExecutionScopedModifierInfo */
class FGameplayEffectExecutionScopedModifierInfoDetails : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** WIP */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
};