// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemEditorPrivatePCH.h"
#include "GameplayEffectDetails.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "BlueprintUtilities.h"
#include "KismetEditorUtilities.h"
#include "AbilitySystemGlobals.h"
#include "GameplayAbilitiesModule.h"
#include "SGameplayAttributeGraphPin.h"
#include "GameplayEffectTemplate.h"

#define LOCTEXT_NAMESPACE "GameplayEffectDetailsCustomization"

DEFINE_LOG_CATEGORY(LogGameplayEffectDetails);

// --------------------------------------------------------SGameplayAttributeGraphPin----------------------------

TSharedRef<IDetailCustomization> FGameplayEffectDetails::MakeInstance()
{
	return MakeShareable(new FGameplayEffectDetails);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FGameplayEffectDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	MyDetailLayout = &DetailLayout;

	TArray< TWeakObjectPtr<UObject> > Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		// I don't know what to do here or what could be expected. Just return to disable all templating functionality
		return;
	}

	TemplateProperty = DetailLayout.GetProperty(FName("Template"), UGameplayEffect::StaticClass());
	ShowAllProperty = DetailLayout.GetProperty(FName("ShowAllProperties"), UGameplayEffect::StaticClass());

	FSimpleDelegate UpdateShowAllDelegate = FSimpleDelegate::CreateSP(this, &FGameplayEffectDetails::OnShowAllChange);
	ShowAllProperty->SetOnPropertyValueChanged(UpdateShowAllDelegate);

	FSimpleDelegate UpdatTemplateDelegate = FSimpleDelegate::CreateSP(this, &FGameplayEffectDetails::OnTemplateChange);
	TemplateProperty->SetOnPropertyValueChanged(UpdatTemplateDelegate);
	
	
	// Hide properties where necessary
	UGameplayEffect* Obj = Cast<UGameplayEffect>(Objects[0].Get());
	if (Obj && !Obj->ShowAllProperties)
	{
		UGameplayEffectTemplate* Template = Obj->Template;

		UE_LOG(LogGameplayEffectDetails, Warning, TEXT("ObjectBeingEditor: %s"), *Obj->GetName());
		if (Template)
		{
			UGameplayEffect* DefObj = Obj->Template->GetClass()->GetDefaultObject<UGameplayEffect>();

			for (TFieldIterator<UProperty> PropIt(UGameplayEffect::StaticClass(), EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
			{
				UProperty* Property = *PropIt;
				TSharedPtr<IPropertyHandle> PropHandle = DetailLayout.GetProperty(Property->GetFName());
				HideProperties(DetailLayout, PropHandle, Template);
			}
		}
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

/** Recursively hide properties that are not default editable according to the Template */
bool FGameplayEffectDetails::HideProperties(IDetailLayoutBuilder& DetailLayout, TSharedPtr<IPropertyHandle> PropHandle, UGameplayEffectTemplate* Template)
{
	UProperty* UProp = PropHandle->GetProperty();

	// Never hide the Template or ShowAllProperties properties
	if (TemplateProperty->GetProperty() == UProp || ShowAllProperty->GetProperty() == UProp)
	{
		return false;
	}

	// Don't hide the EditableProperties
	for (FString MatchStr : Template->EditableProperties)
	{
		if (MatchStr.Equals(UProp->GetName(), ESearchCase::IgnoreCase))
		{			
			return false;
		}
	}
	
	// Recurse into children - if they are all hidden then we are hidden.
	uint32 NumChildren = 0;
	PropHandle->GetNumChildren(NumChildren);

	bool AllChildrenHidden = true;
	for (uint32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = PropHandle->GetChildHandle(ChildIdx);
		AllChildrenHidden &= HideProperties(DetailLayout, ChildHandle, Template);
	}

	if (AllChildrenHidden)
	{
		DetailLayout.HideProperty(PropHandle);
		return true;
	}

	return false;
}

void FGameplayEffectDetails::OnShowAllChange()
{
	MyDetailLayout->ForceRefreshDetails();
}

void FGameplayEffectDetails::OnTemplateChange()
{
	TArray< TWeakObjectPtr<UObject> > Objects;
	MyDetailLayout->GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		return;
	}

	UGameplayEffect* Obj = Cast<UGameplayEffect>(Objects[0].Get());
	UGameplayEffect* Template = Obj->Template;

	if (Template)
	{
		// Copy any non-default properties from the template into the current editable object
		UGameplayEffect* DefObj = Template->GetClass()->GetDefaultObject<UGameplayEffect>();
		for (TFieldIterator<UProperty> PropIt(UGameplayEffect::StaticClass(), EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (!Property->Identical_InContainer(Template, DefObj))
			{
				Property->CopyCompleteValue_InContainer(Obj, Template);
			}

			// Default to only showing template properties after changing template type
			Obj->ShowAllProperties = false;
		}
	}

	MyDetailLayout->ForceRefreshDetails();
}


//-------------------------------------------------------------------------------------


#undef LOCTEXT_NAMESPACE