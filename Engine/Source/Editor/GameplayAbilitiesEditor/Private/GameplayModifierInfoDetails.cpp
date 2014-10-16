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

			if ( PropName == GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, ModifierType) )
			{
				ChildHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateSP(this, &FGameplayModifierInfoCustomization::OnModifierTypeChanged, StructPropertyHandle) );
			}

			if ( PropName == GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, EffectType) )
			{
				ChildHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateSP(this, &FGameplayModifierInfoCustomization::OnEffectTypeChanged, StructPropertyHandle) );
			}

			if ( PropName == GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, ModifierOp) )
			{
				ChildHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateSP(this, &FGameplayModifierInfoCustomization::OnModifierOpChanged, StructPropertyHandle) );
			}
		
			StructBuilder.AddChildProperty(ChildHandle)
				.Visibility( TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FGameplayModifierInfoCustomization::GetPropertyVisibility, PropName)) );
		}
	}
}

void FGameplayModifierInfoCustomization::OnModifierTypeChanged(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	UpdateHiddenProperties(StructPropertyHandle);
}

void FGameplayModifierInfoCustomization::OnEffectTypeChanged(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	UpdateHiddenProperties(StructPropertyHandle);
}

void FGameplayModifierInfoCustomization::OnModifierOpChanged(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	// When changing to a callback op, automatically add an initial entry in the callbacks list.
	// When changing back to a non-callback op, clear the entry if it is the only one and has no data (just to be clean)
	TArray<void*> StructPtrs;
	StructPropertyHandle->AccessRawData(StructPtrs);
	if (StructPtrs.Num() == 1)
	{
		FGameplayModifierInfo& Modifier = *reinterpret_cast<FGameplayModifierInfo*>(StructPtrs[0]);

		if ( Modifier.ModifierOp == EGameplayModOp::Callback )
		{
			// We switched to Callback. See if we should add an initial callback.
			if ( Modifier.Callbacks.Num() == 0 )
			{
				new(Modifier.Callbacks) FGameplayModifierCallback();
			}
		}
		else
		{
			// We switched away from Callback. See if we only have a single item with no data. If so, remove it.
			if ( Modifier.Callbacks.Num() == 1 )
			{
				const FGameplayModifierCallback& Callback = Modifier.Callbacks[0];
				if ( Callback.ExtensionClass == nullptr && Callback.SourceAttributeModifiers.Num() == 0 && Callback.TargetAttributeModifiers.Num() == 0 )
				{
					Modifier.Callbacks.Empty();
				}
			}
		}
	}

	UpdateHiddenProperties(StructPropertyHandle);
}

EVisibility FGameplayModifierInfoCustomization::GetPropertyVisibility(FName PropName) const
{
	return HiddenProperties.Contains(PropName) ? EVisibility::Collapsed : EVisibility::Visible;
}

void FGameplayModifierInfoCustomization::UpdateHiddenProperties(const TSharedRef<IPropertyHandle>& StructPropertyHandle)
{
	HiddenProperties.Empty();

	TArray<const void*> StructPtrs;
	StructPropertyHandle->AccessRawData(StructPtrs);
	if ( StructPtrs.Num() > 0 )
	{
		const FGameplayModifierInfo& FirstModifier = *reinterpret_cast<const FGameplayModifierInfo*>(StructPtrs[0]);
		EGameplayMod::Type CommonModType = FirstModifier.ModifierType;
		EGameplayModEffect::Type CommonModEffect = FirstModifier.EffectType;
		EGameplayModOp::Type CommonModOp = FirstModifier.ModifierOp;
		bool bAllSameModType = true;
		bool bAllSameModEffect = true;
		bool bAllSameModOp = true;

		for ( const void* Ptr : StructPtrs )
		{
			const FGameplayModifierInfo& Modifier = *reinterpret_cast<const FGameplayModifierInfo*>(Ptr);

			if ( Modifier.ModifierType != CommonModType )
			{
				bAllSameModType = false;
			}

			if ( Modifier.EffectType != CommonModEffect )
			{
				bAllSameModEffect = false;
			}

			if ( Modifier.ModifierOp != CommonModOp )
			{
				bAllSameModOp = false;
			}
		}
		
		if ( bAllSameModType && CommonModType == EGameplayMod::Attribute )
		{
			HiddenProperties.Add(GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, EffectType));
			HiddenProperties.Add(GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, TargetEffect));
		}

		if ( bAllSameModEffect && CommonModEffect != EGameplayModEffect::LinkedGameplayEffect )
		{
			HiddenProperties.Add(GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, TargetEffect));
		}

		if ( bAllSameModOp )
		{
			if ( CommonModOp == EGameplayModOp::Callback )
			{
				HiddenProperties.Add(GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, Magnitude));
				HiddenProperties.Add(GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, LevelInfo));
			}
			else
			{
				HiddenProperties.Add(GET_MEMBER_NAME_CHECKED(FGameplayModifierInfo, Callbacks));
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE