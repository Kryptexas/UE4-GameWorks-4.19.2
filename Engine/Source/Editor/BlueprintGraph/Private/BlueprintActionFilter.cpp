// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintNodeSpawnerUtils.h"
#include "BlueprintVariableNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintBoundEventNodeSpawner.h"
#include "EdGraphSchema_K2.h"		// for FBlueprintMetadata
#include "BlueprintEditorUtils.h"	// for FindBlueprintForGraph()
#include "ObjectEditorUtils.h"		// for IsFunctionHiddenFromClass()/IsVariableCategoryHiddenFromClass()
#include "K2Node_VariableSet.h"
#include "K2Node_VariableGet.h"
// "impure" node types (utilized in BlueprintActionFilterImpl::IsImpure)
#include "K2Node_IfThenElse.h"
#include "K2Node_MultiGate.h"
#include "K2Node_MakeArray.h"
#include "K2Node_Message.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_DynamicCast.h"

/*******************************************************************************
 * Static BlueprintActionFilter Helpers
 ******************************************************************************/

namespace BlueprintActionFilterImpl
{
	/**
	 * 
	 * 
	 * @param  Blueprint	
	 * @return 
	 */
	static UClass* GetBlueprintClass(UBlueprint const* const Blueprint);	
	
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
	 * Utility method to check and see if the specified field is a public global
	 * or static field (that is has a persistent extent that span's the 
	 * program's lifetime).
	 *
	 * @param  Field	The field you want to check.
	 * @return True if the field is global/static (and public), false if it has a limited extent (or is private/protected).
	 */
	static bool IsGloballyAccessible(UField const* Field);
	
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
	 * Rejection test that checks to see if the node-spawner has any associated
	 * "non-target" fields that are global/static.
	 * 
	 * @param  Filter			Holds the TagetClass context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return 
	 */
	static bool IsPersistentNonTargetField(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the node-spawner is associated with 
	 * a field that belongs to a class that is not white-listed (ignores global/
	 * static fields).
	 * 
	 * @param  Filter					Holds the class context for this test.
	 * @param  BlueprintAction			The action you wish to query.
	 * @param  bPermitNonTargetGlobals	Determines if this test should pass for external global/static fields.
	 * @return True if the action is associated with a non-whitelisted class member.
	 */
	static bool IsNonTargetMemeber(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction, bool const bPermitNonTargetGlobals);

	/**
	 * Rejection test that checks to see if the node-spawner is associated with 
	 * a field that is hidden from the specified blueprint (via metadata).
	 * 
	 * @param  Filter			Holds the class context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action is associated with a hidden field.
	 */
	static bool IsFieldCategoryHidden(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Rejection test that checks to see if the node-spawner would produce a 
	 * node type that isn't white-listed. 
	 * 
	 * @param  Filter			Holds the class context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @param  bMatchExplicitly When true, the node's class has to perfectly match a white-listed type (sub-classes don't count).
	 * @return True if the action would produce a non-whitelisted node.
	 */
	static bool IsFilteredNodeType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction, bool const bMatchExplicitly = false);

	/**
	 * Rejection test that checks to see if the node-spawner is tied to a 
	 * specific object that is not currently selected.
	 * 
	 * 
	 * @param  Filter			Holds the selected object context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would produce a bound node, tied to an object that isn't selected.
	 */
	static bool IsBoundToUnselectedObject(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsOutOfScopeLocalVariable(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsSchemaIncompatible(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * 
	 * 
	 * @param  BlueprintAction	
	 * @param  Pin	
	 * @return 
	 */
	static bool HasMatchingPin(UBlueprintNodeSpawner const* BlueprintAction, UEdGraphPin const* Pin);

	/**
	 * 
	 * 
	 * @param  Pin	
	 * @param  MemberField	
	 * @param  MemberNodeSpawner
	 * @return 
	 */
	static bool IsPinCompatibleWithTargetSelf(UEdGraphPin const* Pin, UField const* MemberField, UBlueprintNodeSpawner const* MemberNodeSpawner);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsFunctionMissingPinParam(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsMissmatchedPropertyType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsMissingMatchingPinParam(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Dynamic casts should only show results for casting to classes that the context pin is a child of (and not itself)
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsNotSubClassCast(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction);
};


//------------------------------------------------------------------------------
UClass* BlueprintActionFilterImpl::GetBlueprintClass(UBlueprint const* const Blueprint)
{
	return (Blueprint->SkeletonGeneratedClass != nullptr) ? Blueprint->SkeletonGeneratedClass : Blueprint->ParentClass;
}



//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsImpure(UBlueprintNodeSpawner const* NodeSpawner)
{
	bool bIsImpure = false;
	
	if (UFunction const* Function = FBlueprintNodeSpawnerUtils::GetAssociatedFunction(NodeSpawner))
	{
		bIsImpure = (Function->HasAnyFunctionFlags(FUNC_BlueprintPure) == false);
	}
	else if (NodeSpawner->NodeClass != nullptr)
	{
		UClass* NodeClass = NodeSpawner->NodeClass;
		// TODO: why are some of these "impure"?... we shouldn't have hardcoded 
		//       node types here (game modules cannot add their node types here,
		//       so we should find another way of identifying "pure" node types)
		bIsImpure = (NodeClass == UK2Node_IfThenElse::StaticClass()) ||
			(NodeClass == UK2Node_MultiGate::StaticClass()) ||
			(NodeClass == UK2Node_MakeArray::StaticClass()) ||
			(NodeClass == UK2Node_Message::StaticClass()) ||
			(NodeClass == UK2Node_ExecutionSequence::StaticClass());
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
	else if (UFunction const* Function = FBlueprintNodeSpawnerUtils::GetAssociatedFunction(NodeSpawner))
	{
		bIsLatent = (Function->HasMetaData(FBlueprintMetadata::MD_Latent) != false);
	}

	return bIsLatent;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsGloballyAccessible(UField const* Field)
{
	bool bHasPersistentExtents = false; // is global or static
	bool bIsPublic = Field->HasAnyFlags(RF_Public);
	
	UClass* ClassOuter = Cast<UClass>(Field->GetOuter());
	// outer is probably a UPackage (for like global enums, structs, etc.)
	if (ClassOuter == nullptr)
	{
		bHasPersistentExtents = true;
	}
	else if (UFunction const* Function = Cast<UFunction>(Field))
	{
		bIsPublic |= !Function->HasMetaData(FBlueprintMetadata::MD_Protected) &&
			!Function->HasMetaData(FBlueprintMetadata::MD_Private);
		
		bHasPersistentExtents = Function->HasAnyFunctionFlags(FUNC_Static);
	}
	
	return bIsPublic && bHasPersistentExtents;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsEventUnimplementable(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UBlueprintEventNodeSpawner const* EventSpawner = Cast<UBlueprintEventNodeSpawner>(BlueprintAction))
	{
		if (UFunction const* EventFunc = EventSpawner->GetEventFunction())
		{
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
	}
	
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsFieldInaccessible(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UField const* ClassField = FBlueprintNodeSpawnerUtils::GetAssociatedField(BlueprintAction))
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
	
	if (UClass const* ActionClass = FBlueprintNodeSpawnerUtils::GetAssociatedClass(BlueprintAction))
	{
		if (ActionClass->HasMetaData(FBlueprintMetadata::MD_RestrictedToClasses))
		{
			FString const& ClassRestrictions = ActionClass->GetMetaData(FBlueprintMetadata::MD_RestrictedToClasses);
			for (UBlueprint const* Blueprint : FilterContext.Blueprints)
			{
				bool bIsClassListed = false;
				
				UClass* BpClass = GetBlueprintClass(Blueprint);
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

	if (UProperty const* const Property = FBlueprintNodeSpawnerUtils::GetAssociatedProperty(BlueprintAction))
	{
		UClass const* const NodeClass = BlueprintAction->NodeClass;
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
	else if (UClass* ActionClass = FBlueprintNodeSpawnerUtils::GetAssociatedClass(BlueprintAction))
	{
		bIsFilteredOut = ActionClass->HasAnyClassFlags(CLASS_Deprecated);
	}
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsPersistentNonTargetField(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UField const* ClassField = FBlueprintNodeSpawnerUtils::GetAssociatedField(BlueprintAction))
	{
		bIsFilteredOut = IsGloballyAccessible(ClassField);
		
		UClass* FieldClass = ClassField->GetOwnerClass();
		if (bIsFilteredOut && (FieldClass != nullptr))
		{
			for (UClass const* Class : Filter.TargetClasses)
			{
				bool const bIsInternalMemberField = Class->IsChildOf(FieldClass);
				if (bIsInternalMemberField)
				{
					bIsFilteredOut = false;
					break;
				}
			}
		}
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsNonTargetMemeber(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction, bool bPermitNonTargetGlobals)
{
	bool bIsFilteredOut = false;
	if (UField const* ClassField = FBlueprintNodeSpawnerUtils::GetAssociatedField(BlueprintAction))
	{
		UClass* FieldClass = Cast<UClass>(ClassField->GetOuter());
		bool const bIsInterfaceMethod = FieldClass->IsChildOf<UInterface>();

		// global (and static library) fields can stay (unless explicitly
		// excluded... save that for a separate test)
		bool const bSkip = bPermitNonTargetGlobals && IsGloballyAccessible(ClassField);
		if ((FieldClass != nullptr) && !bSkip)
		{
			for (UClass const* Class : Filter.TargetClasses)
			{
				bool const bIsInternalFunc = Class->IsChildOf(FieldClass) || (bIsInterfaceMethod && Class->ImplementsInterface(FieldClass));
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

	if (UField const* AssociatedField = FBlueprintNodeSpawnerUtils::GetAssociatedField(BlueprintAction))
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

			for (int32 ClassIndex = 0; !bIsFilteredOut && (ClassIndex < Filter.TargetClasses.Num()); ++ClassIndex)
			{
				bIsFilteredOut = IsFieldHiddenDelegate.Execute(Filter.TargetClasses[ClassIndex]);
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
static bool BlueprintActionFilterImpl::IsIncompatibleWithGraphType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UClass* NodeClass = BlueprintAction->NodeClass)
	{
		if (UEdGraphNode* NodeCDO = NodeClass->GetDefaultObject<UEdGraphNode>())
		{
			for (UEdGraph* Graph : FilterContext.Graphs)
			{
				if (!NodeCDO->IsCompatibleWithGraph(Graph))
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
static bool BlueprintActionFilterImpl::IsFilteredNodeType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction, bool const bExcludeChildClasses)
{
	bool bIsFilteredOut = (Filter.PermittedNodeTypes.Num() > 0);

	UClass* NodeClass = BlueprintAction->NodeClass;
	if (NodeClass != nullptr)
	{
		for (TSubclassOf<UEdGraphNode> AllowedClass : Filter.PermittedNodeTypes)
		{
			if ((NodeClass->IsChildOf(AllowedClass) && !bExcludeChildClasses) ||
				(AllowedClass == NodeClass))
			{
				bIsFilteredOut = false;
				break;
			}
		}
		
		for (int32 ClassIndx = 0; !bIsFilteredOut && (ClassIndx < Filter.RejectedNodeTypes.Num()); ++ClassIndx)
		{
			TSubclassOf<UEdGraphNode> ExcludedClass = Filter.RejectedNodeTypes[ClassIndx];
			if ((bExcludeChildClasses && (ExcludedClass == NodeClass)) ||
				NodeClass->IsChildOf(ExcludedClass))
			{
				bIsFilteredOut = true;
				break;
			}
		}
	}
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsBoundToUnselectedObject(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UBlueprintBoundEventNodeSpawner const* BoundSpawner = Cast<UBlueprintBoundEventNodeSpawner>(BlueprintAction))
	{
		bIsFilteredOut = true;
		for (UObject const* SelectedObj : Filter.Context.SelectedObjects)
		{
			if (BoundSpawner->CanBind(SelectedObj))
			{
				bIsFilteredOut = false;
				break;
			}
		}
	}
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsOutOfScopeLocalVariable(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UBlueprintVariableNodeSpawner const* VarSpawner = Cast<UBlueprintVariableNodeSpawner>(BlueprintAction))
	{
		if (VarSpawner->IsLocalVariable())
		{
			bIsFilteredOut = (Filter.Context.Graphs.Num() == 0);

			UEdGraph const* VarOuter = Cast<UEdGraph const>(VarSpawner->GetVarOuter());
			for (UEdGraph* Graph : Filter.Context.Graphs)
			{
				if (Graph != VarOuter)
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
static bool BlueprintActionFilterImpl::IsSchemaIncompatible(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	UEdGraphNode const* NodeCDO = BlueprintAction->NodeClass->GetDefaultObject<UEdGraphNode>();

	auto IsSchemaIncompatibleLambda = [NodeCDO](TArray<UEdGraph*> const& GraphList)->bool
	{
		bool bIsCompatible = true;
		for (UEdGraph const* Graph : GraphList)
		{
			if (!NodeCDO->CanCreateUnderSpecifiedSchema(Graph->GetSchema()))
			{
				bIsCompatible = false;
				break;
			}
		}
		return !bIsCompatible;
	};
	
	if (FilterContext.Graphs.Num() > 0)
	{
		bIsFilteredOut = IsSchemaIncompatibleLambda(FilterContext.Graphs);
	}
	else
	{
		bIsFilteredOut = true;
		for (UBlueprint const* Blueprint : FilterContext.Blueprints)
		{
			TArray<UEdGraph*> BpGraphList;
			Blueprint->GetAllGraphs(BpGraphList);

			if (!IsSchemaIncompatibleLambda(BpGraphList))
			{
				bIsFilteredOut = false;
				break;
			}
		}
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::HasMatchingPin(UBlueprintNodeSpawner const* BlueprintAction, UEdGraphPin const* Pin)
{
	bool bHasCompatiblePin = false;

	UEdGraph* OuterGraph = Pin->GetOwningNode()->GetGraph();
	if (UEdGraphNode* TemplateNode = BlueprintAction->GetTemplateNode(OuterGraph))
	{
		if (TemplateNode->Pins.Num() == 0)
		{
			TemplateNode->AllocateDefaultPins();
		}

		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(OuterGraph);
		check(Blueprint != nullptr);
		UEdGraphSchema_K2 const* Schema = CastChecked<UEdGraphSchema_K2>(OuterGraph->GetSchema());

		UClass* CallingContext = GetBlueprintClass(Blueprint);
		UK2Node* K2TemplateNode = Cast<UK2Node>(TemplateNode);
		
		for (int32 PinIndex = 0; !bHasCompatiblePin && (PinIndex < TemplateNode->Pins.Num()); ++PinIndex)
		{
			UEdGraphPin* TemplatePin = TemplateNode->Pins[PinIndex];
			if (!Schema->ArePinsCompatible(Pin, TemplatePin, CallingContext))
			{
				continue;
			}
			bHasCompatiblePin = true;

			if (K2TemplateNode != nullptr)
			{
				FString DisallowedReason;
				// to catch wildcard connections that are prevented
				bHasCompatiblePin = !K2TemplateNode->IsConnectionDisallowed(TemplatePin, Pin, DisallowedReason);
			}
		}
	}	

	return bHasCompatiblePin;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsPinCompatibleWithTargetSelf(UEdGraphPin const* Pin, UField const* MemberField, UBlueprintNodeSpawner const* MemberNodeSpawner)
{
	bool bIsCompatible = false;
	checkSlow(FBlueprintNodeSpawnerUtils::GetAssociatedField(MemberNodeSpawner) == MemberField);

	UClass* OwnerClass = Cast<UClass>(MemberField->GetOuter());
	if ((Pin->Direction == EGPD_Output) && (OwnerClass != nullptr))
	{
		FEdGraphPinType const& PinType = Pin->PinType;
		UEdGraphSchema const* const PinSchema = Pin->GetSchema();
		checkSlow(PinSchema != nullptr);

		UClass const* PinObjClass = nullptr;
		if (PinSchema->IsSelfPin(*Pin))
		{
			UBlueprint const* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Pin->GetOwningNode());
			PinObjClass = GetBlueprintClass(Blueprint);
		}
		else if (PinType.PinSubCategoryObject.IsValid()) // if PC_Object/PC_Interface
		{
			PinObjClass = Cast<UClass>(PinType.PinSubCategoryObject.Get());
		}
		
		if (PinObjClass != nullptr)
		{
			if (PinObjClass->IsChildOf(OwnerClass) || PinObjClass->ImplementsInterface(OwnerClass))
			{
				bIsCompatible = true;
				if (PinType.bIsArray)
				{
					if (UFunction const* Function = Cast<UFunction>(MemberField))
					{
						bIsCompatible = UK2Node_CallFunction::CanFunctionSupportMultipleTargets(Function);
					}
					else
					{
						UEdGraph* OuterGraph = Pin->GetOwningNode()->GetGraph();
						if (UK2Node* TemplateNode = Cast<UK2Node>(MemberNodeSpawner->GetTemplateNode(OuterGraph)))
						{
							bIsCompatible = TemplateNode->AllowMultipleSelfs(/*bInputAsArray =*/true);
						}	
					}
				}
			}
		}
	}

	return bIsCompatible;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsFunctionMissingPinParam(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UFunction const* AssociatedFunc = FBlueprintNodeSpawnerUtils::GetAssociatedFunction(BlueprintAction))
	{
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		bool const bIsEventSpawner = BlueprintAction->NodeClass->IsChildOf<UK2Node_Event>();

		for (int32 PinIndex = 0; !bIsFilteredOut && (PinIndex < Filter.Context.Pins.Num()); ++PinIndex)
		{
			UEdGraphPin const* ContextPin = Filter.Context.Pins[PinIndex];

			FEdGraphPinType const& PinType = ContextPin->PinType;
			UK2Node const* const K2Node = CastChecked<UK2Node const>(ContextPin->GetOwningNode());
			EEdGraphPinDirection const PinDir = ContextPin->Direction;

			if (K2Schema->IsExecPin(*ContextPin))
			{
				bIsFilteredOut = (bIsEventSpawner && (PinDir == EGPD_Output)) || !IsImpure(BlueprintAction);
			}
			else
			{
				// event nodes have their parameters as outputs (even though
				// the function signature would have them as inputs), so we 
				// want to flip the connotation here
				bool const bWantsOutputConnection = (PinDir == EGPD_Input) ^ bIsEventSpawner;

				if (K2Schema->FunctionHasParamOfType(AssociatedFunc, K2Node->GetBlueprint(), PinType, bWantsOutputConnection))
				{
					bIsFilteredOut = false;
				}
				else
				{
					// need to take "Target" self pins into consideration for objects
					bIsFilteredOut = bIsEventSpawner || !IsPinCompatibleWithTargetSelf(ContextPin, AssociatedFunc, BlueprintAction);
				}
			}
			
		}
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsMissmatchedPropertyType(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UProperty const* Property = FBlueprintNodeSpawnerUtils::GetAssociatedProperty(BlueprintAction))
	{
		TArray<UEdGraphPin*> const& ContextPins = Filter.Context.Pins;
		if (ContextPins.Num() > 0)
		{
			bool const bIsDelegate = Property->IsA<UMulticastDelegateProperty>();
			bool const bIsGetter   = BlueprintAction->NodeClass->IsChildOf<UK2Node_VariableGet>();
			bool const bIsSetter   = BlueprintAction->NodeClass->IsChildOf<UK2Node_VariableSet>();

			//K2Schema->ConvertPropertyToPinType(VariableProperty, /*out*/ VariablePin->PinType);
			for (int32 PinIndex = 0; !bIsFilteredOut && PinIndex < ContextPins.Num(); ++PinIndex)
			{
				UEdGraphPin const* ContextPin = ContextPins[PinIndex];
				FEdGraphPinType const& ContextPinType = ContextPin->PinType;
				UEdGraphSchema_K2 const* K2Schema = CastChecked<UEdGraphSchema_K2 const>(ContextPin->GetSchema());

				// have to account for "self" context pin
				if (IsPinCompatibleWithTargetSelf(ContextPin, Property, BlueprintAction))
				{
					continue;
				}
				else if (bIsDelegate)
				{
					// there are a lot of different delegate nodes, so let's  
					// just iterate over all the pins
					bIsFilteredOut = !HasMatchingPin(BlueprintAction, ContextPin);
				}
				else if (ContextPinType.PinCategory == K2Schema->PC_Exec)
				{
					// setters are impure, and therefore should have exec pins
					bIsFilteredOut = bIsGetter;
				}
				else if (bIsGetter || bIsSetter)
				{
					bIsFilteredOut = true;

					EEdGraphPinDirection const PinDir = ContextPin->Direction;
					if ((PinDir == EGPD_Input) && bIsGetter)
					{
						FEdGraphPinType OutputPinType;
						K2Schema->ConvertPropertyToPinType(Property, OutputPinType);
						bIsFilteredOut = !K2Schema->ArePinTypesCompatible(OutputPinType, ContextPinType);
					}
					else if ((PinDir == EGPD_Output) && bIsSetter)
					{
						FEdGraphPinType InputPinType;
						K2Schema->ConvertPropertyToPinType(Property, InputPinType);
						bIsFilteredOut = !K2Schema->ArePinTypesCompatible(ContextPinType, InputPinType);
					}	
				}
				else
				{
					ensureMsg(false, TEXT("Unhandled property/node pair, we've probably made some bad assuptions."));
				}
			}
		}
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsMissingMatchingPinParam(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;

	UField const* AssociatedField = FBlueprintNodeSpawnerUtils::GetAssociatedField(BlueprintAction);
	// we have a separate tests for field nodes (IsFunctionMissingPinParam/IsMissmatchedPropertyType)
	if (AssociatedField == nullptr)
	{
		for (UEdGraphPin const* ContextPin : Filter.Context.Pins)
		{
			if (!HasMatchingPin(BlueprintAction, ContextPin))
			{
				bIsFilteredOut = true;
				break;
			}
		}
	}

	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsNotSubClassCast(FBlueprintActionFilter const& Filter, UBlueprintNodeSpawner const* BlueprintAction)
{
	bool bIsFilteredOut = false;

	if(BlueprintAction->NodeClass && BlueprintAction->NodeClass == UK2Node_DynamicCast::StaticClass())
	{
		for(UEdGraphPin const* ContextPin : Filter.Context.Pins)
		{
			// Only worry about removing cast nodes when dragging off output pins
			if(ContextPin->Direction == EGPD_Output)
			{
				UEdGraph* OuterGraph = ContextPin->GetOwningNode()->GetGraph();
				if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(BlueprintAction->GetTemplateNode(OuterGraph)))
				{
					if( ContextPin->PinType.PinSubCategoryObject.IsValid() && CastNode->TargetType)
					{
						UClass* ContextPinClass = Cast<UClass>(ContextPin->PinType.PinSubCategoryObject.Get());
						UClass* ResultPinClass = CastNode->TargetType;
						if( ContextPinClass == ResultPinClass || !ResultPinClass->IsChildOf(ContextPinClass) )
						{
							bIsFilteredOut = true;
							break;
						}
					}
				}
			}
		}
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
	//       rejects on average the later it should be added (they're executed
	//       in reverse order, so user added tests are ran first and the ones 
	//       here are ran last)
	//

	// add first the most expensive tests (they will be ran last, and therefore
	// should be operating on a smaller subset of node-spawners)
	//
	// this test in-particular spawns a template-node and then calls 
	// AllocateDefaultPins() which is costly, so it should be very last!
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsMissingMatchingPinParam));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsMissmatchedPropertyType));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsFunctionMissingPinParam));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsIncompatibleImpureNode));
	
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsFieldCategoryHidden));	
	if (Flags & BPFILTER_RejectPersistentNonTargetFields)
	{
		AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsPersistentNonTargetField));
	}
	
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsFieldInaccessible));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsNotSubClassCast));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsEventUnimplementable));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsPermissionNotGranted));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsRestrictedClassMember));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsIncompatibleWithGraphType));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsSchemaIncompatible));

	if (!(Flags & BPFILTER_PermitDeprecated))
	{
		AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsDeprecated));
	}

	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsFilteredNodeType, (Flags & BPFILTER_RejectPermittedNodeSubClasses)   != 0));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsNonTargetMemeber, (Flags & BPFILTER_RejectPersistentNonTargetFields) == 0));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsBoundToUnselectedObject));
	AddIsFilteredTest(FIsFilteredDelegate::CreateStatic(IsOutOfScopeLocalVariable));

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
	// iterate backwards so that custom user test are ran first (and the slow
	// internal tests are ran last).
	for (int32 TestIndex = FilterTests.Num()-1; TestIndex >= 0; --TestIndex)
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
