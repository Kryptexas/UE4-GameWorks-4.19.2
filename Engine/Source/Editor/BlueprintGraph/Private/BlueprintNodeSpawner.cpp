// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintNodeSpawner.h"

/*******************************************************************************
 * UBlueprintNodeSpawner
*******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintNodeSpawner* UBlueprintNodeSpawner::Create(UClass* const NodeClass, FCustomizeNodeDelegate CustomizeNodeDelegate/* = FCustomizeNodeDelegate()*/)
{
	check(NodeClass != nullptr);
	check(NodeClass->IsChildOf<UEdGraphNode>());
	
	UBlueprintNodeSpawner* NodeSpawner = NewObject<UBlueprintNodeSpawner>(GetTransientPackage());
	NodeSpawner->NodeClass = NodeClass;
	NodeSpawner->CustomizeNodeDelegate = CustomizeNodeDelegate;
	
	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner::UBlueprintNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
	, CachedNodeTemplate(nullptr)
{
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::Invoke(UEdGraph* ParentGraph) const
{
	UEdGraphNode* NewNode = nullptr;	
	if (NodeClass != nullptr)
	{
		NewNode = NewObject<UEdGraphNode>(ParentGraph, NodeClass);
		check(NewNode != nullptr);

		bool bIsTemplateNode = ParentGraph->HasAnyFlags(RF_Transient);		
		if (CustomizeNodeDelegate.IsBound())
		{
			CustomizeNodeDelegate.Execute(NewNode, bIsTemplateNode);
		}

		if (!bIsTemplateNode)
		{
			NewNode->SetFlags(RF_Transactional);
			NewNode->AllocateDefaultPins();
		}
	}

	return NewNode;
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
FText UBlueprintNodeSpawner::GetDefaultSearchKeywords() const
{
	// DO NOT make a template node and query it here (the separate ui building
	// code can do that if it likes)... this is meant to be an overridable
	// alternative to that (for perf reasons)
	return FText::GetEmpty();
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintNodeSpawner::MakeTemplateNode(UEdGraph* Outer) const
{
	check(Outer != nullptr);
	check(Outer->HasAnyFlags(RF_Transient));
	
	// @TODO: Do we need to respawn when the outers don't match? If not, this'll
	//        save time for subsequent menu generation... if the old outer gets
	//        destroyed then CachedNodeTemplate should be nulled (so we should
	//        be covered on that front)... maybe we should just check the
	//        graph's class type (and graph type)
	if (CachedNodeTemplate == nullptr)// || (CachedNodeTemplate->GetOuter() != Outer))
	{
		CachedNodeTemplate = Invoke(Outer);
		if (CachedNodeTemplate != nullptr)
		{
			CachedNodeTemplate->SetFlags(RF_Transient | RF_ArchetypeObject);
		}
	}
	
	return CachedNodeTemplate;
}
