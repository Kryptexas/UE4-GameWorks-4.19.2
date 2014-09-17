// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintActionDatabase.h"
#include "EdGraphSchema_K2.h"       // for CanUserKismetCallFunction()
#include "KismetEditorUtilities.h"	// for IsClassABlueprintSkeleton(), IsClassABlueprintInterface(), GetBoundsForSelectedNodes(), etc.
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintDelegateNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintComponentNodeSpawner.h"
#include "BlueprintBoundEventNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#include "Stats.h"					// for RETURN_QUICK_DECLARE_CYCLE_STAT()
#include "ScopedTimers.h"			// for FScopedDurationTimer
#include "CallbackDevice.h"			// for FCoreDelegates::OnAssetLoaded
#include "ModuleManager.h"
#include "AssetRegistryModule.h"	// for OnAssetAdded()/OnAssetRemoved()
#include "BlueprintEditorUtils.h"	// for FindBlueprintForGraph()
#include "BlueprintActionDatabaseRegistrar.h"

// used below in FBlueprintNodeSpawnerFactory::MakeMacroNodeSpawner()
#include "K2Node_MacroInstance.h"
// used below in FBlueprintNodeSpawnerFactory::MakeComponentBoundEventSpawner()/MakeActorBoundEventSpawner()
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_ActorBoundEvent.h"
// used below in FBlueprintNodeSpawnerFactory::MakeMessageNodeSpawner()
#include "K2Node_Message.h"
// used below in BlueprintActionDatabaseImpl::AddClassPropertyActions()
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "K2Node_ClearDelegate.h"
#include "K2Node_AssignDelegate.h"
// used below in BlueprintActionDatabaseImpl::AddClassCastActions()
#include "K2Node_DynamicCast.h"
#include "K2Node_ClassDynamicCast.h"
// used below in BlueprintActionDatabaseImpl::GetNodeSpectificActions()
#include "EdGraph/EdGraphNode_Comment.h"
#include "EdGraph/EdGraphNode_Documentation.h"

#define LOCTEXT_NAMESPACE "BlueprintActionDatabase"

/*******************************************************************************
 * FBlueprintNodeSpawnerFactory
 ******************************************************************************/

namespace FBlueprintNodeSpawnerFactory
{
	/**
	 * Constructs a UK2Node_MacroInstance spawner. Evolved from 
	 * FK2ActionMenuBuilder::AttachMacroGraphAction(). Sets up the spawner to 
	 * set spawned nodes with the supplied macro.
	 *
	 * @param  MacroGraph	The macro you want spawned nodes referencing.
	 * @return A new node-spawner, setup to spawn a UK2Node_MacroInstance.
	 */
	static UBlueprintNodeSpawner* MakeMacroNodeSpawner(UEdGraph* MacroGraph);

	/**
	 * Constructs a UK2Node_Message spawner. Sets up the spawner to set 
	 * spawned nodes with the supplied function.
	 *
	 * @param  InterfaceFunction	The function you want spawned nodes referencing.
	 * @return A new node-spawner, setup to spawn a UK2Node_Message.
	 */
	static UBlueprintNodeSpawner* MakeMessageNodeSpawner(UFunction* InterfaceFunction);

	/**
	 * Constructs a UEdGraphNode_Comment spawner. Since UEdGraphNode_Comment is
	 * not a UK2Node then we can't have it create a spawner for itself (using 
	 * UK2Node's GetMenuActions() method).
	 *
	 * @param  DocNodeType	The node class type that you want the spawner to be responsible for.
	 *
	 * @return A new node-spawner, setup to spawn a UEdGraphNode_Comment.
	 */
	template <class DocNodeType>
	static UBlueprintNodeSpawner* MakeDocumentationNodeSpawner();

	/**
	 * Constructs a delegate binding node along with a connected event that is  
	 * triggered from the specified delegate.
	 * 
	 * @param  DelegateProperty	The delegate the spawner will bind to.
	 * @return A new node-spawner, setup to spawn a UK2Node_AssignDelegate.
	 */
	static UBlueprintNodeSpawner* MakeAssignDelegateNodeSpawner(UMulticastDelegateProperty* DelegateProperty);

	/**
	 * 
	 * 
	 * @param  DelegateProperty	
	 * @return 
	 */
	static UBlueprintNodeSpawner* MakeComponentBoundEventSpawner(UMulticastDelegateProperty* DelegateProperty);

	/**
	 * 
	 * 
	 * @param  DelegateProperty	
	 * @return 
	 */
	static UBlueprintNodeSpawner* MakeActorBoundEventSpawner(UMulticastDelegateProperty* DelegateProperty);
};

//------------------------------------------------------------------------------
static UBlueprintNodeSpawner* FBlueprintNodeSpawnerFactory::MakeMacroNodeSpawner(UEdGraph* MacroGraph)
{
	check(MacroGraph != nullptr);
	check(MacroGraph->GetSchema()->GetGraphType(MacroGraph) == GT_Macro);

	UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(UK2Node_MacroInstance::StaticClass());
	check(NodeSpawner != nullptr);

	auto CustomizeMacroNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, TWeakObjectPtr<UEdGraph> MacroGraph)
	{
		UK2Node_MacroInstance* MacroNode = CastChecked<UK2Node_MacroInstance>(NewNode);
		if (MacroGraph.IsValid())
		{
			MacroNode->SetMacroGraph(MacroGraph.Get());
		}
	};

	TWeakObjectPtr<UEdGraph> GraphPtr = MacroGraph;
	// @TODO: Needs access to the MacroGraph to be favoritable...
	NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeMacroNodeLambda, GraphPtr);

	return NodeSpawner;
}

//------------------------------------------------------------------------------
static UBlueprintNodeSpawner* FBlueprintNodeSpawnerFactory::MakeMessageNodeSpawner(UFunction* InterfaceFunction)
{
	check(InterfaceFunction != nullptr);
	check(FKismetEditorUtilities::IsClassABlueprintInterface(CastChecked<UClass>(InterfaceFunction->GetOuter())));

	UBlueprintFunctionNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(UK2Node_Message::StaticClass(), InterfaceFunction);
	check(NodeSpawner != nullptr);

	auto SetNodeFunctionLambda = [](UEdGraphNode* NewNode, UField const* FuncField)
	{
		UK2Node_Message* MessageNode = CastChecked<UK2Node_Message>(NewNode);
		MessageNode->FunctionReference.SetExternalMember(FuncField->GetFName(), FuncField->GetOwnerClass());
	};
	NodeSpawner->SetNodeFieldDelegate = UBlueprintFunctionNodeSpawner::FSetNodeFieldDelegate::CreateStatic(SetNodeFunctionLambda);

	return NodeSpawner;
}

//------------------------------------------------------------------------------
template <class DocNodeType>
static UBlueprintNodeSpawner* FBlueprintNodeSpawnerFactory::MakeDocumentationNodeSpawner()
{
	UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(DocNodeType::StaticClass());
	check(NodeSpawner != nullptr);

	auto CustomizeMessageNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode)
	{
		DocNodeType* DocNode = CastChecked<DocNodeType>(NewNode);

		UEdGraph* OuterGraph = NewNode->GetGraph();
		check(OuterGraph != nullptr);
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(OuterGraph);
		check(Blueprint != nullptr);

		float const OldNodePosX = NewNode->NodePosX;
		float const OldNodePosY = NewNode->NodePosY;

		static const float DocNodePadding = 50.0f;
		FSlateRect Bounds(OldNodePosX - DocNodePadding, OldNodePosY - DocNodePadding, OldNodePosX + DocNodePadding, OldNodePosY + DocNodePadding);
		FKismetEditorUtilities::GetBoundsForSelectedNodes(Blueprint, Bounds, DocNodePadding);
		DocNode->SetBounds(Bounds);
	};

	NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeMessageNodeLambda);

	return NodeSpawner;
}

//------------------------------------------------------------------------------
static UBlueprintNodeSpawner* FBlueprintNodeSpawnerFactory::MakeAssignDelegateNodeSpawner(UMulticastDelegateProperty* DelegateProperty)
{
	// @TODO: it'd be awesome to have both nodes spawned by this available for 
	//        context pin matching (the delegate inputs and the event outputs)
	return UBlueprintDelegateNodeSpawner::Create(UK2Node_AssignDelegate::StaticClass(), DelegateProperty);
}

//------------------------------------------------------------------------------
static UBlueprintNodeSpawner* FBlueprintNodeSpawnerFactory::MakeComponentBoundEventSpawner(UMulticastDelegateProperty* DelegateProperty)
{
	return UBlueprintBoundEventNodeSpawner::Create(UK2Node_ComponentBoundEvent::StaticClass(), DelegateProperty);
}

//------------------------------------------------------------------------------
static UBlueprintNodeSpawner* FBlueprintNodeSpawnerFactory::MakeActorBoundEventSpawner(UMulticastDelegateProperty* DelegateProperty)
{
	return UBlueprintBoundEventNodeSpawner::Create(UK2Node_ActorBoundEvent::StaticClass(), DelegateProperty);
}

/*******************************************************************************
 * Static FBlueprintActionDatabase Helpers
 ******************************************************************************/

namespace BlueprintActionDatabaseImpl
{
	typedef FBlueprintActionDatabase::FActionList FActionList;

	/**
	 * Mimics UEdGraphSchema_K2::CanUserKismetAccessVariable(); however, this 
	 * omits the filtering that CanUserKismetAccessVariable() does (saves that 
	 * for later with FBlueprintActionFilter).
	 * 
	 * @param  Property		The property you want to check.
	 * @return True if the property can be seen from a blueprint.
	 */
	static bool IsPropertyBlueprintVisible(UProperty const* const Property);

