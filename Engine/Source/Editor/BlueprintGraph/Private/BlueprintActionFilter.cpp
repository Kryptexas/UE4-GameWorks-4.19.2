// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintComponentNodeSpawner.h"
#include "BlueprintPropertyNodeSpawner.h"
#include "EdGraphSchema_K2.h"		// for FBlueprintMetadata
#include "BlueprintEditorUtils.h"	// for FindBlueprintForGraph()
#include "ObjectEditorUtils.h"		// for IsFunctionHiddenFromClass()/IsVariableCategoryHiddenFromClass()
#include "K2Node_VariableSet.h"
// "impure" node types (utilized in BlueprintActionFilterImpl::IsImpure)
#include "K2Node_IfThenElse.h"
#include "K2Node_MultiGate.h"
#include "K2Node_MakeArray.h"
#include "K2Node_Message.h"
#include "K2Node_ExecutionSequence.h"

/*******************************************************************************
 * Static BlueprintActionFilter Helpers
 ******************************************************************************/

namespace BlueprintActionFilterImpl
{	
	/**
	 * All actions are associated with some class, whether it's the class of the
	 * node it will spawn, or the outer of the associated field (property,
	 * function, etc.).
	 * 
	 * @param  BlueprintAction	The node-spawner you want an associated class for.
	 * @return The action's associated class (null if it doesn't have one).
	 */
	static UClass* GetActionClass(UBlueprintNodeSpawner const* BlueprintAction);
	
	/**
	 * Certain node-spawners are associated with specific UFields (functions, 
	 * properties, etc.). This attempts to retrieve the field from the spawner.
	 * 
	 * @param  BlueprintAction	The node-spawner you want an associated field for.
	 * @return The action's associated field (null if it doesn't have one).
	 */
	static UField const* GetAssociatedField(UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Certain node-spawners are associated with specific UFunction (call-
	 * function, and event spawners). This attempts to retrieve the function 
	 * from the spawner.
	 * 
	 * @param  BlueprintAction	The node-spawner you want an associated function for.
	 * @return The action's associated function (null if it doesn't have one).
	 */
	static UFunction const* GetAssociatedFunction(UBlueprintNodeSpawner const* BlueprintAction);
	
	/**
	 * Utility method to check and see if the specified node-spawner would 
	 * produce an impure node.
	 * 
	 * @param  NodeSpawner	The action you want to query.
	 * @return True if the action will spawn an impure node, false if not (or unknown).
	 */
	static bool IsImpure(UBlueprintNodeSpawner const* NodeSpawner);

	/**
	 * Utility method to check and see if the specified node-spawner would 
	 * produce a latent node.
	 * 
	 * @param  NodeSpawner	The action you want to query.
	 * @return True if the action will spawn a latent node, false if not (or unknown).
	 */
	static bool IsLatent(UBlueprintNodeSpawner const* NodeSpawner);
	
