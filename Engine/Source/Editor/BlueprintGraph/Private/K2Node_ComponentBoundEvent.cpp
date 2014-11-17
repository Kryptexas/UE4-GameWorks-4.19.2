// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_ComponentBoundEvent::UK2Node_ComponentBoundEvent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UK2Node_ComponentBoundEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("DelegatePropertyName"), FText::FromName(DelegatePropertyName));
	Args.Add(TEXT("ComponentPropertyName"), FText::FromName(ComponentPropertyName));
	return FText::Format(LOCTEXT("ComponentBoundEvent_Title", "{DelegatePropertyName} ({ComponentPropertyName})"), Args);
}

void UK2Node_ComponentBoundEvent::InitializeComponentBoundEventParams(UObjectProperty* InComponentProperty, const UMulticastDelegateProperty* InDelegateProperty)
{
	if( InComponentProperty && InDelegateProperty )
	{
		ComponentPropertyName = InComponentProperty->GetFName();
		DelegatePropertyName = InDelegateProperty->GetFName();
		EventSignatureName = InDelegateProperty->SignatureFunction->GetFName();
		EventSignatureClass = CastChecked<UClass>(InDelegateProperty->SignatureFunction->GetOuter());
		CustomFunctionName = FName( *FString::Printf(TEXT("BndEvt__%s_%s_%s"), *InComponentProperty->GetName(), *GetName(), *EventSignatureName.ToString()) );
		bOverrideFunction = false;
		bInternalEvent = true;
	}
}

UClass* UK2Node_ComponentBoundEvent::GetDynamicBindingClass() const
{
	return UComponentDelegateBinding::StaticClass();
}

void UK2Node_ComponentBoundEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UComponentDelegateBinding* ComponentBindingObject = CastChecked<UComponentDelegateBinding>(BindingObject);

	FBlueprintComponentDelegateBinding Binding;
	Binding.ComponentPropertyName = ComponentPropertyName;
	Binding.DelegatePropertyName = DelegatePropertyName;
	Binding.FunctionNameToBind = CustomFunctionName;

	ComponentBindingObject->ComponentDelegateBindings.Add(Binding);
}

bool UK2Node_ComponentBoundEvent::IsUsedByAuthorityOnlyDelegate() const
{
	UMulticastDelegateProperty* TargetDelegateProp = GetTargetDelegateProperty();
	return (TargetDelegateProp && TargetDelegateProp->HasAnyPropertyFlags(CPF_BlueprintAuthorityOnly));
}

UMulticastDelegateProperty* UK2Node_ComponentBoundEvent::GetTargetDelegateProperty() const
{
	return Cast<UMulticastDelegateProperty>(FindField<UMulticastDelegateProperty>(EventSignatureClass, DelegatePropertyName));
}


FString UK2Node_ComponentBoundEvent::GetTooltip() const
{
	UMulticastDelegateProperty* TargetDelegateProp = GetTargetDelegateProperty();
	if (TargetDelegateProp)
	{
		return TargetDelegateProp->GetToolTipText().ToString();
	}
	else
	{
		return DelegatePropertyName.ToString();
	}
}

FString UK2Node_ComponentBoundEvent::GetDocumentationLink() const
{
	if (EventSignatureClass)
	{
		return FString::Printf(TEXT("Shared/GraphNodes/Blueprint/%s%s"), EventSignatureClass->GetPrefixCPP(), *EventSignatureClass->GetName());
	}

	return FString();
}

FString UK2Node_ComponentBoundEvent::GetDocumentationExcerptName() const
{
	return DelegatePropertyName.ToString();
}

#undef LOCTEXT_NAMESPACE