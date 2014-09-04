// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "EdGraphSchema_K2.h"	// for FBlueprintMetadata
#include "GameFramework/Actor.h"
#include "BlueprintVariableNodeSpawner.h"

#define LOCTEXT_NAMESPACE "BlueprintFunctionNodeSpawner"

/*******************************************************************************
 * Static UBlueprintFunctionNodeSpawner Helpers
 ******************************************************************************/

//------------------------------------------------------------------------------
namespace BlueprintFunctionNodeSpawnerImpl
{
	FVector2D BindingOffset = FVector2D::ZeroVector;

	/**
	 * 
	 * 
	 * @param  NewNode	
	 * @param  BoundObject	
	 * @return 
	 */
	static bool BindFunctionNode(UK2Node_CallFunction* NewNode, UObject* BoundObject);

	/**
	 * 
	 * 
	 * @param  NewNode	
	 * @param  BindingSpawner	
	 * @return 
	 */
	template <class NodeType>
	static bool BindFunctionNode(UK2Node_CallFunction* NewNode, UBlueprintNodeSpawner* BindingSpawner);

	/**
	 * 
	 * 
	 * @param  InputNode
	 * @return 
	 */
	static FVector2D CalculateBindingPosition(UEdGraphNode* const InputNode);
}

//------------------------------------------------------------------------------
static bool BlueprintFunctionNodeSpawnerImpl::BindFunctionNode(UK2Node_CallFunction* NewNode, UObject* BoundObject)
{
	bool bSuccessfulBinding = false;

	bool const bIsTemplateNode = (NewNode->GetOutermost() == GetTransientPackage());
	if (!bIsTemplateNode)
	{
		if (UProperty const* BoundProperty = Cast<UProperty>(BoundObject))
		{
			UBlueprintNodeSpawner* TempNodeSpawner = UBlueprintVariableNodeSpawner::Create(UK2Node_VariableGet::StaticClass(), BoundProperty);
			bSuccessfulBinding = BindFunctionNode<UK2Node_VariableGet>(NewNode, TempNodeSpawner);
		}
		else if (AActor* BoundActor = Cast<AActor>(BoundObject))
		{
			auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, AActor* ActorInst)
			{
				UK2Node_Literal* ActorRefNode = CastChecked<UK2Node_Literal>(NewNode);
				ActorRefNode->SetObjectRef(ActorInst);
			};
			UBlueprintNodeSpawner::FCustomizeNodeDelegate PostSpawnDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda, BoundActor);
			
			UBlueprintNodeSpawner* TempNodeSpawner = UBlueprintNodeSpawner::Create<UK2Node_Literal>(/*Outer =*/GetTransientPackage(), PostSpawnDelegate);
			bSuccessfulBinding = BindFunctionNode<UK2Node_Literal>(NewNode, TempNodeSpawner);
		}		
	}
	return bSuccessfulBinding;
}

//------------------------------------------------------------------------------
template <class NodeType>
static bool BlueprintFunctionNodeSpawnerImpl::BindFunctionNode(UK2Node_CallFunction* NewNode, UBlueprintNodeSpawner* BindingSpawner)
{
	bool bSuccessfulBinding = false;

	FVector2D BindingPos = CalculateBindingPosition(NewNode);
	UEdGraph* ParentGraph = NewNode->GetGraph();
	NodeType* BindingNode = CastChecked<NodeType>(BindingSpawner->Invoke(ParentGraph, BindingPos));

	BindingOffset.Y += UEdGraphSchema_K2::EstimateNodeHeight(BindingNode);

	// @TODO: Move this down into Invoke()
	ParentGraph->Modify();
	ParentGraph->AddNode(BindingNode, /*bFromUI =*/false, /*bSelectNewNode =*/false);

	UEdGraphPin* LiteralOutput = BindingNode->GetValuePin();
	UEdGraphPin* CallSelfInput = NewNode->FindPin(GetDefault<UEdGraphSchema_K2>()->PN_Self);
	// connect the new "get-var" node with the spawned function node
	if ((LiteralOutput != nullptr) && (CallSelfInput != nullptr))
	{
		LiteralOutput->MakeLinkTo(CallSelfInput);
		bSuccessfulBinding = true;
	}

	return bSuccessfulBinding;
}

