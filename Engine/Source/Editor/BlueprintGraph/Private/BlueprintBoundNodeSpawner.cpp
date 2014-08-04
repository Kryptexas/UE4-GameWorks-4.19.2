// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintComponentNodeSpawner.h"
#include "K2Node_ComponentBoundEvent.h"
#include "BlueprintPropertyNodeSpawner.h"
#include "K2Node_AddComponent.h"
#include "K2Node_CallFunction.h"
#include "ComponentAssetBroker.h"

#define LOCTEXT_NAMESPACE "BlueprintBoundNodeSpawner"

/*******************************************************************************
 * Static UBlueprintBoundNodeSpawner Helpers
 ******************************************************************************/

namespace BlueprintBoundNodeSpawnerImpl
{
	static void BindAddComponentNodeWithAsset(UEdGraphNode* NewNode, UObject* AssetObj);
	static void BindFunctionNodeWithProperty(UEdGraphNode* NewNode, UObject* PropertyObj);
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
		
		UEdGraphPin* LiteralOutput = GetVarNode->GetValuePin();
		UEdGraphPin* CallSelfInput = FuncNode->FindPin(GetDefault<UEdGraphSchema_K2>()->PN_Self);
		// connect the new "get-var" node with the spawned function node
		if ((LiteralOutput != nullptr) && (CallSelfInput != nullptr))
		{
			LiteralOutput->MakeLinkTo(CallSelfInput);
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
	
	bool const bIsTemplateNode = ParentGraph->HasAnyFlags(RF_Transient);
	if (!bIsTemplateNode && NewNode->IsA<UK2Node_AddComponent>())
	{
		BindAddComponentNodeWithAsset(NewNode, BoundObj);
	}
	else if (NewNode->IsA<UK2Node_CallFunction>())
	{
		BindFunctionNodeWithProperty(NewNode, BoundObj);
	}
	
	return NewNode;
}

//------------------------------------------------------------------------------
FText UBlueprintBoundNodeSpawner::GetDefaultMenuName() const
{
	check(SubSpawner != nullptr);
	
	FText MenuName;
	if (NodeClass == UK2Node_ComponentBoundEvent::StaticClass())
	{
		if (UBlueprintPropertyNodeSpawner* PropSpawner = Cast<UBlueprintPropertyNodeSpawner>(SubSpawner))
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
