// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintComponentNodeSpawner.h"
#include "K2Node_AddComponent.h"

#define LOCTEXT_NAMESPACE "BlueprintComponenetNodeSpawner"

/*******************************************************************************
 * UBlueprintComponentNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintComponentNodeSpawner* UBlueprintComponentNodeSpawner::Create(TSubclassOf<UActorComponent> const ComponentClass, UObject* Outer/* = nullptr*/)
{
	check(ComponentClass != nullptr);

	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintComponentNodeSpawner* NodeSpawner = NewObject<UBlueprintComponentNodeSpawner>(Outer);
	NodeSpawner->ComponentClass = ComponentClass;
	NodeSpawner->NodeClass      = UK2Node_AddComponent::StaticClass();

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintComponentNodeSpawner::UBlueprintComponentNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
// Evolved from a combination of FK2ActionMenuBuilder::CreateAddComponentAction()
// and FEdGraphSchemaAction_K2AddComponent::PerformAction().
UEdGraphNode* UBlueprintComponentNodeSpawner::Invoke(UEdGraph* ParentGraph) const
{
	UK2Node_AddComponent* NewNode = NewObject<UK2Node_AddComponent>(ParentGraph);
	UBlueprint* Blueprint = NewNode->GetBlueprint();

	UFunction* AddComponentFunc = FindFieldChecked<UFunction>(AActor::StaticClass(), UK2Node_AddComponent::GetAddComponentFunctionName());
	NewNode->FunctionReference.SetFromField<UFunction>(AddComponentFunc, FBlueprintEditorUtils::IsActorBased(Blueprint));

	bool bIsTemplateNode = ParentGraph->HasAnyFlags(RF_Transient);
	if (CustomizeNodeDelegate.IsBound())
	{
		CustomizeNodeDelegate.Execute(NewNode, bIsTemplateNode);
	}

	if (!bIsTemplateNode)
	{
		NewNode->SetFlags(RF_Transactional);
		NewNode->AllocateDefaultPins();

		check(ComponentClass != nullptr);
		UActorComponent* ComponentTemplate = ConstructObject<UActorComponent>(ComponentClass, Blueprint->GeneratedClass);
		ComponentTemplate->SetFlags(RF_ArchetypeObject);

		// @TODO: what is this is a template?
		Blueprint->ComponentTemplates.Add(ComponentTemplate);

		// set the name of the template as the default for the TemplateName param
		UEdGraphPin* TemplateNamePin = NewNode->GetTemplateNamePinChecked();
		if (TemplateNamePin != nullptr)
		{
			TemplateNamePin->DefaultValue = ComponentTemplate->GetName();
		}

		// set the return type to be the type of the template
		UEdGraphPin* ReturnPin = NewNode->GetReturnValuePin();
		if (ReturnPin != nullptr)
		{
			ReturnPin->PinType.PinSubCategoryObject = *ComponentClass;
		}

		// @TODO:
// 		if (ComponentAsset != NULL)
// 		{
// 			FComponentAssetBrokerage::AssignAssetToComponent(ComponentTemplate, ComponentAsset);
// 		}

		NewNode->ReconstructNode();
	}

	return NewNode;
}

//------------------------------------------------------------------------------
FText UBlueprintComponentNodeSpawner::GetDefaultMenuName() const
{
	check(ComponentClass != nullptr);
	return FText::Format(LOCTEXT("AddComponentMenuName", "Add {0}"), FText::FromName(ComponentClass->GetFName()));
}

//------------------------------------------------------------------------------
FText UBlueprintComponentNodeSpawner::GetDefaultMenuCategory() const
{
	check(ComponentClass != nullptr);

	FText ClassGroup;
	TArray<FString> ClassGroupNames;
	ComponentClass->GetClassGroupNames(ClassGroupNames);

	static FText const DefaultClassGroup(LOCTEXT("DefaultClassGroup", "Common"));
	// 'Common' takes priority over other class groups
	if (ClassGroupNames.Contains(DefaultClassGroup.ToString()) || (ClassGroupNames.Num() == 0)) 
	{
		ClassGroup = DefaultClassGroup;
	}
	else
	{
		ClassGroup = FText::FromString(ClassGroupNames[0]);
	}

	return FText::Format(LOCTEXT("ComponentCategory", "Add Component|{0}"), ClassGroup);
}

//------------------------------------------------------------------------------
TSubclassOf<UActorComponent> UBlueprintComponentNodeSpawner::GetComponentClass() const
{
	return ComponentClass;
}

#undef LOCTEXT_NAMESPACE
