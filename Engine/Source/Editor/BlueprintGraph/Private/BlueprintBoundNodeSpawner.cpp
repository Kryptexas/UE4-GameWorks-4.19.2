// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintBoundNodeSpawner.h"
#include "K2Node_ComponentBoundEvent.h"
#include "BlueprintPropertyNodeSpawner.h"
#include "K2Node_AddComponent.h"
#include "K2Node_CallFunction.h"
#include "ComponentAssetBroker.h"
#include "Editor/EditorEngine.h"	// for GetFriendlyName()
#include "ObjectEditorUtils.h"		// for GetCategory()

#define LOCTEXT_NAMESPACE "BlueprintBoundNodeSpawner"

/*******************************************************************************
 * Static UBlueprintBoundNodeSpawner Helpers
 ******************************************************************************/

namespace BlueprintBoundNodeSpawnerImpl
{
	/**
	 * For UK2Node_AddComponent nodes. Spawns the component with a certain 
	 * asset (mesh, child-actor, etc.).
	 * 
	 * @param  NewNode	The newly spawned node that needs to be bound.
	 * @param  AssetObj	The asset object you want bound to the node.
	 */
	static void BindAddComponentNodeWithAsset(UEdGraphNode* NewNode, UObject* AssetObj);

	/**
	 * Binds function nodes to specific properties. Results in a call to a 
	 * function on the supplied property.
	 * 
	 * @param  NewNode		The newly spawned node that needs to be bound.
	 * @param  PropertyObj	The property object you want bound to the node.
	 */
	static void BindFunctionNodeWithProperty(UEdGraphNode* NewNode, UObject* PropertyObj);

	/**
	 * 
	 * 
	 * @param  NewNode	
	 * @param  DelegateProperty	
	 * @return 
	 */
	static void BindEventNodeToDelegate(UEdGraphNode* NewNode, UObject* DelegateProperty);
};

//------------------------------------------------------------------------------
static void BlueprintBoundNodeSpawnerImpl::BindAddComponentNodeWithAsset(UEdGraphNode* NewNode, UObject* AssetObj)
{
	// @TODO: assert the UObject is an asset
	UK2Node_AddComponent* AddCompNode = CastChecked<UK2Node_AddComponent>(NewNode);
	
	if (UActorComponent* ComponentTemplate = AddCompNode->GetTemplateFromNode())
	{
		FComponentAssetBrokerage::AssignAssetToComponent(ComponentTemplate, AssetObj);
	}
}

//------------------------------------------------------------------------------
static void BlueprintBoundNodeSpawnerImpl::BindFunctionNodeWithProperty(UEdGraphNode* NewNode, UObject* PropertyObj)
{
	UK2Node_CallFunction* FuncNode = CastChecked<UK2Node_CallFunction>(NewNode);
	if (UProperty const* BindingProperty = Cast<UProperty>(PropertyObj))
	{
		// @TODO: unnecessary allocation, could do this all by hand, or with a local
		UBlueprintNodeSpawner* TempNodeSpawner = UBlueprintPropertyNodeSpawner::Create<UK2Node_VariableGet>(BindingProperty);
		
		float const EstimatedVarNodeWidth = 224.0f;
		FVector2D VarNodePos;
		VarNodePos.X = FuncNode->NodePosX - EstimatedVarNodeWidth;
		VarNodePos.Y = FuncNode->NodePosY;
		
		float const EstimatedVarNodeHeight  = 48.0f;
		float const EstimatedFuncNodeHeight = UEdGraphSchema_K2::EstimateNodeHeight(FuncNode);
		float const FuncNodeMidYCoordinate  = FuncNode->NodePosY + (EstimatedFuncNodeHeight / 2.0f);
		VarNodePos.Y = FuncNodeMidYCoordinate - (EstimatedVarNodeWidth / 2.0f);
		
		UEdGraph* ParentGraph = FuncNode->GetGraph();
		UK2Node_VariableGet* GetVarNode = CastChecked<UK2Node_VariableGet>(TempNodeSpawner->Invoke(ParentGraph, VarNodePos));

		ParentGraph->Modify();
		ParentGraph->AddNode(GetVarNode, /*bFromUI =*/false, /*bSelectNewNode =*/false); 
		
		UEdGraphPin* LiteralOutput = GetVarNode->GetValuePin();
		UEdGraphPin* CallSelfInput = FuncNode->FindPin(GetDefault<UEdGraphSchema_K2>()->PN_Self);
		// connect the new "get-var" node with the spawned function node
		if ((LiteralOutput != nullptr) && (CallSelfInput != nullptr))
		{
			LiteralOutput->MakeLinkTo(CallSelfInput);
		}
	}
}

