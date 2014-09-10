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
	static bool IsImpure(FBlueprintActionInfo& NodeSpawner);

	/**
	 * Utility method to check and see if the specified node-spawner would 
	 * produce a latent node.
	 * 
	 * @param  NodeSpawner	The action you want to query.
	 * @return True if the action will spawn a latent node, false if not (or unknown).
	 */
	static bool IsLatent(FBlueprintActionInfo& NodeSpawner);
	
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
	static bool IsEventUnimplementable(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner has an 
	 * associated field that is not accessible by the blueprint (it's private or
	 * protected).
	 * 
	 * @param  Filter			Holds the blueprint context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the node-spawner is associated with a private field.
	 */
	static bool IsFieldInaccessible(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner has an 
	 * associated class that is "restricted" and thusly, hidden from the 
	 * blueprint.
	 * 
	 * @param  Filter			Holds the blueprint context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the node-spawner belongs to a class that is restricted to certain blueprints (not this one).
	 */
	static bool IsRestrictedClassMember(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a variable-set node when the property is read-only (in this
	 * blueprint).
	 * 
	 * @param  Filter			Holds the blueprint context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a variable-set node for a read-only property.
	 */
	static bool IsPermissionNotGranted(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a node (or comes from an associated class) that is deprecated.
	 * 
	 * @param  Filter			Filter context (unused) for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node that is deprecated.
	 */
	static bool IsDeprecated(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce an impure node, incompatible with the specified graphs.
	 * 
	 * @param  Filter			Holds the graph context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn an impure node (and the graph doesn't allow them).
	 */
	static bool IsIncompatibleImpureNode(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the supplied node-spawner would 
	 * produce a node incompatible with the specified graph.
	 * 
	 * @param  Filter			Holds the graph context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would spawn a node (and the graph wouldn't allow it).
	 */
	static bool IsIncompatibleWithGraphType(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);
	
	/**
	 * Rejection test that checks to see if the node-spawner has any associated
	 * "non-target" fields that are global/static.
	 * 
	 * @param  Filter			Holds the TagetClass context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return 
	 */
	static bool IsPersistentNonTargetField(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

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
	static bool IsNonTargetMemeber(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction, bool const bPermitNonTargetGlobals);

	/**
	 * Rejection test that checks to see if the node-spawner is associated with 
	 * a field that is hidden from the specified blueprint (via metadata).
	 * 
	 * @param  Filter			Holds the class context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action is associated with a hidden field.
	 */
	static bool IsFieldCategoryHidden(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Rejection test that checks to see if the node-spawner would produce a 
	 * node type that isn't white-listed. 
	 * 
	 * @param  Filter			Holds the class context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @param  bMatchExplicitly When true, the node's class has to perfectly match a white-listed type (sub-classes don't count).
	 * @return True if the action would produce a non-whitelisted node.
	 */
	static bool IsFilteredNodeType(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction, bool const bMatchExplicitly = false);

	/**
	 * Rejection test that checks to see if the node-spawner is tied to a 
	 * specific object that is not currently selected.
	 * 
	 * 
	 * @param  Filter			Holds the selected object context for this test.
	 * @param  BlueprintAction	The action you wish to query.
	 * @return True if the action would produce a bound node, tied to an object that isn't selected.
	 */
	static bool IsUnBoundBindingSpawner(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsOutOfScopeLocalVariable(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsSchemaIncompatible(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * 
	 * 
	 * @param  BlueprintAction	
	 * @param  Pin	
	 * @return 
	 */
	static bool HasMatchingPin(FBlueprintActionInfo& BlueprintAction, UEdGraphPin const* Pin);

	/**
	 * 
	 * 
	 * @param  Pin	
	 * @param  MemberField	
	 * @param  MemberNodeSpawner
	 * @return 
	 */
	static bool IsPinCompatibleWithTargetSelf(UEdGraphPin const* Pin, UField const* MemberField, FBlueprintActionInfo& MemberNodeSpawner);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsFunctionMissingPinParam(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsMissmatchedPropertyType(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * 
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsMissingMatchingPinParam(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Dynamic casts should only show results for casting to classes that the 
	 * context pin is a child of (and not itself).
	 * 
	 * @param  Filter	
	 * @param  BlueprintAction	
	 * @return 
	 */
	static bool IsNotSubClassCast(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);
};


//------------------------------------------------------------------------------
UClass* BlueprintActionFilterImpl::GetBlueprintClass(UBlueprint const* const Blueprint)
{
	return (Blueprint->SkeletonGeneratedClass != nullptr) ? Blueprint->SkeletonGeneratedClass : Blueprint->ParentClass;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsImpure(FBlueprintActionInfo& BlueprintAction)
{
	bool bIsImpure = false;
	
	if (UFunction const* Function = BlueprintAction.GetAssociatedFunction())
	{
		bIsImpure = (Function->HasAnyFunctionFlags(FUNC_BlueprintPure) == false);
	}
	else
	{
		UClass const* NodeClass = BlueprintAction.GetNodeClass();
		check(NodeClass != nullptr);

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
static bool BlueprintActionFilterImpl::IsLatent(FBlueprintActionInfo& BlueprintAction)
{
	bool bIsLatent = false;

	UClass const* NodeClass = BlueprintAction.GetNodeClass();
	if ((NodeClass != nullptr) && NodeClass->IsChildOf<UK2Node_BaseAsyncTask>())
	{
		bIsLatent = true;
	}
	else if (UFunction const* Function = BlueprintAction.GetAssociatedFunction())
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
static bool BlueprintActionFilterImpl::IsEventUnimplementable(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UBlueprintEventNodeSpawner const* EventSpawner = Cast<UBlueprintEventNodeSpawner>(BlueprintAction.NodeSpawner))
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
static bool BlueprintActionFilterImpl::IsFieldInaccessible(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UField const* ClassField = BlueprintAction.GetAssociatedMemberField())
	{
		bool bIsProtected = ClassField->HasMetaData(FBlueprintMetadata::MD_Protected);
		bool bIsPrivate   = ClassField->HasMetaData(FBlueprintMetadata::MD_Private);

		UClass* FieldOwner = ClassField->GetOwnerClass();
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
static bool BlueprintActionFilterImpl::IsRestrictedClassMember(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UClass const* ActionClass = BlueprintAction.GetOwnerClass())
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
static bool BlueprintActionFilterImpl::IsPermissionNotGranted(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	if (UProperty const* const Property = BlueprintAction.GetAssociatedProperty())
	{
		UClass const* const NodeClass = BlueprintAction.GetNodeClass();
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
static bool BlueprintActionFilterImpl::IsDeprecated(FBlueprintActionFilter const& /*Filter*/, FBlueprintActionInfo& BlueprintAction)
{
	checkSlow(BlueprintAction.GetNodeClass() != nullptr);

	bool bIsFilteredOut = false;	
	if (BlueprintAction.GetNodeClass()->HasAnyClassFlags(CLASS_Deprecated))
	{
		bIsFilteredOut = true;
	}
	else if (UClass const* ActionClass = BlueprintAction.GetOwnerClass())
	{
		bIsFilteredOut = ActionClass->HasAnyClassFlags(CLASS_Deprecated);
	}
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsPersistentNonTargetField(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UField const* ClassField = BlueprintAction.GetAssociatedMemberField())
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
static bool BlueprintActionFilterImpl::IsNonTargetMemeber(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction, bool bPermitNonTargetGlobals)
{
	bool bIsFilteredOut = false;
	if (UField const* ClassField = BlueprintAction.GetAssociatedMemberField())
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
static bool BlueprintActionFilterImpl::IsFieldCategoryHidden(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	if (UField const* AssociatedField = BlueprintAction.GetAssociatedMemberField())
	{
		DECLARE_DELEGATE_RetVal_OneParam(bool, FIsFieldHiddenDelegate, UClass*);
		FIsFieldHiddenDelegate IsFieldHiddenDelegate;

		if (UFunction const* Function = BlueprintAction.GetAssociatedFunction())
		{
			auto IsFunctionHiddenLambda = [](UClass* Class, UFunction const* Function)->bool
			{
				return FObjectEditorUtils::IsFunctionHiddenFromClass(Function, Class);
			};
			IsFieldHiddenDelegate = FIsFieldHiddenDelegate::CreateStatic(IsFunctionHiddenLambda, Function);
		}
		else if (UProperty const* Property = BlueprintAction.GetAssociatedProperty())
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
static bool BlueprintActionFilterImpl::IsIncompatibleImpureNode(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
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
static bool BlueprintActionFilterImpl::IsIncompatibleWithGraphType(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;
	
	if (UClass const* NodeClass = BlueprintAction.GetNodeClass())
	{
		if (UEdGraphNode const* NodeCDO = CastChecked<UEdGraphNode>(NodeClass->ClassDefaultObject))
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
static bool BlueprintActionFilterImpl::IsFilteredNodeType(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction, bool const bExcludeChildClasses)
{
	bool bIsFilteredOut = (Filter.PermittedNodeTypes.Num() > 0);

	UClass const* NodeClass = BlueprintAction.GetNodeClass();
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
static bool BlueprintActionFilterImpl::IsUnBoundBindingSpawner(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool const bIsBindingSpecificSpawner = (Cast<UBlueprintBoundEventNodeSpawner>(BlueprintAction.NodeSpawner) != nullptr);

	bool bIsFilteredOut = false;
	if (bIsBindingSpecificSpawner)
	{
		bIsFilteredOut = (BlueprintAction.GetBindings().Num() <= 0);
	}
	return bIsFilteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionFilterImpl::IsOutOfScopeLocalVariable(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UBlueprintVariableNodeSpawner const* VarSpawner = Cast<UBlueprintVariableNodeSpawner>(BlueprintAction.NodeSpawner))
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
static bool BlueprintActionFilterImpl::IsSchemaIncompatible(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	UClass const* NodeClass = BlueprintAction.GetNodeClass();
	UEdGraphNode const* NodeCDO = CastChecked<UEdGraphNode>(NodeClass->ClassDefaultObject);
	checkSlow(NodeCDO != nullptr);

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
static bool BlueprintActionFilterImpl::HasMatchingPin(FBlueprintActionInfo& BlueprintAction, UEdGraphPin const* Pin)
{
	bool bHasCompatiblePin = false;

	UEdGraph* OuterGraph = Pin->GetOwningNode()->GetGraph();
	if (UEdGraphNode* TemplateNode = BlueprintAction.NodeSpawner->GetTemplateNode(OuterGraph))
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
static bool BlueprintActionFilterImpl::IsPinCompatibleWithTargetSelf(UEdGraphPin const* Pin, UField const* MemberField, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsCompatible = false;
	checkSlow(BlueprintAction.GetAssociatedMemberField() == MemberField);

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
						if (UK2Node* TemplateNode = Cast<UK2Node>(BlueprintAction.NodeSpawner->GetTemplateNode(OuterGraph)))
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
static bool BlueprintActionFilterImpl::IsFunctionMissingPinParam(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UFunction const* AssociatedFunc = BlueprintAction.GetAssociatedFunction())
	{
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		bool const bIsEventSpawner = BlueprintAction.GetNodeClass()->IsChildOf<UK2Node_Event>();

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
static bool BlueprintActionFilterImpl::IsMissmatchedPropertyType(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;
	if (UProperty const* Property = BlueprintAction.GetAssociatedProperty())
	{
		TArray<UEdGraphPin*> const& ContextPins = Filter.Context.Pins;
		if (ContextPins.Num() > 0)
		{
			bool const bIsDelegate = Property->IsA<UMulticastDelegateProperty>();
			bool const bIsGetter   = BlueprintAction.GetNodeClass()->IsChildOf<UK2Node_VariableGet>();
			bool const bIsSetter   = BlueprintAction.GetNodeClass()->IsChildOf<UK2Node_VariableSet>();

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
static bool BlueprintActionFilterImpl::IsMissingMatchingPinParam(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;

	UField const* AssociatedField = BlueprintAction.GetAssociatedMemberField();
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
static bool BlueprintActionFilterImpl::IsNotSubClassCast(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFilteredOut = false;

	if (BlueprintAction.GetNodeClass() == UK2Node_DynamicCast::StaticClass())
	{
		for(UEdGraphPin const* ContextPin : Filter.Context.Pins)
		{
			// Only worry about removing cast nodes when dragging off output pins
			if (ContextPin->Direction != EGPD_Output)
			{
				break;
			}
			
			if (ContextPin->PinType.PinSubCategoryObject.IsValid())
			{
				UClass const* CastClass = BlueprintAction.GetOwnerClass();
				checkSlow(CastClass != nullptr);

				UClass const* ContextPinClass = Cast<UClass>(ContextPin->PinType.PinSubCategoryObject.Get());
				if ((ContextPinClass == CastClass) || !CastClass->IsChildOf(ContextPinClass))
				{
					bIsFilteredOut = true;
					break;
				}
			}
		}
	}
	return bIsFilteredOut;
}

/*******************************************************************************
 * FBlueprintActionInfo
 ******************************************************************************/

/** */
namespace EBlueprintActionInfoFlags
{
	enum 
	{
		CachedClass    = (1 << 0),
		CachedField    = (1 << 1),
		CachedProperty = (1 << 2),
		CachedFunction = (1 << 3),
	};
}

//------------------------------------------------------------------------------
FBlueprintActionInfo::FBlueprintActionInfo(UObject const* ActionOwnerIn, UBlueprintNodeSpawner const* Action)
	: NodeSpawner(Action)
	, Bindings()
	, ActionOwner(ActionOwnerIn)
	, CacheFlags(0)
	, CachedOwnerClass(nullptr)
	, CachedActionField(nullptr)
	, CachedActionProperty(nullptr)
	, CachedActionFunction(nullptr)
{
	checkSlow(NodeSpawner != nullptr);
}

//------------------------------------------------------------------------------
FBlueprintActionInfo::FBlueprintActionInfo(FBlueprintActionInfo const& Rhs, IBlueprintNodeBinder::FBindingSet const& InBindings)
	: NodeSpawner(Rhs.NodeSpawner)
	, Bindings(InBindings)
	, ActionOwner(Rhs.ActionOwner)
	, CacheFlags(Rhs.CacheFlags)
	, CachedOwnerClass(Rhs.CachedOwnerClass)
	, CachedActionField(Rhs.CachedActionField)
	, CachedActionProperty(Rhs.CachedActionProperty)
	, CachedActionFunction(Rhs.CachedActionFunction)
{
	checkSlow(NodeSpawner != nullptr);
}

//------------------------------------------------------------------------------
UObject const* FBlueprintActionInfo::GetActionOwner()
{
	return ActionOwner;
}

//------------------------------------------------------------------------------
IBlueprintNodeBinder::FBindingSet const& FBlueprintActionInfo::GetBindings() const
{
	return Bindings;
}

//------------------------------------------------------------------------------
UClass const* FBlueprintActionInfo::GetOwnerClass()
{
	if ((CacheFlags & EBlueprintActionInfoFlags::CachedClass) == 0)
	{
		CachedOwnerClass = Cast<UClass>(ActionOwner);
		if (CachedOwnerClass == GetNodeClass())
		{
			CachedOwnerClass = nullptr;
		}

		if (CachedOwnerClass == nullptr)
		{
			CachedOwnerClass = GetAssociatedMemberField()->GetOwnerClass();
		}
		CacheFlags |= EBlueprintActionInfoFlags::CachedClass;
	}
	return CachedOwnerClass;
}

//------------------------------------------------------------------------------
UClass const* FBlueprintActionInfo::GetNodeClass()
{
	UClass const* NodeClass = NodeSpawner->NodeClass;
	checkSlow(NodeClass != nullptr);
	return NodeClass;
}

//------------------------------------------------------------------------------
UField const* FBlueprintActionInfo::GetAssociatedMemberField()
{
	if ((CacheFlags & EBlueprintActionInfoFlags::CachedField) == 0)
	{
		CachedActionField = FBlueprintNodeSpawnerUtils::GetAssociatedField(NodeSpawner);
		CacheFlags |= EBlueprintActionInfoFlags::CachedField;
	}
	return CachedActionField;
}

//------------------------------------------------------------------------------
UProperty const* FBlueprintActionInfo::GetAssociatedProperty()
{
	if ((CacheFlags & EBlueprintActionInfoFlags::CachedProperty) == 0)
	{
		if (CacheFlags & EBlueprintActionInfoFlags::CachedField)
		{
			CachedActionProperty = Cast<UProperty>(CachedActionField);
		}
		else
		{
			CachedActionProperty = FBlueprintNodeSpawnerUtils::GetAssociatedProperty(NodeSpawner);
			CachedActionField = CachedActionProperty;
		}
		CacheFlags |= (EBlueprintActionInfoFlags::CachedProperty | EBlueprintActionInfoFlags::CachedField);
	}
	return CachedActionProperty;
}

//------------------------------------------------------------------------------
UFunction const* FBlueprintActionInfo::GetAssociatedFunction()
{
	if ((CacheFlags & EBlueprintActionInfoFlags::CachedFunction) == 0)
	{
		if (CacheFlags & EBlueprintActionInfoFlags::CachedField)
		{
			CachedActionFunction = Cast<UFunction>(CachedActionField);
		}
		else
		{
			CachedActionFunction = FBlueprintNodeSpawnerUtils::GetAssociatedFunction(NodeSpawner);
			CachedActionField = CachedActionFunction;
		}
		CacheFlags |= (EBlueprintActionInfoFlags::CachedFunction | EBlueprintActionInfoFlags::CachedField);
	}
	return CachedActionFunction;
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
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsMissingMatchingPinParam));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsMissmatchedPropertyType));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsFunctionMissingPinParam));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsIncompatibleImpureNode));
	
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsFieldCategoryHidden));
	if (Flags & BPFILTER_RejectPersistentNonTargetFields)
	{
		AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsPersistentNonTargetField));
	}
	
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsFieldInaccessible));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsNotSubClassCast));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsEventUnimplementable));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsPermissionNotGranted));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsRestrictedClassMember));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsIncompatibleWithGraphType));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsSchemaIncompatible));

	if (!(Flags & BPFILTER_PermitDeprecated))
	{
		AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsDeprecated));
	}

	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsFilteredNodeType, (Flags & BPFILTER_RejectPermittedNodeSubClasses) != 0));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsNonTargetMemeber, (Flags & BPFILTER_RejectPersistentNonTargetFields) == 0));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsUnBoundBindingSpawner));
	AddRejectionTest(FRejectionTestDelegate::CreateStatic(IsOutOfScopeLocalVariable));

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
void FBlueprintActionFilter::AddRejectionTest(FRejectionTestDelegate IsFilteredDelegate)
{
	if (IsFilteredDelegate.IsBound())
	{
		FilterTests.Add(IsFilteredDelegate);
	}
}

//------------------------------------------------------------------------------
bool FBlueprintActionFilter::IsFiltered(FBlueprintActionInfo& BlueprintAction)
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
bool FBlueprintActionFilter::IsFilteredByThis(FBlueprintActionInfo& BlueprintAction) const
{
	FBlueprintActionFilter const& FilterRef = *this;

	bool bIsFiltered = false;
	// iterate backwards so that custom user test are ran first (and the slow
	// internal tests are ran last).
	for (int32 TestIndex = FilterTests.Num()-1; TestIndex >= 0; --TestIndex)
	{
		FRejectionTestDelegate const& RejectionTestDelegate = FilterTests[TestIndex];
		check(RejectionTestDelegate.IsBound());

		if (RejectionTestDelegate.Execute(FilterRef, BlueprintAction))
		{
			bIsFiltered = true;
			break;
		}
	}

	return bIsFiltered;
}
