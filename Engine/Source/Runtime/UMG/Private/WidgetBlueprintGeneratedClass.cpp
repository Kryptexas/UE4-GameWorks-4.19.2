// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprintGeneratedClass

UWidgetBlueprintGeneratedClass::UWidgetBlueprintGeneratedClass(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	WidgetTree = ConstructObject<UWidgetTree>(UWidgetTree::StaticClass(), this);
}

void UWidgetBlueprintGeneratedClass::Link(FArchive& Ar, bool bRelinkExistingProperties)
{
	Super::Link(Ar, bRelinkExistingProperties);

	// @TODO: Shouldn't be necessary to clear these, but currently the class gets linked twice during compilation
	WidgetNodeProperties.Empty();

	// Initialize derived members
	for ( TFieldIterator<UProperty> It(this); It; ++It )
	{
		if ( UStructProperty* StructProp = Cast<UStructProperty>(*It) )
		{
			if ( StructProp->Struct->IsChildOf(FWidgetNode_Base::StaticStruct()) )
			{
				WidgetNodeProperties.Add(StructProp);
			}
		}
	}
}

void UWidgetBlueprintGeneratedClass::CreateComponentsForActor(AActor* Actor) const
{
	Super::CreateComponentsForActor(Actor);

	// Duplicate the graph, keeping track of what was duplicated
	TMap<UObject*, UObject*> DuplicatedObjectList;

	FObjectDuplicationParameters Parameters(const_cast<UWidgetTree*>( WidgetTree ), Actor);
	Parameters.CreatedObjects = &DuplicatedObjectList;

	UWidgetTree* ClonedTree = CastChecked<UWidgetTree>(StaticDuplicateObjectEx(Parameters));

	AUserWidget* WidgetActor = CastChecked<AUserWidget>(Actor);
	UClass* ActorClass = Actor->GetClass();

	for ( USlateWrapperComponent* Widget : ClonedTree->WidgetTemplates )
	{
		// Not fatal if NULL, but shouldn't happen
		if ( !ensure(Widget != NULL) )
		{
			continue;
		}

		FName NewName = *( FString::Printf(TEXT("WidgetComponent__%d"), Actor->SerializedComponents.Num()) );

		FString VariableName = Widget->GetName();

		Widget->bCreatedByConstructionScript = true; // Indicate it comes from a blueprint so it gets cleared when we rerun construction scripts
		Actor->SerializedComponents.Add(Widget); // Add to array so it gets saved
		Widget->SetNetAddressable();	// This component has a stable name that can be referenced for replication

		// Find property with the same name as the template and assign the new Timeline to it
		UObjectPropertyBase* Prop = FindField<UObjectPropertyBase>(ActorClass, *VariableName);
		if ( Prop )
		{
			Prop->SetObjectPropertyValue_InContainer(Actor, Widget);
		}

		// Perform binding
		for ( const FDelegateRuntimeBinding& Binding : Bindings )
		{
			//TODO UMG Terrible performance, improve with Maps.
			if ( Binding.ObjectName == VariableName )
			{
				UFunction* BoundFunction = WidgetActor->FindFunction(Binding.FunctionName);

				FString DelegateName = Binding.PropertyName.ToString() + "Delegate";

				for ( TFieldIterator<UProperty> It(Widget->GetClass()); It; ++It )
				{
					if ( UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(*It) )
					{
						if ( DelegateProp->GetName() == DelegateName || DelegateProp->GetFName() == Binding.PropertyName )
						{
							FScriptDelegate* ScriptDelegate = DelegateProp->GetPropertyValuePtr_InContainer(Widget);
							ScriptDelegate->BindUFunction(WidgetActor, Binding.FunctionName);
							break;
						}
					}
				}
			}
		}

		Widget->RegisterComponent();

#if WITH_EDITOR
		Widget->ConnectEditorData();
#endif
	}
}

#undef LOCTEXT_NAMESPACE