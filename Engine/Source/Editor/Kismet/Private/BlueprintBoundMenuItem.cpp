// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintBoundMenuItem.h"
#include "BlueprintActionMenuItem.h"
#include "BlueprintNodeSpawner.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "BlueprintBoundMenuItem"

/*******************************************************************************
* Static FBlueprintBoundMenuItem Helpers
******************************************************************************/

namespace BlueprintBoundMenuItemImpl
{
	/**
	 * 
	 * 
	 * @param  ObjectsList	
	 * @return 
	 */
	static UClass* FindCommonBaseClass(IBlueprintNodeBinder::FBindingSet const& ObjectsList);
}

//------------------------------------------------------------------------------
static UClass* BlueprintBoundMenuItemImpl::FindCommonBaseClass(IBlueprintNodeBinder::FBindingSet const& ObjectsList)
{
	UClass* CommonClass = nullptr;
	for (TWeakObjectPtr<UObject> BoundObjPtr : ObjectsList)
	{
		if (UObject const* BoundObj = BoundObjPtr.Get())
		{
			if (CommonClass == nullptr)
			{
				CommonClass = BoundObj->GetClass();
			}
			else
			{
				while (!BoundObj->IsA(CommonClass))
				{
					CommonClass = CommonClass->GetSuperClass();
				}
			}
		}
	}

	if (CommonClass == nullptr)
	{
		CommonClass = UObject::StaticClass();
	}
	return CommonClass;
}

/*******************************************************************************
 * FBlueprintBoundMenuItem
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintBoundMenuItem::FBlueprintBoundMenuItem(UBlueprintNodeSpawner const* BoundSpawnerIn, int32 MenuGrouping/* = 0*/)
	: BoundSpawner(BoundSpawnerIn) 
{
	Grouping = MenuGrouping;
}

//------------------------------------------------------------------------------
UEdGraphNode* FBlueprintBoundMenuItem::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D const Location, bool bSelectNewNode/* = true*/)
{
	UEdGraphNode* NewNode = nullptr;
	FVector2D CurrentLocation = Location;

	IBlueprintNodeBinder::FBindingSet Bindings;
	for (auto BoundObjIt = BoundObjects.CreateConstIterator(); BoundObjIt; )
	{
		do
		{
			if (BoundObjIt->IsValid())
			{
				Bindings.Add(BoundObjIt->Get());
			}
			++BoundObjIt;

		} while ( BoundObjIt && (BoundSpawner->CanBindMultipleObjects() || (Bindings.Num() == 0)) );

		FBlueprintActionMenuItem BlueprintActionItem(BoundSpawner, Bindings);
		NewNode = BlueprintActionItem.PerformAction(ParentGraph, FromPin, CurrentLocation, bSelectNewNode);

		// Increase the node location a safe distance so follow-up nodes are not stacked
		CurrentLocation.Y += UEdGraphSchema_K2::EstimateNodeHeight(NewNode);

		// Clear the bindings array for the next pass
		Bindings.Empty();

		// @TODO: select ALL spawned nodes
	}
	
	return NewNode;
}

//------------------------------------------------------------------------------
UEdGraphNode* FBlueprintBoundMenuItem::PerformAction(UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, FVector2D const Location, bool bSelectNewNode/* = true*/)
{
	UEdGraphPin* FromPin = nullptr;
	if (FromPins.Num() > 0)
	{
		FromPin = FromPins[0];
	}
	
	UEdGraphNode* SpawnedNode = PerformAction(ParentGraph, FromPin, Location, bSelectNewNode);
	// try auto-wiring the rest of the pins (if there are any)
	for (int32 PinIndex = 1; PinIndex < FromPins.Num(); ++PinIndex)
	{
		SpawnedNode->AutowireNewNode(FromPins[PinIndex]);
	}
	
	return SpawnedNode;
}

//------------------------------------------------------------------------------
void FBlueprintBoundMenuItem::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	
	// these don't get saved to disk, but we want to make sure the objects don't
	// get GC'd while the action array is around
	Collector.AddReferencedObject(BoundSpawner);
}

//------------------------------------------------------------------------------
void FBlueprintBoundMenuItem::AddBindings(IBlueprintNodeBinder::FBindingSet const& BindingSet)
{
	BoundObjects.Append(BindingSet);

	// Allow the binding to generate a menu description:
	FText NewMenuDescription = BoundSpawner->GetDefaultMenuName(BoundObjects);
	if(!NewMenuDescription.IsEmpty())
	{
		MenuDescription = NewMenuDescription;
	}
}

//------------------------------------------------------------------------------
FSlateBrush const* FBlueprintBoundMenuItem::GetMenuIcon(FSlateColor& ColorOut)
{
	// @TODO: 
	return nullptr;
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner const* FBlueprintBoundMenuItem::GetBoundAction() const
{
	return BoundSpawner;
}

#undef LOCTEXT_NAMESPACE
