// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintUnloadedAssetNodeSpawner.h"

#define LOCTEXT_NAMESPACE "BlueprintUnloadedAssetNodeSpawner"

/*******************************************************************************
 * BlueprintUnloadedAssetNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintUnloadedAssetNodeSpawner* UBlueprintUnloadedAssetNodeSpawner::Create(TSubclassOf<UEdGraphNode> NodeClass, const FAssetData& AssetData, FText ActionMenuDescription, UObject* Outer/* = nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintUnloadedAssetNodeSpawner* NodeSpawner = NewObject<UBlueprintUnloadedAssetNodeSpawner>(Outer);
	NodeSpawner->NodeClass = NodeClass;
	NodeSpawner->AssetData = AssetData;
	NodeSpawner->ActionMenuDescription = ActionMenuDescription;

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintUnloadedAssetNodeSpawner::UBlueprintUnloadedAssetNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
FText UBlueprintUnloadedAssetNodeSpawner::GetDefaultMenuName(FBindingSet const& Bindings) const
{
	return ActionMenuDescription;
}

#undef LOCTEXT_NAMESPACE
