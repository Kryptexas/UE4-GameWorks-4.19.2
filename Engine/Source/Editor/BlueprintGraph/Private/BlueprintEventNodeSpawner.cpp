// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintEventNodeSpawner.h"
#include "EdGraphSchema_K2.h" // for GetFriendlySignitureName()

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

	if (CustomName != NAME_None)
	{
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
	}	
	return FoundNode;
}

/*******************************************************************************
 * UBlueprintEventNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintEventNodeSpawner* UBlueprintEventNodeSpawner::Create(UFunction const* const EventFunc, UObject* Outer/* = nullptr*/)
{
	check(EventFunc != nullptr);

	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintEventNodeSpawner* NodeSpawner = NewObject<UBlueprintEventNodeSpawner>(Outer);
	NodeSpawner->EventFunc = EventFunc;
	NodeSpawner->NodeClass = UK2Node_Event::StaticClass();

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintEventNodeSpawner* UBlueprintEventNodeSpawner::Create(TSubclassOf<UK2Node_Event> NodeClass, FName CustomEventName, UObject* Outer/* = nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintEventNodeSpawner* NodeSpawner = NewObject<UBlueprintEventNodeSpawner>(Outer);
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
UEdGraphNode* UBlueprintEventNodeSpawner::Invoke(UEdGraph* ParentGraph, FVector2D const Location) const
{
	check(ParentGraph != nullptr);
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(ParentGraph);
	// look to see if a node for this event already exists (only one node is
	// allowed per event, per blueprint)
	UK2Node_Event const* PreExistingNode = FindPreExistingEvent(Blueprint);
	// @TODO: casting away the const is bad form!
	UK2Node_Event* EventNode = const_cast<UK2Node_Event*>(PreExistingNode);

	bool const bIsCustomEvent = IsForCustomEvent();
	check(bIsCustomEvent || (EventFunc != nullptr));

	FName EventName = CustomEventName;
	if (!bIsCustomEvent)
	{
		EventName  = EventFunc->GetFName();	
	}

	// if there is no existing node, then we can happily spawn one into the graph
	if (EventNode == nullptr)
	{
		auto PostSpawnLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, UFunction const* EventFunc, FName EventName, FCustomizeNodeDelegate UserDelegate)
		{
			UK2Node_Event* EventNode = CastChecked<UK2Node_Event>(NewNode);
			if (EventFunc != nullptr)
			{
				EventNode->EventSignatureName  = EventName;
				EventNode->EventSignatureClass = EventFunc->GetOuterUClass();
				EventNode->bOverrideFunction   = true;
			}
			else if (!bIsTemplateNode)
			{
				EventNode->CustomFunctionName = EventName;
			}

			UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
		};

		FCustomizeNodeDelegate PostSpawnDelegate = FCustomizeNodeDelegate::CreateStatic(PostSpawnLambda, EventFunc, EventName, CustomizeNodeDelegate);
		EventNode = Cast<UK2Node_Event>(Super::Invoke(ParentGraph, Location, PostSpawnDelegate));
	}
	// else, a node for this event already exists, and we should return that 
	// (the FBlueprintActionMenuItem should detect this and focus in on it).

	return EventNode;
}

//------------------------------------------------------------------------------
FText UBlueprintEventNodeSpawner::GetDefaultMenuName() const
{
	if (CachedMenuName.IsOutOfDate())
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

		CachedMenuName = FText::Format(LOCTEXT("EventWithSignatureName", "Event {0}"), EventName);
	}
	return CachedMenuName;
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

//------------------------------------------------------------------------------
UK2Node_Event const* UBlueprintEventNodeSpawner::FindPreExistingEvent(UBlueprint* Blueprint) const
{
	UK2Node_Event* PreExistingNode = nullptr;

	check(Blueprint != nullptr);
	if (IsForCustomEvent())
	{
		PreExistingNode = UBlueprintEventNodeSpawnerImpl::FindCustomEventNode(Blueprint, CustomEventName);
	}
	else
	{
		check(EventFunc != nullptr);
		UClass* ClassOwner = EventFunc->GetOwnerClass();

		PreExistingNode = FBlueprintEditorUtils::FindOverrideForFunction(Blueprint, ClassOwner, EventFunc->GetFName());
	}

	return PreExistingNode;
}

//------------------------------------------------------------------------------
bool UBlueprintEventNodeSpawner::IsForCustomEvent() const
{
	return (EventFunc == nullptr);
}

#undef LOCTEXT_NAMESPACE
