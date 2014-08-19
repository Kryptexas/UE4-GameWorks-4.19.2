// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintBoundNodeSpawner.h"
#include "BlueprintNodeSpawnerUtils.h"
#include "BlueprintVariableNodeSpawner.h"
#include "BlueprintDelegateNodeSpawner.h"
#include "K2Node_AddComponent.h"
#include "K2Node_ComponentBoundEvent.h"
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
	 * 
	 * 
	 * @param  NewNode	
	 * @param  NodeSpawner	
	 * @param  BindingObj	
	 * @return 
	 */
	static bool BindComponentEventNode(UK2Node_ComponentBoundEvent* NewNode, UBlueprintNodeSpawner const* NodeSpawner, UObject* BindingObj);

	/**
	 * For UK2Node_AddComponent nodes. Spawns the component with a certain 
	 * asset (mesh, child-actor, etc.).
	 * 
	 * @param  NewNode	The newly spawned node that needs to be bound.
	 * @param  AssetObj	The asset object you want bound to the node.
	 */
	static bool BindAddComponentNodeWithAsset(UK2Node_AddComponent* NewNode, UObject* AssetObj);

	/**
	 * Binds function nodes to specific properties. Results in a call to a 
	 * function on the supplied property.
	 * 
	 * @param  NewNode		The newly spawned node that needs to be bound.
	 * @param  PropertyObj	The property object you want bound to the node.
	 */
	static bool BindFunctionNodeWithVariable(UK2Node_CallFunction* NewNode, UObject* PropertyObj);
};

//------------------------------------------------------------------------------
static bool BlueprintBoundNodeSpawnerImpl::BindComponentEventNode(UK2Node_ComponentBoundEvent* NewNode, UBlueprintNodeSpawner const* NodeSpawner, UObject* BindingObj)
{
	bool bSuccessfulBinding = false;
	if (UObjectProperty* ComponentProperty = Cast<UObjectProperty>(BindingObj))
	{
		UProperty const* ActionProperty = FBlueprintNodeSpawnerUtils::GetAssociatedProperty(NodeSpawner);
		if (UMulticastDelegateProperty const* DelegateProperty = Cast<UMulticastDelegateProperty>(ActionProperty))
		{
			NewNode->InitializeComponentBoundEventParams(ComponentProperty, DelegateProperty);
			bSuccessfulBinding = true;
		}
	}

	return bSuccessfulBinding;
}

//------------------------------------------------------------------------------
static bool BlueprintBoundNodeSpawnerImpl::BindAddComponentNodeWithAsset(UK2Node_AddComponent* NewNode, UObject* AssetObj)
{
	bool bSuccessfulBinding = false;

	bool const bIsTemplateNode = (NewNode->GetOutermost() == GetTransientPackage());
	if (!bIsTemplateNode)
	{
		if (UActorComponent* ComponentTemplate = NewNode->GetTemplateFromNode())
		{
			// @TODO: ensure the UObject is an asset
			bSuccessfulBinding = FComponentAssetBrokerage::AssignAssetToComponent(ComponentTemplate, AssetObj);
		}
	}	
	return bSuccessfulBinding;
}

//------------------------------------------------------------------------------
static bool BlueprintBoundNodeSpawnerImpl::BindFunctionNodeWithVariable(UK2Node_CallFunction* NewNode, UObject* PropertyObj)
{
	bool bSuccessfulBinding = false;
	UProperty const* BindingProperty = Cast<UProperty>(PropertyObj);

	bool const bIsTemplateNode = (NewNode->GetOutermost() == GetTransientPackage());
	// @TODO: if we get it so pins can be matched with connected nodes, then we
	//        should forgo the bIsTemplateNode check and let it bind.
	if ((BindingProperty != nullptr) && bIsTemplateNode)
	{
		// @TODO: unnecessary allocation, could do this all by hand, or with a local
		UBlueprintNodeSpawner* TempNodeSpawner = UBlueprintVariableNodeSpawner::Create(UK2Node_VariableGet::StaticClass(), BindingProperty);
		
		float const EstimatedVarNodeWidth = 224.0f;
		FVector2D VarNodePos;
		VarNodePos.X = NewNode->NodePosX - EstimatedVarNodeWidth;
		VarNodePos.Y = NewNode->NodePosY;
		
		float const EstimatedVarNodeHeight  = 48.0f;
		float const EstimatedFuncNodeHeight = UEdGraphSchema_K2::EstimateNodeHeight(NewNode);
		float const FuncNodeMidYCoordinate = NewNode->NodePosY + (EstimatedFuncNodeHeight / 2.0f);
		VarNodePos.Y = FuncNodeMidYCoordinate - (EstimatedVarNodeWidth / 2.0f);
		
		UEdGraph* ParentGraph = NewNode->GetGraph();
		UK2Node_VariableGet* GetVarNode = CastChecked<UK2Node_VariableGet>(TempNodeSpawner->Invoke(ParentGraph, VarNodePos));

		ParentGraph->Modify();
		ParentGraph->AddNode(GetVarNode, /*bFromUI =*/false, /*bSelectNewNode =*/false); 
		
		UEdGraphPin* LiteralOutput = GetVarNode->GetValuePin();
		UEdGraphPin* CallSelfInput = NewNode->FindPin(GetDefault<UEdGraphSchema_K2>()->PN_Self);
		// connect the new "get-var" node with the spawned function node
		if ((LiteralOutput != nullptr) && (CallSelfInput != nullptr))
		{
			LiteralOutput->MakeLinkTo(CallSelfInput);
			bSuccessfulBinding = true;
		}
	}

	return bSuccessfulBinding;
}

