// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "EdGraphSchema_K2.h"	// for FBlueprintMetadata

/*******************************************************************************
 * UBlueprintFunctionNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
// Evolved from FK2ActionMenuBuilder::AddSpawnInfoForFunction()
UBlueprintFunctionNodeSpawner* UBlueprintFunctionNodeSpawner::Create(UFunction const* const Function)
{
	check(Function != nullptr);

	bool const bIsPure = Function->HasAllFunctionFlags(FUNC_BlueprintPure);
	bool const bHasArrayPointerParms = Function->HasMetaData(TEXT("ArrayParm"));
	bool const bIsCommutativeAssociativeBinaryOp = Function->HasMetaData(FBlueprintMetadata::MD_CommutativeAssociativeBinaryOperator);
	bool const bIsMaterialParamCollectionFunc = Function->HasMetaData(FBlueprintMetadata::MD_MaterialParameterCollectionFunction);

	UBlueprintFunctionNodeSpawner* NodeSpawner = NewObject<UBlueprintFunctionNodeSpawner>(GetTransientPackage());
	NodeSpawner->Function  = Function;

	if (bIsCommutativeAssociativeBinaryOp && bIsPure)
	{
		NodeSpawner->NodeClass = UK2Node_CommutativeAssociativeBinaryOperator::StaticClass();
	}
	else if (bIsMaterialParamCollectionFunc)
	{
		NodeSpawner->NodeClass = UK2Node_CallMaterialParameterCollectionFunction::StaticClass();
	}
	// @TODO:
	//   else if CallOnMember => UK2Node_CallFunctionOnMember
	//   else if bIsParentContext => UK2Node_CallParentFunction
	else if (bHasArrayPointerParms)
	{
		NodeSpawner->NodeClass = UK2Node_CallArrayFunction::StaticClass();
	}
	else
	{
		NodeSpawner->NodeClass = UK2Node_CallFunction::StaticClass();
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
UEdGraphNode* UBlueprintFunctionNodeSpawner::Invoke(UEdGraph* ParentGraph) const
{
	UEdGraphNode* NewNode = Super::Invoke(ParentGraph);
	// user could have changed the node class (to something like
	// UK2Node_BaseAsyncTask, which also wraps a function)
	if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(NewNode))
	{
		FuncNode->SetFromFunction(Function);
	}
	
	bool bIsTemplateNode = ParentGraph->HasAnyFlags(RF_Transient);
	if (CustomizeNodeDelegate.IsBound())
	{
		CustomizeNodeDelegate.Execute(NewNode, bIsTemplateNode);
	}
	if (!bIsTemplateNode)
	{
		NewNode->SetFlags(RF_Transactional);
		NewNode->AllocateDefaultPins();
	}
 	
	return NewNode;
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
