// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemEditorPrivatePCH.h"
#include "GameplayModifierInfoDetails.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

#define LOCTEXT_NAMESPACE "GameplayModifierInfoCustomization"

TSharedRef<IPropertyTypeCustomization> FGameplayModifierInfoCustomization::MakeInstance()
{
	return MakeShareable(new FGameplayModifierInfoCustomization());
}

void FGameplayModifierInfoCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	];
}

void FGameplayModifierInfoCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	UpdateHiddenProperties(StructPropertyHandle);

	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		const TSharedRef< IPropertyHandle > ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();

		// Only add this property if it wasn't already marked hidden in GameplayEffectDetails
		if ( !ChildHandle->IsCustomized() )
		{
			const FName PropName = ChildHandle->GetProperty()->GetFName();

			if ( PropName == GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, ModifierOp) )
			{
				ChildHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateSP(this, &FGameplayModifierInfoCustomization::OnModifierOpChanged, StructPropertyHandle) );
			}
		
			StructBuilder.AddChildProperty(ChildHandle)
				.Visibility( TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FGameplayModifierInfoCustomization::GetPropertyVisibility, PropName)) );
		}
	}
}

void FGameplayModifierInfoCustomization::OnModifierOpChanged(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	UpdateHiddenProperties(StructPropertyHandle);
}

EVisibility FGameplayModifierInfoCustomization::GetPropertyVisibility(FName PropName) const
{
	return HiddenProperties.Contains(PropName) ? EVisibility::Collapsed : EVisibility::Visible;
}

void FGameplayModifierInfoCustomization::UpdateHiddenProperties(const TSharedRef<IPropertyHandle>& StructPropertyHandle)
{

}


#undef LOCTEXT_NAMESPACE
