// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintFieldNodeSpawner.h"

#define LOCTEXT_NAMESPACE "BlueprintFieldNodeSpawner"

//------------------------------------------------------------------------------
UBlueprintFieldNodeSpawner* UBlueprintFieldNodeSpawner::Create(TSubclassOf<UK2Node> NodeClass, UField const* const Field, UObject* Outer/* = nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}
	UBlueprintFieldNodeSpawner* NodeSpawner = NewObject<UBlueprintFieldNodeSpawner>(Outer);
	NodeSpawner->Field     = Field;
	NodeSpawner->NodeClass = NodeClass;
	
	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintFieldNodeSpawner::UBlueprintFieldNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
	, Field(nullptr)
{
}

//------------------------------------------------------------------------------
FBlueprintNodeSpawnerSignature UBlueprintFieldNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSpawnerSignature SpawnerSignature = Super::GetSpawnerSignature();
	SpawnerSignature.AddSubObject(Field);

	return SpawnerSignature;
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintFieldNodeSpawner::Invoke(UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location) const
{
	auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, UField const* Field, FSetNodeFieldDelegate SetFieldDelegate, FCustomizeNodeDelegate UserDelegate)
	{
		SetFieldDelegate.ExecuteIfBound(NewNode, Field);
		UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
	};

	FCustomizeNodeDelegate PostSpawnSetupDelegate = FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda, GetField(), SetNodeFieldDelegate, CustomizeNodeDelegate);
	UEdGraphNode* SpawnedNode = Super::Invoke(ParentGraph, Bindings, Location, PostSpawnSetupDelegate);

	return SpawnedNode;
}

//------------------------------------------------------------------------------
UField const* UBlueprintFieldNodeSpawner::GetField() const
{
	return Field;
}

#undef LOCTEXT_NAMESPACE
