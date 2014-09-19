// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintNodeTemplateCache.h"

/*******************************************************************************
 * Static UBlueprintNodeSpawner Helpers
 ******************************************************************************/

namespace BlueprintNodeSpawnerImpl
{
	/**
	 * 
	 * 
	 * @return 
	 */
	FBlueprintNodeTemplateCache* GetSharedTemplateCache(bool const bNoInit = false);
}

//------------------------------------------------------------------------------
FBlueprintNodeTemplateCache* BlueprintNodeSpawnerImpl::GetSharedTemplateCache(bool const bNoInit)
{
	static FBlueprintNodeTemplateCache* NodeTemplateManager = nullptr;
	if (!bNoInit && (NodeTemplateManager == nullptr))
	{
		NodeTemplateManager = new FBlueprintNodeTemplateCache();
	}
	return NodeTemplateManager;
}

/*******************************************************************************
 * UBlueprintNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintNodeSpawner* UBlueprintNodeSpawner::Create(TSubclassOf<UEdGraphNode> const NodeClass, UObject* Outer/* = nullptr*/, FCustomizeNodeDelegate CustomizeNodeDelegate/* = FCustomizeNodeDelegate()*/)
{
	check(NodeClass != nullptr);
	check(NodeClass->IsChildOf<UEdGraphNode>());

	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}
	
	UBlueprintNodeSpawner* NodeSpawner = NewObject<UBlueprintNodeSpawner>(Outer);
	NodeSpawner->NodeClass = NodeClass;
	NodeSpawner->CustomizeNodeDelegate = CustomizeNodeDelegate;
	
	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner::UBlueprintNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner::~UBlueprintNodeSpawner()
{
	// @TODO: What if, on shutdown, the "SharedTemplateCache" is destroyed 
	//        first? Then we'd be working with a bad pointer here
	if (FBlueprintNodeTemplateCache* TemplateCache = BlueprintNodeSpawnerImpl::GetSharedTemplateCache(/*bNoInit =*/true))
	{
		TemplateCache->ClearCachedTemplate(this);
	}
}

//------------------------------------------------------------------------------
void UBlueprintNodeSpawner::Prime()
{
	if (UEdGraphNode* CachedTemplateNode = GetTemplateNode())
	{
		// since we're priming incrementally, someone could have already
		// requested this template, and allocated its pins (don't need to do 
		// redundant work)
		if (CachedTemplateNode->Pins.Num() == 0)
		{
			// in certain scenarios we need pin information from the 
			// spawner (to help filter by pin context)
			CachedTemplateNode->AllocateDefaultPins();
		}

		//
		// let the node cache any FText::Format() operations that may be used...
		if (UK2Node* K2NodeTemplate = Cast<UK2Node>(CachedTemplateNode))
		{
			K2NodeTemplate->GetMenuCategory();
		}
		CachedTemplateNode->GetTooltipText();
		CachedTemplateNode->GetNodeTitle(ENodeTitleType::MenuTitle);
	}

	// in case any of these cache FText::Format() operations
	FBindingSet EmptyContext;
	GetDefaultMenuName(EmptyContext);
	GetDefaultMenuCategory();
	GetDefaultMenuTooltip();
}

//------------------------------------------------------------------------------
FBlueprintNodeSignature UBlueprintNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSignature SpawnerSignature;
	if (UK2Node const* NodeTemplate = Cast<UK2Node>(GetTemplateNode()))
	{
		SpawnerSignature = NodeTemplate->GetSignature();
	}

	if (!SpawnerSignature.IsValid())
	{
		SpawnerSignature.SetNodeClass(NodeClass);
	}
	return SpawnerSignature;
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::Invoke(UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location) const
{
	return SpawnNode(NodeClass, ParentGraph, Bindings, Location, CustomizeNodeDelegate);
}

//------------------------------------------------------------------------------
FText UBlueprintNodeSpawner::GetDefaultMenuName(FBindingSet const& Bindings) const
{
	// DO NOT make a template node and query it here (the separate ui building
	// code can do that if it likes)... this is meant to be an overridable
	// alternative to that (for perf reasons)
	return FText::GetEmpty();
}

//------------------------------------------------------------------------------
FText UBlueprintNodeSpawner::GetDefaultMenuCategory() const
{
	// DO NOT make a template node and query it here (the separate ui building
	// code can do that if it likes)... this is meant to be an overridable
	// alternative to that (for perf reasons)
	return FText::GetEmpty();
}

//------------------------------------------------------------------------------
FText UBlueprintNodeSpawner::GetDefaultMenuTooltip() const
{
	// DO NOT make a template node and query it here (the separate ui building
	// code can do that if it likes)... this is meant to be an overridable
	// alternative to that (for perf reasons)
	return FText::GetEmpty();
}

//------------------------------------------------------------------------------
FString UBlueprintNodeSpawner::GetDefaultSearchKeywords() const
{
	// DO NOT make a template node and query it here (the separate ui building
	// code can do that if it likes)... this is meant to be an overridable
	// alternative to that (for perf reasons)
	return FString();
}

//------------------------------------------------------------------------------
FName UBlueprintNodeSpawner::GetDefaultMenuIcon(FLinearColor& ColorOut) const
{
	ColorOut = FLinearColor::White;
	// DO NOT make a template node and query it here (the separate ui building
	// code can do that if it likes)... this is meant to be an overridable
	// alternative to that (for perf reasons)
	return FName();
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::GetTemplateNode(UEdGraph* Outer, FBindingSet const& Bindings) const 
{       
	UEdGraphNode* TemplateNode = BlueprintNodeSpawnerImpl::GetSharedTemplateCache()->GetNodeTemplate(this, Outer);

	if (TemplateNode && Bindings.Num() > 0) 
	{ 
		UEdGraphNode* BoundTemplateNode = DuplicateObject(TemplateNode, TemplateNode->GetOuter()); 
		ApplyBindings(BoundTemplateNode, Bindings); 
		return BoundTemplateNode; 
	} 
	return TemplateNode; 
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::GetTemplateNode(ENoInit) const
{
	return BlueprintNodeSpawnerImpl::GetSharedTemplateCache()->GetNodeTemplate(this, NoInit);
}
// 
// //------------------------------------------------------------------------------
// UEdGraphNode* UBlueprintNodeSpawner::Invoke(UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location, FCustomizeNodeDelegate PostSpawnDelegate) const
// {
// 	return SpawnNode(NodeClass, ParentGraph, Bindings, Location, PostSpawnDelegate);
// }

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::SpawnEdGraphNode(TSubclassOf<UEdGraphNode> InNodeClass, UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location, FCustomizeNodeDelegate PostSpawnDelegate) const
{
	UEdGraphNode* NewNode = nullptr;
	if (InNodeClass != nullptr)
	{
		NewNode = NewObject<UEdGraphNode>(ParentGraph, InNodeClass);
		check(NewNode != nullptr);
		NewNode->CreateNewGuid();

		// position the node before invoking PostSpawnDelegate (in case it 
		// wishes to modify this positioning)
		NewNode->NodePosX = Location.X;
		NewNode->NodePosY = Location.Y;

		bool const bIsTemplateNode = FBlueprintNodeTemplateCache::IsTemplateOuter(ParentGraph);
		PostSpawnDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);

		if (!bIsTemplateNode)
		{
			NewNode->SetFlags(RF_Transactional);
			NewNode->AllocateDefaultPins();
			NewNode->PostPlacedNewNode();
		}

		ApplyBindings(NewNode, Bindings);
	}

	return NewNode;
}
