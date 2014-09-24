// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintComponentNodeSpawner.h"
#include "K2Node_AddComponent.h"
#include "ClassIconFinder.h" // for FindIconNameForClass()
#include "BlueprintNodeTemplateCache.h" // for IsTemplateOuter()
#include "ComponentAssetBroker.h" // for GetComponentsForAsset()/AssignAssetToComponent()

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
		UK2Node_AddComponent* AddCompNode = CastChecked<UK2Node_AddComponent>(NewNode);
		UBlueprint* Blueprint = AddCompNode->GetBlueprint();
		
		UFunction* AddComponentFunc = FindFieldChecked<UFunction>(AActor::StaticClass(), UK2Node_AddComponent::GetAddComponentFunctionName());
		AddCompNode->FunctionReference.SetFromField<UFunction>(AddComponentFunc, FBlueprintEditorUtils::IsActorBased(Blueprint));

		UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
	};

	FCustomizeNodeDelegate PostSpawnDelegate = FCustomizeNodeDelegate::CreateStatic(PostSpawnLambda, CustomizeNodeDelegate);
	// let SpawnNode() allocate default pins (so we can modify them)
	UK2Node_AddComponent* NewNode = Super::SpawnNode<UK2Node_AddComponent>(NodeClass, ParentGraph, FBindingSet(), Location, PostSpawnDelegate);

	// set the return type to be the type of the template
	UEdGraphPin* ReturnPin = NewNode->GetReturnValuePin();
	if (ReturnPin != nullptr)
	{
		ReturnPin->PinType.PinSubCategoryObject = *ComponentClass;
	}

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
	}

	// apply bindings, after we've setup the template pin
	ApplyBindings(NewNode, Bindings);

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
	FText MenuName = CachedMenuName.GetCachedText();

	if (Bindings.Num() > 0)
	{
		FText AssetName;
		if (UObject* AssetBinding = Bindings.CreateConstIterator()->Get())
		{
			AssetName = FText::FromName(AssetBinding->GetFName());
		}

		FText const ComponentTypeName = FText::FromName(ComponentClass->GetFName());
		MenuName = FText::Format(LOCTEXT("AddBoundComponentMenuName", "Add {0} (as {1})"), AssetName, ComponentTypeName);
	}
	return MenuName;
}

//------------------------------------------------------------------------------
FText UBlueprintComponentNodeSpawner::GetDefaultMenuTooltip() const
{
	FText const ComponentTypeName = FText::FromName(ComponentClass->GetFName());
	// @TODO: consider caching this, and provide a separate tooltip for when the
	//        node would be bound ("Spawn a {0} using {1}").
	return FText::Format(LOCTEXT("AddComponentTooltip", "Spawn a {0}"), ComponentTypeName);
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
FString UBlueprintComponentNodeSpawner::GetDefaultSearchKeywords() const
{
	FString SearchKeywords = ComponentClass->GetMetaData(FBlueprintMetadata::MD_FunctionKeywords);
	// add at least one character, so that the menu item doesn't attempt to
	// ping a template node
	return SearchKeywords.AppendChar(TEXT(' '));
}

//------------------------------------------------------------------------------
FName UBlueprintComponentNodeSpawner::GetDefaultMenuIcon(FLinearColor& ColorOut) const
{
	check(ComponentClass != nullptr);
	return FClassIconFinder::FindIconNameForClass(ComponentClass);
}

//------------------------------------------------------------------------------
bool UBlueprintComponentNodeSpawner::IsBindingCompatible(UObject const* BindingCandidate) const
{
	bool bCanBindWith = false;
	if (BindingCandidate->IsAsset())
	{
		TArray< TSubclassOf<UActorComponent> > ComponentClasses = FComponentAssetBrokerage::GetComponentsForAsset(BindingCandidate);
		bCanBindWith = ComponentClasses.Contains(ComponentClass);
	}
	return bCanBindWith;
}

//------------------------------------------------------------------------------
bool UBlueprintComponentNodeSpawner::BindToNode(UEdGraphNode* Node, UObject* Binding) const
{
	bool bSuccessfulBinding = false;
	UK2Node_AddComponent* AddCompNode = CastChecked<UK2Node_AddComponent>(Node);

	UActorComponent* ComponentTemplate = AddCompNode->GetTemplateFromNode();
	if (ComponentTemplate != nullptr)
	{
		bSuccessfulBinding = FComponentAssetBrokerage::AssignAssetToComponent(ComponentTemplate, Binding);
		AddCompNode->ReconstructNode();
	}
	return bSuccessfulBinding;
}

//------------------------------------------------------------------------------
TSubclassOf<UActorComponent> UBlueprintComponentNodeSpawner::GetComponentClass() const
{
	return ComponentClass;
}

#undef LOCTEXT_NAMESPACE