//------------------------------------------------------------------------------
static void BlueprintBoundNodeSpawnerImpl::BindEventNodeToDelegate(UEdGraphNode* NewNode, UObject* InDelegateProperty)
{
	UMulticastDelegateProperty* DelegateProperty = CastChecked<UMulticastDelegateProperty>(InDelegateProperty);
	if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(NewNode))
	{
		FVector2D const SpawnLocation(EventNode->NodePosX, EventNode->NodePosY);
		FVector2D const EventNodeOffset(-150.f, -150.f);
		EventNode->NodePosX += EventNodeOffset.X;
		EventNode->NodePosY += EventNodeOffset.Y;

		// @TODO: unnecessary allocation, could do this all by hand, or with a local
		UBlueprintNodeSpawner* DelegateNodeSpawner = UBlueprintPropertyNodeSpawner::Create<UK2Node_AddDelegate>(DelegateProperty);

		UEdGraph* ParentGraph = EventNode->GetGraph();
		UK2Node_AddDelegate* DelegateNode = CastChecked<UK2Node_AddDelegate>(DelegateNodeSpawner->Invoke(ParentGraph, SpawnLocation));

		ParentGraph->Modify();
		ParentGraph->AddNode(DelegateNode, /*bFromUI =*/false, /*bSelectNewNode =*/false);

		UEdGraphPin* DelegateInputPin = DelegateNode->GetDelegatePin();
		UEdGraphPin* EventDelegatePin = EventNode->FindPinChecked(UK2Node_Event::DelegateOutputName);

		if ((DelegateInputPin != nullptr) && (EventDelegatePin != nullptr))
		{
			UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
			if (K2Schema->TryCreateConnection(EventDelegatePin, DelegateInputPin))
			{
				// refresh the node once connected to the delegate (it should
				// conform it's pins to match the delegate signature)
				EventNode->ReconstructNode();
			}
		}
	}
}

