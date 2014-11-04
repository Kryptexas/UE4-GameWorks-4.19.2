// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemEditorPrivatePCH.h"
#include "GameplayEffectExecutionScopedModifierInfoDetails.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "GameplayEffect.h"

#define LOCTEXT_NAMESPACE "GameplayEffectExecutionScopedModifierInfoDetailsCustomization"

TSharedRef<IPropertyTypeCustomization> FGameplayEffectExecutionScopedModifierInfoDetails::MakeInstance()
{
	return MakeShareable(new FGameplayEffectExecutionScopedModifierInfoDetails());
}

void FGameplayEffectExecutionScopedModifierInfoDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	];
}

void FGameplayEffectExecutionScopedModifierInfoDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// todo: Implement me!
}

#undef LOCTEXT_NAMESPACE