	/**
	 * 
	 * 
	 * @param  Class	
	 * @param  ActionListOut	
	 */
	static void GetClassMemberActions(UClass* const Class, FActionList& ActionListOut);

	/**
	 * Loops over all of the class's functions and creates a node-spawners for
	 * any that are viable for blueprint use. Evolved from 
	 * FK2ActionMenuBuilder::GetFuncNodesForClass(), plus a series of other
	 * FK2ActionMenuBuilder methods (GetAllInterfaceMessageActions,
	 * GetEventsForBlueprint, etc). 
	 *
	 * Ideally, any node that is constructed from a UFunction should go in here 
	 * (so we only ever loop through the class's functions once). We handle
	 * UK2Node_CallFunction alongside UK2Node_Event.
	 *
	 * @param  Class			The class whose functions you want node-spawners for.
	 * @param  ActionListOut	The list you want populated with the new spawners.
	 */
	static void AddClassFunctionActions(UClass const* const Class, FActionList& ActionListOut);

	/**
	 * Loops over all of the class's properties and creates node-spawners for 
	 * any that are viable for blueprint use. Evolved from certain parts of 
	 * FK2ActionMenuBuilder::GetAllActionsForClass().
	 *
	 * @param  Class			The class whose properties you want node-spawners for.
	 * @param  ActionListOut	The list you want populated with the new spawners.
	 */
	static void AddClassPropertyActions(UClass const* const Class, FActionList& ActionListOut);

	/**
	 * Evolved from FClassDynamicCastHelper::GetClassDynamicCastNodes(). If the
	 * specified class is a viable blueprint variable type, then two cast nodes
	 * are added for it (UK2Node_DynamicCast, and UK2Node_ClassDynamicCast).
	 *
	 * @param  Class			The class who you want cast nodes for (they cast to this class).
	 * @param  ActionListOut	The list you want populated with the new spawners.
	 */
	static void AddClassCastActions(UClass* const Class, FActionList& ActionListOut);

	/**
	 * Evolved from K2ActionMenuBuilder's GetAddComponentClasses(). If the
	 * specified class is a component type (and can be spawned), then a 
	 * UK2Node_AddComponent spawner is created and added to ActionListOut.
	 *
	 * @param  ComponentClass	The class who you want a spawner for (the component class).
	 * @param  ActionListOut	The list you want populated with the new spawner.
	 */
	static void AddComponentClassActions(TSubclassOf<UActorComponent> const ComponentClass, FActionList& ActionListOut);

	/**
	 * If the associated class is a blueprint generated class, then this will
	 * loop over the blueprint's graphs and create any node-spawners associated
	 * with those graphs (like UK2Node_MacroInstance spawners for macro graphs).
	 *
	 * @param  Blueprint		The blueprint which you want graph associated node-spawners for.
	 * @param  ActionListOut	The list you want populated with new spawners.
	 */
	static void AddBlueprintGraphActions(UBlueprint const* const Blueprint, FActionList& ActionListOut);

	/**
	 * Emulates UEdGraphSchema::GetGraphContextActions(). If the supplied class  
	 * is a node type, then it will query the node's CDO for any actions it 
	 * wishes to add. This helps us keep the code in this file paired down, and 
	 * makes it easily extensible for new node types.
	 * 
	 * @param  NodeClass		The class which you want node-spawners for.
	 * @param  ActionListOut	The list you want populated with new spawners.
	 */
	static void GetNodeSpecificActions(TSubclassOf<UEdGraphNode const> const NodeClass, FBlueprintActionDatabaseRegistrar& Registrar);

	/**
	 * Callback to refresh the database when a blueprint has been altered 
	 * (clears the database entries for the blueprint's classes and recreates 
	 * them... a property/function could have been added/removed).
	 * 
	 * @param  InBlueprint	The blueprint that has been altered.
	 */
	static void OnBlueprintChanged(UBlueprint* InBlueprint);

	/**
	 * Callback to refresh the database when a new object has just been loaded 
	 * (to catch blueprint classes that weren't in the initial set, etc.) 
	 * 
	 * @param  NewObject	The object that was just loaded.
	 */
	static void OnAssetLoaded(UObject* NewObject);

	/**
	 * Callback to refresh the database when a new object has just been created 
	 * (to catch new blueprint classes that weren't in the initial set, etc.) 
	 * 
	 * @param  NewAssetInfo	Data regarding the newly added asset.
	 */
	static void OnAssetAdded(FAssetData const& NewAssetInfo);

	/**
	 * Callback to clear out object references so that a object can be deleted 
	 * without resistance from the actions cached here. Objects passed here
	 * won't necessarily be deleted (the user could still choose to cancel).
	 * 
	 * @param  ObjectsForDelete	A list of objects that MIGHT be deleted.
	 */
	static void OnAssetsPendingDelete(TArray<UObject*> const& ObjectsForDelete);