/*******************************************************************************
 * UBlueprintBoundNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner* UBlueprintBoundNodeSpawner::Create(TSubclassOf<UEdGraphNode> NodeClass, UObject* BoundObject, UObject* Outer/*= nullptr*/)
{
	UBlueprintNodeSpawner* SubSpawner = UBlueprintNodeSpawner::Create(NodeClass);
	return Create(SubSpawner, BoundObject, Outer);
}

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner* UBlueprintBoundNodeSpawner::Create(UBlueprintNodeSpawner* SubSpawner, UObject* BoundObject, UObject* Outer/*= nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}
	check(SubSpawner != nullptr);

	UBlueprintBoundNodeSpawner* NodeSpawner = NewObject<UBlueprintBoundNodeSpawner>(Outer);
	NodeSpawner->NodeClass   = SubSpawner->NodeClass;
	NodeSpawner->SubSpawner  = DuplicateObject(SubSpawner, NodeSpawner);
	NodeSpawner->SubSpawner->CustomizeNodeDelegate = SubSpawner->CustomizeNodeDelegate;
	NodeSpawner->BoundObjPtr = BoundObject;

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner::UBlueprintBoundNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
	, BoundObjPtr(nullptr)
{
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintBoundNodeSpawner::Invoke(UEdGraph* ParentGraph, FVector2D const Location) const
{
	using namespace BlueprintBoundNodeSpawnerImpl;
	check(SubSpawner != nullptr);

	UEdGraphNode* NewNode = SubSpawner->Invoke(ParentGraph, Location);
	
	UObject* BoundObj = nullptr;
	if (BoundObjPtr.IsValid())
	{
		BoundObj = BoundObjPtr.Get();
	}
	
	bool const bIsTemplateNode = (ParentGraph->GetOutermost() == GetTransientPackage());
	if (!bIsTemplateNode && NewNode->IsA<UK2Node_AddComponent>())
	{
		BindAddComponentNodeWithAsset(NewNode, BoundObj);
	}
	else if (NewNode->IsA<UK2Node_CallFunction>())
	{
		BindFunctionNodeWithProperty(NewNode, BoundObj);
	}
	else if (BoundObj != nullptr)
	{
		if (!bIsTemplateNode && BoundObj->IsA<UMulticastDelegateProperty>())
		{
			BindEventNodeToDelegate(NewNode, BoundObj);
		}
	}
	
	return NewNode;
}

//------------------------------------------------------------------------------
FText UBlueprintBoundNodeSpawner::GetDefaultMenuName() const
{
	check(SubSpawner != nullptr);

	bool const bShowFriendlyNames = GetDefault<UEditorStyleSettings>()->bShowFriendlyNames;
	
	FText MenuName;
	if (NodeClass == UK2Node_ComponentBoundEvent::StaticClass())
	{
		if (UBlueprintPropertyNodeSpawner* PropSpawner = Cast<UBlueprintPropertyNodeSpawner>(SubSpawner))
		{
			FText const PropertyName = FText::FromName(PropSpawner->GetProperty()->GetFName());
			MenuName = FText::Format(LOCTEXT("ComponentEventName", "Add {0}"), PropertyName);
		}
	}
	else if (BoundObjPtr.IsValid() && BoundObjPtr->IsA<UMulticastDelegateProperty>())
	{
		UMulticastDelegateProperty* DelegateProperty = CastChecked<UMulticastDelegateProperty>(BoundObjPtr.Get());
		FText const PropertyName = FText::FromString(bShowFriendlyNames ? UEditorEngine::GetFriendlyName(DelegateProperty) : DelegateProperty->GetName());

		MenuName = FText::Format(LOCTEXT("ComponentEventName", "Assign {0}"), PropertyName);
	}
	else
	{
		MenuName = SubSpawner->GetDefaultMenuName();
	}
		
	return MenuName;
}

//------------------------------------------------------------------------------
FText UBlueprintBoundNodeSpawner::GetDefaultMenuCategory() const
{
	check(BoundObjPtr != nullptr);
	check(SubSpawner != nullptr);
	
	FText DefaultCategory = SubSpawner->GetDefaultMenuCategory();
	if (NodeClass == UK2Node_ComponentBoundEvent::StaticClass())
	{
		FText const PropertyName = FText::FromName(BoundObjPtr->GetFName());
		DefaultCategory = FText::Format(LOCTEXT("ComponentEventCategory", "Add Event for {0}|{1}"), PropertyName, DefaultCategory);
	}
	else if (NodeClass == UK2Node_CallFunction::StaticClass())
	{
		FText const PropertyName = FText::FromName(BoundObjPtr->GetFName());
		DefaultCategory = FText::Format(LOCTEXT("ComponentEventCategory", "Call Function on {0}|{1}"), PropertyName, DefaultCategory);
	}
	else if (UMulticastDelegateProperty* DelegateProperty = Cast<UMulticastDelegateProperty>(BoundObjPtr.Get()))
	{
		DefaultCategory = FText::FromString(FObjectEditorUtils::GetCategory(DelegateProperty));
		if (DefaultCategory.IsEmpty())
		{
			DefaultCategory = LOCTEXT("BoundDelegateCategory", "Event Dispatchers");
		}
	}	
	return DefaultCategory;
}

//------------------------------------------------------------------------------
UObject const* UBlueprintBoundNodeSpawner::GetBoundObject() const
{
	UObject* RawBoundObj = nullptr;
	if (BoundObjPtr.IsValid())
	{
		RawBoundObj = BoundObjPtr.Get();
	}
	return RawBoundObj;
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner const* UBlueprintBoundNodeSpawner::GetSubSpawner() const
{
	return SubSpawner;
}

#undef LOCTEXT_NAMESPACE
