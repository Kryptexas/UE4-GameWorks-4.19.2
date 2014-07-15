// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintEventNodeSpawner.h"
#include "EdGraphSchema_K2.h"			// for GetFriendlySignitureName()
#include "BlueprintActionMenuBuilder.h"	// for AddEventCategory

#define LOCTEXT_NAMESPACE "BlueprintEventNodeSpawner"

/*******************************************************************************
 * Static UBlueprintEventNodeSpawner Helpers
 ******************************************************************************/

namespace UBlueprintEventNodeSpawnerImpl
{
	/**
	 * Helper function for scanning a blueprint for a certain, custom named,
	 * event.
	 * 
	 * @param  Blueprint	The blueprint you want to look through
	 * @param  CustomName	The event name you want to check for.
	 * @return Null if no event was found, otherwise a pointer to the named event.
	 */
	static UK2Node_Event* FindCustomEventNode(UBlueprint* Blueprint, FName const CustomName);
}

//------------------------------------------------------------------------------
UK2Node_Event* UBlueprintEventNodeSpawnerImpl::FindCustomEventNode(UBlueprint* Blueprint, FName const CustomName)
{
	UK2Node_Event* FoundNode = nullptr;

	TArray<UK2Node_Event*> AllEvents;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(Blueprint, AllEvents);

	for (UK2Node_Event* EventNode : AllEvents)
	{
		if (EventNode->CustomFunctionName == CustomName)
		{
			FoundNode = EventNode;
			break;
		}
	}
	return FoundNode;
}

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
UBlueprintEventNodeSpawner* UBlueprintEventNodeSpawner::Create(TSubclassOf<UK2Node_Event> NodeClass, FName CustomEventName)
{
	UBlueprintEventNodeSpawner* NodeSpawner = NewObject<UBlueprintEventNodeSpawner>(GetTransientPackage());
	NodeSpawner->NodeClass       = NodeClass;
	NodeSpawner->CustomEventName = CustomEventName;

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

	UK2Node_Event* EventNode = nullptr;
	
	FName EventName = CustomEventName;
	if (EventFunc != nullptr)
	{
		check(EventFunc != nullptr);
		EventName = EventFunc->GetFName();
	}
	
	UClass* ClassOwner = Blueprint->GeneratedClass;
	// look to see if a node for this event already exists (only one node is
	// allowed per event, per blueprint)
	if (EventFunc != nullptr)
	{
		ClassOwner = CastChecked<UClass>(EventFunc->GetOuter());
		EventNode  = FBlueprintEditorUtils::FindOverrideForFunction(Blueprint, ClassOwner, EventName);
	}
	else
	{
		EventNode  = UBlueprintEventNodeSpawnerImpl::FindCustomEventNode(Blueprint, EventName);
	}

	// if there is no existing node, then we can happily spawn one into the graph
	if (EventNode == nullptr)
	{
		bool bIsTemplateNode = ParentGraph->HasAnyFlags(RF_Transient);

		EventNode = CastChecked<UK2Node_Event>(Super::Invoke(ParentGraph));
		if (EventFunc != nullptr)
		{
			EventNode->EventSignatureName  = EventName;
			EventNode->EventSignatureClass = ClassOwner;
			EventNode->bOverrideFunction   = true;
		}
		else if (!bIsTemplateNode)
		{
			EventNode->CustomFunctionName  = CustomEventName;
		}

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
	FText EventName;
	if (EventFunc != nullptr)
	{
		EventName = FText::FromString(UEdGraphSchema_K2::GetFriendlySignitureName(EventFunc));
	}
	else if (!CustomEventName.IsNone())
	{
		EventName = FText::FromName(CustomEventName);
	}

	if (!EventName.IsEmpty())
	{
		EventName = FText::Format(LOCTEXT("EventWithSignatureName", "Event {0}"), EventName);
	}
	return EventName;
}

//------------------------------------------------------------------------------
FText UBlueprintEventNodeSpawner::GetDefaultSearchKeywords() const
{
	FText Keywords = FText::GetEmpty();
	if (EventFunc != nullptr)
	{
		Keywords = FText::FromString(UK2Node_CallFunction::GetKeywordsForFunction(EventFunc));
	}
	return Keywords;
}

//------------------------------------------------------------------------------
UFunction const* UBlueprintEventNodeSpawner::GetEventFunction() const
{
	return EventFunc;
}

#undef LOCTEXT_NAMESPACE