	/**
	 * Rejection test that checks to see if the supplied node-spawner would
	 * produce an event that does NOT belong to the specified blueprint.
	 * 
	 * @param  Filter			Holds the blueprint context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the node-spawner would produce an event incompatible with the specified blueprint(s).
	 */
	static bool IsEventUnimplementable(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner has an 
	 * associated field that is not accessible by the blueprint (it's private or
	 * protected).
	 * 
	 * @param  Filter			Holds the blueprint context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the node-spawner is associated with a private field.
	 */
	static bool IsFieldInaccessible(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner has an 
	 * associated class that is "restricted" and thusly, hidden from the 
	 * blueprint.
	 * 
	 * @param  Filter			Holds the blueprint context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the node-spawner belongs to a class that is restricted to certain blueprints (not this one).
	 */
	static bool IsRestrictedClassMember(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a variable-set node when the property is read-only (in this
	 * blueprint).
	 * 
	 * @param  Filter			Holds the blueprint context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a variable-set node for a read-only property.
	 */
	static bool IsPermissionNotGranted(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a node (or comes from an associated class) that is deprecated.
	 * 
	 * @param  Filter			Filter context (unused) for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node that is deprecated.
	 */
	static bool IsDeprecated(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce an impure node, incompatible with the specified graphs.
	 * 
	 * @param  Filter			Holds the graph context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn an impure node (and the graph doesn't allow them).
	 */
	static bool IsIncompatibleImpureNode(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a node incompatible with the specified graph.
	 * 
	 * @param  Filter			Holds the graph context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node (and the graph wouldn't allow it).
	 */
	static bool IsIncompatibleWithGraphType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a node incompatible with an ubergraph.
	 * 
	 * @param  Context			Holds the blueprint/graph/pin context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node not allowed in ubergraphs.
	 */
	static bool IsIncompatibleWithUbergraph(FBlueprintActionContext const& Context, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a node incompatible with an animation graph.
	 * 
	 * @param  Context			Holds the blueprint/graph/pin context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node not allowed in animation graphs.
	 */
	static bool IsIncompatibleWithAnimGraph(FBlueprintActionContext const& Context, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a node incompatible with a macro graph.
	 * 
	 * @param  Context			Holds the blueprint/graph/pin context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node not allowed in macro graphs.
	 */
	static bool IsIncompatibleWithMacroGraph(FBlueprintActionContext const& Context, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce an node incompatible with a function graph.
	 * 
	 * @param  Context			Holds the blueprint/graph/pin context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node not allowed in function graphs.
	 */
	static bool IsIncompatibleWithFuncGraph(FBlueprintActionContext const& Context, UBlueprintNodeSpawner const* BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the node-spawner has any associated
	 * fields that are global (structs, enums, etc.), or if the field belongs to
	 * a static library class.
	 * 
	 * @param  Context			Holds the context (unused) for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node not allowed in function graphs.
	 */
	static bool IsGlobalField(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the node-spawner is associated with 
	 * a field that belongs to a class that is not white-listed (ignores global
	 * fields).
	 * 
	 * @param  Context			Holds the class context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action is associated with a non-whitelisted class member.
	 */
	static bool IsExternalField(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the node-spawner is associated with 
	 * a field that is hidden from the specified blueprint (via metadata).
	 * 
	 * @param  Context			Holds the class context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action is associated with a hidden field.
	 */
	static bool IsFieldCategoryHidden(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the node-spawner would produce a 
	 * node that couldn't be pasted into the specified graph(s) (check against 
	 * the node's CDO to save on perf, to keep from spawning a temp node).
	 * 
	 * @param  Context			Holds the graph context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would produce a node that cannot be pasted in the graph.
	 */
	static bool IsNodeUnpastable(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the node-spawner would produce a 
	 * node type that isn't white-listed. 
	 * 
	 * @param  Context			Holds the class context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @param  bMatchExplicitly When true, the node's class has to perfectly match a white-listed type (sub-classes don't count).
	 * @return True if the action would produce a non-whitelisted node.
	 */
	static bool IsFilteredNodeType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction, bool const bMatchExplicitly = false);
};

//------------------------------------------------------------------------------
static UClass* BlueprintActionFilterImpl::GetActionClass(UBlueprintNodeSpawner const* BlueprintAction)
{
	UClass* ActionClass = BlueprintAction->NodeClass;
	
	if (UBlueprintFunctionNodeSpawner const* FuncNodeSpawner = Cast<UBlueprintFunctionNodeSpawner>(BlueprintAction))
	{
		UFunction const* Function = FuncNodeSpawner->GetFunction();
		check(Function != nullptr);
		ActionClass = Function->GetOuterUClass();
	}
	else if (UBlueprintEventNodeSpawner const* EventSpawner = Cast<UBlueprintEventNodeSpawner>(BlueprintAction))
	{
		UFunction const* EventFunc = EventSpawner->GetEventFunction();
		check(EventFunc != nullptr);
		ActionClass = EventFunc->GetOuterUClass();
	}
	else if (UBlueprintPropertyNodeSpawner const* PropertySpawner = Cast<UBlueprintPropertyNodeSpawner>(BlueprintAction))
	{
		UProperty const* Property = PropertySpawner->GetProperty();
		check(Property != nullptr);
		ActionClass = Cast<UClass>(Property->GetOuterUField());
	}
	else if (UBlueprintComponentNodeSpawner const* ComponentSpawner = Cast<UBlueprintComponentNodeSpawner>(BlueprintAction))
	{
		ActionClass = ComponentSpawner->GetComponentClass();
	}
	
	return ActionClass;
}

//------------------------------------------------------------------------------
static UField const* BlueprintActionFilterImpl::GetAssociatedField(UBlueprintNodeSpawner const* BlueprintAction)
{
	UField const* ClassField = nullptr;

	if (UFunction const* Function = GetAssociatedFunction(BlueprintAction))
	{
		ClassField = Function;
	}
	else if (UBlueprintPropertyNodeSpawner const* PropertySpawner = Cast<UBlueprintPropertyNodeSpawner>(BlueprintAction))
	{
		ClassField = PropertySpawner->GetProperty();
	}

	return ClassField;
}

//------------------------------------------------------------------------------
static UFunction const* BlueprintActionFilterImpl::GetAssociatedFunction(UBlueprintNodeSpawner const* BlueprintAction)
{
	UFunction const* Function = nullptr;

	if (UBlueprintFunctionNodeSpawner const* FuncNodeSpawner = Cast<UBlueprintFunctionNodeSpawner>(BlueprintAction))
	{
		Function = FuncNodeSpawner->GetFunction();
	}
	else if (UBlueprintEventNodeSpawner const* EventSpawner = Cast<UBlueprintEventNodeSpawner>(BlueprintAction))
	{
		Function = EventSpawner->GetEventFunction();
	}

	return Function;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsImpure(UBlueprintNodeSpawner const* NodeSpawner)
{
	bool bIsImpure = false;

	UClass* NodeClass = NodeSpawner->NodeClass;
	if (NodeClass != nullptr)
	{
		// TODO: why are some of these "impure"?
		bIsImpure = (NodeClass == UK2Node_IfThenElse::StaticClass()) ||
			(NodeClass == UK2Node_MultiGate::StaticClass()) ||
			(NodeClass == UK2Node_MakeArray::StaticClass()) ||
			(NodeClass == UK2Node_Message::StaticClass()) ||
			(NodeClass == UK2Node_ExecutionSequence::StaticClass());
	}
	else if (UFunction const* Function = GetAssociatedFunction(NodeSpawner))
	{
		bIsImpure = (Function->HasAnyFunctionFlags(FUNC_BlueprintPure) != false);
	}

	return bIsImpure;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsLatent(UBlueprintNodeSpawner const* NodeSpawner)
{
	bool bIsLatent = false;

	UClass* NodeClass = NodeSpawner->NodeClass;
	if ((NodeClass != nullptr) && NodeClass->IsChildOf<UK2Node_BaseAsyncTask>())
	{
		bIsLatent = true;
	}
	else if (UFunction const* Function = GetAssociatedFunction(NodeSpawner))
	{
		bIsLatent = (Function->HasMetaData(FBlueprintMetadata::MD_Latent) != false);
	}

	return bIsLatent;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsEventUnimplementable(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UBlueprintEventNodeSpawner const* EventSpawner = Cast<UBlueprintEventNodeSpawner>(BlueprintAction))
	{
		UFunction const* EventFunc = EventSpawner->GetEventFunction();
		check(EventFunc != nullptr);
		
		UClass* FuncOwner = EventFunc->GetOuterUClass();
		for (UBlueprint const* Blueprint : FilterContext.Blueprints)
		{
			UClass* BpClass = Blueprint->GeneratedClass;
			check(BpClass != nullptr);
			
			// if this function belongs directly to this blueprint, then it is
			// already implemented here (this action however is valid for sub-
			// classes, as they can override the event's functionality)
			if (FuncOwner == BpClass)
			{
				bIsFilteredOut = true;
				break;
			}
			
			// you can only implement events that you inherit; so if this
			// blueprint is not a subclass of the event's owner, then we're not
			// allowed to implement it
			if (!BpClass->IsChildOf(FuncOwner))
			{
				bIsFilteredOut = true;
				break;
			}
		}
	}
	
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsFieldInaccessible(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UField const* ClassField = GetAssociatedField(BlueprintAction))
	{
		bool bIsProtected = ClassField->HasMetaData(FBlueprintMetadata::MD_Protected);
		bool bIsPrivate   = ClassField->HasMetaData(FBlueprintMetadata::MD_Private);

		UClass* FieldOwner = Cast<UClass>(ClassField->GetOuter());
		for (UBlueprint const* Blueprint : FilterContext.Blueprints)
		{
			UClass* BpClass = Blueprint->GeneratedClass;
			check(BpClass != nullptr);
			
			// private functions are only accessible from the class they belong to
			if (bIsPrivate && (FieldOwner != BpClass))
			{
				bIsFilteredOut = true;
				break;
			}
			else if (bIsProtected && !BpClass->IsChildOf(FieldOwner))
			{
				bIsFilteredOut = true;
				break;
			}
		}
	}
	
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsRestrictedClassMember(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UClass const* ActionClass = GetActionClass(BlueprintAction))
	{
		if (ActionClass->HasMetaData(FBlueprintMetadata::MD_RestrictedToClasses))
		{
			FString const& ClassRestrictions = ActionClass->GetMetaData(FBlueprintMetadata::MD_RestrictedToClasses);
			for (UBlueprint const* Blueprint : FilterContext.Blueprints)
			{
				bool bIsClassListed = false;
				
				UClass* BpClass = Blueprint->SkeletonGeneratedClass;
				// walk the class inheritance chain to see if this class is one
				// of the allowed
				while (!bIsClassListed && (BpClass != nullptr))
				{
					FString const ClassName = BpClass->GetName();
					bIsClassListed = (ClassName == ClassRestrictions) || !!FCString::StrfindDelim(*ClassRestrictions, *ClassName, TEXT(" "));
					
					BpClass = BpClass->GetSuperClass();
				}
				
				// if the blueprint class wasn't listed as one of the few
				// classes that this can be accessed from, then filter it out
				if (!bIsClassListed)
				{
					bIsFilteredOut = true;
					break;
				}
			}
		}
	}
	
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsPermissionNotGranted(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	if (UBlueprintPropertyNodeSpawner const* PropertySpawner = Cast<UBlueprintPropertyNodeSpawner>(BlueprintAction))
	{
		UProperty const* const Property = PropertySpawner->GetProperty();
		check(Property != nullptr);
		UClass const* const NodeClass = PropertySpawner->NodeClass;

		for (UBlueprint const* Blueprint : FilterContext.Blueprints)
		{
			bool const bIsReadOnly = FBlueprintEditorUtils::IsPropertyReadOnlyInCurrentBlueprint(Blueprint, Property);
			if (bIsReadOnly && NodeClass && NodeClass->IsChildOf<UK2Node_VariableSet>())
			{
				bIsFilteredOut = true;
			}
		}
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsDeprecated(FBlueprintActionFilter const& /*Filter*/, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;	
	if ((BlueprintAction->NodeClass != nullptr) && BlueprintAction->NodeClass->HasAnyClassFlags(CLASS_Deprecated))
	{
		bIsFilteredOut = true;
	}
	else if (UClass* ActionClass = GetActionClass(BlueprintAction))
	{
		bIsFilteredOut = ActionClass->HasAnyClassFlags(CLASS_Deprecated);
	}
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsGlobalField(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UField const* ClassField = GetAssociatedField(BlueprintAction))
	{
		UClass* FieldClass = Cast<UClass>(ClassField->GetOuter());
		bIsFilteredOut = (FieldClass == nullptr) || FieldClass->IsChildOf<UBlueprintFunctionLibrary>();
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsExternalField(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UField const* ClassField = GetAssociatedField(BlueprintAction))
	{		
		UClass* FieldClass = Cast<UClass>(ClassField->GetOuter());
		bool const bIsGlobal = (FieldClass == nullptr) || FieldClass->IsChildOf<UBlueprintFunctionLibrary>();

		// global (and static library) fields can stay (unless explicitly excluded... save that for a separate test)
		if (!bIsGlobal)
		{
			for (UClass const* Class : Filter.OwnerClasses)
			{
				bool const bIsInternalFunc = Class->IsChildOf(FieldClass);	
				if (!bIsInternalFunc)
				{
					bIsFilteredOut = true;
					break;
				}
			}
		}
	}
	
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsFieldCategoryHidden(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	if (UField const* AssociatedField = GetAssociatedField(BlueprintAction))
	{
		DECLARE_DELEGATE_RetVal_OneParam(bool, FIsFieldHiddenDelegate, UClass*);
		FIsFieldHiddenDelegate IsFieldHiddenDelegate;

		if (UFunction const* Function = Cast<UFunction>(AssociatedField))
		{
			auto IsFunctionHiddenLambda = [](UClass* Class, UFunction const* Function)->bool
			{
				return FObjectEditorUtils::IsFunctionHiddenFromClass(Function, Class);
			};
			IsFieldHiddenDelegate = FIsFieldHiddenDelegate::CreateStatic(IsFunctionHiddenLambda, Function);
		}
		else if (UProperty const* Property = Cast<UProperty>(AssociatedField))
		{
			auto IsPropertyHiddenLambda = [](UClass* Class, UProperty const* Property)->bool
			{
				return FObjectEditorUtils::IsVariableCategoryHiddenFromClass(Property, Class);
			};
			IsFieldHiddenDelegate = FIsFieldHiddenDelegate::CreateStatic(IsPropertyHiddenLambda, Property);
		}

		if (IsFieldHiddenDelegate.IsBound())
		{
			for (UBlueprint* Blueprint : FilterContext.Blueprints)
			{
				if (IsFieldHiddenDelegate.Execute(Blueprint->GeneratedClass))
				{
					bIsFilteredOut = true;
					break;
				}
			}

			for (int32 ClassIndex = 0; !bIsFilteredOut && (ClassIndex < Filter.OwnerClasses.Num()); ++ClassIndex)
			{
				bIsFilteredOut = IsFieldHiddenDelegate.Execute(Filter.OwnerClasses[ClassIndex]);
			}
		}
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsIncompatibleImpureNode(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bAllowImpureNodes = true;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	for (UEdGraph* Graph : FilterContext.Graphs)
	{
		if (UEdGraphSchema_K2* K2Schema = Graph->Schema->GetDefaultObject<UEdGraphSchema_K2>())
		{
			bAllowImpureNodes &= K2Schema->DoesGraphSupportImpureFunctions(Graph);
		}
	}
	
	bool bIsFilteredOut = !bAllowImpureNodes && IsImpure(BlueprintAction);
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsIncompatibleWithUbergraph(FBlueprintActionContext const& /*Context*/, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsIncompatibleWithAnimGraph(FBlueprintActionContext const& /*Context*/, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = IsLatent(BlueprintAction); // latent actions are only allowed in ubergraphs
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsIncompatibleWithMacroGraph(FBlueprintActionContext const& /*Context*/, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = IsLatent(BlueprintAction); // latent actions are only allowed in ubergraphs
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsIncompatibleWithFuncGraph(FBlueprintActionContext const& /*Context*/, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = IsLatent(BlueprintAction); // latent actions are only allowed in ubergraphs
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsIncompatibleWithGraphType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	uint32 GraphTypesTestedMask = 0x00;
	for (UEdGraph* Graph : FilterContext.Graphs)
	{
		EGraphType GraphType = GT_MAX;
		if (UEdGraphSchema_K2* K2Schema = Graph->Schema->GetDefaultObject<UEdGraphSchema_K2>())
		{
			GraphType = K2Schema->GetGraphType(Graph);
		}
		
		uint32 GraphTypeBit = (1 << GraphType);
		if (GraphTypesTestedMask & GraphTypeBit)
		{
			continue;
		}
		
		switch (GraphType)
		{
		case GT_Ubergraph:
			{
				bIsFilteredOut = IsIncompatibleWithUbergraph(FilterContext, BlueprintAction);
				break;
			}
		case GT_Animation:
			{
				bIsFilteredOut = IsIncompatibleWithAnimGraph(FilterContext, BlueprintAction);
				break;
			}
		case GT_Macro:
			{
				bIsFilteredOut = IsIncompatibleWithMacroGraph(FilterContext, BlueprintAction);
				break;
			}
		case GT_Function:
			{
				bIsFilteredOut = IsIncompatibleWithFuncGraph(FilterContext, BlueprintAction);
				break;
			}
		}
		
		if (bIsFilteredOut)
		{
			break;
		}
		GraphTypesTestedMask |= GraphTypeBit;
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsNodeUnpastable(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	for (UEdGraph* Graph : FilterContext.Graphs)
	{		
		UClass* NodeClass = BlueprintAction->NodeClass;
		if (NodeClass == nullptr)
		{
			continue;
		}
		else if (NodeClass->IsChildOf<UK2Node_Event>())// @TODO: make all event nodes, this: BlueprintAction->IsA<UBlueprintEventNodeSpawner>())
		{
			// events need a valid function to check CanPasteHere() (we can't use the CDO)
			// @TODO: modify the CDO's function reference, and then restore it
			continue;
		}

		// wouldn't want to spawn a temp node here (too costly to be doing while
		// filtering)... if a node that is unpasteble slips through, then the 
		// user should be notified through ui feedback way (we purposely do this
		// with the palette, so that it can have an all encompassing list)
		if (UEdGraphNode* NodeCDO = NodeClass->GetDefaultObject<UEdGraphNode>())
		{
			if (!NodeCDO->CanPasteHere(Graph, Graph->Schema->GetDefaultObject<UEdGraphSchema>()))
			{
				bIsFilteredOut = true;
				break;
			}
		}
	}
	
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsFilteredNodeType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction, bool const bExcludeChildClasses)
{
	bool bIsFilteredOut = false;

	UClass* NodeClass = BlueprintAction->NodeClass;
	if (NodeClass != nullptr)
	{
		for (UClass* AllowedClass : Filter.NodeTypes)
		{
			if ((bExcludeChildClasses && (AllowedClass != NodeClass)) ||
				!NodeClass->IsChildOf(AllowedClass))
			{
				bIsFilteredOut = false;
				break;
			}
		}
	}
	else
	{
		// this node's class type is unknown, filter it out!
		bIsFilteredOut = (Filter.NodeTypes.Num() > 0);
	}

	return bIsFilteredOut;
}

/*******************************************************************************
 * FBlueprintActionFilter
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintActionFilter::FBlueprintActionFilter(uint32 Flags/*= 0x00*/)
{
	using namespace BlueprintActionFilterImpl;

	//
	// NOTE: The order of these tests can have perf implications, the more one
	//       rejects on average the sooner it should be added (they're executed
	//       in order added)
	//

	if (!(Flags & BPFILTER_IncludeDeprecated))
	{
		AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsDeprecated));
	}
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsFilteredNodeType, (Flags & BPFILTER_ExcludeChildNodeTypes) != 0));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsEventUnimplementable));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsFieldInaccessible));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsRestrictedClassMember));
	if (Flags & BPFILTER_ExcludeGlobalFields)
	{
		AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsGlobalField));
	}
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsExternalField));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsIncompatibleImpureNode));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsPermissionNotGranted));
	//AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsIncompatibleWithGraphType));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsNodeUnpastable));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsFieldCategoryHidden));

	// @TODO: account for all K2ActionMenuBuilder checks...
	//    NodeCDO->CanCreateUnderSpecifiedSchema(this)
	//    FBlueprintEditorUtils::DoesSupportTimelines()
	//    FKismetEditorUtilities::IsClassABlueprintInterface()
	//    FK2ActionMenuBuilder::GetPaletteActions()
	//       is allowed in level-script?
	//       is allowed in non-actor?
	//       is allowed in actor?
	//    FK2ActionMenuBuilder::GetAllActionsForClass()
	//       show variables?
	//       show protected?
	//       show delegates?
	//    FK2ActionMenuBuilder::GetFuncNodesForClass()
	//       show inherited?
	//    FK2ActionMenuBuilder::GetContextAllowedNodeTypes()
	//       impure funcs?
	//       construction script?
	//    UEdGraphSchema_K2::CanFunctionBeUsedInClass()

	// @TODO: account for implemented interface methods
}

//------------------------------------------------------------------------------
void FBlueprintActionFilter::AddIsFilteredTest(FIsFilteredDelegate IsFilteredDelegate)
{
	if (IsFilteredDelegate.IsBound())
	{
		FilterTests.Add(IsFilteredDelegate);
	}
}

//------------------------------------------------------------------------------
bool FBlueprintActionFilter::IsFiltered(UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFiltered = IsFilteredByThis(BlueprintAction);
	if (!bIsFiltered)
	{
		for (FBlueprintActionFilter& OrFilter : OrFilters)
		{
			if (OrFilter.IsFiltered(BlueprintAction))
			{
				bIsFiltered = true;
				break;
			}
		}
	}

	if (bIsFiltered)
	{
		for (FBlueprintActionFilter& AndFilter : AndFilters)
		{
			bIsFiltered &= AndFilter.IsFiltered(BlueprintAction);
		}
	}

	return bIsFiltered;
}

//------------------------------------------------------------------------------
FBlueprintActionFilter const& FBlueprintActionFilter::operator|=(FBlueprintActionFilter const& Rhs)
{
	OrFilters.Add(Rhs);
	return *this;
}

//------------------------------------------------------------------------------
FBlueprintActionFilter const& FBlueprintActionFilter::operator&=(FBlueprintActionFilter const& Rhs)
{
	AndFilters.Add(Rhs);
	return *this;
}

//------------------------------------------------------------------------------
bool FBlueprintActionFilter::IsFilteredByThis(UBlueprintNodeSpawner const* BlueprintAction) const
{
	FBlueprintActionFilter const& FilterRef = *this;

	bool bIsFiltered = false;
	for (int32 TestIndex = 0; TestIndex < FilterTests.Num(); ++TestIndex)
	{
		FIsFilteredDelegate const& IsFilteredDelegate = FilterTests[TestIndex];
		check(IsFilteredDelegate.IsBound());

		if (IsFilteredDelegate.Execute(FilterRef, BlueprintAction))
		{
			bIsFiltered = true;
			break;
		}
	}
	return bIsFiltered;
}
