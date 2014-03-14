// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"


UComponentDelegateBinding::UComponentDelegateBinding(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UComponentDelegateBinding::BindDynamicDelegates(AActor* InInstance) const
{
	for(int32 BindIdx=0; BindIdx<ComponentDelegateBindings.Num(); BindIdx++)
	{
		const FBlueprintComponentDelegateBinding& Binding = ComponentDelegateBindings[BindIdx];
		// Get the function we want to bind
		UFunction* FunctionToBind = FindField<UFunction>(InInstance->GetClass(), Binding.FunctionNameToBind);
		// Get the property that points to the component we want to assign to
		UObjectProperty* ObjProp = FindField<UObjectProperty>(InInstance->GetClass(), Binding.ComponentPropertyName);
		// If we have both of those..
		if(ObjProp != NULL && FunctionToBind != NULL)
		{
			// ..see if there is actually a component assigned
			UActorComponent* Component = Cast<UActorComponent>(ObjProp->GetObjectPropertyValue_InContainer(InInstance));
			if(Component != NULL)
			{
				// If there is, find the delegate property on it
				UMulticastDelegateProperty* DelegateProp = FindField<UMulticastDelegateProperty>(Component->GetClass(), Binding.DelegatePropertyName);
				if(DelegateProp)
				{
					// Found that, finally bind function on the actor to this delegate
					FMulticastScriptDelegate* TargetDelegate = DelegateProp->GetPropertyValuePtr_InContainer(Component);

					FScriptDelegate Delegate;
					Delegate.SetFunctionName(Binding.FunctionNameToBind);
					Delegate.SetObject(InInstance);
					TargetDelegate->AddUnique(Delegate);
				}
			}
		}
	}
}