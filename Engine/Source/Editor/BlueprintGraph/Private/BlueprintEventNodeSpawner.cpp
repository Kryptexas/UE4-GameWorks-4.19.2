// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintEventNodeSpawner.h"
#include "EdGraphSchema_K2.h"	// for GetFriendlySignitureName()

#define LOCTEXT_NAMESPACE "BlueprintEventNodeSpawner"

/*******************************************************************************
 * UBlueprintEventNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintEventNodeSpawner* UBlueprintEventNodeSpawner::Create(UFunction const* const EventFunc)
{
	check(EventFunc != nullptr);

	UBlueprintEventNodeSpawner* NodeSpawner = NewObject<UBlueprintEventNodeSpawner>(GetTransientPackage());
	NodeSpawner->EventFunc = EventFunc;
	NodeSpawner->NodeClass = UK2Node_Event::StaticClass();

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintEventNodeSpawner::UBlueprintEventNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
	, EventFunc(nullptr)
{
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintEventNodeSpawner::Invoke(UEdGraph* ParentGraph) const
{
	check(ParentGraph != nullptr);
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(ParentGraph);

	check(EventFunc != nullptr);

	UClass* ClassOwner = CastChecked<UClass>(EventFunc->GetOuter());
	// look to see if a node for this event already exists (only one node is
	// allowed per event, per blueprint)
	UK2Node_Event* EventNode = FBlueprintEditorUtils::FindOverrideForFunction(Blueprint, ClassOwner, EventFunc->GetFName());

	// if there is no existing node, then we can happily spawn one into the graph
	if (EventNode == nullptr)
	{
		EventNode = NewObject<UK2Node_Event>(ParentGraph);
		EventNode->EventSignatureName  = EventFunc->GetFName();
		EventNode->EventSignatureClass = ClassOwner;
		EventNode->bOverrideFunction   = true;

		bool bIsTemplateNode = ParentGraph->HasAnyFlags(RF_Transient);
		if (!bIsTemplateNode)
		{
			EventNode->SetFlags(RF_Transactional);
			EventNode->AllocateDefaultPins();
		}
	}
	// else, a node for this event already exists, and we should return that 
	// (the FBlueprintActionMenuItem should detect this and focus in on it).

	return EventNode;
}

//------------------------------------------------------------------------------
FText UBlueprintEventNodeSpawner::GetDefaultMenuName() const
{
	check(EventFunc != nullptr);
	
	FString const EventName = UEdGraphSchema_K2::GetFriendlySignitureName(EventFunc);
	return FText::Format(LOCTEXT("EventWithSignatureName", "Event {0}"), FText::FromString(EventName));
}

//------------------------------------------------------------------------------
FText UBlueprintEventNodeSpawner::GetDefaultMenuCategory() const
{
	// @TODO: 
	return Super::GetDefaultMenuCategory();
}

//------------------------------------------------------------------------------
UFunction const* UBlueprintEventNodeSpawner::GetEventFunction() const
{
	return EventFunc;
}

#undef LOCTEXT_NAMESPACE
