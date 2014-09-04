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
UEdGraphNode* UBlueprintNodeSpawner::Invoke(UEdGraph* ParentGraph, FVector2D const Location) const
{
	return Invoke(ParentGraph, Location, CustomizeNodeDelegate);
}

//------------------------------------------------------------------------------
FText UBlueprintNodeSpawner::GetDefaultMenuName() const
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
bool UBlueprintNodeSpawner::CanBind(UObject const* BindingCandidate) const 
{
	return false;
}

//------------------------------------------------------------------------------
bool UBlueprintNodeSpawner::BindToNode(UEdGraphNode* Node, UObject* Binding) const
{
	return false;
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::GetTemplateNode(UEdGraph* Outer) const
{	
	UEdGraphNode* TemplateNode = BlueprintNodeSpawnerImpl::GetSharedTemplateCache()->GetNodeTemplate(this, Outer);
	return TemplateNode;
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::Invoke(UEdGraph* ParentGraph, FVector2D const Location, FCustomizeNodeDelegate PostSpawnDelegate) const
{
	UEdGraphNode* NewNode = nullptr;
	if (NodeClass != nullptr)
	{
		NewNode = NewObject<UEdGraphNode>(ParentGraph, NodeClass);
		check(NewNode != nullptr);
		NewNode->CreateNewGuid();

		// position the node before invoking PostSpawnDelegate (in case it 
		// wishes to modify this positioning)
		NewNode->NodePosX = Location.X;
		NewNode->NodePosY = Location.Y;

		bool const bIsTemplateNode = (ParentGraph->GetOutermost() == GetTransientPackage());
		PostSpawnDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);

		if (!bIsTemplateNode)
		{
			NewNode->SetFlags(RF_Transactional);
			NewNode->AllocateDefaultPins();
			NewNode->PostPlacedNewNode();
		}
	}

	return NewNode;
}