//------------------------------------------------------------------------------
static FVector2D BlueprintFunctionNodeSpawnerImpl::CalculateBindingPosition(UEdGraphNode* const InputNode)
{
	float const EstimatedVarNodeWidth = 224.0f;
	FVector2D AttachingNodePos;
	AttachingNodePos.X = InputNode->NodePosX - EstimatedVarNodeWidth;
	AttachingNodePos.Y = InputNode->NodePosY;

	float const EstimatedVarNodeHeight = 48.0f;
	float const EstimatedFuncNodeHeight = UEdGraphSchema_K2::EstimateNodeHeight(InputNode);
	float const FuncNodeMidYCoordinate = InputNode->NodePosY + (EstimatedFuncNodeHeight / 2.0f);
	AttachingNodePos.Y = FuncNodeMidYCoordinate - (EstimatedVarNodeWidth / 2.0f);

	AttachingNodePos += BindingOffset;
	return AttachingNodePos;
}

/*******************************************************************************
 * UBlueprintFunctionNodeSpawner
 ******************************************************************************/

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
	UEdGraphNode* SpawnedNode = Super::Invoke(ParentGraph, Location, PostSpawnSetupDelegate);

	// if this spawner was set up to spawn 
	BlueprintFunctionNodeSpawnerImpl::BindingOffset = FVector2D::ZeroVector;
	Bind(SpawnedNode);

	return SpawnedNode;
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
FString UBlueprintFunctionNodeSpawner::GetDefaultSearchKeywords() const
{
	check(Function != nullptr);
	return UK2Node_CallFunction::GetKeywordsForFunction(Function);
}

//------------------------------------------------------------------------------
bool UBlueprintFunctionNodeSpawner::CanBindMultipleObjects() const
{
	check(Function != nullptr);
	return UK2Node_CallFunction::CanFunctionSupportMultipleTargets(Function);
}

//------------------------------------------------------------------------------
bool UBlueprintFunctionNodeSpawner::CanBind(UObject const* BindingCandidate) const
{
	bool bCanBind = false;
	if (Function != nullptr)
	{
		// @TODO: don't know exactly why we can only bind non-pure or cost 
		//        functions... this is mirrored after FK2ActionMenuBuilder::GetFunctionCallsOnSelectedActors()
		//        and FK2ActionMenuBuilder::GetFunctionCallsOnSelectedComponents(),
		//        where we make the same stipulation
		bool const bIsImperative = !Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
		bool const bIsConstFunc  =  Function->HasAnyFunctionFlags(FUNC_Const);

		if (bIsImperative || bIsConstFunc)
		{
			UClass* BindingClass = BindingCandidate->GetClass();
			if (UObjectProperty const* ObjectProperty = Cast<UObjectProperty>(BindingCandidate))
			{
				BindingClass = ObjectProperty->PropertyClass;
			}

			if (UClass const* FuncOwner = Function->GetOwnerClass())
			{
				bCanBind = BindingClass->IsChildOf(FuncOwner);
			}
		}
	}
	return bCanBind;
}

//------------------------------------------------------------------------------
bool UBlueprintFunctionNodeSpawner::BindToNode(UEdGraphNode* Node, UObject* Binding) const
{
	return BlueprintFunctionNodeSpawnerImpl::BindFunctionNode(CastChecked<UK2Node_CallFunction>(Node), Binding);
}

//------------------------------------------------------------------------------
UFunction const* UBlueprintFunctionNodeSpawner::GetFunction() const
{
	return Function;
}

#undef LOCTEXT_NAMESPACE
