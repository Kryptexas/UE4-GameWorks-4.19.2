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
	 * @param  BoundSpawner	
	 * @return 
	 */
	static FText GetMenuCategoryFromAction(UBlueprintNodeSpawner* BoundSpawner);

	/**
	 * 
	 * 
	 * @param  ObjectsList	
	 * @return 
	 */
	static UClass* FindCommonBaseClass(TSet< TWeakObjectPtr<UObject> > const& ObjectsList);
}

//------------------------------------------------------------------------------
static FText BlueprintBoundMenuItemImpl::GetMenuCategoryFromAction(UBlueprintNodeSpawner* BoundSpawner)
{
	TSet< TWeakObjectPtr<UObject> > const& BoundObjects = BoundSpawner->GetBindings();
	check(BoundObjects.Num() > 0);

	FText BoundObjectName;
	if (BoundObjects.Num() > 1)
	{
		UClass* CommonBaseClass = FindCommonBaseClass(BoundObjects);
		BoundObjectName = FText::Format(LOCTEXT("MultipleBindingsCategoryPostfix", "Selected {0}s"), CommonBaseClass->GetDisplayNameText());
	}
	else if (UObject const* BoundObject = BoundObjects.CreateConstIterator()->Get())
	{
		BoundObjectName = FText::FromName(BoundObject->GetFName());
	}

	FText MenuCategory = BoundSpawner->GetDefaultMenuCategory();
	if (BoundSpawner->NodeClass->IsChildOf<UK2Node_Event>())
	{
		MenuCategory = FText::Format(LOCTEXT("ComponentEventCategory", "Add Event for {0}|{1}"), BoundObjectName, MenuCategory);
	}
	else if (BoundSpawner->NodeClass->IsChildOf<UK2Node_CallFunction>())
	{
		MenuCategory = FText::Format(LOCTEXT("ComponentEventCategory", "Call Function on {0}|{1}"), BoundObjectName, MenuCategory);
	}

	return MenuCategory;
}

//------------------------------------------------------------------------------
static UClass* BlueprintBoundMenuItemImpl::FindCommonBaseClass(TSet< TWeakObjectPtr<UObject> > const& ObjectsList)
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
FBlueprintBoundMenuItem::FBlueprintBoundMenuItem(UBlueprintNodeSpawner* BoundSpawnerIn, int32 MenuGrouping/* = 0*/)
	: BoundSpawner(BoundSpawnerIn) 
{
	Grouping = MenuGrouping;
	Category = BlueprintBoundMenuItemImpl::GetMenuCategoryFromAction(BoundSpawnerIn).ToString();
}

//------------------------------------------------------------------------------
UEdGraphNode* FBlueprintBoundMenuItem::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D const Location, bool bSelectNewNode/* = true*/)
{
	FBlueprintActionMenuItem BlueprintActionItem(BoundSpawner, nullptr, FSlateColor(FLinearColor::White));

	UEdGraphNode* NewNode = nullptr;
	for (auto BoundObjIt = BoundObjects.CreateConstIterator(); BoundObjIt; )
	{
		do
		{
			if (BoundObjIt->IsValid())
			{
				BoundSpawner->AddBinding(BoundObjIt->Get());
			}
			++BoundObjIt;

		} while (BoundObjIt && (BoundSpawner->CanBindMultipleObjects() || !BoundSpawner->IsBindingSet()));

		NewNode = BlueprintActionItem.PerformAction(ParentGraph, FromPin, Location, bSelectNewNode);
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
bool FBlueprintBoundMenuItem::AddBinding(UObject const* Binding)
{
	bool const bIsCompatible = (Binding != nullptr) && BoundSpawner->CanBind(Binding);
	if (bIsCompatible)
	{
		BoundObjects.Add(Binding);
	}
	return bIsCompatible;
}

//------------------------------------------------------------------------------
FSlateBrush const* FBlueprintBoundMenuItem::GetMenuIcon(FSlateColor& ColorOut)
{
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