	/**
	 * Callback to refresh the database when an object has been delete (to clear 
	 * any related classes that were stored in the database) 
	 * 
	 * @param  AssetInfo	Data regarding the freshly removed asset.
	 */
	static void OnAssetRemoved(FAssetData const& AssetInfo);

	/**
	 * Callback to clear all levels from the database when a world is destroyed 
	 * 
	 * @param  DestroyedWorld	The world that was destroyed
	 */
	static void OnWorldDestroyed(UWorld* DestroyedWorld);

	/**
	 * Returns TRUE if the Object is valid for the database
	 *
	 * @param Object		Object to check for validity
	 * @return				TRUE if the Blueprint is valid for the database
	 */
	static bool IsObjectValidForDatabase(UObject const* Object);

	/** 
	 * Assets that we cleared from the database (to remove references, and make 
	 * way for a delete), but in-case the class wasn't deleted we need them 
	 * tracked here so we can add them back in.
	 */
	TSet<UObject*> PendingDelete;

	/** */
	bool bIsInitializing = false;
}

//------------------------------------------------------------------------------
static bool BlueprintActionDatabaseImpl::IsPropertyBlueprintVisible(UProperty const* const Property)
{
	bool const bIsAccessible = Property->HasAllPropertyFlags(CPF_BlueprintVisible);

	bool const bIsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
	bool const bIsAssignableOrCallable = Property->HasAnyPropertyFlags(CPF_BlueprintAssignable | CPF_BlueprintCallable);

	return !Property->HasAnyPropertyFlags(CPF_Parm) && (bIsAccessible || (bIsDelegate && bIsAssignableOrCallable));
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::GetClassMemberActions(UClass* const Class, FActionList& ActionListOut)
{
	// class field actions (nodes that represent and perform actions on
	// specific fields of the class... functions, properties, etc.)
	{
		AddClassFunctionActions(Class, ActionListOut);
		AddClassPropertyActions(Class, ActionListOut);
		// class UEnum actions are added by individual nodes via GetNodeSpecificActions()
		// class UScriptStruct actions are added by individual nodes via GetNodeSpecificActions()
	}

	AddClassCastActions(Class, ActionListOut);

	if (Class->IsChildOf<UActorComponent>())
	{
		AddComponentClassActions(Class, ActionListOut);
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::AddClassFunctionActions(UClass const* const Class, FActionList& ActionListOut)
{
	using namespace FBlueprintNodeSpawnerFactory; // for MakeMessageNodeSpawner()
	check(Class != nullptr);

	// loop over all the functions in the specified class; exclude-super because 
	// we can always get the super functions by looking up that class separately 
	for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
	{
		UFunction* Function = *FunctionIt;

		if (UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(Function))
		{
			if (UBlueprintEventNodeSpawner* NodeSpawner = UBlueprintEventNodeSpawner::Create(Function))
			{
				ActionListOut.Add(NodeSpawner);
			}
		}
		
		if (UEdGraphSchema_K2::CanUserKismetCallFunction(Function))
		{
			// @TODO: if this is a Blueprint, and this function is from a 
			//        Blueprint "implemented interface", then we don't need to 
			//        include it (the function is accounted for in from the 
			//        interface class).
			UBlueprintFunctionNodeSpawner* FuncSpawner = UBlueprintFunctionNodeSpawner::Create(Function);
			ActionListOut.Add(FuncSpawner);

			if (FKismetEditorUtilities::IsClassABlueprintInterface(Class))
			{
				ActionListOut.Add(MakeMessageNodeSpawner(Function));
			}
		}
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::AddClassPropertyActions(UClass const* const Class, FActionList& ActionListOut)
{
	using namespace FBlueprintNodeSpawnerFactory; // for MakeDelegateNodeSpawner()

	bool const bIsComponent  = Class->IsChildOf<UActorComponent>();
	bool const bIsActorClass = Class->IsChildOf<AActor>();
	
	// loop over all the properties in the specified class; exclude-super because 
	// we can always get the super properties by looking up that class separately 
	for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		if (!IsPropertyBlueprintVisible(Property))
		{
			continue;
		}

 		bool const bIsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
 		if (bIsDelegate)
 		{
			UMulticastDelegateProperty* DelegateProperty = CastChecked<UMulticastDelegateProperty>(Property);
			if (DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable))
			{
				UBlueprintNodeSpawner* AddSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_AddDelegate::StaticClass(), DelegateProperty);
				ActionListOut.Add(AddSpawner);
				
				UBlueprintNodeSpawner* AssignSpawner = MakeAssignDelegateNodeSpawner(DelegateProperty);
				ActionListOut.Add(AssignSpawner);
			}
			
			if (DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintCallable))
			{
				UBlueprintNodeSpawner* CallSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_CallDelegate::StaticClass(), DelegateProperty);
				ActionListOut.Add(CallSpawner);
			}
			
			UBlueprintNodeSpawner* RemoveSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_RemoveDelegate::StaticClass(), DelegateProperty);
			ActionListOut.Add(RemoveSpawner);
			UBlueprintNodeSpawner* ClearSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_ClearDelegate::StaticClass(), DelegateProperty);
			ActionListOut.Add(ClearSpawner);

			if (bIsComponent)
			{
				ActionListOut.Add(MakeComponentBoundEventSpawner(DelegateProperty));
			}
			else if (bIsActorClass)
			{
				ActionListOut.Add(MakeActorBoundEventSpawner(DelegateProperty));
			}
 		}
		else
		{
			UBlueprintVariableNodeSpawner* GetterSpawner = UBlueprintVariableNodeSpawner::Create(UK2Node_VariableGet::StaticClass(), Property);
			ActionListOut.Add(GetterSpawner);
			UBlueprintVariableNodeSpawner* SetterSpawner = UBlueprintVariableNodeSpawner::Create(UK2Node_VariableSet::StaticClass(), Property);
			ActionListOut.Add(SetterSpawner);
		}
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::AddClassCastActions(UClass* const Class, FActionList& ActionListOut)
{
	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
	bool bIsCastPermitted  = UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Class);

	if (bIsCastPermitted)
	{
		auto CustomizeCastNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, UClass* TargetType)
		{
			UK2Node_DynamicCast* CastNode = CastChecked<UK2Node_DynamicCast>(NewNode);
			CastNode->TargetType = TargetType;
		};

		UBlueprintNodeSpawner* CastObjNodeSpawner = UBlueprintNodeSpawner::Create(UK2Node_DynamicCast::StaticClass());
		// @TODO: Needs access to the Class to be favoritable...
		CastObjNodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeCastNodeLambda, Class);
		ActionListOut.Add(CastObjNodeSpawner);

		UBlueprintNodeSpawner* CastClassNodeSpawner = UBlueprintNodeSpawner::Create(UK2Node_ClassDynamicCast::StaticClass());
		CastClassNodeSpawner->CustomizeNodeDelegate = CastObjNodeSpawner->CustomizeNodeDelegate;
		ActionListOut.Add(CastClassNodeSpawner);
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::AddComponentClassActions(TSubclassOf<UActorComponent> const ComponentClass, FActionList& ActionListOut)
{
	if ((ComponentClass != nullptr) && !ComponentClass->HasAnyClassFlags(CLASS_Abstract) && ComponentClass->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent))
	{
		if (UBlueprintComponentNodeSpawner* NodeSpawner = UBlueprintComponentNodeSpawner::Create(ComponentClass))
		{
			ActionListOut.Add(NodeSpawner);
		}
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::AddBlueprintGraphActions(UBlueprint const* const Blueprint, FActionList& ActionListOut)
{
	using namespace FBlueprintNodeSpawnerFactory; // for MakeMacroNodeSpawner()

	for (auto GraphIt = Blueprint->MacroGraphs.CreateConstIterator(); GraphIt; ++GraphIt)
	{
		ActionListOut.Add(MakeMacroNodeSpawner(*GraphIt));
	}

	// local variables
	for (auto GraphIt = Blueprint->FunctionGraphs.CreateConstIterator(); GraphIt; ++GraphIt)
	{
		UEdGraph* FunctionGraph = (*GraphIt);

		TArray<UK2Node_FunctionEntry*> GraphEntryNodes;
		FunctionGraph->GetNodesOfClass<UK2Node_FunctionEntry>(GraphEntryNodes);

		for (UK2Node_FunctionEntry* FunctionEntry : GraphEntryNodes)
		{
			for (FBPVariableDescription const& LocalVar : FunctionEntry->LocalVariables)
			{
				UBlueprintNodeSpawner* GetVarSpawner = UBlueprintVariableNodeSpawner::Create(UK2Node_VariableGet::StaticClass(), FunctionGraph, LocalVar);
				ActionListOut.Add(GetVarSpawner);
				UBlueprintNodeSpawner* SetVarSpawner = UBlueprintVariableNodeSpawner::Create(UK2Node_VariableSet::StaticClass(), FunctionGraph, LocalVar);
				ActionListOut.Add(SetVarSpawner);
			}
		}
	}	
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::GetNodeSpecificActions(TSubclassOf<UEdGraphNode const> const NodeClass, FBlueprintActionDatabaseRegistrar& Registrar)
{
	using namespace FBlueprintNodeSpawnerFactory; // for MakeDocumentationNodeSpawner()

	if (NodeClass->IsChildOf<UK2Node>() && !NodeClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UK2Node const* const NodeCDO = NodeClass->GetDefaultObject<UK2Node>();
		check(NodeCDO != nullptr);
		NodeCDO->GetMenuActions(Registrar);
	}
	// unfortunately, UEdGraphNode_Comment is not a UK2Node and therefore cannot
	// leverage UK2Node's GetMenuActions() function, so here we HACK it in
	//
	// @TODO: DO NOT follow this example! Do as I say, not as I do! If we need
	//        to support other nodes in a similar way, then we should come up
	//        with a better (more generalized) solution.
	else if (NodeClass == UEdGraphNode_Comment::StaticClass())
	{
		Registrar.AddBlueprintAction(MakeDocumentationNodeSpawner<UEdGraphNode_Comment>());
	}
	else if (NodeClass == UEdGraphNode_Documentation::StaticClass())
	{
		// @TODO: BOO! (see comment above)
		Registrar.AddBlueprintAction(MakeDocumentationNodeSpawner<UEdGraphNode_Documentation>());
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::OnBlueprintChanged(UBlueprint* Blueprint)
{
	if (IsObjectValidForDatabase(Blueprint))
	{
		FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
		ActionDatabase.RefreshAssetActions(Blueprint);
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::OnAssetLoaded(UObject* NewObject)
{
	if (UBlueprint* NewBlueprint = Cast<UBlueprint>(NewObject))
	{
		OnBlueprintChanged(NewBlueprint);
		NewBlueprint->OnChanged().AddStatic(&BlueprintActionDatabaseImpl::OnBlueprintChanged);
	}
	else
	{
		FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
		ActionDatabase.RefreshAssetActions(NewObject);
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::OnAssetAdded(FAssetData const& NewAssetInfo)
{
	if (NewAssetInfo.IsAssetLoaded())
	{
		UObject* AssetObject = NewAssetInfo.GetAsset();
		if (UBlueprint* NewBlueprint = Cast<UBlueprint>(AssetObject))
		{
			FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
			// this callback can be triggered twice from the asset registry (once 
			// when it is initially created, and again when it is permanently 
			// saved)
			bool const bNewlyCreated = (ActionDatabase.GetAllActions().Find(NewBlueprint) == nullptr);

			OnBlueprintChanged(NewBlueprint);
			// have to be careful not to register this callback twice for the 
			// blueprint (since this function can be triggered multiple times 
			// for the same asset)
			if (bNewlyCreated)
			{
				NewBlueprint->OnChanged().AddStatic(&BlueprintActionDatabaseImpl::OnBlueprintChanged);
			}			
		}
		else 
		{
			FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
			ActionDatabase.RefreshAssetActions(AssetObject);
		}
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::OnAssetsPendingDelete(TArray<UObject*> const& ObjectsForDelete)
{
	FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
	for (UObject* DeletingObject : ObjectsForDelete)
	{
		// have to temporarily remove references (so that this delete isn't 
		// blocked by dangling references)
		ActionDatabase.ClearAssetActions(DeletingObject);

		// in case they choose not to delete the object, we need to add 
		// these back in to the database, so we track them here
		PendingDelete.Add(DeletingObject);
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::OnAssetRemoved(FAssetData const& AssetInfo)
{
	FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();

	if (AssetInfo.IsAssetLoaded())
	{
		UObject* AssetObject = AssetInfo.GetAsset();
		if (UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetObject))
		{
			BlueprintAsset->OnChanged().RemoveStatic(&BlueprintActionDatabaseImpl::OnBlueprintChanged);
		}
		// the delete went through, so we don't need to track these for re-add
		PendingDelete.Remove(AssetObject);
	}
}

//------------------------------------------------------------------------------
static void BlueprintActionDatabaseImpl::OnWorldDestroyed(UWorld* DestroyedWorld)
{
	for(auto It = DestroyedWorld->GetLevelIterator() ; It ; ++It)
	{
		if(const ULevelScriptBlueprint* LevelScript = (*It)->GetLevelScriptBlueprint(true))
		{
			FBlueprintActionDatabase::Get().ClearAssetActions((UObject*)LevelScript);
		}
	}
}

//------------------------------------------------------------------------------
static bool BlueprintActionDatabaseImpl::IsObjectValidForDatabase(UObject const* Object)
{
	bool bReturn = false;
	if(Object->IsAsset())
	{
		bReturn = true;
	}
	else if(UBlueprint const* Blueprint = Cast<UBlueprint>(Object))
	{
		// Level scripts are sometimes not assets because they have not been saved yet, but they are still valid for the database.
		bReturn = FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint);
	}
	return bReturn;
}

/*******************************************************************************
 * FBlueprintActionDatabase
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintActionDatabase& FBlueprintActionDatabase::Get()
{
	static FBlueprintActionDatabase* DatabaseInst = nullptr;
	if (DatabaseInst == nullptr)
	{
		DatabaseInst = new FBlueprintActionDatabase();
	}

	return *DatabaseInst;
}

//------------------------------------------------------------------------------
FBlueprintActionDatabase::FBlueprintActionDatabase()
{
	RefreshAll();
	for (TObjectIterator<UBlueprint> BpIt; BpIt; ++BpIt)
	{
		BpIt->OnChanged().AddStatic(&BlueprintActionDatabaseImpl::OnBlueprintChanged);
	}
	FCoreDelegates::OnAssetLoaded.AddStatic(&BlueprintActionDatabaseImpl::OnAssetLoaded);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.OnAssetAdded().AddStatic(&BlueprintActionDatabaseImpl::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddStatic(&BlueprintActionDatabaseImpl::OnAssetRemoved);

	FEditorDelegates::OnAssetsPreDelete.AddStatic(&BlueprintActionDatabaseImpl::OnAssetsPendingDelete);

	GEngine->OnWorldDestroyed().AddStatic(&BlueprintActionDatabaseImpl::OnWorldDestroyed);
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabase::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (auto& ActionListIt : ActionRegistry)
	{
		Collector.AddReferencedObjects(ActionListIt.Value);
	}
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabase::Tick(float DeltaTime)
{
	const double DurationThreshold = FMath::Min(0.003, DeltaTime * 0.01);

	// entries that were removed from the database, in preparation for a delete
	// (but the user ended up not deleting the object)
	for (UObject const* AssetObj : BlueprintActionDatabaseImpl::PendingDelete)
	{
		if (IsValid(AssetObj))
		{
			RefreshAssetActions(AssetObj);
		}
	}
	BlueprintActionDatabaseImpl::PendingDelete.Empty();

	double TotalFrameDuration = 0.0;
	while ((ActionPrimingQueue.Num() > 0) && (TotalFrameDuration < DurationThreshold))
	{
		auto ActionIndex = ActionPrimingQueue.CreateIterator();	
			
		TWeakObjectPtr<UObject> ActionsKey = ActionIndex.Key();
		if (ActionsKey.IsValid())
		{
			// make sure this class is still listed in the database
			if (FActionList* ClassActionList = ActionRegistry.Find(ActionsKey.Get()))
			{
				int32& ActionListIndex = ActionIndex.Value();
				for (; (ActionListIndex < ClassActionList->Num()) && (TotalFrameDuration < DurationThreshold); ++ActionListIndex)
				{
					FScopedDurationTimer ScopedTimer(TotalFrameDuration);

					UBlueprintNodeSpawner* Action = (*ClassActionList)[ActionListIndex];
					Action->Prime();
				}

				if (ActionListIndex >= ClassActionList->Num())
				{
					ActionPrimingQueue.Remove(ActionsKey);
				}
			}
			else
			{
				ActionPrimingQueue.Remove(ActionsKey);
			}
		}
		else
		{
			ActionPrimingQueue.Remove(ActionsKey);
		}
	}
}

//------------------------------------------------------------------------------
TStatId FBlueprintActionDatabase::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FBlueprintActionDatabase, STATGROUP_Tickables);
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabase::RefreshAll()
{
	TGuardValue<bool> ScopedInitialization(BlueprintActionDatabaseImpl::bIsInitializing, true);

	ActionRegistry.Empty();
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* const Class = (*ClassIt);
		RefreshClassActions(Class);
	}
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabase::RefreshClassActions(UClass* const Class)
{
	using namespace BlueprintActionDatabaseImpl;
	check(Class != nullptr);

	bool const bOutOfDateClass   = Class->HasAnyClassFlags(CLASS_NewerVersionExists);
	bool const bIsBlueprintClass = (Cast<UBlueprintGeneratedClass>(Class) != nullptr);	

	if (bOutOfDateClass)
	{
		ActionRegistry.Remove(Class);
		return;
	}
	else if (bIsBlueprintClass)
	{
		UBlueprint const* Blueprint = Cast<UBlueprint>(Class->ClassGeneratedBy);
		if ((Blueprint != nullptr) && BlueprintActionDatabaseImpl::IsObjectValidForDatabase(Blueprint))
		{
			// to prevent us from hitting this twice on init (once for the skel 
			// class, again for the generated class)
			bool const bRefresh = !bIsInitializing || (Blueprint->SkeletonGeneratedClass == nullptr) ||
				(Blueprint->SkeletonGeneratedClass == Class);

			if (bRefresh)
			{
				RefreshAssetActions(Blueprint);
			}
		}
	}
	// here we account for "autonomous" standalone nodes, and any nodes that
	// exist in a separate module; each UK2Node has a chance to append its
	// own actions (presumably ones that would spawn that node)...
	else if (Class->IsChildOf<UEdGraphNode>())
	{
		FActionList& ClassActionList = ActionRegistry.FindOrAdd(Class);
		if (!bIsInitializing)
		{
			ClassActionList.Empty();
		}

		FBlueprintActionDatabaseRegistrar Registrar(ActionRegistry, ActionPrimingQueue, Class);
		if (!bIsInitializing)
		{
			// if this a call to RefreshClassActions() from somewhere other than 
			// RefreshAll(), then we should only add actions for this class (the
			// node could be adding actions, probably duplicate ones for assets)
			Registrar.ActionKeyFilter = Class;
		}
	
		// also, should catch any actions dealing with global UFields (like
		// global structs, enums, etc.; elements that wouldn't be caught
		// normally when sifting through fields on all known classes)		
		GetNodeSpecificActions(Class, Registrar);
		// don't worry, the registrar marks new actions for priming
	}
	else
	{
		FActionList& ClassActionList = ActionRegistry.FindOrAdd(Class);
		if (!bIsInitializing)
		{
			ClassActionList.Empty();
			// if we're only refreshing this class (and not init'ing the whole 
			// database), then we have to reach out to individual nodes in case 
			// they'd add entries for this as well
			FBlueprintActionDatabaseRegistrar Registrar(ActionRegistry, ActionPrimingQueue);
			Registrar.ActionKeyFilter = Class; // only want actions for this class

			RegisterAllNodeActions(Registrar);
		}
		GetClassMemberActions(Class, ClassActionList);

		// queue the newly added actions for priming
		if (ClassActionList.Num() > 0)
		{
			ActionPrimingQueue.Add(Class, 0);
		}
		else
		{
			ActionRegistry.Remove(Class);
		}
	}

	// @TODO: account for all K2ActionMenuBuilder methods...
	//   GetLiteralsFromActorSelection() - UK2Node_Literal
	//   GetAnimNotifyMenuItems()
	//   GetMatineeControllers() - UK2Node_MatineeController
	//   GetEventDispatcherNodesForClass()
	//   GetBoundEventsFromActorSelection() - handle with filter
	//   GetFunctionCallsOnSelectedActors() - handle with filter
	//   GetAddComponentActionsUsingSelectedAssets() 
	//   GetFunctionCallsOnSelectedComponents() - handle with filter
	//   GetBoundEventsFromComponentSelection() - handle with filter 
	//   GetPinAllowedMacros()
	//   GetFuncNodesWithPinType()
	//   GetVariableGettersSettersForClass()
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabase::RefreshAssetActions(UObject const* const AssetObject)
{
	using namespace BlueprintActionDatabaseImpl;
	check(BlueprintActionDatabaseImpl::IsObjectValidForDatabase(AssetObject));

	FActionList& AssetActionList = ActionRegistry.FindOrAdd(AssetObject);
	AssetActionList.Empty();

	if (UBlueprint const* Blueprint = Cast<UBlueprint>(AssetObject))
	{
		AddBlueprintGraphActions(Blueprint, AssetActionList);
		if (UClass* SkeletonClass = Blueprint->SkeletonGeneratedClass)
		{
			GetClassMemberActions(SkeletonClass, AssetActionList);
		}
	}

	FBlueprintActionDatabaseRegistrar Registrar(ActionRegistry, ActionPrimingQueue);
	Registrar.ActionKeyFilter = AssetObject; // make sure actions only associated with this asset get added
	// nodes may have actions they wish to add actions for this asset
	RegisterAllNodeActions(Registrar);

	if (AssetActionList.Num() > 0)
	{
		// queue these assets for priming
		ActionPrimingQueue.Add(AssetObject, 0);
	}
	else
	{
		ClearAssetActions(AssetObject);
	}
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabase::ClearAssetActions(UObject const* const AssetObject)
{
	check(BlueprintActionDatabaseImpl::IsObjectValidForDatabase(AssetObject));
	ActionRegistry.Remove(AssetObject);
}

//------------------------------------------------------------------------------
FBlueprintActionDatabase::FActionRegistry const& FBlueprintActionDatabase::GetAllActions()
{
	// if this is the first time that we're querying for actions, generate the
	// list before returning it
	if (ActionRegistry.Num() == 0)
	{
		RefreshAll();
	}
	return ActionRegistry;
}

//------------------------------------------------------------------------------
int32 FBlueprintActionDatabase::EstimatedSize() const
{
	int32 TotalSize = sizeof(*this);
	for (TObjectIterator<UBlueprintNodeSpawner> NodeSpawnerIt; NodeSpawnerIt; ++NodeSpawnerIt)
	{
		TotalSize += sizeof(**NodeSpawnerIt);
	}
	return TotalSize;
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabase::RegisterAllNodeActions(FBlueprintActionDatabaseRegistrar& Registrar)
{
	// nodes may have actions they wish to add for this asset
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* NodeClass = *ClassIt;
		if (!NodeClass->IsChildOf<UK2Node>())
		{
			continue;
		}

		TGuardValue< TSubclassOf<UEdGraphNode> > ScopedNodeClass(Registrar.GeneratingClass, NodeClass);
		BlueprintActionDatabaseImpl::GetNodeSpecificActions(NodeClass, Registrar);
	}
}

#undef LOCTEXT_NAMESPACE
