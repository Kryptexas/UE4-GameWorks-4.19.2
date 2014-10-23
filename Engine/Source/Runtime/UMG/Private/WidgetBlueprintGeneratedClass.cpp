// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieScene.h"
#include "WidgetAnimation.h"
#include "Engine/InputDelegateBinding.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprintGeneratedClass

UWidgetBlueprintGeneratedClass::UWidgetBlueprintGeneratedClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
	//for ( TFieldIterator<UProperty> It(this); It; ++It )
	//{
	//	if ( UStructProperty* StructProp = Cast<UStructProperty>(*It) )
	//	{
	//		if ( StructProp->Struct->IsChildOf(FWidgetNode_Base::StaticStruct()) )
	//		{
	//			WidgetNodeProperties.Add(StructProp);
	//		}
	//	}
	//}
}

void UWidgetBlueprintGeneratedClass::InitializeWidget(UUserWidget* UserWidget) const
{
	UWidgetTree* ClonedTree = DuplicateObject<UWidgetTree>( WidgetTree, UserWidget );

	if ( ClonedTree )
	{
		ClonedTree->SetFlags(RF_Transactional);

		UserWidget->WidgetTree = ClonedTree;

		UClass* WidgetBlueprintClass = UserWidget->GetClass();

		for(UWidgetAnimation* Animation : Animations)
		{
			UWidgetAnimation* Anim = DuplicateObject<UWidgetAnimation>( Animation, UserWidget );

			if( Anim->MovieScene )
			{
				// Find property with the same name as the template and assign the new widget to it.
				UObjectPropertyBase* Prop = FindField<UObjectPropertyBase>(WidgetBlueprintClass, Anim->MovieScene->GetFName());
				if(Prop)
				{
					Prop->SetObjectPropertyValue_InContainer(UserWidget, Anim);
				}
			}
		}

		ClonedTree->ForEachWidget([&] (UWidget* Widget) {
			// Not fatal if NULL, but shouldn't happen
			if ( !ensure(Widget != nullptr) )
			{
				return;
			}

			// TODO UMG Make this an FName
			FString VariableName = Widget->GetName();

			Widget->bCreatedByConstructionScript = true; // Indicate it comes from a blueprint so it gets cleared when we rerun construction scripts

			// Find property with the same name as the template and assign the new widget to it.
			UObjectPropertyBase* Prop = FindField<UObjectPropertyBase>(WidgetBlueprintClass, *VariableName);
			if ( Prop )
			{
				Prop->SetObjectPropertyValue_InContainer(UserWidget, Widget);
				UObject* Value = Prop->GetObjectPropertyValue_InContainer(UserWidget);
				check(Value == Widget);
			}

			// Perform binding
			for ( const FDelegateRuntimeBinding& Binding : Bindings )
			{
				//TODO UMG Make this faster.
				if ( Binding.ObjectName == VariableName )
				{
					UDelegateProperty* DelegateProperty = FindField<UDelegateProperty>(Widget->GetClass(), FName(*( Binding.PropertyName.ToString() + TEXT("Delegate") )));
					if ( !DelegateProperty )
					{
						DelegateProperty = FindField<UDelegateProperty>(Widget->GetClass(), Binding.PropertyName);
					}

					if ( DelegateProperty )
					{
						FScriptDelegate* ScriptDelegate = DelegateProperty->GetPropertyValuePtr_InContainer(Widget);
						if ( ScriptDelegate )
						{
							ScriptDelegate->BindUFunction(UserWidget, Binding.FunctionName);
						}
					}
				}
			}

	#if WITH_EDITOR
			Widget->ConnectEditorData();
	#endif
		});

		// Bind any delegates on widgets
		BindDynamicDelegates(UserWidget);

		//TODO UMG Add OnWidgetInitialized?
	}
}

#undef LOCTEXT_NAMESPACE
