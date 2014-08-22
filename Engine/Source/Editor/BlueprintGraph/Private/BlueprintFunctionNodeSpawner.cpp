// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "EdGraphSchema_K2.h"	// for FBlueprintMetadata

#define LOCTEXT_NAMESPACE "BlueprintFunctionNodeSpawner"

//------------------------------------------------------------------------------
// Evolved from FK2ActionMenuBuilder::AddSpawnInfoForFunction()
UBlueprintFunctionNodeSpawner* UBlueprintFunctionNodeSpawner::Create(UFunction const* const Function, UObject* Outer/* = nullptr*/)
{
	check(Function != nullptr);

	bool const bIsPure = Function->HasAllFunctionFlags(FUNC_BlueprintPure);
	bool const bHasArrayPointerParms = Function->HasMetaData(TEXT("ArrayParm"));
	bool const bIsCommutativeAssociativeBinaryOp = Function->HasMetaData(FBlueprintMetadata::MD_CommutativeAssociativeBinaryOperator);
	bool const bIsMaterialParamCollectionFunc = Function->HasMetaData(FBlueprintMetadata::MD_MaterialParameterCollectionFunction);
	bool const bIsDataTableFunc = Function->HasMetaData(FBlueprintMetadata::MD_DataTablePin);

	TSubclassOf<UK2Node_CallFunction> NodeClass;
	if (bIsCommutativeAssociativeBinaryOp && bIsPure)
	{
		NodeClass = UK2Node_CommutativeAssociativeBinaryOperator::StaticClass();
	}
	else if (bIsMaterialParamCollectionFunc)
	{
		NodeClass = UK2Node_CallMaterialParameterCollectionFunction::StaticClass();
	}
	else if (bIsDataTableFunc)
	{
		NodeClass = UK2Node_CallDataTableFunction::StaticClass();
	}
	// @TODO:
	//   else if CallOnMember => UK2Node_CallFunctionOnMember
	//   else if bIsParentContext => UK2Node_CallParentFunction
	else if (bHasArrayPointerParms)
	{
		NodeClass = UK2Node_CallArrayFunction::StaticClass();
	}
	else
	{
		NodeClass = UK2Node_CallFunction::StaticClass();
	}

	return Create(NodeClass, Function, Outer);
}

//------------------------------------------------------------------------------
UBlueprintFunctionNodeSpawner* UBlueprintFunctionNodeSpawner::Create(TSubclassOf<UK2Node_CallFunction> NodeClass, UFunction const* const Function, UObject* Outer/* = nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}
	UBlueprintFunctionNodeSpawner* NodeSpawner = NewObject<UBlueprintFunctionNodeSpawner>(Outer);
	NodeSpawner->Function = Function;

	if (NodeClass == nullptr)
	{
		NodeSpawner->NodeClass = UK2Node_CallFunction::StaticClass();
	}
	else
	{
		NodeSpawner->NodeClass = NodeClass;
	}

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintFunctionNodeSpawner::UBlueprintFunctionNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
	, Function(nullptr)
{
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintFunctionNodeSpawner::Invoke(UEdGraph* ParentGraph, FVector2D const Location) const
{
	auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, UFunction const* Function, FCustomizeNodeDelegate UserDelegate)
	{
		// user could have changed the node class (to something like
		// UK2Node_BaseAsyncTask, which also wraps a function)
		if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(NewNode))
		{
			FuncNode->SetFromFunction(Function);
		}

		UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
	};

	FCustomizeNodeDelegate PostSpawnSetupDelegate = FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda, Function, CustomizeNodeDelegate);
	return Super::Invoke(ParentGraph, Location, PostSpawnSetupDelegate);
}

//------------------------------------------------------------------------------
FText UBlueprintFunctionNodeSpawner::GetDefaultMenuName() const
{
	check(Function != nullptr);
	return FText::FromString(UK2Node_CallFunction::GetUserFacingFunctionName(Function));
}

//------------------------------------------------------------------------------
FText UBlueprintFunctionNodeSpawner::GetDefaultMenuCategory() const
{
	check(Function != nullptr);
	return FText::FromString(UK2Node_CallFunction::GetDefaultCategoryForFunction(Function, TEXT("")));
}

//------------------------------------------------------------------------------
FText UBlueprintFunctionNodeSpawner::GetDefaultMenuTooltip() const
{
	check(Function != nullptr);
	return FText::FromString(UK2Node_CallFunction::GetDefaultTooltipForFunction(Function));
}

//------------------------------------------------------------------------------
FText UBlueprintFunctionNodeSpawner::GetDefaultSearchKeywords() const
{
	check(Function != nullptr);
	return FText::FromString(UK2Node_CallFunction::GetKeywordsForFunction(Function));
}

//------------------------------------------------------------------------------
UFunction const* UBlueprintFunctionNodeSpawner::GetFunction() const
{
	return Function;
}

#undef LOCTEXT_NAMESPACE