/*******************************************************************************
 * UBlueprintBoundNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner* UBlueprintBoundNodeSpawner::Create(TSubclassOf<UEdGraphNode> NodeClass, UClass* BoundObjType, UObject* Outer/*= nullptr*/)
{
	UBlueprintNodeSpawner* SubSpawner = UBlueprintNodeSpawner::Create(NodeClass);
	return Create(SubSpawner, BoundObjType, Outer);
}

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner* UBlueprintBoundNodeSpawner::Create(UBlueprintNodeSpawner* SubSpawner, UClass* BoundObjType, UObject* Outer/*= nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}
	check(SubSpawner != nullptr);
	check(BoundObjType != nullptr);

	UBlueprintBoundNodeSpawner* NodeSpawner = NewObject<UBlueprintBoundNodeSpawner>(Outer);
	NodeSpawner->NodeClass     = SubSpawner->NodeClass;
	NodeSpawner->SubSpawner    = SubSpawner;
	NodeSpawner->BoundObjClass = BoundObjType;

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner::UBlueprintBoundNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
	, BoundObjClass(UObject::StaticClass())
	, BoundObjPtr(nullptr)
{
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintBoundNodeSpawner::Invoke(UEdGraph* ParentGraph, FVector2D const Location) const
{
	using namespace BlueprintBoundNodeSpawnerImpl;
	check(SubSpawner != nullptr);

	FCustomizeNodeDelegate UserNodeSetup = SubSpawner->CustomizeNodeDelegate;
	// some binding is important to do before the node is fully formed, so we 
	// have to construct this callback for our sub-spawner to use when spawning
	// the node.
	auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, UObject* BoundObject, UBlueprintNodeSpawner* NodeSpawner, FCustomizeNodeDelegate UserDelegate)
	{
		// user could have changed the node class (to something like
		// UK2Node_BaseAsyncTask, which also wraps a function)
		if (UK2Node_ComponentBoundEvent* ComponentEventNode = Cast<UK2Node_ComponentBoundEvent>(NewNode))
		{
			BindComponentEventNode(ComponentEventNode, NodeSpawner, BoundObject);
		}
		else if (UK2Node_AddComponent* AddComponentNode = Cast<UK2Node_AddComponent>(NewNode))
		{
			BindAddComponentNodeWithAsset(AddComponentNode, BoundObject);
		}

		UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
	};

	UObject* BoundObj = BoundObjPtr.Get();
	SubSpawner->CustomizeNodeDelegate = FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda, BoundObj, SubSpawner, UserNodeSetup);

	UEdGraphNode* NewNode = SubSpawner->Invoke(ParentGraph, Location);
	// restore the old custom delegate on the sub-spawner
	SubSpawner->CustomizeNodeDelegate = UserNodeSetup;

	// this simply spawns an additional node and attaches it, so it is ok if 
	// we bind this after the sub-spawner's Invoke() (it doesn't affect the 
	// initial node setup).
	if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(NewNode))
	{
		BindFunctionNodeWithVariable(FuncNode, BoundObj);
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
		if (UBlueprintDelegateNodeSpawner* PropSpawner = Cast<UBlueprintDelegateNodeSpawner>(SubSpawner))
		{
			FText const PropertyName = FText::FromName(PropSpawner->GetProperty()->GetFName());
			MenuName = FText::Format(LOCTEXT("ComponentEventName", "Add {0}"), PropertyName);
		}
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
	return DefaultCategory;
}

//------------------------------------------------------------------------------
bool UBlueprintBoundNodeSpawner::IsOfBindingType(UObject const* BindingCandidate) const
{
	bool bIsCompatible = false;
	if (BindingCandidate == nullptr)
	{
		bIsCompatible = (BoundObjClass == nullptr);
	}
	else if (BoundObjClass != nullptr)
	{
		UClass* ObjectClass = BindingCandidate->GetClass();
		bIsCompatible = ObjectClass->IsChildOf(BoundObjClass);

		if (UObjectProperty const* ObjProperty = Cast<UObjectProperty>(BindingCandidate))
		{
			if (UField const* AssociatedField = FBlueprintNodeSpawnerUtils::GetAssociatedField(this))
			{
				AssociatedField->GetOwnerClass()->IsChildOf(ObjProperty->PropertyClass);
			}
		}
	}

	return bIsCompatible;
}

//------------------------------------------------------------------------------
bool UBlueprintBoundNodeSpawner::SetBoundObject(UObject const* Object)
{ 
	bool bSuccess = false;
	if ((Object == nullptr) || IsOfBindingType(Object))
	{	
		bSuccess = true;
		BoundObjPtr = Object;
	}
	else
	{
		ensureMsgf(false, TEXT("Cannot bind object (%s) with this node-spawner, it is not a '%s'"), *Object->GetName(), *BoundObjClass->GetName());
	}
	return bSuccess;
}

//------------------------------------------------------------------------------
UObject const* UBlueprintBoundNodeSpawner::GetBoundObject() const
{
	// Get() does IsValid() checks internally for us
	return BoundObjPtr.Get();
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner const* UBlueprintBoundNodeSpawner::GetSubSpawner() const
{
	return SubSpawner;
}

#undef LOCTEXT_NAMESPACE
