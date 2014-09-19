// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintComponentNodeSpawner.h"
#include "K2Node_AddComponent.h"
#include "ClassIconFinder.h" // for FindIconNameForClass()
#include "BlueprintNodeTemplateCache.h" // for IsTemplateOuter()

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
FBlueprintNodeSignature UBlueprintComponentNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSignature SpawnerSignature(NodeClass);
	SpawnerSignature.AddSubObject(ComponentClass);
	return SpawnerSignature;
}

//------------------------------------------------------------------------------
// Evolved from a combination of FK2ActionMenuBuilder::CreateAddComponentAction()
// and FEdGraphSchemaAction_K2AddComponent::PerformAction().
UEdGraphNode* UBlueprintComponentNodeSpawner::Invoke(UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location) const
{
	check(ComponentClass != nullptr);
	
	auto PostSpawnLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FCustomizeNodeDelegate UserDelegate)
	{
		UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
		
		UK2Node_AddComponent* AddCompNode = CastChecked<UK2Node_AddComponent>(NewNode);
		UBlueprint* Blueprint = AddCompNode->GetBlueprint();
		
		UFunction* AddComponentFunc = FindFieldChecked<UFunction>(AActor::StaticClass(), UK2Node_AddComponent::GetAddComponentFunctionName());
		AddCompNode->FunctionReference.SetFromField<UFunction>(AddComponentFunc, FBlueprintEditorUtils::IsActorBased(Blueprint));
	};
	FCustomizeNodeDelegate PostSpawnDelegate = FCustomizeNodeDelegate::CreateStatic(PostSpawnLambda, CustomizeNodeDelegate);
	
	UK2Node_AddComponent* NewNode = Super::SpawnNode<UK2Node_AddComponent>(NodeClass, ParentGraph, Bindings, Location, PostSpawnDelegate);

	bool const bIsTemplateNode = FBlueprintNodeTemplateCache::IsTemplateOuter(ParentGraph);
	if (!bIsTemplateNode)
	{
		UBlueprint* Blueprint = NewNode->GetBlueprint();
		UActorComponent* ComponentTemplate = ConstructObject<UActorComponent>(ComponentClass, Blueprint->GeneratedClass);
		ComponentTemplate->SetFlags(RF_ArchetypeObject);

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
FText UBlueprintComponentNodeSpawner::GetDefaultMenuName(FBindingSet const& Bindings) const
{
	check(ComponentClass != nullptr);
	if (CachedMenuName.IsOutOfDate())
	{
		// FText::Format() is slow, so we cache this to save on performance
		CachedMenuName = FText::Format(LOCTEXT("AddComponentMenuName", "Add {0}"), FText::FromName(ComponentClass->GetFName()));
	}
	return CachedMenuName;
}

//------------------------------------------------------------------------------
FText UBlueprintComponentNodeSpawner::GetDefaultMenuCategory() const
{
	check(ComponentClass != nullptr);

	if (CachedCategory.IsOutOfDate())
	{
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
		// FText::Format() is slow, so we cache this to save on performance
		CachedCategory = FText::Format(LOCTEXT("ComponentCategory", "Add Component|{0}"), ClassGroup);
	}
	return CachedCategory;
}

//------------------------------------------------------------------------------
FName UBlueprintComponentNodeSpawner::GetDefaultMenuIcon(FLinearColor& ColorOut) const
{
	check(ComponentClass != nullptr);
	return FClassIconFinder::FindIconNameForClass(ComponentClass);
}

//------------------------------------------------------------------------------
TSubclassOf<UActorComponent> UBlueprintComponentNodeSpawner::GetComponentClass() const
{
	return ComponentClass;
}

#undef LOCTEXT_NAMESPACE
