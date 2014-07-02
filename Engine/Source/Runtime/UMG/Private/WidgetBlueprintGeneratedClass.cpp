// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprintGeneratedClass

UWidgetBlueprintGeneratedClass::UWidgetBlueprintGeneratedClass(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UWidgetBlueprintGeneratedClass::PostInitProperties()
{
	Super::PostInitProperties();

	//// Create a widget tree if one doesn't already exist.
	//if ( WidgetTree == NULL )
	//{
	//	WidgetTree = ConstructObject<UWidgetTree>(UWidgetTree::StaticClass(), this);
	//}

	////WidgetTree->SetFlags(RF_DefaultSubObject);
	//WidgetTree->SetFlags(RF_Transactional);
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

void UWidgetBlueprintGeneratedClass::InitializeWidget(UUserWidget* UserWidget) const
{
	UWidgetTree* ClonedTree = DuplicateObject<UWidgetTree>( WidgetTree, UserWidget );
	//check(ClonedTree);

	if ( ClonedTree )
	{
		ClonedTree->SetFlags(RF_Transactional);

		UserWidget->WidgetTree = ClonedTree;

		UClass* WidgetBlueprintClass = UserWidget->GetClass();

		TArray<UWidget*> ClonedWidgets;
		ClonedTree->GetAllWidgets(ClonedWidgets);

		for ( UWidget* Widget : ClonedWidgets )
		{
			// Not fatal if NULL, but shouldn't happen
			if ( !ensure(Widget != NULL) )
			{
				continue;
			}

			FString VariableName = Widget->GetName();

			Widget->bCreatedByConstructionScript = true; // Indicate it comes from a blueprint so it gets cleared when we rerun construction scripts
			UserWidget->Components.Add(Widget); // Add to array so it gets saved

			// Find property with the same name as the template and assign the new widget to it.
			UObjectPropertyBase* Prop = FindField<UObjectPropertyBase>(WidgetBlueprintClass, *VariableName);
			if ( Prop )
			{
				Prop->SetObjectPropertyValue_InContainer(UserWidget, Widget);
			}

			// Perform binding
			for ( const FDelegateRuntimeBinding& Binding : Bindings )
			{
				//TODO UMG Terrible performance, improve with Maps.
				if ( Binding.ObjectName == VariableName )
				{
					UFunction* BoundFunction = UserWidget->FindFunction(Binding.FunctionName);

					FString DelegateName = Binding.PropertyName.ToString() + "Delegate";

					for ( TFieldIterator<UProperty> It(Widget->GetClass()); It; ++It )
					{
						if ( UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(*It) )
						{
							if ( DelegateProp->GetName() == DelegateName || DelegateProp->GetFName() == Binding.PropertyName )
							{
								FScriptDelegate* ScriptDelegate = DelegateProp->GetPropertyValuePtr_InContainer(Widget);
								ScriptDelegate->BindUFunction(UserWidget, Binding.FunctionName);
								break;
							}
						}
					}
				}
			}

	#if WITH_EDITOR
			Widget->ConnectEditorData();
	#endif
		}

		// Bind any delegates on widgets
		BindDynamicDelegates(UserWidget);

		//TODO Add OnWidgetInitialized
	}
}

#undef LOCTEXT_NAMESPACE