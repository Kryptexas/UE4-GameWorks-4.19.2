// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EdGraphSchema_K2.cpp
=============================================================================*/


#include "BlueprintGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "ComponentAssetBroker.h"

#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "EngineKismetLibraryClasses.h"
#include "ComponentAssetBroker.h"
#include "AssetData.h"
#include "Editor/UnrealEd/Public/EdGraphUtilities.h"
#include "DefaultValueHelper.h"
#include "ObjectEditorUtils.h"
#include "ActorEditorUtils.h"

#include "K2ActionMenuBuilder.h"

//////////////////////////////////////////////////////////////////////////
// FBlueprintMetadata

const FName FBlueprintMetadata::MD_AllowableBlueprintVariableType(TEXT("BlueprintType"));
const FName FBlueprintMetadata::MD_NotAllowableBlueprintVariableType(TEXT("NotBlueprintType"));

const FName FBlueprintMetadata::MD_BlueprintSpawnableComponent(TEXT("BlueprintSpawnableComponent"));
const FName FBlueprintMetadata::MD_IsBlueprintBase(TEXT("IsBlueprintBase"));

const FName FBlueprintMetadata::MD_Protected(TEXT("BlueprintProtected"));
const FName FBlueprintMetadata::MD_Latent(TEXT("Latent"));
const FName FBlueprintMetadata::MD_UnsafeForConstructionScripts(TEXT("UnsafeDuringActorConstruction"));
const FName FBlueprintMetadata::MD_FunctionCategory(TEXT("Category"));
const FName FBlueprintMetadata::MD_DeprecatedFunction(TEXT("DeprecatedFunction"));
const FName FBlueprintMetadata::MD_DeprecationMessage(TEXT("DeprecationMessage"));
const FName FBlueprintMetadata::MD_CompactNodeTitle(TEXT("CompactNodeTitle"));

const FName FBlueprintMetadata::MD_ExposeOnSpawn(TEXT("ExposeOnSpawn"));
const FName FBlueprintMetadata::MD_DefaultToSelf(TEXT("DefaultToSelf"));
const FName FBlueprintMetadata::MD_WorldContext(TEXT("WorldContext"));
const FName FBlueprintMetadata::MD_AutoCreateRefTerm(TEXT("AutoCreateRefTerm"));

const FName FBlueprintMetadata::MD_ShowHiddenSelfPins(TEXT("ShowHiddenSelfPins"));
const FName FBlueprintMetadata::MD_Private(TEXT("BlueprintPrivate"));

const FName FBlueprintMetadata::MD_BlueprintInternalUseOnly(TEXT("BlueprintInternalUseOnly"));
const FName FBlueprintMetadata::MD_NeedsLatentFixup(TEXT("NeedsLatentFixup"));

const FName FBlueprintMetadata::MD_LatentCallbackTarget(TEXT("LatentCallbackTarget"));
const FName FBlueprintMetadata::MD_AllowPrivateAccess(TEXT("AllowPrivateAccess"));

const FName FBlueprintMetadata::MD_ExposeFunctionCategories(TEXT("ExposeFunctionCategories"));

const FName FBlueprintMetadata::MD_CannotImplementInterfaceInBlueprint(TEXT("CannotImplementInterfaceInBlueprint"));
const FName FBlueprintMetadata::MD_ProhibitedInterfaces(TEXT("ProhibitedInterfaces"));

const FName FBlueprintMetadata::MD_FunctionKeywords(TEXT("Keywords"));

const FName FBlueprintMetadata::MD_ExpandEnumAsExecs(TEXT("ExpandEnumAsExecs"));

const FName FBlueprintMetadata::MD_CommutativeAssociativeBinaryOperator(TEXT("CommutativeAssociativeBinaryOperator"));
const FName FBlueprintMetadata::MD_MaterialParameterCollectionFunction(TEXT("MaterialParameterCollectionFunction"));

const FName FBlueprintMetadata::MD_Tooltip(TEXT("Tooltip"));

//////////////////////////////////////////////////////////////////////////

#define LOCTEXT_NAMESPACE "KismetSchema"

UEdGraphSchema_K2::FPinTypeTreeInfo::FPinTypeTreeInfo(const FString& InFriendlyCategoryName, const FString& CategoryName, const UEdGraphSchema_K2* Schema, const FString& InTooltip, bool bInReadOnly/*=false*/)
{
	Init(InFriendlyCategoryName, CategoryName, Schema, InTooltip, bInReadOnly);
}

UEdGraphSchema_K2::FPinTypeTreeInfo::FPinTypeTreeInfo(const FString& CategoryName, const UEdGraphSchema_K2* Schema, const FString& InTooltip, bool bInReadOnly/*=false*/)
{
	Init(CategoryName, CategoryName, Schema, InTooltip, bInReadOnly);
}

void UEdGraphSchema_K2::FPinTypeTreeInfo::Init(const FString& InFriendlyCategoryName, const FString& CategoryName, const UEdGraphSchema_K2* Schema, const FString& InTooltip, bool bInReadOnly)
{
	check( !CategoryName.IsEmpty() );
	check( Schema );

	FriendlyCategoryName = InFriendlyCategoryName;
	Tooltip = InTooltip;
	PinType.PinCategory = CategoryName;
	PinType.PinSubCategory = TEXT("");
	PinType.PinSubCategoryObject = NULL;

	bReadOnly = bInReadOnly;

	if (Schema->DoesTypeHaveSubtypes(FriendlyCategoryName))
	{
		TArray<UObject*> Subtypes;
		Schema->GetVariableSubtypes(FriendlyCategoryName, Subtypes);
		for (auto it = Subtypes.CreateIterator(); it; ++it)
		{
			FString SubtypeTooltip = CategoryName;
			UStruct* Struct = Cast<UStruct>(*it);
			if(Struct != NULL)
			{
				SubtypeTooltip = Struct->GetToolTipText().ToString();
				if(SubtypeTooltip.Len() == 0)
				{
					SubtypeTooltip = Struct->GetName();
				}
			}

			Children.Add( MakeShareable(new FPinTypeTreeInfo(CategoryName, *it, SubtypeTooltip)) );
		}
	}
}

UEdGraphSchema_K2::FPinTypeTreeInfo::FPinTypeTreeInfo(const FString& CategoryName, UObject* SubCategoryObject, const FString& InTooltip, bool bInReadOnly/*=false*/)
{
	check( !CategoryName.IsEmpty() );
	check( SubCategoryObject );

	Tooltip = InTooltip;
	PinType.PinCategory = CategoryName;
	PinType.PinSubCategoryObject = SubCategoryObject;

	bReadOnly = bInReadOnly;
}

UEdGraphSchema_K2::UEdGraphSchema_K2(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	PC_Exec = TEXT("exec");
	PC_Meta = TEXT("meta");
	PC_Boolean = TEXT("bool");
	PC_Byte = TEXT("byte");
	PC_Class = TEXT("class");
	PC_Int = TEXT("int");
	PC_Float = TEXT("float");
	PC_Name = TEXT("name");
	PC_Delegate = TEXT("delegate");
	PC_MCDelegate = TEXT("mcdelegate");
	PC_Object = TEXT("object");
	PC_String = TEXT("string");
	PC_Text = TEXT("text");
	PC_Struct = TEXT("struct");
	PC_Wildcard = TEXT("wildcard");
	PSC_Self = TEXT("self");
	PSC_Index = TEXT("index");
	PN_Execute = TEXT("execute");
	PN_Then = TEXT("then");
	PN_Completed = TEXT("Completed");
	PN_DelegateEntry = TEXT("delegate");
	PN_EntryPoint = TEXT("EntryPoint");
	PN_Self = TEXT("self");
	PN_Else = TEXT("else");
	PN_Loop = TEXT("loop");
	PN_After = TEXT("after");
	PN_ReturnValue = TEXT("ReturnValue");
	PN_ObjectToCast = TEXT("Object");
	PN_Condition = TEXT("Condition");
	PN_Start = TEXT("Start");
	PN_Stop = TEXT("Stop");
	PN_Index = TEXT("Index");
	PN_CastSucceeded = TEXT("then");
	PN_CastFailed = TEXT("CastFailed");
	PN_CastedValuePrefix = TEXT("As");
	PN_MatineeFinished = TEXT("Finished");
	FN_UserConstructionScript = TEXT("UserConstructionScript");
	FN_ExecuteUbergraphBase = TEXT("ExecuteUbergraph");
	GN_EventGraph = TEXT("EventGraph");
	GN_AnimGraph = TEXT("AnimGraph");
	VR_DefaultCategory = TEXT("Default");
	AG_LevelReference = 100;
}



bool UEdGraphSchema_K2::DoesFunctionHaveOutParameters( const UFunction* Function ) const
{
	if ( Function != NULL )
	{
		for ( TFieldIterator<UProperty> PropertyIt(Function); PropertyIt; ++PropertyIt )
		{
			if ( PropertyIt->PropertyFlags & CPF_OutParm )
			{
				return true;
			}
		}
	}

	return false;
}

bool UEdGraphSchema_K2::CanFunctionBeUsedInClass(const UClass* InClass, UFunction* InFunction, const UEdGraph* InDestGraph, uint32 InFunctionTypes, bool bInShowInherited, bool bInCalledForEach, const FFunctionTargetInfo& InTargetInfo) const
{
	bool bLatentFuncs = true;
	bool bIsConstructionScript = false;

	if(InDestGraph != NULL)
	{
		bLatentFuncs = (GetGraphType(InDestGraph) == GT_Ubergraph);
		bIsConstructionScript = IsConstructionScript(InDestGraph);
	}

	if (CanUserKismetCallFunction(InFunction))
	{
		// See if this is the kind of function we are looking for
		const bool bPureFuncs = (InFunctionTypes & FT_Pure) != 0;
		const bool bImperativeFuncs = (InFunctionTypes & FT_Imperative) != 0;
		const bool bConstFuncs = (InFunctionTypes & FT_Const) != 0;
		const bool bProtectedFuncs = (InFunctionTypes & FT_Protected) != 0;

		const bool bIsPureFunc = (InFunction->HasAnyFunctionFlags(FUNC_BlueprintPure) != false);
		const bool bIsConstFunc = (InFunction->HasAnyFunctionFlags(FUNC_Const) != false);
		const bool bIsLatent = InFunction->HasMetaData(FBlueprintMetadata::MD_Latent);
		const bool bIsBlueprintProtected = InFunction->GetBoolMetaData(FBlueprintMetadata::MD_Protected);
		const bool bIsUnsafeForConstruction = InFunction->GetBoolMetaData(FBlueprintMetadata::MD_UnsafeForConstructionScripts);	
		const bool bFunctionHidden = FObjectEditorUtils::IsFunctionHiddenFromClass(InFunction, InClass);

		const bool bFunctionStatic = InFunction->HasAllFunctionFlags(FUNC_Static);
		const bool bHasReturnParams = (InFunction->GetReturnProperty() != NULL);
		const bool bHasArrayPointerParms = InFunction->HasMetaData(TEXT("ArrayParm"));
		const bool bAllowForEachCall = !bFunctionStatic && !bIsLatent && !bIsPureFunc && !bIsConstFunc && !bHasReturnParams && !bHasArrayPointerParms;

		const bool bClassIsAnActor = InClass->IsChildOf( AActor::StaticClass() );
		const bool bClassIsAComponent = InClass->IsChildOf( UActorComponent::StaticClass() );

		const bool bFunctionHasReturnOrOutParameters = bHasReturnParams || DoesFunctionHaveOutParameters(InFunction);

		// This will evaluate to false if there are multiple actors selected and the function has a return value or out parameters
		const bool bAllowReturnValuesForNoneOrSingleActors = !bClassIsAnActor || InTargetInfo.Actors.Num() <= 1 || !bFunctionHasReturnOrOutParameters;

		if (((bIsPureFunc && bPureFuncs) || (!bIsPureFunc && bImperativeFuncs) || (bIsConstFunc && bConstFuncs))
			&& (!bIsLatent || bLatentFuncs)
			&& (!bIsBlueprintProtected || bProtectedFuncs)
			&& (!bIsUnsafeForConstruction || !bIsConstructionScript)
			&& !bFunctionHidden
			&& (bAllowForEachCall || !bInCalledForEach)
			&& bAllowReturnValuesForNoneOrSingleActors )
		{
			return true;
		}
	}

	return false;
}

UFunction* UEdGraphSchema_K2::GetCallableParentFunction(UFunction* Function) const
{
	if( Function )
	{
		const FName FunctionName = Function->GetFName();

		// Search up the parent scopes
		UClass* ParentClass = CastChecked<UClass>(Function->GetOuter())->GetSuperClass();
		UFunction* ClassFunction = ParentClass->FindFunctionByName(FunctionName);

		return ClassFunction;
	}

	return NULL;
}

bool UEdGraphSchema_K2::CanUserKismetCallFunction(const UFunction* Function)
{
	return Function->HasAllFunctionFlags(FUNC_BlueprintCallable) && !Function->HasAllFunctionFlags(FUNC_Delegate) && !Function->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) && !Function->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction);
}

bool UEdGraphSchema_K2::CanKismetOverrideFunction(const UFunction* Function)
{
	return Function->HasAllFunctionFlags(FUNC_BlueprintEvent) && !Function->HasAllFunctionFlags(FUNC_Delegate) && !Function->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) && !Function->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction);
}

struct FNoOutputParametersHelper 
{
	static bool Check(const UFunction* InFunction)
	{
		check(InFunction);
		for (TFieldIterator<UProperty> PropIt(InFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			UProperty* FuncParam = *PropIt;
			if(FuncParam->HasAnyPropertyFlags(CPF_ReturnParm) || (FuncParam->HasAnyPropertyFlags(CPF_OutParm) && !FuncParam->HasAnyPropertyFlags(CPF_ReferenceParm) && !FuncParam->HasAnyPropertyFlags(CPF_ConstParm)))
			{
				return false;
			}
		}

		return true;
	}
};

bool UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(const UFunction* InFunction)
{
	// First check we are override-able
	if (!CanKismetOverrideFunction(InFunction))
	{
		return false;
	}

	// Then look to see if we have any output, return, or reference params
	return FNoOutputParametersHelper::Check(InFunction);
}

bool UEdGraphSchema_K2::FunctionCanBeUsedInDelegate(const UFunction* InFunction)
{	
	if (!InFunction || 
		!CanUserKismetCallFunction(InFunction) ||
		InFunction->HasMetaData(FBlueprintMetadata::MD_Latent) ||
		InFunction->HasAllFunctionFlags(FUNC_BlueprintPure))
	{
		return false;
	}

	return FNoOutputParametersHelper::Check(InFunction);
}

FString UEdGraphSchema_K2::GetFriendlySignitureName(const UFunction* Function)
{
	return UK2Node_CallFunction::GetUserFacingFunctionName( Function );
}

void UEdGraphSchema_K2::GetAutoEmitTermParameters(const UFunction* Function, TArray<FString>& AutoEmitParameterNames) const
{
	AutoEmitParameterNames.Empty();

	if( Function->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) )
	{
		FString MetaData = Function->GetMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm);
		MetaData.ParseIntoArray(&AutoEmitParameterNames, TEXT(","), true);
	}
}

bool UEdGraphSchema_K2::FunctionHasParamOfType(const UFunction* InFunction, UBlueprint const* CallingContext, const FEdGraphPinType& DesiredPinType, bool bWantOutput) const
{
	TSet<FString> HiddenPins;
	FBlueprintEditorUtils::GetHiddenPinsForFunction(CallingContext, InFunction, HiddenPins);

	// Iterate over all params of function
	for (TFieldIterator<UProperty> PropIt(InFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* FuncParam = *PropIt;

		// Ensure that this isn't a hidden parameter
		if (!HiddenPins.Contains(FuncParam->GetName()))
		{
			// See if this is the direction we want (input or output)
			const bool bIsFunctionInput = !FuncParam->HasAnyPropertyFlags(CPF_OutParm) || FuncParam->HasAnyPropertyFlags(CPF_ReferenceParm);
			if ((!bIsFunctionInput && bWantOutput) || (bIsFunctionInput && !bWantOutput))
			{
				// See if this pin has compatible types
				FEdGraphPinType ParamPinType;
				bool bConverted = ConvertPropertyToPinType(FuncParam, ParamPinType);
				if (bConverted)
				{
					if (bIsFunctionInput && ArePinTypesCompatible(DesiredPinType, ParamPinType))
					{
						return true;
					}
					else if (!bIsFunctionInput && ArePinTypesCompatible(ParamPinType, DesiredPinType))
					{
						return true;
					}
				}
			}
		}
	}

	// Boo, no pin of this type
	return false;
}

void UEdGraphSchema_K2::AddExtraFunctionFlags(const UEdGraph* CurrentGraph, int32 ExtraFlags) const
{
	for (auto It = CurrentGraph->Nodes.CreateConstIterator(); It; ++It)
	{
		if (UK2Node_FunctionEntry* Node = Cast<UK2Node_FunctionEntry>(*It))
		{
			Node->ExtraFlags |= ExtraFlags;
		}
	}
}

void UEdGraphSchema_K2::MarkFunctionEntryAsEditable(const UEdGraph* CurrentGraph, bool bNewEditable) const
{
	for (auto It = CurrentGraph->Nodes.CreateConstIterator(); It; ++It)
	{
		if (UK2Node_EditablePinBase* Node = Cast<UK2Node_EditablePinBase>(*It))
		{
			Node->bIsEditable = bNewEditable;
		}
	}
}

void UEdGraphSchema_K2::ListFunctionsMatchingSignatureAsDelegates(FGraphContextMenuBuilder& ContextMenuBuilder, const UClass* Class, const UFunction* SignatureToMatch) const
{
	check(Class);

	for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
	{
		const UFunction* TrialFunction = *FunctionIt;
		if (CanUserKismetCallFunction(TrialFunction) && TrialFunction->IsSignatureCompatibleWith(SignatureToMatch))
		{
			FString Description(TrialFunction->GetName());
			FString Tooltip = FString::Printf(TEXT("Existing function '%s' as delegate"), *(TrialFunction->GetName())); //@TODO: Need a better tooltip

			// @TODO
		}
	}

}

bool UEdGraphSchema_K2::IsActorValidForLevelScriptRefs(const AActor* TestActor, const ULevelScriptBlueprint* Blueprint) const
{
	check(Blueprint);
	
	return TestActor
		&& (TestActor->GetLevel() == Blueprint->GetLevel())
		&& FKismetEditorUtilities::IsActorValidForLevelScript(TestActor);
}

void UEdGraphSchema_K2::ReplaceSelectedNode(UEdGraphNode* SourceNode, AActor* TargetActor)
{
	check(SourceNode);

	if (TargetActor != NULL)
	{
		UK2Node_Literal* LiteralNode = (UK2Node_Literal*)(SourceNode);
		if (LiteralNode)
		{
			const FScopedTransaction Transaction( LOCTEXT("ReplaceSelectedNodeUndoTransaction", "Replace Selected Node") );

			LiteralNode->Modify();
			LiteralNode->SetObjectRef( TargetActor );
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(CastChecked<UEdGraph>(SourceNode->GetOuter()));
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
	}
}

void UEdGraphSchema_K2::AddSelectedReplaceableNodes( UBlueprint* Blueprint, const UEdGraphNode* InGraphNode, FMenuBuilder* MenuBuilder ) const
{
	ULevelScriptBlueprint* LevelBlueprint = Cast<ULevelScriptBlueprint>(Blueprint);
	
	if (LevelBlueprint)
	{
		//Only allow replace object reference functionality for literal nodes
		if( InGraphNode->IsA( UK2Node_Literal::StaticClass() ) )
		{
			UK2Node_Literal* LiteralNode = (UK2Node_Literal*)(InGraphNode);

			if( LiteralNode )
			{
				USelection* SelectedActors = GEditor->GetSelectedActors();
				for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
				{
					// We only care about actors that are referenced in the world for literals, and also in the same level as this blueprint
					AActor* Actor = Cast<AActor>(*Iter);
					if( LiteralNode->GetObjectRef() != Actor && IsActorValidForLevelScriptRefs(Actor, LevelBlueprint) )
					{
						FText Description = FText::Format( LOCTEXT("ChangeToActorName", "Change to <{0}>"), FText::FromString( Actor->GetActorLabel() ) );
						FText ToolTip = LOCTEXT("ReplaceNodeReferenceToolTip", "Replace node reference");
						MenuBuilder->AddMenuEntry( Description, ToolTip, FSlateIcon(), FUIAction(
							FExecuteAction::CreateUObject((UEdGraphSchema_K2*const)this, &UEdGraphSchema_K2::ReplaceSelectedNode, const_cast< UEdGraphNode* >(InGraphNode), Actor) ) );
					}
				}
			}
		}
	}
}



bool UEdGraphSchema_K2::CanUserKismetAccessVariable(const UProperty* Property, const UClass* InClass, EDelegateFilterMode FilterMode)
{
	const bool bIsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
	const bool bIsAccessible = Property->HasAllPropertyFlags(CPF_BlueprintVisible);
	const bool bIsAssignableOrCallable = Property->HasAnyPropertyFlags(CPF_BlueprintAssignable | CPF_BlueprintCallable);
	
	const bool bPassesDelegateFilter = (bIsAccessible && !bIsDelegate && (FilterMode != MustBeDelegate)) || 
		(bIsAssignableOrCallable && bIsDelegate && (FilterMode != CannotBeDelegate));

	const bool bHidden = FObjectEditorUtils::IsVariableCategoryHiddenFromClass(Property, InClass);

	return !Property->HasAnyPropertyFlags(CPF_Parm) && bPassesDelegateFilter && !bHidden;
}

bool UEdGraphSchema_K2::ClassHasBlueprintAccessibleMembers(const UClass* InClass) const
{
	// @TODO Don't show other blueprints yet...
	UBlueprint* ClassBlueprint = UBlueprint::GetBlueprintFromClass(InClass);
	if (!InClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists) && (ClassBlueprint == NULL))
	{
		// Find functions
		for (TFieldIterator<UFunction> FunctionIt(InClass, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
		{
			UFunction* Function = *FunctionIt;
			const bool bIsBlueprintProtected = Function->GetBoolMetaData(FBlueprintMetadata::MD_Protected);
			const bool bHidden = FObjectEditorUtils::IsFunctionHiddenFromClass(Function, InClass);
			if (UEdGraphSchema_K2::CanUserKismetCallFunction(Function) && !bIsBlueprintProtected && !bHidden)
			{
				return true;
			}
		}

		// Find vars
		for (TFieldIterator<UProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			if (CanUserKismetAccessVariable(Property, InClass, CannotBeDelegate))
			{
				return true;
			}
		}
	}

	return false;
}

bool UEdGraphSchema_K2::IsAllowableBlueprintVariableType(const class UEnum* InEnum)
{
	return InEnum && (InEnum->GetBoolMetaData(FBlueprintMetadata::MD_AllowableBlueprintVariableType) || InEnum->IsA<UUserDefinedEnum>());
}

bool UEdGraphSchema_K2::IsAllowableBlueprintVariableType(const class UClass* InClass)
{
	if (InClass)
	{
		// No Skeleton classes or reinstancing classes (they would inherit the BlueprintType metadata)
		if (FKismetEditorUtilities::IsClassABlueprintSkeleton(InClass)
			|| InClass->HasAnyClassFlags(CLASS_NewerVersionExists))
		{
			return false;
		}

		// UObject is an exception, and is always a blueprint-able type
		if(InClass == UObject::StaticClass())
		{
			return true;
		}

		const UClass* ParentClass = InClass;
		while(ParentClass)
		{
			// Climb up the class hierarchy and look for "BlueprintType" and "NotBlueprintType" to see if this class is allowed.
			if(ParentClass->GetBoolMetaData(FBlueprintMetadata::MD_AllowableBlueprintVariableType)
				|| ParentClass->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent))
			{
				return true;
			}
			else if(ParentClass->GetBoolMetaData(FBlueprintMetadata::MD_NotAllowableBlueprintVariableType))
			{
				return false;
			}
			ParentClass = ParentClass->GetSuperClass();
		}
	}
	
	return false;
}

bool UEdGraphSchema_K2::IsAllowableBlueprintVariableType(const class UScriptStruct *InStruct)
{
	return InStruct && (InStruct->GetBoolMetaDataHierarchical(FBlueprintMetadata::MD_AllowableBlueprintVariableType));
}

bool UEdGraphSchema_K2::DoesGraphSupportImpureFunctions(const UEdGraph* InGraph) const
{
	const EGraphType GraphType = GetGraphType(InGraph);
	const bool bAllowImpureFuncs = GraphType != GT_Animation; //@TODO: It's really more nuanced than this (e.g., in a function someone wants to write as pure)

	return bAllowImpureFuncs;
}

// if node is a get/set variable and the variable it refers to does not exist
static bool IsUsingNonExistantVariable(const UEdGraphNode* InGraphNode, UBlueprint* OwnerBlueprint)
{
	bool bNonExistantVariable = false;
	const bool bBreakOrMakeStruct = 
		InGraphNode->IsA(UK2Node_BreakStruct::StaticClass()) || 
		InGraphNode->IsA(UK2Node_MakeStruct::StaticClass());
	if (!bBreakOrMakeStruct)
	{
		if (const UK2Node_Variable* Variable = Cast<const UK2Node_Variable>(InGraphNode))
		{
			if (Variable->VariableReference.IsSelfContext())
			{
				TArray<FName> CurrentVars;
				FBlueprintEditorUtils::GetClassVariableList(OwnerBlueprint, CurrentVars);
				if (false == CurrentVars.Contains(Variable->GetVarName()))
				{
					bNonExistantVariable = true;
				}
			}
		}
	}
	return bNonExistantVariable;
}

void UEdGraphSchema_K2::GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FMenuBuilder* MenuBuilder, bool bIsDebugging) const
{
	check(CurrentGraph);
	UBlueprint* OwnerBlueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(CurrentGraph);
	
	if (InGraphPin != NULL)
	{
		MenuBuilder->BeginSection("EdGraphSchemaPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
		{
			if (!bIsDebugging)
		    {
			    // Break pin links
			    if (InGraphPin->LinkedTo.Num() > 1)
			    {
				    MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().BreakPinLinks );
			    }
    
			    // Add the change pin type action, if this is a select node
			    if (InGraphNode->IsA(UK2Node_Select::StaticClass()))
			    {
				    MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().ChangePinType);
			    }
    
			    // add sub menu for break link to
			    if (InGraphPin->LinkedTo.Num() > 0)
			    {
				    if(InGraphPin->LinkedTo.Num() > 1)
				    {
					    MenuBuilder->AddSubMenu(
						    LOCTEXT("BreakLinkTo", "Break Link To..."),
						    LOCTEXT("BreakSpecificLinks", "Break a specific link..."),
						    FNewMenuDelegate::CreateUObject( (UEdGraphSchema_K2*const)this, &UEdGraphSchema_K2::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(InGraphPin)));
				    }
				    else
				    {
					    ((UEdGraphSchema_K2*const)this)->GetBreakLinkToSubMenuActions(*MenuBuilder, const_cast<UEdGraphPin*>(InGraphPin));
				    }
			    }
    
			    // Conditionally add the var promotion pin if this is an output pin and it's not an exec pin
			    if (InGraphPin->PinType.PinCategory != PC_Exec)
			    {
				    MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().PromoteToVariable );
			    }
    
			    // Conditionally add the execution path pin removal if this is an execution branching node
			    if( InGraphPin->Direction == EGPD_Output && InGraphPin->GetOwningNode())
			    {
				    if ( InGraphPin->GetOwningNode()->IsA(UK2Node_ExecutionSequence::StaticClass()) ||  InGraphPin->GetOwningNode()->IsA(UK2Node_Switch::StaticClass()) )
				    {
					    MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().RemoveExecutionPin );
				    }
			    }
		    }
		}
		MenuBuilder->EndSection();

		// Add the watch pin / unwatch pin menu items
		MenuBuilder->BeginSection("EdGraphSchemaWatches", LOCTEXT("WatchesHeader", "Watches"));
		{
			if (!IsMetaPin(*InGraphPin))
			{
				const UEdGraphPin* WatchedPin = ((InGraphPin->Direction == EGPD_Input) && (InGraphPin->LinkedTo.Num() > 0)) ? InGraphPin->LinkedTo[0] : InGraphPin;
				if (FKismetDebugUtilities::IsPinBeingWatched(OwnerBlueprint, WatchedPin))
				{
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().StopWatchingPin );
				}
				else
				{
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().StartWatchingPin );
				}
			}
		}
		MenuBuilder->EndSection();
	}
	else if (InGraphNode != NULL)
	{
		if (IsUsingNonExistantVariable(InGraphNode, OwnerBlueprint))
		{
			MenuBuilder->BeginSection("EdGraphSchemaNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
			{
				GetNonExistentVariableMenu(InGraphNode,OwnerBlueprint, MenuBuilder);
			}
			MenuBuilder->EndSection();
		}
		else
		{
			MenuBuilder->BeginSection("EdGraphSchemaNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
			{
				if (!bIsDebugging)
				{
					// Replaceable node display option
					AddSelectedReplaceableNodes( OwnerBlueprint, InGraphNode, MenuBuilder );

					// Node contextual actions
					MenuBuilder->AddMenuEntry( FGenericCommands::Get().Delete );
					MenuBuilder->AddMenuEntry( FGenericCommands::Get().Cut );
					MenuBuilder->AddMenuEntry( FGenericCommands::Get().Copy );
					MenuBuilder->AddMenuEntry( FGenericCommands::Get().Duplicate );
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().ReconstructNodes );
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().BreakNodeLinks );

					// tunnel nodes have option to open function editor
					if (InGraphNode->IsA(UK2Node_Tunnel::StaticClass()))
					{
						MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().EditTunnel );
					}

					// Conditionally add the action to add an execution pin, if this is an execution node
					if( InGraphNode->IsA(UK2Node_ExecutionSequence::StaticClass()) || InGraphNode->IsA(UK2Node_Switch::StaticClass()) )
					{
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().AddExecutionPin );
					}

					// Conditionally add the action to create a super function call node, if this is an event or function entry
					if( InGraphNode->IsA(UK2Node_Event::StaticClass()) || InGraphNode->IsA(UK2Node_FunctionEntry::StaticClass()) )
					{
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().AddParentNode );
					}

					// Conditionally add the actions to add or remove an option pin, if this is a select node
					if (InGraphNode->IsA(UK2Node_Select::StaticClass()))
					{
						MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().AddOptionPin);
						MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().RemoveOptionPin);
					}

					// Conditionally add the action to find instances of the node if it is a custom event
					if (InGraphNode->IsA(UK2Node_CustomEvent::StaticClass()))
					{
						MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().FindInstancesOfCustomEvent);
					}

					// Don't show the "Assign selected Actor" option if more than one actor is selected
					if (InGraphNode->IsA(UK2Node_ActorBoundEvent::StaticClass()) && GEditor->GetSelectedActorCount() == 1)
					{
						MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().AssignReferencedActor);
					}

					// show search for references for variable nodes
					if (InGraphNode->IsA(UK2Node_Variable::StaticClass()))
					{
						MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().FindVariableReferences);
					}

					MenuBuilder->AddMenuEntry(FGenericCommands::Get().Rename, NAME_None, LOCTEXT("Rename", "Rename"), LOCTEXT("Rename_Tooltip", "Renames selected function or variable in blueprint.") );
				}

				// Select referenced actors in the level
				MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().SelectReferenceInLevel );
			}
			MenuBuilder->EndSection(); //EdGraphSchemaNodeActions

			if (!bIsDebugging)
			{
				// Collapse/expand nodes
				MenuBuilder->BeginSection("EdGraphSchemaOrganization", LOCTEXT("OrganizationHeader", "Organization"));
				{
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().CollapseNodes );
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().CollapseSelectionToFunction );
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().CollapseSelectionToMacro );
					MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().ExpandNodes );

					if(InGraphNode->IsA(UK2Node_Composite::StaticClass()))
					{
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().PromoteSelectionToFunction );
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().PromoteSelectionToMacro );
					}
				}
				MenuBuilder->EndSection();
			}

			// Add breakpoint actions
			if (const UK2Node* K2Node = Cast<const UK2Node>(InGraphNode))
			{
				if (!K2Node->IsNodePure())
				{
					MenuBuilder->BeginSection("EdGraphSchemaBreakpoints", LOCTEXT("BreakpointsHeader", "Breakpoints"));
					{
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().ToggleBreakpoint );
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().AddBreakpoint );
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().RemoveBreakpoint );
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().EnableBreakpoint );
						MenuBuilder->AddMenuEntry( FGraphEditorCommands::Get().DisableBreakpoint );
					}
					MenuBuilder->EndSection();
				}
			}
		}
	}

	Super::GetContextMenuActions(CurrentGraph, InGraphNode, InGraphPin, MenuBuilder, bIsDebugging);
}


void UEdGraphSchema_K2::OnCreateNonExistentVariable( UK2Node_Variable* Variable,  UBlueprint* OwnerBlueprint)
{
	if (UEdGraphPin* Pin = Variable->FindPin(Variable->GetVarNameString()))
	{
		if (FBlueprintEditorUtils::AddMemberVariable(OwnerBlueprint,Variable->GetVarName(), Pin->PinType))
		{
			Variable->VariableReference.SetSelfMember( Variable->GetVarName() );
		}
	}	
}

void UEdGraphSchema_K2::OnReplaceVariableForVariableNode( UK2Node_Variable* Variable, UBlueprint* OwnerBlueprint, FString VariableName)
{
	if (UEdGraphPin* Pin = Variable->FindPin(Variable->GetVarNameString()))
	{
		Variable->VariableReference.SetSelfMember( FName(*VariableName) );
		Pin->PinName = VariableName;
	}
}

void UEdGraphSchema_K2::GetReplaceNonExistentVariableMenu(FMenuBuilder& MenuBuilder, UK2Node_Variable* Variable,  UBlueprint* OwnerBlueprint)
{
	if (UEdGraphPin* Pin = Variable->FindPin(Variable->GetVarNameString()))
	{
		TArray<FName> Variables;
		FBlueprintEditorUtils::GetNewVariablesOfType(OwnerBlueprint, Pin->PinType, Variables);

		for (TArray<FName>::TIterator VarIt(Variables); VarIt; ++VarIt)
		{
			const FText AlternativeVar = FText::FromName( *VarIt );
			const FText Desc = FText::Format( LOCTEXT("ReplaceNonExistantVarToolTip", "Variable '{0}' does not exist, replace with matching variable '{0}'?"), Variable->GetVarNameText(), AlternativeVar );

			MenuBuilder.AddMenuEntry( AlternativeVar, Desc, FSlateIcon(), FUIAction(
				FExecuteAction::CreateStatic(&UEdGraphSchema_K2::OnReplaceVariableForVariableNode, const_cast<UK2Node_Variable* >(Variable),OwnerBlueprint, (*VarIt).ToString() ) ) );
		}
	}
}

void UEdGraphSchema_K2::GetNonExistentVariableMenu( const UEdGraphNode* InGraphNode, UBlueprint* OwnerBlueprint, FMenuBuilder* MenuBuilder ) const
{

	if (const UK2Node_Variable* Variable = Cast<const UK2Node_Variable>(InGraphNode))
	{
		// create missing variable
		{			
			const FText Label = FText::Format( LOCTEXT("CreateNonExistentVar", "Create variable '{0}'"), Variable->GetVarNameText());
			const FText Desc = FText::Format( LOCTEXT("CreateNonExistentVarToolTip", "Variable '{0}' does not exist, create it?"), Variable->GetVarNameText());
			MenuBuilder->AddMenuEntry( Label, Desc, FSlateIcon(), FUIAction(
				FExecuteAction::CreateStatic( &UEdGraphSchema_K2::OnCreateNonExistentVariable, const_cast<UK2Node_Variable* >(Variable),OwnerBlueprint) ) );
		}

		// delete this node
		{			
			const FText Desc = FText::Format( LOCTEXT("DeleteNonExistentVarToolTip", "Referenced variable '{0}' does not exist, delete this node?"), Variable->GetVarNameText());
			MenuBuilder->AddMenuEntry( FGenericCommands::Get().Delete, NAME_None, FGenericCommands::Get().Delete->GetLabel(), Desc);
		}

		// replace with matching variables
		if (UEdGraphPin* Pin = Variable->FindPin(Variable->GetVarNameString()))
		{
			TArray<FName> Variables;
			FBlueprintEditorUtils::GetNewVariablesOfType(OwnerBlueprint, Pin->PinType, Variables);

			if (Variables.Num() > 0)
			{
				MenuBuilder->AddSubMenu(
					FText::Format( LOCTEXT("ReplaceVariableWith", "Replace variable '{0}' with..."), Variable->GetVarNameText()),
					FText::Format( LOCTEXT("ReplaceVariableWithToolTip", "Variable '{0}' does not exist, replace with another variable?"), Variable->GetVarNameText()),
					FNewMenuDelegate::CreateStatic( &UEdGraphSchema_K2::GetReplaceNonExistentVariableMenu,
					const_cast<UK2Node_Variable*>(Variable),OwnerBlueprint));
			}
		}
	}
}

void UEdGraphSchema_K2::GetBreakLinkToSubMenuActions( class FMenuBuilder& MenuBuilder, UEdGraphPin* InGraphPin )
{
	// Make sure we have a unique name for every entry in the list
	TMap< FString, uint32 > LinkTitleCount;

	// Add all the links we could break from
	for(TArray<class UEdGraphPin*>::TConstIterator Links(InGraphPin->LinkedTo); Links; ++Links)
	{
		UEdGraphPin* Pin = *Links;
		FString TitleString = Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView);
		FText Title = FText::FromString( TitleString );
		if ( Pin->PinName != TEXT("") )
		{
			TitleString = FString::Printf(TEXT("%s (%s)"), *TitleString, *Pin->PinName);

			// Add name of connection if possible
			FFormatNamedArguments Args;
			Args.Add( TEXT("NodeTitle"), Title );
			Args.Add( TEXT("PinName"), FText::FromString( Pin->PinName ) );
			Title = FText::Format( LOCTEXT("BreakDescPin", "{NodeTitle} ({PinName})"), Args );
		}

		uint32 &Count = LinkTitleCount.FindOrAdd( TitleString );

		FText Description;
		FFormatNamedArguments Args;
		Args.Add( TEXT("NodeTitle"), Title );
		Args.Add( TEXT("NumberOfNodes"), Count );

		if ( Count == 0 )
		{
			Description = FText::Format( LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args );
		}
		else
		{
			Description = FText::Format( LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args );
		}
		++Count;

		MenuBuilder.AddMenuEntry( Description, Description, FSlateIcon(), FUIAction(
			FExecuteAction::CreateUObject((USoundClassGraphSchema*const)this, &USoundClassGraphSchema::BreakSinglePinLink, const_cast< UEdGraphPin* >(InGraphPin), *Links) ) );
	}
}

void UEdGraphSchema_K2::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	FBlueprintGraphActionListBuilder BlueprintContextMenuBuilder(ContextMenuBuilder.CurrentGraph);
	BlueprintContextMenuBuilder.FromPin = ContextMenuBuilder.FromPin;
	BlueprintContextMenuBuilder.SelectedObjects.Append(ContextMenuBuilder.SelectedObjects);
	check(BlueprintContextMenuBuilder.Blueprint != NULL);

	// Run thru all nodes and add any menu items they want to add
	Super::GetGraphContextActions(BlueprintContextMenuBuilder);

	// Now do schema-specific stuff
	FK2ActionMenuBuilder(this).GetGraphContextActions(BlueprintContextMenuBuilder);
	ContextMenuBuilder.Append(BlueprintContextMenuBuilder);
}

void UEdGraphSchema_K2::GetAllActions(FBlueprintPaletteListBuilder& PaletteBuilder)
	{
	const UEdGraphSchema_K2* K2SchemaInst = GetDefault<UEdGraphSchema_K2>();
	FK2ActionMenuBuilder(K2SchemaInst).GetAllActions(PaletteBuilder);
	}

void UEdGraphSchema_K2::GetPaletteActions(FBlueprintPaletteListBuilder& ActionMenuBuilder, TWeakObjectPtr<UClass> FilterClass/* = NULL*/)
	{
	const UEdGraphSchema_K2* K2SchemaInst = GetDefault<UEdGraphSchema_K2>();
	FK2ActionMenuBuilder(K2SchemaInst).GetPaletteActions(ActionMenuBuilder, FilterClass);
}

const FPinConnectionResponse UEdGraphSchema_K2::DetermineConnectionResponseOfCompatibleTypedPins(const UEdGraphPin* PinA, const UEdGraphPin* PinB, const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const
{
	// Now check to see if there are already connections and this is an 'exclusive' connection
	const bool bBreakExistingDueToExecOutput = IsExecPin(*OutputPin) && (OutputPin->LinkedTo.Num() > 0);
	const bool bBreakExistingDueToDataInput = !IsExecPin(*InputPin) && (InputPin->LinkedTo.Num() > 0);

	bool bMultipleSelfException = false;
	const UK2Node* OwningNode = Cast<UK2Node>(InputPin->GetOwningNode());
	if (bBreakExistingDueToDataInput && 
		IsSelfPin(*InputPin) && 
		OwningNode &&
		OwningNode->AllowMultipleSelfs(false) &&
		!InputPin->PinType.bIsArray &&
		!OutputPin->PinType.bIsArray)
	{
		//check if the node wont be expanded as foreach call, if there is a link to an array
		bool bAnyArrayInput = false;
		for(int InputLinkIndex = 0; InputLinkIndex < InputPin->LinkedTo.Num(); InputLinkIndex++)
		{
			if(const UEdGraphPin* Pin = InputPin->LinkedTo[InputLinkIndex])
			{
				if(Pin->PinType.bIsArray)
				{
					bAnyArrayInput = true;
					break;
				}
			}
		}
		bMultipleSelfException = !bAnyArrayInput;
	}

	if (bBreakExistingDueToExecOutput)
	{
		const ECanCreateConnectionResponse ReplyBreakOutputs = (PinA == OutputPin) ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B;
		return FPinConnectionResponse(ReplyBreakOutputs, TEXT("Replace existing output connections"));
	}
	else if (bBreakExistingDueToDataInput && !bMultipleSelfException)
	{
		const ECanCreateConnectionResponse ReplyBreakInputs = (PinA == InputPin) ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B;
		return FPinConnectionResponse(ReplyBreakInputs, TEXT("Replace existing input connections"));
	}
	else
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
	}
}

const FPinConnectionResponse UEdGraphSchema_K2::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Find the calling context in case one of the pins is of type object and has a value of Self
	UClass* CallingContext = NULL;
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(PinA->GetOwningNode());
	if (Blueprint)
	{
		CallingContext = (Blueprint->GeneratedClass != NULL) ? Blueprint->GeneratedClass : Blueprint->ParentClass;
	}

	const UK2Node* OwningNodeA = Cast<UK2Node>(PinA->GetOwningNode());
	const UK2Node* OwningNodeB = Cast<UK2Node>(PinB->GetOwningNode());

	// Make sure the pins are not on the same node
	if (OwningNodeA == OwningNodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are on the same node"));
	}

	// node can disallow the connection
	{
		FString RespondMessage;
		if(OwningNodeA && OwningNodeA->IsConnectionDisallowed(PinA, PinB, RespondMessage))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, RespondMessage);
		}
		if(OwningNodeB && OwningNodeB->IsConnectionDisallowed(PinB, PinA, RespondMessage))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, RespondMessage);
		}
	}

	// Compare the directions
	const UEdGraphPin* InputPin = NULL;
	const UEdGraphPin* OutputPin = NULL;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Directions are not compatible"));
	}

	bool bIgnoreArray = false;
	if(const UK2Node* OwningNode = Cast<UK2Node>(InputPin->GetOwningNode()))
	{
		const bool bAllowMultipleSelfs = OwningNode->AllowMultipleSelfs(true); // it applies also to ForEachCall
		const bool bNotAnArrayFunction = !InputPin->PinType.bIsArray;
		const bool bSelfPin = IsSelfPin(*InputPin);
		bIgnoreArray = bAllowMultipleSelfs && bNotAnArrayFunction && bSelfPin;
	}

	// Compare the types
	const bool bTypesMatch = ArePinsCompatible(OutputPin, InputPin, CallingContext, bIgnoreArray);

	if (bTypesMatch)
	{
		return DetermineConnectionResponseOfCompatibleTypedPins(PinA, PinB, InputPin, OutputPin);
	}
	else
	{
		// Autocasting
		FName DummyName;
		UK2Node* DummyNode;

		const bool bCanAutocast = SearchForAutocastFunction(OutputPin, InputPin, /*out*/ DummyName);
		const bool bCanAutoConvert = FindSpecializedConversionNode(OutputPin, InputPin, false, /* out */ DummyNode);

		if (bCanAutocast || bCanAutoConvert)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, FString::Printf(TEXT("Convert %s to %s"), *TypeToString(OutputPin->PinType), *TypeToString(InputPin->PinType)));
		}
		else
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FString::Printf(TEXT("%s is not compatible with %s"), *TypeToString(PinA->PinType), *TypeToString(PinB->PinType)));
		}
	}
}

bool UEdGraphSchema_K2::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(PinA->GetOwningNode());

	bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	return bModified;
}

bool UEdGraphSchema_K2::SearchForAutocastFunction(const UEdGraphPin* OutputPin, const UEdGraphPin* InputPin, /*out*/ FName& TargetFunction) const
{
	// NOTE: Under no circumstances should anyone *ever* add a questionable cast to this function.
	// If it could be at all confusing why a function is provided, to even a novice user, err on the side of do not cast!!!
	// This includes things like string->int (does it do length, atoi, or what?) that would be autocasts in a traditional scripting language

	TargetFunction = NAME_None;

	const UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
	const UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
	const UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
	const UScriptStruct* LinearColorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("LinearColor"));
	const UScriptStruct* ColorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Color"));

	const UScriptStruct* InputStructType = Cast<const UScriptStruct>(InputPin->PinType.PinSubCategoryObject.Get());
	const UScriptStruct* OutputStructType = Cast<const UScriptStruct>(OutputPin->PinType.PinSubCategoryObject.Get());

	if (OutputPin->PinType.bIsArray != InputPin->PinType.bIsArray)
	{
		// We don't autoconvert between arrays and non-arrays.  Those are handled by specialized conversions
	}
	else if (OutputPin->PinType.PinCategory == PC_Int)
	{
		if (InputPin->PinType.PinCategory == PC_Float)
		{
			TargetFunction = TEXT("Conv_IntToFloat");
		}
		else if (InputPin->PinType.PinCategory == PC_String)
		{
			TargetFunction = TEXT("Conv_IntToString");
		}
		else if ((InputPin->PinType.PinCategory == PC_Byte) && (InputPin->PinType.PinSubCategoryObject == NULL))
		{
			TargetFunction = TEXT("Conv_IntToByte");
		}
		else if (InputPin->PinType.PinCategory == PC_Boolean)
		{
			TargetFunction = TEXT("Conv_IntToBool");
		}
		else if(InputPin->PinType.PinCategory == PC_Text)
		{
			TargetFunction = TEXT("Conv_IntToText");
		}
	}
	else if (OutputPin->PinType.PinCategory == PC_Float)
	{
		if (InputPin->PinType.PinCategory == PC_Int)
		{
			TargetFunction = TEXT("FFloor");
		}
		else if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == VectorStruct))
		{
			TargetFunction = TEXT("Conv_FloatToVector");
		}
		else if (InputPin->PinType.PinCategory == PC_String)
		{
			TargetFunction = TEXT("Conv_FloatToString");
		}
		else if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == LinearColorStruct))
		{
			TargetFunction = TEXT("Conv_FloatToLinearColor");
		}
		else if(InputPin->PinType.PinCategory == PC_Text)
		{
			TargetFunction = TEXT("Conv_FloatToText");
		}
	}
	else if (OutputPin->PinType.PinCategory == PC_Struct)
	{
		if (OutputStructType == VectorStruct)
		{
			if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == TransformStruct))
			{
				TargetFunction = TEXT("Conv_VectorToTransform");
			}
			if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == LinearColorStruct))
			{
				TargetFunction = TEXT("Conv_VectorToLinearColor");
			}
			else if (InputPin->PinType.PinCategory == PC_String)
			{
				TargetFunction = TEXT("Conv_VectorToString");
			}
			// NOTE: Did you see the note above about unsafe and unclear casts?
		}
		else if(OutputStructType == RotatorStruct)
		{
			if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == TransformStruct))
			{
				TargetFunction = TEXT("MakeTransform");
			}
			else if (InputPin->PinType.PinCategory == PC_String)
			{
				TargetFunction = TEXT("Conv_RotatorToString");
			}
			// NOTE: Did you see the note above about unsafe and unclear casts?
		}
		else if(OutputStructType == LinearColorStruct)
		{
			if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == ColorStruct))
			{
				TargetFunction = TEXT("Conv_LinearColorToColor");
			}
			else if (InputPin->PinType.PinCategory == PC_String)
			{
				TargetFunction = TEXT("Conv_ColorToString");
			}
			else if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == VectorStruct))
			{
				TargetFunction = TEXT("Conv_LinearColorToVector");
			}
		}
		else if(OutputStructType == ColorStruct)
		{
			if ((InputPin->PinType.PinCategory == PC_Struct) && (InputStructType == LinearColorStruct))
			{
				TargetFunction = TEXT("Conv_ColorToLinearColor");
			}
		}
	}
	else if (OutputPin->PinType.PinCategory == PC_Boolean)
	{
		if (InputPin->PinType.PinCategory == PC_String)
		{
			TargetFunction = TEXT("Conv_BoolToString");
		}
		else if (InputPin->PinType.PinCategory == PC_Int)
		{
			TargetFunction = TEXT("Conv_BoolToInt");
		}
		else if (InputPin->PinType.PinCategory == PC_Float)
		{
			TargetFunction = TEXT("Conv_BoolToFloat");
		}
		else if ((InputPin->PinType.PinCategory == PC_Byte) && (InputPin->PinType.PinSubCategoryObject == NULL))
		{
			TargetFunction = TEXT("Conv_BoolToByte");
		}
	}
	else if (OutputPin->PinType.PinCategory == PC_Byte &&
			 (OutputPin->PinType.PinSubCategoryObject == NULL || !OutputPin->PinType.PinSubCategoryObject->IsA(UEnum::StaticClass())))
	{
		if (InputPin->PinType.PinCategory == PC_String)
		{
			TargetFunction = TEXT("Conv_ByteToString");
		}
		else if (InputPin->PinType.PinCategory == PC_Int)
		{
			TargetFunction = TEXT("Conv_ByteToInt");
		}
		else if (InputPin->PinType.PinCategory == PC_Float)
		{
			TargetFunction = TEXT("Conv_ByteToFloat");
		}
	}
	else if (OutputPin->PinType.PinCategory == PC_Name)
	{
		if (InputPin->PinType.PinCategory == PC_String)
		{
			TargetFunction = TEXT("Conv_NameToString");
		}
		else if (InputPin->PinType.PinCategory == PC_Text)
		{
			TargetFunction = TEXT("Conv_NameToText");
		}
	}
	else if (OutputPin->PinType.PinCategory == PC_String)
	{
		if (InputPin->PinType.PinCategory == PC_Name)
		{
			TargetFunction = TEXT("Conv_StringToName");
		}
		else if (InputPin->PinType.PinCategory == PC_Int)
		{
			TargetFunction = TEXT("Conv_StringToInt");
		}
		else if (InputPin->PinType.PinCategory == PC_Float)
		{
			TargetFunction = TEXT("Conv_StringToFloat");
		}
		else if (InputPin->PinType.PinCategory == PC_Text)
		{
			TargetFunction = TEXT("Conv_StringToText");
		}
		else
		{
			// NOTE: Did you see the note above about unsafe and unclear casts?
		}
	}
	else if (OutputPin->PinType.PinCategory == PC_Text)
	{
		if(InputPin->PinType.PinCategory == PC_String)
		{
			TargetFunction = TEXT("Conv_TextToString");
		}
	}

	return TargetFunction != NAME_None;
}

bool UEdGraphSchema_K2::FindSpecializedConversionNode(const UEdGraphPin* OutputPin, const UEdGraphPin* InputPin, bool bCreateNode, /*out*/ UK2Node*& TargetNode) const
{
	bool bCanConvert = false;
	TargetNode = NULL;

	// Conversion for scalar -> array
	if( (!OutputPin->PinType.bIsArray && InputPin->PinType.bIsArray) && ArePinTypesCompatible(OutputPin->PinType, InputPin->PinType, NULL, true) )
	{
		bCanConvert = true;
		if(bCreateNode)
		{
			TargetNode = NewObject<UK2Node_MakeArray>();
		}
	}
	// If connecting an object to a 'call function' self pin, and not currently compatible, see if there is a property we can call a function on
	else if( InputPin->GetOwningNode()->IsA(UK2Node_CallFunction::StaticClass()) && IsSelfPin(*InputPin) && (OutputPin->PinType.PinCategory == PC_Object) )
	{
		UK2Node_CallFunction* CallFunctionNode = (UK2Node_CallFunction*)(InputPin->GetOwningNode());
		UClass* OutputPinClass = Cast<UClass>(OutputPin->PinType.PinSubCategoryObject.Get());

		UClass* FunctionClass = CallFunctionNode->FunctionReference.GetMemberParentClass(CallFunctionNode);
		if(FunctionClass != NULL && OutputPinClass != NULL)
		{
			// Iterate over object properties..
			for (TFieldIterator<UObjectProperty> PropIt(OutputPinClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				UObjectProperty* ObjProp = *PropIt;
				// .. if we have a blueprint visible var, and is of the type which contains this function..
				if(ObjProp->HasAllPropertyFlags(CPF_BlueprintVisible) && ObjProp->PropertyClass->IsChildOf(FunctionClass))
				{
					// say we can convert
					bCanConvert = true;
					// Create 'get variable' node
					if(bCreateNode)
					{
						UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>();
						GetNode->VariableReference.SetFromField<UProperty>(ObjProp, false);
						TargetNode = GetNode;
					}
				}
			}

		}	
	}

	if(!bCanConvert)
	{
		// CHECK ENUM TO NAME CAST
		const bool bInoputMatch = InputPin && !InputPin->PinType.bIsArray && ((PC_Name == InputPin->PinType.PinCategory) || (PC_String == InputPin->PinType.PinCategory));
		const bool bOutputMatch = OutputPin && !OutputPin->PinType.bIsArray && (PC_Byte == OutputPin->PinType.PinCategory) && (NULL != Cast<UEnum>(OutputPin->PinType.PinSubCategoryObject.Get()));
		if(bOutputMatch && bInoputMatch)
		{
			bCanConvert = true;
			if(bCreateNode)
			{
				check(NULL == TargetNode);
				if(PC_Name == InputPin->PinType.PinCategory)
				{
					TargetNode = NewObject<UK2Node_GetEnumeratorName>();
				}
				else if(PC_String == InputPin->PinType.PinCategory)
				{
					TargetNode = NewObject<UK2Node_GetEnumeratorNameAsString>();
				}
			}
		}
	}

	if (!bCanConvert && InputPin && OutputPin)
	{
		// CHECK BYTE TO ENUM CAST
		UEnum* Enum = Cast<UEnum>(InputPin->PinType.PinSubCategoryObject.Get());
		const bool bInoputMatch = !InputPin->PinType.bIsArray && (PC_Byte == InputPin->PinType.PinCategory) && Enum;
		const bool bOutputMatch = !OutputPin->PinType.bIsArray && (PC_Byte == OutputPin->PinType.PinCategory);
		if(bOutputMatch && bInoputMatch)
		{
			bCanConvert = true;
			if(bCreateNode)
			{
				auto CastByteToEnum = NewObject<UK2Node_CastByteToEnum>();
				CastByteToEnum->Enum = Enum;
				CastByteToEnum->bSafe = true;
				TargetNode = CastByteToEnum;
			}
		}
	}

	return bCanConvert;
}

void UEdGraphSchema_K2::AutowireConversionNode(UEdGraphPin* InputPin, UEdGraphPin* OutputPin, UEdGraphNode* ConversionNode) const
{
	bool bAllowInputConnections = true;
	bool bAllowOutputConnections = true;

	for (int32 PinIndex = 0; PinIndex < ConversionNode->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* TestPin = ConversionNode->Pins[PinIndex];

		if ((TestPin->Direction == EGPD_Input) && (ArePinTypesCompatible(OutputPin->PinType, TestPin->PinType)))
		{
			if(bAllowOutputConnections && TryCreateConnection(TestPin, OutputPin))
			{
				// Successful connection, do not allow more output connections
				bAllowOutputConnections = false;
			}
		}
		else if ((TestPin->Direction == EGPD_Output) && (ArePinTypesCompatible(TestPin->PinType, InputPin->PinType)))
		{
			if(bAllowInputConnections && TryCreateConnection(TestPin, InputPin))
			{
				// Successful connection, do not allow more input connections
				bAllowInputConnections = false;
			}
		}
	}
}

bool UEdGraphSchema_K2::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	// Determine which pin is an input and which pin is an output
	UEdGraphPin* InputPin = NULL;
	UEdGraphPin* OutputPin = NULL;
	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return false;
	}

	FName TargetFunctionName;
	TSubclassOf<UK2Node> ConversionNodeClass;

	UK2Node* TemplateConversionNode = NULL;

	if (SearchForAutocastFunction(OutputPin, InputPin, /*out*/ TargetFunctionName))
	{
		// Create a new call function node for the casting operator
		UClass* ClassContainingConversionFunction = NULL; //@TODO: Should probably return this from the search function too

		UK2Node_CallFunction* TemplateNode = NewObject<UK2Node_CallFunction>();
		TemplateNode->FunctionReference.SetExternalMember(TargetFunctionName, ClassContainingConversionFunction);
		//TemplateNode->bIsBeadFunction = true;

		TemplateConversionNode = TemplateNode;
	}
	else
	{
		FindSpecializedConversionNode(OutputPin, InputPin, true, /*out*/ TemplateConversionNode);
	}

	if (TemplateConversionNode != NULL)
	{
		// Determine where to position the new node (assuming it isn't going to get beaded)
		FVector2D AverageLocation = CalculateAveragePositionBetweenNodes(InputPin, OutputPin);

		UK2Node* ConversionNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node>(InputPin->GetOwningNode()->GetGraph(), TemplateConversionNode, AverageLocation);

		// Connect the cast node up to the output/input pins
		AutowireConversionNode(InputPin, OutputPin, ConversionNode);

		return true;
	}

	return false;
}

FString UEdGraphSchema_K2::IsPinDefaultValid(const UEdGraphPin* Pin, const FString& NewDefaultValue, UObject* NewDefaultObject, const FText& InNewDefaultText) const
{
	const UBlueprint* OwningBP = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Pin->GetOwningNode());
	const bool bIsArray = Pin->PinType.bIsArray;
	const bool bIsReference = Pin->PinType.bIsReference;
	const bool bIsAutoCreateRefTerm = IsAutoCreateRefTerm(Pin);

	if (OwningBP->BlueprintType != BPTYPE_Interface)
	{
		if( !bIsAutoCreateRefTerm )
		{
			if( bIsArray )
			{
				return TEXT("Literal values are not allowed for array parameters.  Use a Make Array node instead");
			}
			else if( bIsReference )
			{
				return TEXT("Literal values are not allowed for pass-by-reference parameters.");
			}
		}
	}

	FString ReturnMsg;
	DefaultValueSimpleValidation(Pin->PinType, Pin->PinName, NewDefaultValue, NewDefaultObject, InNewDefaultText, &ReturnMsg);
	return ReturnMsg;
}

bool UEdGraphSchema_K2::DoesSupportPinWatching() const
{
	return true;
}

bool UEdGraphSchema_K2::IsPinBeingWatched(UEdGraphPin const* Pin) const
{
	// Note: If you crash here; it is likely that you forgot to call Blueprint->OnBlueprintChanged.Broadcast(Blueprint) to invalidate the cached UI state
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Pin->GetOwningNode());
	return FKismetDebugUtilities::IsPinBeingWatched(Blueprint, Pin);
}

void UEdGraphSchema_K2::ClearPinWatch(UEdGraphPin const* Pin) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Pin->GetOwningNode());
	FKismetDebugUtilities::RemovePinWatch(Blueprint, Pin);
}

bool UEdGraphSchema_K2::DefaultValueSimpleValidation(const FEdGraphPinType& PinType, const FString& PinName, const FString& NewDefaultValue, UObject* NewDefaultObject, const FText& InNewDefaultText, FString* OutMsg /*= NULL*/) const
{
#ifdef DVSV_RETURN_MSG
	checkAtCompileTime(false, DVSV_RETURN_MSG_macro_redefined);
#endif
#define DVSV_RETURN_MSG(Str) if(NULL != OutMsg) { *OutMsg = Str; } return false;

	const bool bIsArray = PinType.bIsArray;
	const bool bIsReference = PinType.bIsReference;
	const FString& PinCategory = PinType.PinCategory;
	const FString& PinSubCategory = PinType.PinSubCategory;
	const UObject* PinSubCategoryObject = PinType.PinSubCategoryObject.Get();

	//@TODO: FCString::Atoi, FCString::Atof, and appStringToBool will 'accept' any input, but we should probably catch and warn
	// about invalid input (non numeric for int/byte/float, and non 0/1 or yes/no/true/false for bool)

	if (PinCategory == PC_Boolean)
	{
		// All input is acceptable to some degree
	}
	else if (PinCategory == PC_Byte)
	{
		const UEnum* EnumPtr = Cast<const UEnum>(PinSubCategoryObject);
		if (EnumPtr)
		{
			if (EnumPtr->FindEnumIndex(*NewDefaultValue) == INDEX_NONE)
			{
				DVSV_RETURN_MSG( FString::Printf( TEXT("'%s' is not a valid enumerant of '<%s>'"), *NewDefaultValue, *(EnumPtr->GetName( )) ) );
			}
		}
		else if( !NewDefaultValue.IsEmpty() )
		{
			int32 Value;
			if (!FDefaultValueHelper::ParseInt(NewDefaultValue, Value))
			{
				DVSV_RETURN_MSG( TEXT("Expected a valid unsigned number for a byte property") );
			}
			if ((Value < 0) || (Value > 255))
			{
				DVSV_RETURN_MSG( TEXT("Expected a value between 0 and 255 for a byte property") );
			}
		}
	}
	else if (PinCategory == PC_Class)
	{
		// Should have an object set but no string
		if(!NewDefaultValue.IsEmpty())
		{
			DVSV_RETURN_MSG( FString::Printf(TEXT("String NewDefaultValue '%s' specified on class pin '%s'"), *NewDefaultValue, *(PinName)) );
		}

		if (NewDefaultObject == NULL)
		{
			// Valid self-reference or empty reference
		}
		else
		{
			// Otherwise, we expect to be able to resolve the type at least
			const UClass* DefaultClassType = Cast<const UClass>(NewDefaultObject);
			if (DefaultClassType == NULL)
			{
				DVSV_RETURN_MSG( FString::Printf(TEXT("Literal on pin %s is not a class."), *(PinName)) );
			}
			else
			{
				// @TODO support PinSubCategory == 'self'
				const UClass* PinClassType = Cast<const UClass>(PinSubCategoryObject);
				if (PinClassType == NULL)
				{
					DVSV_RETURN_MSG( FString::Printf(TEXT("Failed to find class for pin %s"), *(PinName)) );
				}
				else
				{
					// Have both types, make sure the specified type is a valid subtype
					if (!DefaultClassType->IsChildOf(PinClassType))
					{
						DVSV_RETURN_MSG( FString::Printf(TEXT("%s isn't a valid subclass of %s (specified on pin %s)"), *NewDefaultObject->GetPathName(), *PinClassType->GetName(), *(PinName)) );
					}
				}
			}
		}
	}
	else if (PinCategory == PC_Float)
	{
		if(!NewDefaultValue.IsEmpty())
		{
			if (!FDefaultValueHelper::IsStringValidFloat(NewDefaultValue))
			{
				DVSV_RETURN_MSG( TEXT("Expected a valid number for an float property") );
			}
		}
	}
	else if (PinCategory == PC_Int)
	{
		if(!NewDefaultValue.IsEmpty())
		{
			if (!FDefaultValueHelper::IsStringValidInteger(NewDefaultValue))
			{
				DVSV_RETURN_MSG( TEXT("Expected a valid number for an integer property") );
			}
		}
	}
	else if (PinCategory == PC_Name)
	{
		if( NewDefaultValue.IsNumeric() )
		{
			DVSV_RETURN_MSG( FString::Printf(TEXT("Invalid default name for pin %s"), *(PinName)) ); 
		}
	}
	else if (PinCategory == PC_Object)
	{
		if(PinSubCategoryObject == NULL && PinSubCategory != PSC_Self)
		{
			DVSV_RETURN_MSG( FString::Printf(TEXT("PinSubCategoryObject on pin '%s' is NULL and PinSubCategory is '%s' not 'self'"), *(PinName), *PinSubCategory) );
		}

		if(PinSubCategoryObject != NULL && PinSubCategory != TEXT(""))
		{
			DVSV_RETURN_MSG( FString::Printf(TEXT("PinSubCategoryObject on pin '%s' is non-NULL but PinSubCategory is '%s', should be empty"), *(PinName), *PinSubCategory) );
		}

		// Should have an object set but no string - 'self' is not a valid NewDefaultValue for PC_Object pins
		if(!NewDefaultValue.IsEmpty())
		{
			DVSV_RETURN_MSG( FString::Printf(TEXT("String NewDefaultValue '%s' specified on object pin '%s'"), *NewDefaultValue, *(PinName)) );
		}

		// Check that the object that is set is of the correct class
		const UClass* ObjectClass = Cast<const UClass>(PinSubCategoryObject);
		if(NewDefaultObject != NULL && ObjectClass != NULL && !NewDefaultObject->IsA(ObjectClass))
		{
			DVSV_RETURN_MSG( FString::Printf(TEXT("%s isn't a %s (specified on pin %s)"), *NewDefaultObject->GetPathName(), *ObjectClass->GetName(), *(PinName)) );		
		}
	}
	else if (PinCategory == PC_String)
	{
		// All strings are valid
	}
	else if (PinCategory == PC_Text)
	{
		// Neither of these should ever be true 
		if( InNewDefaultText.IsTransient() )
		{
			DVSV_RETURN_MSG( TEXT("Invalid text literal, text is transient!") );
		}

		if( InNewDefaultText.IsCultureInvariant() )
		{
			DVSV_RETURN_MSG( TEXT("Invalid text literal, text is culture invariant!") );
		}
	}
	else if (PinCategory == PC_Struct)
	{
		if(PinSubCategory != TEXT(""))
		{
			DVSV_RETURN_MSG( FString::Printf(TEXT("PinSubCategory on pin '%s' is '%s', should be empty"), *(PinName), *PinSubCategory) );
		}

		// Only FRotator and FVector properties are currently allowed to have a valid default value
		const UScriptStruct* StructType = Cast<const UScriptStruct>(PinSubCategoryObject);
		if (StructType == NULL)
		{
			//@TODO: MessageLog.Error(*FString::Printf(TEXT("Failed to find struct named %s (passed thru @@)"), *PinSubCategory), SourceObject);
			DVSV_RETURN_MSG( FString::Printf(TEXT("No struct specified for pin '%s'"), *(PinName)) );
		}
		else if(!NewDefaultValue.IsEmpty())
		{
			UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
			UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
			UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
			UScriptStruct* LinearColorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("LinearColor"));

			if (StructType == VectorStruct)
			{
				if (!FDefaultValueHelper::IsStringValidVector(NewDefaultValue))
				{
					DVSV_RETURN_MSG( TEXT("Invalid value for an FVector") );
				}
			}
			else if (StructType == RotatorStruct)
			{
				FRotator Rot;
				if (!FDefaultValueHelper::IsStringValidRotator(NewDefaultValue))
				{
					DVSV_RETURN_MSG( TEXT("Invalid value for an FRotator") );
				}
			}
			else if (StructType == TransformStruct)
			{
				FTransform Transform;
				if ( !Transform.InitFromString(NewDefaultValue))
				{
					DVSV_RETURN_MSG( TEXT("Invalid value for an FTransform") );
				}
			}
			else if (StructType == LinearColorStruct)
			{
				FLinearColor Color;
				// Color form: "(R=%f,G=%f,B=%f,A=%f)"
				if (!Color.InitFromString(NewDefaultValue))
				{
					DVSV_RETURN_MSG( TEXT("Invalid value for an FLinearColor") );
				}
			}
			else
			{
				// Structs must pass validation at this point, because we need a UStructProperty to run ImportText
				// They'll be verified in FKCHandler_CallFunction::CreateFunctionCallStatement()
			}
		}
	}
	else if (PinCategory == TEXT("CommentType"))
	{
		// Anything is allowed
	}
	else
	{
		//@TODO: MessageLog.Error(*FString::Printf(TEXT("Unsupported type %s on @@"), *UEdGraphSchema_K2::TypeToString(Type)), SourceObject);
		DVSV_RETURN_MSG( FString::Printf(TEXT("Unsupported type %s on pin %s"), *UEdGraphSchema_K2::TypeToString(PinType), *(PinName)) ); 
	}

#undef DVSV_RETURN_MSG

	return true;
}

FLinearColor UEdGraphSchema_K2::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	const FString& TypeString = PinType.PinCategory;
	const UEditorUserSettings& Options = GEditor->AccessEditorUserSettings();

	if (TypeString == PC_Exec)
	{
		return Options.ExecutionPinTypeColor;
	}
	else if (TypeString == PC_Object)
	{
		return Options.ObjectPinTypeColor;
	}
	else if (TypeString == PC_Float)
	{
		return Options.FloatPinTypeColor;
	}
	else if (TypeString == PC_Boolean)
	{
		return Options.BooleanPinTypeColor;
	}
	else if (TypeString == PC_Byte)
	{
		return Options.BytePinTypeColor;
	}
	else if (TypeString == PC_Int)
	{
		return Options.IntPinTypeColor;
	}
	else if (TypeString == PC_Struct)
	{
		static UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
		static UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
		static UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));

		if (PinType.PinSubCategoryObject == VectorStruct)
		{
			// vector
			return Options.VectorPinTypeColor;
		}
		else if (PinType.PinSubCategoryObject == RotatorStruct)
		{
			// rotator
			return Options.RotatorPinTypeColor;
		}
		else if (PinType.PinSubCategoryObject == TransformStruct)
		{
			// transform
			return Options.TransformPinTypeColor;
		}
		else
		{
			return Options.StructPinTypeColor;
		}
	}
	else if (TypeString == PC_String)
	{
		return Options.StringPinTypeColor;
	}
	else if (TypeString == PC_Text)
	{
		return Options.TextPinTypeColor;
	}
	else if (TypeString == PC_Wildcard)
	{
		if (PinType.PinSubCategory == PSC_Index)
		{
			return Options.IndexPinTypeColor;
		}
		else
		{
			return Options.WildcardPinTypeColor;
		}
	}
	else if (TypeString == PC_Name)
	{
		return Options.NamePinTypeColor;
	}
	else if (TypeString == PC_Delegate)
	{
		return Options.DelegatePinTypeColor;
	}
	else if (TypeString == PC_Class)
	{
		return Options.ClassPinTypeColor;
	}

	// Type does not have a defined color!
	return Options.DefaultPinTypeColor;
}

FString UEdGraphSchema_K2::GetPinDisplayName(const UEdGraphPin* Pin) const 
{
	FString DisplayName;

	if (Pin != NULL)
	{
		UEdGraphNode* Node = Pin->GetOwningNode();
		if (Node->ShouldOverridePinNames())
		{
			DisplayName = Node->GetPinNameOverride(*Pin);
		}
		else
		{
			DisplayName = Super::GetPinDisplayName(Pin);
	
			// bit of a hack to hide 'execute' and 'then' pin names
			if ((Pin->PinType.PinCategory == PC_Exec) && 
				((DisplayName == PN_Execute) || (DisplayName == PN_Then)))
			{
				DisplayName = FString(TEXT(""));
			}
		}

		if( GEditor && GetDefault<UEditorStyleSettings>()->bShowFriendlyNames )
		{
			DisplayName = EngineUtils::SanitizeDisplayName(DisplayName, Pin->PinType.PinCategory == PC_Boolean);
		}
	}
	return DisplayName;
}

void UEdGraphSchema_K2::ConstructBasicPinTooltip(const UEdGraphPin& Pin, const FString& PinDescription, FString& TooltipOut) const
{
	// using a local FString so users can use the same variable for PinDescription and TooltipOut
	FString ConstructedTooltip = UEdGraphSchema_K2::TypeToString(Pin.PinType);

	if (UEdGraphNode* PinNode = Pin.GetOwningNode())
	{
		UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(PinNode->GetSchema());
		if (ensure(K2Schema != NULL)) // ensure that this node belongs to this schema
		{
			ConstructedTooltip += TEXT(" ");
			ConstructedTooltip += K2Schema->GetPinDisplayName(&Pin);
		}
	}	

	ConstructedTooltip += FString(TEXT("\n")) + PinDescription;
	TooltipOut = ConstructedTooltip; // using a local FString, so PinDescription and TooltipOut can be the same variable
}

EGraphType UEdGraphSchema_K2::GetGraphType(const UEdGraph* TestEdGraph) const
{
	if (TestEdGraph)
	{
		//@TODO: Should there be a GT_Subgraph type?	
		UEdGraph* GraphToTest = const_cast<UEdGraph*>(TestEdGraph);

		for (UObject* TestOuter = GraphToTest; TestOuter; TestOuter = TestOuter->GetOuter())
		{
			// reached up to the blueprint for the graph
			if (UBlueprint* Blueprint = Cast<UBlueprint>(TestOuter))
			{
				if (Blueprint->BlueprintType == BPTYPE_MacroLibrary ||
					Blueprint->MacroGraphs.Contains(GraphToTest))
				{
					return GT_Macro;
				}
				else if (Blueprint->UbergraphPages.Contains(GraphToTest))
				{
					return GT_Ubergraph;
				}
				else if (Blueprint->FunctionGraphs.Contains(GraphToTest))
				{
					return GT_Function; 
				}
			}
			else
			{
				GraphToTest = Cast<UEdGraph>(TestOuter);
			}
		}
	}
	
	return Super::GetGraphType(TestEdGraph);
}

bool UEdGraphSchema_K2::IsTitleBarPin(const UEdGraphPin& Pin) const
{
	return IsExecPin(Pin);
}

void UEdGraphSchema_K2::CreateMacroGraphTerminators(UEdGraph& Graph, UClass* Class) const
{
	const FName GraphName = Graph.GetFName();

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(&Graph);
	
	// Create the entry/exit tunnels
	{
		FGraphNodeCreator<UK2Node_Tunnel> EntryNodeCreator(Graph);
		UK2Node_Tunnel* EntryNode = EntryNodeCreator.CreateNode();
		EntryNode->bCanHaveOutputs = true;
		EntryNodeCreator.Finalize();
	}

	{
		FGraphNodeCreator<UK2Node_Tunnel> ExitNodeCreator(Graph);
		UK2Node_Tunnel* ExitNode = ExitNodeCreator.CreateNode();
		ExitNode->bCanHaveInputs = true;
		ExitNode->NodePosX = 240;
		ExitNodeCreator.Finalize();
	}
}
		
void UEdGraphSchema_K2::CreateFunctionGraphTerminators(UEdGraph& Graph, UClass* Class) const
{
	const FName GraphName = Graph.GetFName();

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(&Graph);
	check(Blueprint->BlueprintType != BPTYPE_MacroLibrary);

	// Create a function entry node
	FGraphNodeCreator<UK2Node_FunctionEntry> FunctionEntryCreator(Graph);
	UK2Node_FunctionEntry* EntryNode = FunctionEntryCreator.CreateNode();
	EntryNode->SignatureClass = Class;
	EntryNode->SignatureName = GraphName;
	FunctionEntryCreator.Finalize();

	// See if we need to implement a return node
	UFunction* InterfaceToImplement = FindField<UFunction>(Class, GraphName);
	if (InterfaceToImplement)
	{
		// See if any function params are marked as out
		bool bHasOutParam =  false;
		for( TFieldIterator<UProperty> It(InterfaceToImplement); It && (It->PropertyFlags & CPF_Parm); ++It )
		{
			if( It->PropertyFlags & CPF_OutParm )
			{
				bHasOutParam = true;
				break;
			}
		}

		if( bHasOutParam )
		{
			FGraphNodeCreator<UK2Node_FunctionResult> NodeCreator(Graph);
			UK2Node_FunctionResult* ReturnNode = NodeCreator.CreateNode();
			ReturnNode->SignatureClass = Class;
			ReturnNode->SignatureName = GraphName;
			ReturnNode->NodePosX = EntryNode->NodePosX + EntryNode->NodeWidth + 256;
			ReturnNode->NodePosY = EntryNode->NodePosY;
			NodeCreator.Finalize();

			// Auto-connect the pins for entry and exit, so that by default the signature is properly generated
			UEdGraphPin* EntryNodeExec = FindExecutionPin(*EntryNode, EGPD_Output);
			UEdGraphPin* ResultNodeExec = FindExecutionPin(*ReturnNode, EGPD_Input);
			EntryNodeExec->MakeLinkTo(ResultNodeExec);

		}
	}
}

bool UEdGraphSchema_K2::ConvertPropertyToPinType(const UProperty* Property, /*out*/ FEdGraphPinType& TypeOut) const
{
	if (Property == NULL)
	{
		TypeOut.PinCategory = TEXT("bad_type");
		return false;
	}

	TypeOut.PinSubCategory = TEXT("");
	
	// Handle whether or not this is an array property
	const UArrayProperty* ArrayProperty = Cast<const UArrayProperty>(Property);
	const UProperty* TestProperty = ArrayProperty ? ArrayProperty->Inner : Property;
	TypeOut.bIsArray = (ArrayProperty != NULL);
	TypeOut.bIsReference = Property->HasAllPropertyFlags(CPF_OutParm|CPF_ReferenceParm);
	TypeOut.bIsConst     = Property->HasAllPropertyFlags(CPF_ConstParm);

	// Check to see if this is the wildcard property for the target array type
	UFunction* Function = Cast<UFunction>(Property->GetOuter());

	if( UK2Node_CallArrayFunction::IsWildcardProperty(Function, Property))
	{
		TypeOut.PinCategory = PC_Wildcard;
	}
	else if (const UInterfaceProperty* InterfaceProperty = Cast<const UInterfaceProperty>(TestProperty))
	{
		TypeOut.PinCategory = PC_Object;
		TypeOut.PinSubCategoryObject = InterfaceProperty->InterfaceClass;
	}
	else if (const UClassProperty* ClassProperty = Cast<const UClassProperty>(TestProperty))
	{
		TypeOut.PinCategory = PC_Class;
		TypeOut.PinSubCategoryObject = ClassProperty->MetaClass;
	}
	else if (const UAssetClassProperty* AssetClassProperty = Cast<const UAssetClassProperty>(TestProperty))
	{
		TypeOut.PinCategory = PC_Class;
		TypeOut.PinSubCategoryObject = AssetClassProperty->MetaClass;
	}
	else if (const UObjectPropertyBase* ObjectProperty = Cast<const UObjectPropertyBase>(TestProperty))
	{
		TypeOut.PinCategory = PC_Object;
		TypeOut.PinSubCategoryObject = ObjectProperty->PropertyClass;
		TypeOut.bIsWeakPointer = TestProperty->IsA(UWeakObjectProperty::StaticClass());
	}
	else if (const UStructProperty* StructProperty = Cast<const UStructProperty>(TestProperty))
	{
		TypeOut.PinCategory = PC_Struct;
		TypeOut.PinSubCategoryObject = StructProperty->Struct;
	}
	else if (Cast<const UFloatProperty>(TestProperty) != NULL)
	{
		TypeOut.PinCategory = PC_Float;
	}
	else if (Cast<const UIntProperty>(TestProperty) != NULL)
	{
		TypeOut.PinCategory = PC_Int;
	}
	else if (const UByteProperty* ByteProperty = Cast<const UByteProperty>(TestProperty))
	{
		TypeOut.PinCategory = PC_Byte;
		TypeOut.PinSubCategoryObject = ByteProperty->Enum;
	}
	else if (Cast<const UNameProperty>(TestProperty) != NULL)
	{
		TypeOut.PinCategory = PC_Name;
	}
	else if (Cast<const UBoolProperty>(TestProperty) != NULL)
	{
		TypeOut.PinCategory = PC_Boolean;
	}
	else if (Cast<const UStrProperty>(TestProperty) != NULL)
	{
		TypeOut.PinCategory = PC_String;
	}
	else if (Cast<const UTextProperty>(TestProperty) != NULL)
	{
		TypeOut.PinCategory = PC_Text;
	}
	else if (const UMulticastDelegateProperty* MulticastDelegateProperty = Cast<const UMulticastDelegateProperty>(TestProperty))
	{
		TypeOut.PinCategory = PC_MCDelegate;
		TypeOut.PinSubCategoryObject = MulticastDelegateProperty->SignatureFunction;
	}
	else if (const UDelegateProperty* DelegateProperty = Cast<const UDelegateProperty>(TestProperty))
	{
		TypeOut.PinCategory = PC_Delegate;
		TypeOut.PinSubCategoryObject = DelegateProperty->SignatureFunction;
	}
	else
	{
		TypeOut.PinCategory = TEXT("bad_type");
		return false;
	}

	return true;
}

FString UEdGraphSchema_K2::TypeToString(const FEdGraphPinType& Type)
{
	FString PropertyString;

	if (Type.PinSubCategoryObject != NULL)
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		if (Type.PinCategory == Schema->PC_Byte)
		{
			PropertyString = FString::Printf(TEXT("enum'%s'"), *Type.PinSubCategoryObject->GetName());
		}
		else
		{
			if( !Type.bIsWeakPointer )
			{
				UClass* PSCOAsClass = Cast<UClass>(Type.PinSubCategoryObject.Get());
				const bool bIsInterface = PSCOAsClass && PSCOAsClass->HasAnyClassFlags(CLASS_Interface);

				FString CategoryDesc = !bIsInterface ? (*Type.PinCategory) : TEXT("interface");
				PropertyString = FString::Printf(TEXT("%s'%s'"), *CategoryDesc, *Type.PinSubCategoryObject.Get()->GetName());
			}
			else
			{
				PropertyString = FString::Printf(TEXT("weak_ptr_%s'%s'"), *Type.PinCategory, *Type.PinSubCategoryObject->GetName());
			}
		}
	}
	else if (Type.PinSubCategory != TEXT(""))
	{
		PropertyString = FString::Printf(TEXT("%s'%s'"), *Type.PinCategory, *Type.PinSubCategory);
	}
	else
	{
		PropertyString = Type.PinCategory;
	}

	if (Type.bIsArray)
	{
		PropertyString = FString::Printf(TEXT("array[%s]"), *PropertyString);
	}
	else if (Type.bIsReference)
	{
		PropertyString += TEXT(" (by ref)"); 
	}

	return PropertyString;
}

FString UEdGraphSchema_K2::TypeToString(UProperty* const Property)
{
	if (UStructProperty* Struct = Cast<UStructProperty>(Property))
	{
		return FString::Printf(TEXT("struct'%s'"), *Struct->Struct->GetName());
	}
	else if (UClassProperty* Class = Cast<UClassProperty>(Property))
	{
		return FString::Printf(TEXT("class'%s'"), *Class->MetaClass->GetName());
	}
	else if (UInterfaceProperty* Interface = Cast<UInterfaceProperty>(Property))
	{
		return FString::Printf(TEXT("interface'%s'"), *Interface->InterfaceClass->GetName());
	}
	else if (UObjectPropertyBase* Obj = Cast<UObjectPropertyBase>(Property))
	{
		if( Obj->PropertyClass )
		{
			if( Property->IsA(UWeakObjectProperty::StaticClass()) )
			{
				return FString::Printf(TEXT("weak_ptr_object'%s'"), *Obj->PropertyClass->GetName());
			}
			else
			{
				return FString::Printf(TEXT("object'%s'"), *Obj->PropertyClass->GetName());
			}
		}

		return TEXT("");
	}
	else if (UArrayProperty* Array = Cast<UArrayProperty>(Property))
	{
		return FString::Printf(TEXT("array[%s]"), *TypeToString(Array->Inner)); 
	}
	else
	{
		return Property->GetClass()->GetName();
	}
}

void UEdGraphSchema_K2::GetVariableTypeTree( TArray< TSharedPtr<FPinTypeTreeInfo> >& TypeTree, bool bAllowExec, bool bAllowWildCard ) const
{
	// Clear the list
	TypeTree.Empty();

	if( bAllowExec )
	{
		TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Exec, this, LOCTEXT("ExecType", "Execution pin").ToString()) ) );
	}

	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Boolean, this, LOCTEXT("BooleanType", "True or false value").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Byte, this, LOCTEXT("ByteType", "8 bit number").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Int, this, LOCTEXT("IntegerType", "Integer number").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Float, this, LOCTEXT("FloatType", "Floating point number").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Name, this, LOCTEXT("NameType", "A text name").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_String, this, LOCTEXT("StringType", "A text string").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Text, this, LOCTEXT("TextType", "A localizable text string").ToString()) ) );

	// Add in special first-class struct types
	UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
	UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
	UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
 	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Struct, VectorStruct, LOCTEXT("VectorType", "A 3D vector").ToString()) ) );
 	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Struct, RotatorStruct, LOCTEXT("RotatorType", "A 3D rotation").ToString()) ) );
 	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Struct, TransformStruct, LOCTEXT("TransformType", "A 3D transformation, including translation, rotation and 3D scale.").ToString()) ) );
	
	// Add wildcard type
	if (bAllowWildCard)
	{
		TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Wildcard, this, LOCTEXT("WildcardType", "Wildcard type (unspecified).").ToString()) ) );
	}

	// Add the types that have subtrees
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Struct, this, LOCTEXT("StructType", "Struct (value) types.").ToString(), true) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Object, this, LOCTEXT("ObjectType", "Object pointer.").ToString(), true) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Class, this, LOCTEXT("ClassType", "Class pointers.").ToString(), true) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(TEXT("Enum"), PC_Byte, this, LOCTEXT("EnumType", "Enumeration types.").ToString(), true) ) );


}

void UEdGraphSchema_K2::GetVariableIndexTypeTree( TArray< TSharedPtr<FPinTypeTreeInfo> >& TypeTree, bool bAllowExec, bool bAllowWildcard ) const
{
	// Clear the list
	TypeTree.Empty();

	if( bAllowExec )
	{
		TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Exec, this, LOCTEXT("ExecIndexType", "Execution pin").ToString()) ) );
	}

	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Boolean, this, LOCTEXT("BooleanIndexType", "True or false value").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Byte, this, LOCTEXT("ByteIndexType", "8 bit number").ToString()) ) );
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Int, this, LOCTEXT("IntegerIndexType", "Integer number").ToString()) ) );

	// Add wildcard type
	if (bAllowWildcard)
	{
		TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(PC_Wildcard, this, LOCTEXT("WildcardIndexType", "Wildcard type (unspecified).").ToString()) ) );
	}

	// Add the types that have subtrees
	TypeTree.Add( MakeShareable( new FPinTypeTreeInfo(TEXT("Enum"), PC_Byte, this, LOCTEXT("EnumIndexType", "Enumeration types.").ToString(), true) ) );
}

bool UEdGraphSchema_K2::DoesTypeHaveSubtypes(const FString& FriendlyTypeName) const
{
	return (FriendlyTypeName == PC_Struct) || (FriendlyTypeName == PC_Object) || (FriendlyTypeName == PC_Class) || (FriendlyTypeName == TEXT("Enum"));
}

void UEdGraphSchema_K2::GetVariableSubtypes(const FString& Type, TArray<UObject*>& SubtypesList) const
{
	SubtypesList.Empty();

	if (Type == PC_Struct)
	{
		// Find script structs marked with "BlueprintType=true" in their metadata, and add to the list
		for (TObjectIterator<UScriptStruct> StructIt; StructIt; ++StructIt)
		{
			UScriptStruct* ScriptStruct = *StructIt;
			if (UEdGraphSchema_K2::IsAllowableBlueprintVariableType(ScriptStruct))
			{
				SubtypesList.Add(ScriptStruct);
			}
		}
	}
	else if ((Type == PC_Object) || (Type == PC_Class))
	{
		// Generate a list of all potential objects which have "BlueprintType=true" in their metadata
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* CurrentClass = *ClassIt;
			if (UEdGraphSchema_K2::IsAllowableBlueprintVariableType(CurrentClass))
			{
				SubtypesList.Add(CurrentClass);
			}
		}
	}
	else if (Type == TEXT("Enum"))
	{
		// Generate a list of all potential enums which have "BlueprintType=true" in their metadata
		for (TObjectIterator<UEnum> EnumIt; EnumIt; ++EnumIt)
		{
			UEnum* CurrentEnum = *EnumIt;
			if (UEdGraphSchema_K2::IsAllowableBlueprintVariableType(CurrentEnum))
			{
				SubtypesList.Add(CurrentEnum);
			}
		}
	}

	SubtypesList.Sort();
}

bool UEdGraphSchema_K2::ArePinsCompatible(const UEdGraphPin* PinA, const UEdGraphPin* PinB, const UClass* CallingContext, bool bIgnoreArray /*= false*/) const
{
	if ((PinA->Direction == EGPD_Input) && (PinB->Direction == EGPD_Output))
	{
		return ArePinTypesCompatible(PinB->PinType, PinA->PinType, CallingContext, bIgnoreArray);
	}
	else if ((PinB->Direction == EGPD_Input) && (PinA->Direction == EGPD_Output))
	{
		return ArePinTypesCompatible(PinA->PinType, PinB->PinType, CallingContext, bIgnoreArray);
	}
	else
	{
		return false;
	}
}

bool UEdGraphSchema_K2::ArePinTypesCompatible(const FEdGraphPinType& Output, const FEdGraphPinType& Input, const UClass* CallingContext, bool bIgnoreArray /*= false*/) const
{
	if( !bIgnoreArray && (Output.bIsArray != Input.bIsArray) && (Input.PinCategory != PC_Wildcard || Input.bIsArray) )
	{
		return false;
	}
	else if (Output.PinCategory == Input.PinCategory)
	{
		if ((Output.PinSubCategory == Input.PinSubCategory) && (Output.PinSubCategoryObject == Input.PinSubCategoryObject))
		{
			return true;
		}
		else if ((Output.PinCategory == PC_Object) || (Output.PinCategory == PC_Struct) || (Output.PinCategory == PC_Class))
		{
			// Subcategory mismatch, but the two could be castable
			// Only allow a match if the input is a superclass of the output
			const UStruct* OutputObject = (Output.PinSubCategory == PSC_Self) ? CallingContext : Cast<UStruct>(Output.PinSubCategoryObject.Get());
			const UStruct* InputObject  = (Input.PinSubCategory == PSC_Self) ? CallingContext : Cast<UStruct>(Input.PinSubCategoryObject.Get());

			if ((OutputObject != NULL) && (InputObject != NULL))
			{
				// Special Case:  Cannot mix interface and non-interface calls, because the pointer size is different under the hood
				const bool bInputIsInterface  = InputObject->IsChildOf(UInterface::StaticClass());
				const bool bOutputIsInterface = OutputObject->IsChildOf(UInterface::StaticClass());

				if (bInputIsInterface != bOutputIsInterface) 
				{
					UClass const* OutputClass = Cast<const UClass>(OutputObject);
					UClass const* InputClass  = Cast<const UClass>(InputObject);

					if (bInputIsInterface && (OutputClass != NULL))
					{
						return OutputClass->ImplementsInterface(InputClass);
					}
					else if (bOutputIsInterface && (InputClass != NULL))
					{
						return InputClass->ImplementsInterface(OutputClass);
					}
				}				

				return OutputObject->IsChildOf(InputObject) && (bInputIsInterface == bOutputIsInterface);
			}
		}
		else if ((Output.PinCategory == PC_Byte) && (Output.PinSubCategory == Input.PinSubCategory))
		{
			// NOTE: This allows enums to be converted to bytes.  Long-term we don't want to allow that, but we need it
			// for now until we have == for enums in order to be able to compare them.
			if (Input.PinSubCategoryObject == NULL)
			{
				return true;
			}
		}
		else if (PC_Delegate == Output.PinCategory || PC_MCDelegate == Output.PinCategory)
		{
			const UObject* OutObj = Output.PinSubCategoryObject.Get();
			const UObject* InObj = Input.PinSubCategoryObject.Get();
			if ((NULL == OutObj) || (NULL == InObj))
			{
				return true;
			}
			const UFunction* OutFuction = Cast<const UFunction>(OutObj);
			const UFunction* InFuction = Cast<const UFunction>(InObj);
			check(OutFuction && InFuction);
			return OutFuction && InFuction && OutFuction->IsSignatureCompatibleWith(InFuction);
		}
	}
	else if (Output.PinCategory == PC_Wildcard || Input.PinCategory == PC_Wildcard)
	{
		// If this is an Index Wildcard we have to check compatibility for indexing types
		if (Output.PinSubCategory == PSC_Index)
		{
			return IsIndexWildcardCompatible(Input);
		}
		else if (Input.PinSubCategory == PSC_Index)
		{
			return IsIndexWildcardCompatible(Output);
		}

		return true;
	}

	return false;
}

void UEdGraphSchema_K2::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(&TargetNode);

	Super::BreakNodeLinks(TargetNode);
	
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UEdGraphSchema_K2::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links") );

	// cache this here, as BreakPinLinks can trigger a node reconstruction invalidating the TargetPin referenceS
	UBlueprint* const Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(TargetPin.GetOwningNode());



	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UEdGraphSchema_K2::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link") );

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(TargetPin->GetOwningNode());

	Super::BreakSinglePinLink(SourcePin, TargetPin);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UEdGraphSchema_K2::ReconstructNode(UEdGraphNode& TargetNode, bool bIsBatchRequest/*=false*/) const
{
	Super::ReconstructNode(TargetNode, bIsBatchRequest);

	// If the reconstruction is being handled by something doing a batch (i.e. the blueprint autoregenerating itself), defer marking the blueprint as modified to prevent multiple recompiles
	if (!bIsBatchRequest)
	{
		const UK2Node* K2Node = Cast<UK2Node>(&TargetNode);
		if (K2Node && K2Node->NodeCausesStructuralBlueprintChange())
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(&TargetNode);
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
	}
}

bool UEdGraphSchema_K2::CanEncapuslateNode(UEdGraphNode const& TestNode) const
{
	// Can't encapsulate entry points (may relax this restriction in the future, but it makes sense for now)
	return !TestNode.IsA(UK2Node_FunctionTerminator::StaticClass()) && 
		    TestNode.GetClass() != UK2Node_Tunnel::StaticClass(); //Tunnel nodes getting sucked into collapsed graphs fails badly, want to allow derived types though(composite node/Macroinstances)
}

void UEdGraphSchema_K2::HandleGraphBeingDeleted(UEdGraph& GraphBeingRemoved) const
{
	if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(&GraphBeingRemoved))
	{
		// Look for collapsed graph nodes that reference this graph
		TArray<UK2Node_Composite*> CompositeNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Composite>(Blueprint, /*out*/ CompositeNodes);

		TSet<UK2Node_Composite*> NodesToDelete;
		for (int32 i = 0; i < CompositeNodes.Num(); ++i)
		{
			UK2Node_Composite* CompositeNode = CompositeNodes[i];
			if (CompositeNode->BoundGraph == &GraphBeingRemoved)
			{
				NodesToDelete.Add(CompositeNode);
			}
		}

		// Delete the node that owns us
		ensure(NodesToDelete.Num() <= 1);
		for (TSet<UK2Node_Composite*>::TIterator It(NodesToDelete); It; ++It)
		{
			UK2Node_Composite* NodeToDelete = *It;

			// Prevent re-entrancy here
			NodeToDelete->BoundGraph = NULL;

			NodeToDelete->Modify();
			NodeToDelete->DestroyNode();
		}
	}
}

void UEdGraphSchema_K2::TrySetDefaultValue(UEdGraphPin& Pin, const FString& NewDefaultValue) const
{
	FString UseDefaultValue;
	UObject* UseDefaultObject = NULL;
	FText UseDefaultText;

	if ((Pin.PinType.PinCategory == PC_Object) || (Pin.PinType.PinCategory == PC_Class))
	{
		UseDefaultObject = FindObject<UObject>(ANY_PACKAGE, *NewDefaultValue);
		UseDefaultValue = NULL;
	}
	else
	{
		UseDefaultObject = NULL;
		UseDefaultValue = NewDefaultValue;
	}

	// Check the default value and make it an error if it's bogus
	if (IsPinDefaultValid(&Pin, UseDefaultValue, UseDefaultObject, UseDefaultText) == TEXT(""))
	{
		Pin.DefaultObject = UseDefaultObject;
		Pin.DefaultValue = UseDefaultValue;
		Pin.DefaultTextValue = UseDefaultText;
	}

	UEdGraphNode* Node = Pin.GetOwningNode();
	check(Node);
	Node->PinDefaultValueChanged(&Pin);

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Node);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UEdGraphSchema_K2::TrySetDefaultObject(UEdGraphPin& Pin, UObject* NewDefaultObject) const
{
	FText UseDefaultText;

	// Check the default value and make it an error if it's bogus
	if (IsPinDefaultValid(&Pin, FString(TEXT("")), NewDefaultObject, UseDefaultText) == TEXT(""))
	{
		Pin.DefaultObject = NewDefaultObject;
		Pin.DefaultValue = NULL;
		Pin.DefaultTextValue = UseDefaultText;
	}

	UEdGraphNode* Node = Pin.GetOwningNode();
	check(Node);
	Node->PinDefaultValueChanged(&Pin);

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Node);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UEdGraphSchema_K2::TrySetDefaultText(UEdGraphPin& InPin, const FText& InNewDefaultText) const
{
	// No reason to set the FText if it is not a PC_Text.
	if(InPin.PinType.PinCategory == PC_Text)
	{
		// Check the default value and make it an error if it's bogus
		if (IsPinDefaultValid(&InPin, TEXT(""), NULL, InNewDefaultText) == TEXT(""))
		{
			InPin.DefaultObject = NULL;
			InPin.DefaultValue = NULL;
			if(InNewDefaultText.IsEmpty())
			{
				InPin.DefaultTextValue = InNewDefaultText;
			}
			else
			{
				InPin.DefaultTextValue = FText::ChangeKey(TEXT(""), InPin.GetOwningNode()->NodeGuid.ToString() + TEXT("_") + InPin.PinName + FString::FromInt(InPin.GetOwningNode()->Pins.Find(&InPin)), InNewDefaultText);
			}
		}

		UEdGraphNode* Node = InPin.GetOwningNode();
		check(Node);
		Node->PinDefaultValueChanged(&InPin);

		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Node);
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
}

bool UEdGraphSchema_K2::IsAutoCreateRefTerm(const UEdGraphPin* Pin) const
{
	check(Pin != NULL);

	bool bIsAutoCreateRefTerm = false;
	UEdGraphNode* OwningNode = Pin->GetOwningNode();
	UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(OwningNode);
	if (FuncNode)
	{
		UFunction* TargetFunction = FuncNode->GetTargetFunction();
		if (TargetFunction)
		{
			bIsAutoCreateRefTerm = TargetFunction->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm);
		}
	}

	return bIsAutoCreateRefTerm;
}

bool UEdGraphSchema_K2::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	check(Pin != NULL);

	if ((Pin->PinName == PN_Self && Pin->LinkedTo.Num() > 0) || (Pin->PinType.PinCategory == PC_Exec) || (Pin->PinType.bIsReference && !IsAutoCreateRefTerm(Pin)) || Pin->bDefaultValueIsIgnored)
	{
		return true;
	}

	return false;
}

void UEdGraphSchema_K2::SetPinDefaultValueBasedOnType(UEdGraphPin* Pin) const
{
	UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
	UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));

	// Create a useful default value based on the pin type
	if(Pin->PinType.bIsArray)
	{
		Pin->AutogeneratedDefaultValue = TEXT("");
	}
	else if (Pin->PinType.PinCategory == PC_Int)
	{
		Pin->AutogeneratedDefaultValue = TEXT("0");
	}
	else if(Pin->PinType.PinCategory == PC_Byte)
	{
		UEnum* EnumPtr = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get());
		if(EnumPtr)
		{
			// First element of enum can change. If the enum is { A, B, C } and the default value is A, 
			// the defult value should not change when enum will be changed into { N, A, B, C }
			Pin->AutogeneratedDefaultValue = TEXT("");
			Pin->DefaultValue = EnumPtr->GetEnumName(0);
			return;
		}
		else
		{
			Pin->AutogeneratedDefaultValue = TEXT("0");
		}
	}
	else if (Pin->PinType.PinCategory == PC_Float)
	{
		Pin->AutogeneratedDefaultValue = TEXT("0.0");
	}
	else if (Pin->PinType.PinCategory == PC_Boolean)
	{
		Pin->AutogeneratedDefaultValue = TEXT("false");
	}
	else if (Pin->PinType.PinCategory == PC_Name)
	{
		Pin->AutogeneratedDefaultValue = TEXT("None");
	}
	else if ((Pin->PinType.PinCategory == PC_Struct) && ((Pin->PinType.PinSubCategoryObject == VectorStruct) || (Pin->PinType.PinSubCategoryObject == RotatorStruct)))
	{
		Pin->AutogeneratedDefaultValue = TEXT("0, 0, 0");
	}
	else
	{
		Pin->AutogeneratedDefaultValue = TEXT("");
	}

	Pin->DefaultValue = Pin->AutogeneratedDefaultValue;
}

void UEdGraphSchema_K2::ValidateExistingConnections(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Break any newly invalid links
	TArray<UEdGraphPin*> BrokenLinks;
	for (int32 Index = 0; Index < Pin->LinkedTo.Num(); )
	{
		UEdGraphPin* OtherPin = Pin->LinkedTo[Index];
		if (K2Schema->ArePinsCompatible(Pin, OtherPin))
		{
			++Index;
		}
		else
		{
			OtherPin->LinkedTo.Remove(Pin);
			Pin->LinkedTo.RemoveAtSwap(Index);

			BrokenLinks.Add(OtherPin);
		}
	}

	// Cascade the check for changed pin types
	for (TArray<UEdGraphPin*>::TIterator PinIt(BrokenLinks); PinIt; ++PinIt)
	{
		UEdGraphPin* OtherPin = *PinIt;
		OtherPin->GetOwningNode()->PinConnectionListChanged(OtherPin);
	}
}

UFunction* UEdGraphSchema_K2::FindSetVariableByNameFunction(const FEdGraphPinType& PinType)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
	UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
	UScriptStruct* ColorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("LinearColor"));
	UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));

	FName SetFunctionName = NAME_None;
	if(PinType.PinCategory == K2Schema->PC_Int)
	{
		static FName SetIntName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetIntPropertyByName));
		SetFunctionName = SetIntName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Byte)
	{
		static FName SetByteName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetBytePropertyByName));
		SetFunctionName = SetByteName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Float)
	{
		static FName SetFloatName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetFloatPropertyByName));
		SetFunctionName = SetFloatName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Boolean)
	{
		static FName SetBoolName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetBoolPropertyByName));
		SetFunctionName = SetBoolName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Object)
	{
		static FName SetObjectName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetObjectPropertyByName));
		SetFunctionName = SetObjectName;
	}
	else if(PinType.PinCategory == K2Schema->PC_String)
	{
		static FName SetStringName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetStringPropertyByName));
		SetFunctionName = SetStringName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Name)
	{
		static FName SetNameName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetNamePropertyByName));
		SetFunctionName = SetNameName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Struct && PinType.PinSubCategoryObject == VectorStruct)
	{
		static FName SetVectorName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetVectorPropertyByName));
		SetFunctionName = SetVectorName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Struct && PinType.PinSubCategoryObject == RotatorStruct)
	{
		static FName SetRotatorName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetRotatorPropertyByName));
		SetFunctionName = SetRotatorName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Struct && PinType.PinSubCategoryObject == ColorStruct)
	{
		static FName SetLinearColorName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetLinearColorPropertyByName));
		SetFunctionName = SetLinearColorName;
	}
	else if(PinType.PinCategory == K2Schema->PC_Struct && PinType.PinSubCategoryObject == TransformStruct)
	{
		static FName SetTransformName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, SetTransformPropertyByName));
		SetFunctionName = SetTransformName;
	}

	UFunction* Function = NULL;
	if(SetFunctionName != NAME_None)
	{
		if(PinType.bIsArray)
		{
			static FName SetArrayName(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, SetArrayPropertyByName));
			Function = FindField<UFunction>(UKismetArrayLibrary::StaticClass(), SetArrayName);
		}
		else
		{
			Function = FindField<UFunction>(UKismetSystemLibrary::StaticClass(), SetFunctionName);
		}
	}

	return Function;
}

bool UEdGraphSchema_K2::CanPromotePinToVariable( const UEdGraphPin& Pin ) const
{
	const FEdGraphPinType& PinType = Pin.PinType;
	bool bCanPromote = (PinType.PinCategory != PC_Wildcard && PinType.PinCategory != PC_Exec ) ? true : false;

	const UK2Node* Node = Cast<UK2Node>(Pin.GetOwningNode());
	const UBlueprint* OwningBlueprint = Node->GetBlueprint();
	
	if (OwningBlueprint && (OwningBlueprint->BlueprintType == BPTYPE_MacroLibrary))
	{
		// Never allow promotion in macros, because there's not a scope to define them in
		bCanPromote = false;
	}
	else
	{
		if (PinType.PinCategory == PC_Delegate)
		{
			bCanPromote = false;
		}
		else if ((PinType.PinCategory == PC_Object) && (PinType.PinSubCategoryObject != NULL))
		{
			if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
			{
				bCanPromote = UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Class);
			}	
		}
	}
	
	return bCanPromote;
}

void UEdGraphSchema_K2::GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const
{
	DisplayInfo.DocLink = TEXT("Shared/Editors/BlueprintEditor/GraphTypes");
	DisplayInfo.DisplayName = FText::FromString( Graph.GetName() ); // Fallback is graph name

	UFunction* Function = NULL;
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(&Graph);
	if (Blueprint)
	{
		Function = Blueprint->SkeletonGeneratedClass->FindFunctionByName(Graph.GetFName());
	}

	const EGraphType GraphType = GetGraphType(&Graph);
	if (GraphType == GT_Ubergraph)
	{
		DisplayInfo.DocExcerptName = TEXT("EventGraph");

		if (Graph.GetFName() == GN_EventGraph)
		{
			// localized name for the first event graph
			DisplayInfo.DisplayName = LOCTEXT("GraphDisplayName_EventGraph", "EventGraph");
			DisplayInfo.Tooltip = DisplayInfo.DisplayName.ToString();
		}
		else
		{
			DisplayInfo.Tooltip = Graph.GetName();
		}
	}
	else if (GraphType == GT_Function)
	{
		if ( Graph.GetFName() == FN_UserConstructionScript )
		{
			DisplayInfo.DisplayName = LOCTEXT("GraphDisplayName_ConstructionScript", "ConstructionScript");

			DisplayInfo.Tooltip = LOCTEXT("GraphTooltip_ConstructionScript", "Function executed when Blueprint is placed or modified.").ToString();
			DisplayInfo.DocExcerptName = TEXT("ConstructionScript");
		}
		else
		{
			// If we found a function from this graph, grab its tooltip
			if (Function)
			{
				DisplayInfo.Tooltip = UK2Node_CallFunction::GetDefaultTooltipForFunction(Function);
			}
			else
			{
				DisplayInfo.Tooltip = Graph.GetName();
			}

			DisplayInfo.DocExcerptName = TEXT("FunctionGraph");
		}
	}
	else if (GraphType == GT_Macro)
	{
		// Show macro description if set
		FKismetUserDeclaredFunctionMetadata* MetaData = UK2Node_MacroInstance::GetAssociatedGraphMetadata(&Graph);
		DisplayInfo.Tooltip = (MetaData->ToolTip.Len() > 0) ? MetaData->ToolTip : Graph.GetName();

		DisplayInfo.DocExcerptName = TEXT("MacroGraph");
	}
	else if (GraphType == GT_Animation)
	{
		DisplayInfo.DisplayName = LOCTEXT("GraphDisplayName_AnimGraph", "AnimGraph");

		DisplayInfo.Tooltip = LOCTEXT("GraphTooltip_AnimGraph", "Graph used to blend together different animations.").ToString();
		DisplayInfo.DocExcerptName = TEXT("AnimGraph");
	}
	else if (GraphType == GT_StateMachine)
	{
		DisplayInfo.Tooltip = Graph.GetName();
		DisplayInfo.DocExcerptName = TEXT("StateMachine");
	}

	// Add pure to notes if set
	if (Function && Function->HasAnyFunctionFlags(FUNC_BlueprintPure))
	{
		DisplayInfo.Notes.Add(TEXT("pure"));
	}

	// Mark transient graphs as obviously so
	if (Graph.HasAllFlags(RF_Transient))
	{
		DisplayInfo.DisplayName = FText::FromString( FString::Printf(TEXT("$$ %s $$"), *DisplayInfo.DisplayName.ToString()) );
		DisplayInfo.Notes.Add(TEXT("intermediate build product"));
	}
}

bool UEdGraphSchema_K2::IsSelfPin(const UEdGraphPin& Pin) const 
{
	return (Pin.PinName == PN_Self);
}

bool UEdGraphSchema_K2::IsDelegateCategory(const FString& Category) const
{
	return (Category == PC_Delegate);
}

FVector2D UEdGraphSchema_K2::CalculateAveragePositionBetweenNodes(UEdGraphPin* InputPin, UEdGraphPin* OutputPin)
{
	UEdGraphNode* InputNode = InputPin->GetOwningNode();
	UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
	const FVector2D InputCorner(InputNode->NodePosX, InputNode->NodePosY);
	const FVector2D OutputCorner(OutputNode->NodePosX, OutputNode->NodePosY);
	
	return (InputCorner + OutputCorner) * 0.5f;
}

bool UEdGraphSchema_K2::IsCompositeGraph( const UEdGraph* TestEdGraph ) const
{
	check(TestEdGraph);

	const EGraphType GraphType = GetGraphType(TestEdGraph);
	if(GraphType == GT_Function) 
	{
		//Find the Tunnel node for composite graph and see if its output is a composite node
		for(auto I = TestEdGraph->Nodes.CreateConstIterator();I;++I)
		{
			UEdGraphNode* Node = *I;
			if(auto Tunnel = Cast<UK2Node_Tunnel>(Node))
			{
				if(auto OutNode = Tunnel->OutputSourceNode)
				{
					if(Cast<UK2Node_Composite>(OutNode))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

void UEdGraphSchema_K2::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const 
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	if ((Blueprint != NULL) && FBlueprintEditorUtils::IsActorBased(Blueprint))
	{
		float XOffset = 0.0f;
		for(int32 AssetIdx=0; AssetIdx < Assets.Num(); AssetIdx++)
		{
			FVector2D Position = GraphPosition + (AssetIdx * FVector2D(XOffset, 0.0f));

			UObject* Asset = Assets[AssetIdx].GetAsset();

			TSubclassOf<UActorComponent> DestinationComponentType = FComponentAssetBrokerage::GetPrimaryComponentForAsset(Asset->GetClass());

			// Make sure we have an asset type that's registered with the component list
			if (DestinationComponentType != NULL)
			{
				TSharedPtr<FEdGraphSchemaAction_K2AddComponent> Action = FK2ActionMenuBuilder::CreateAddComponentAction(GetTransientPackage(), Blueprint, DestinationComponentType, Asset);
				Action->PerformAction(Graph, NULL, GraphPosition);
			}
		}
	}
}

void UEdGraphSchema_K2::DroppedAssetsOnNode(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphNode* Node) const
{
	// @TODO: Should dropping on component node change the component?
}

void UEdGraphSchema_K2::DroppedAssetsOnPin(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphPin* Pin) const
{
	// If dropping onto an 'object' pin, try and set the literal
	if(Pin->PinType.PinCategory == PC_Object)
	{
		UClass* PinClass = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get());
		if(PinClass != NULL)
		{
			// Find first asset of type of the pin
			UObject* Asset = FAssetData::GetFirstAssetDataOfClass(Assets, PinClass).GetAsset();
			if(Asset != NULL)
			{
				TrySetDefaultObject(*Pin, Asset);
			}
		}
	}
}

void UEdGraphSchema_K2::GetAssetsNodeHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphNode* HoverNode, FString& OutTooltipText, bool& OutOkIcon) const 
{ 
	// No comment at the moment because this doesn't do anything
	OutTooltipText = TEXT("");
	OutOkIcon = false;
}

void UEdGraphSchema_K2::GetAssetsPinHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphPin* HoverPin, FString& OutTooltipText, bool& OutOkIcon) const 
{ 
	OutTooltipText = TEXT("");
	OutOkIcon = false;

	// If dropping onto an 'object' pin, try and set the literal
	if(HoverPin->PinType.PinCategory == PC_Object)
	{
		UClass* PinClass = Cast<UClass>(HoverPin->PinType.PinSubCategoryObject.Get());
		if(PinClass != NULL)
		{
			// Find first asset of type of the pin
			FAssetData AssetData = FAssetData::GetFirstAssetDataOfClass(Assets, PinClass);
			if(AssetData.IsValid())
			{
				OutOkIcon = true;
				OutTooltipText = FString::Printf(TEXT("Assign %s to this pin"), *(AssetData.AssetName.ToString()));
			}
			else
			{
				OutOkIcon = false;
				OutTooltipText = FString::Printf(TEXT("Not compatible with this pin"));
			}
		}
	}
}

bool UEdGraphSchema_K2::FadeNodeWhenDraggingOffPin(const UEdGraphNode* Node, const UEdGraphPin* Pin) const
{
	if(Node && Pin && (PC_Delegate == Pin->PinType.PinCategory) && (EGPD_Input == Pin->Direction))
	{
		//When dragging off a delegate pin, we should duck the alpha of all nodes except the Custom Event nodes that are compatible with the delegate signature
		//This would help reinforce the connection between delegates and their matching events, and make it easier to see at a glance what could be matched up.
		if(const UK2Node_Event* EventNode = Cast<const UK2Node_Event>(Node))
		{
			const UEdGraphPin* DelegateOutPin = EventNode->FindPin(UK2Node_Event::DelegateOutputName);
			if ((NULL != DelegateOutPin) && 
				(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW != CanCreateConnection(DelegateOutPin, Pin).Response))
			{
				return false;
			}
		}

		if(const UK2Node_CreateDelegate* CreateDelegateNode = Cast<const UK2Node_CreateDelegate>(Node))
		{
			const UEdGraphPin* DelegateOutPin = CreateDelegateNode->GetDelegateOutPin();
			if ((NULL != DelegateOutPin) && 
				(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW != CanCreateConnection(DelegateOutPin, Pin).Response))
			{
				return false;
			}
		}
		
		return true;
	}
	return false;
}

void UEdGraphSchema_K2::BackwardCompatibilityNodeConversion(UEdGraph* Graph) const 
{
	if (Graph)
	{
		TArray<UK2Node_SpawnActor*> SpawnActorNodes;
		Graph->GetNodesOfClass(SpawnActorNodes);
		for(auto It = SpawnActorNodes.CreateIterator(); It; ++It)
		{
			UK2Node_SpawnActor* const OldSpawnActor = *It;
			check(OldSpawnActor);
			UEdGraphPin* OldBlueprintPin = OldSpawnActor->GetBlueprintPin();
			check(OldBlueprintPin);

			if ((0 == OldBlueprintPin->LinkedTo.Num()) && (NULL != OldBlueprintPin->DefaultObject))
			{
				const auto SpawnActorFromClass = NewObject<UK2Node_SpawnActorFromClass>(Graph);
				check(SpawnActorFromClass);
				SpawnActorFromClass->SetFlags(RF_Transactional);
				Graph->AddNode(SpawnActorFromClass, false, false);
				SpawnActorFromClass->CreateNewGuid();
				SpawnActorFromClass->PostPlacedNewNode();
				SpawnActorFromClass->AllocateDefaultPins();
				SpawnActorFromClass->NodePosX = OldSpawnActor->NodePosX;
				SpawnActorFromClass->NodePosY = OldSpawnActor->NodePosY;

				const auto ClassPin = SpawnActorFromClass->GetClassPin();
				check(ClassPin);
				const auto UsedBlueprint = Cast<UBlueprint>(OldBlueprintPin->DefaultObject);
				check(NULL != UsedBlueprint);
				ensure(NULL != *UsedBlueprint->GeneratedClass);
				const auto Blueprint = OldSpawnActor->GetBlueprint();
				TrySetDefaultObject(*ClassPin, *UsedBlueprint->GeneratedClass);
				if (ClassPin->DefaultObject != *UsedBlueprint->GeneratedClass)
				{
					FString ErrorStr = IsPinDefaultValid(ClassPin, FString(), *UsedBlueprint->GeneratedClass, FText());
					UE_LOG(LogBlueprint, Warning, TEXT("BackwardCompatibilityNodeConversion Error 'cannot set class' in blueprint: %s actor bp: %s, reason: %s"),
						Blueprint ? *Blueprint->GetName() : TEXT("Unknown"),
						UsedBlueprint ? *UsedBlueprint->GetName() : TEXT("Unknown"),
						ErrorStr.IsEmpty() ? TEXT("Unknown") : *ErrorStr);
				}

				TArray<UEdGraphPin*> OldPins;
				OldPins.Add(OldBlueprintPin);
				for(auto PinIter = SpawnActorFromClass->Pins.CreateIterator(); PinIter; ++PinIter)
				{
					UEdGraphPin* const Pin = *PinIter;
					check(Pin);
					if (ClassPin != Pin)
					{
						const auto OldPin = OldSpawnActor->FindPin(Pin->PinName);
						if(NULL != OldPin)
						{
							OldPins.Add(OldPin);
							if(!MovePinLinks(*OldPin, *Pin).CanSafeConnect())
							{
								UE_LOG(LogBlueprint, Warning, TEXT("BackwardCompatibilityNodeConversion Error 'cannot connect' in blueprint: %s actor bp: %s, pin: %s"), 
									Blueprint ? *Blueprint->GetName() : TEXT("Unknown"), 
									UsedBlueprint ? *UsedBlueprint->GetName() : TEXT("Unknown"),
									*Pin->PinName);
							}
						}
						else
						{
							UE_LOG(LogBlueprint, Warning, TEXT("BackwardCompatibilityNodeConversion Error 'missing old pin' in blueprint: %s actor bp: %s"), 
								Blueprint ? *Blueprint->GetName() : TEXT("Unknown"), 
								UsedBlueprint ? *UsedBlueprint->GetName() : TEXT("Unknown"),
								Pin ? *Pin->PinName : TEXT("Unknown"));
						}
					}
				}
				OldSpawnActor->BreakAllNodeLinks();
				for(auto PinIter = OldSpawnActor->Pins.CreateIterator(); PinIter; ++PinIter)
				{
					if(!OldPins.Contains(*PinIter))
					{
						UEdGraphPin* Pin = *PinIter;
						UE_LOG(LogBlueprint, Warning, TEXT("BackwardCompatibilityNodeConversion Error 'missing new pin' in blueprint: %s actor bp: %s"), 
							Blueprint ? *Blueprint->GetName() : TEXT("Unknown"), 
							UsedBlueprint ? *UsedBlueprint->GetName() : TEXT("Unknown"),
							Pin ? *Pin->PinName : TEXT("Unknown"));
					}
				}
				Graph->RemoveNode(OldSpawnActor);
			}
		}

		/** Fix the old Make/Break Vector, Make/Break Vector 2D, and Make/Break Rotator nodes to use the native function call versions */

		TArray<UK2Node_MakeStruct*> MakeStructNodes;
		Graph->GetNodesOfClass<UK2Node_MakeStruct>(MakeStructNodes);
		for(auto It = MakeStructNodes.CreateIterator(); It; ++It)
		{
			UK2Node_MakeStruct* OldMakeStructNode = *It;
			check(NULL != OldMakeStructNode);

			// Check to see if the struct has a native make/break that we should try to convert to.
			if(OldMakeStructNode->StructType->GetBoolMetaData(TEXT("HasNativeMakeBreak")))
			{
				UFunction* MakeNodeFunction = NULL;

				// If any pins need to change their names during the conversion, add them to the map.
				TMap<FString, FString> OldPinToNewPinMap;

				if(OldMakeStructNode->StructType->GetName() == TEXT("Rotator"))
				{
					MakeNodeFunction = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetMathLibrary"))->FindFunctionByName(TEXT("MakeRot"));
					OldPinToNewPinMap.Add(TEXT("Rotator"), TEXT("ReturnValue"));
				}
				else if(OldMakeStructNode->StructType->GetName() == TEXT("Vector"))
				{
					MakeNodeFunction = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetMathLibrary"))->FindFunctionByName(TEXT("MakeVector"));
					OldPinToNewPinMap.Add(TEXT("Vector"), TEXT("ReturnValue"));
				}
				else if(OldMakeStructNode->StructType->GetName() == TEXT("Vector2D"))
				{
					MakeNodeFunction = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetMathLibrary"))->FindFunctionByName(TEXT("MakeVector2D"));
					OldPinToNewPinMap.Add(TEXT("Vector2D"), TEXT("ReturnValue"));
				}

				if(MakeNodeFunction)
				{
					UK2Node_CallFunction* CallFunctionNode = NewObject<UK2Node_CallFunction>(Graph);
					check(CallFunctionNode);
					CallFunctionNode->SetFlags(RF_Transactional);
					Graph->AddNode(CallFunctionNode, false, false);
					CallFunctionNode->SetFromFunction(MakeNodeFunction);
					CallFunctionNode->CreateNewGuid();
					CallFunctionNode->PostPlacedNewNode();
					CallFunctionNode->AllocateDefaultPins();
					CallFunctionNode->NodePosX = OldMakeStructNode->NodePosX;
					CallFunctionNode->NodePosY = OldMakeStructNode->NodePosY;

					for(int32 PinIdx = 0; PinIdx < OldMakeStructNode->Pins.Num(); ++PinIdx)
					{
						UEdGraphPin* OldPin = OldMakeStructNode->Pins[PinIdx];
						UEdGraphPin* NewPin = NULL;

						// Check to see if the pin name is mapped to a new one, if it is use it, otherwise just search for the pin under the old name
						FString* NewPinNamePtr = OldPinToNewPinMap.Find(OldPin->PinName);
						if(NewPinNamePtr)
						{
							NewPin = CallFunctionNode->FindPin(*NewPinNamePtr);
						}
						else
						{
							NewPin = CallFunctionNode->FindPin(OldPin->PinName);
						}
						check(NewPin);

						if(!Graph->GetSchema()->MovePinLinks(*OldPin, *NewPin).CanSafeConnect())
						{
							const UBlueprint* Blueprint = OldMakeStructNode->GetBlueprint();
							UE_LOG(LogBlueprint, Warning, TEXT("BackwardCompatibilityNodeConversion Error 'cannot safetly move pin %s to %s' in blueprint: %s"), 
								*OldPin->PinName, 
								*NewPin->PinName,
								Blueprint ? *Blueprint->GetName() : TEXT("Unknown"));
						}
					}
					OldMakeStructNode->DestroyNode();
				}
			}
		}

		TArray<UK2Node_BreakStruct*> BreakStructNodes;
		Graph->GetNodesOfClass<UK2Node_BreakStruct>(BreakStructNodes);
		for(auto It = BreakStructNodes.CreateIterator(); It; ++It)
		{
			UK2Node_BreakStruct* OldBreakStructNode = *It;
			check(NULL != OldBreakStructNode);

			// Check to see if the struct has a native make/break that we should try to convert to.
			if(OldBreakStructNode->StructType->GetBoolMetaData(TEXT("HasNativeMakeBreak")))
			{
				UFunction* BreakNodeFunction = NULL;

				// If any pins need to change their names during the conversion, add them to the map.
				TMap<FString, FString> OldPinToNewPinMap;

				if(OldBreakStructNode->StructType->GetName() == TEXT("Rotator"))
				{
					BreakNodeFunction = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetMathLibrary"))->FindFunctionByName(TEXT("BreakRot"));
					OldPinToNewPinMap.Add(TEXT("Rotator"), TEXT("InRot"));
				}
				else if(OldBreakStructNode->StructType->GetName() == TEXT("Vector"))
				{
					BreakNodeFunction = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetMathLibrary"))->FindFunctionByName(TEXT("BreakVector"));
					OldPinToNewPinMap.Add(TEXT("Vector"), TEXT("InVec"));
				}
				else if(OldBreakStructNode->StructType->GetName() == TEXT("Vector2D"))
				{
					BreakNodeFunction = FindObject<UClass>(ANY_PACKAGE, TEXT("KismetMathLibrary"))->FindFunctionByName(TEXT("BreakVector2D"));
					OldPinToNewPinMap.Add(TEXT("Vector2D"), TEXT("InVec"));
				}

				if(BreakNodeFunction)
				{
					UK2Node_CallFunction* CallFunctionNode = NewObject<UK2Node_CallFunction>(Graph);
					check(CallFunctionNode);
					CallFunctionNode->SetFlags(RF_Transactional);
					Graph->AddNode(CallFunctionNode, false, false);
					CallFunctionNode->SetFromFunction(BreakNodeFunction);
					CallFunctionNode->CreateNewGuid();
					CallFunctionNode->PostPlacedNewNode();
					CallFunctionNode->AllocateDefaultPins();
					CallFunctionNode->NodePosX = OldBreakStructNode->NodePosX;
					CallFunctionNode->NodePosY = OldBreakStructNode->NodePosY;

					for(int32 PinIdx = 0; PinIdx < OldBreakStructNode->Pins.Num(); ++PinIdx)
					{
						UEdGraphPin* OldPin = OldBreakStructNode->Pins[PinIdx];
						UEdGraphPin* NewPin = NULL;

						// Check to see if the pin name is mapped to a new one, if it is use it, otherwise just search for the pin under the old name
						FString* NewPinNamePtr = OldPinToNewPinMap.Find(OldPin->PinName);
						if(NewPinNamePtr)
						{
							NewPin = CallFunctionNode->FindPin(*NewPinNamePtr);
						}
						else
						{
							NewPin = CallFunctionNode->FindPin(OldPin->PinName);
						}
						check(NewPin);

						if(!Graph->GetSchema()->MovePinLinks(*OldPin, *NewPin).CanSafeConnect())
						{
							const UBlueprint* Blueprint = OldBreakStructNode->GetBlueprint();
							UE_LOG(LogBlueprint, Warning, TEXT("BackwardCompatibilityNodeConversion Error 'cannot safetly move pin %s to %s' in blueprint: %s"), 
								*OldPin->PinName, 
								*NewPin->PinName,
								Blueprint ? *Blueprint->GetName() : TEXT("Unknown"));
						}
					}
					OldBreakStructNode->DestroyNode();
				}
			}
		}
	}
}

UEdGraphNode* UEdGraphSchema_K2::CreateSubstituteNode(UEdGraphNode* Node, const UEdGraph* Graph, FObjectInstancingGraph* InstanceGraph) const
{
	// If this is an event node, create a unique custom event node as a substitute
	UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
	if(EventNode)
	{
		if(!Graph)
		{
			// Use the node's graph (outer) if an explicit graph was not specified
			Graph = Node->GetGraph();
		}

		// Find the Blueprint that owns the graph
		UBlueprint* Blueprint = Graph ? FBlueprintEditorUtils::FindBlueprintForGraph(Graph) : NULL;
		if(Blueprint && Blueprint->SkeletonGeneratedClass)
		{
			// Gather all names in use by the Blueprint class
			TArray<FName> ExistingNamesInUse;
			FBlueprintEditorUtils::GetFunctionNameList(Blueprint, ExistingNamesInUse);
			FBlueprintEditorUtils::GetClassVariableList(Blueprint, ExistingNamesInUse);

			// Allow the old object name to be used in the graph
			FName ObjName = EventNode->GetFName();
			UObject* Found = FindObject<UObject>(EventNode->GetOuter(), *ObjName.ToString());
			if(Found)
			{
				Found->Rename(NULL, NULL, REN_None);
			}

			// Create a custom event node to replace the original event node imported from text
			UK2Node_CustomEvent* CustomEventNode = ConstructObject<UK2Node_CustomEvent>(UK2Node_CustomEvent::StaticClass(), EventNode->GetOuter(), ObjName, EventNode->GetFlags(), NULL, true, InstanceGraph);

			// Ensure that it is editable
			CustomEventNode->bIsEditable = true;

			// Set grid position to match that of the target node
			CustomEventNode->NodePosX = EventNode->NodePosX;
			CustomEventNode->NodePosY = EventNode->NodePosY;

			// Build a function name that is appropriate for the event we're replacing
			FString FunctionName;
			const UK2Node_ActorBoundEvent* ActorBoundEventNode = Cast<const UK2Node_ActorBoundEvent>(EventNode);
			const UK2Node_ComponentBoundEvent* CompBoundEventNode = Cast<const UK2Node_ComponentBoundEvent>(EventNode);
			if(ActorBoundEventNode)
			{
				FString TargetName = TEXT("None");
				if(ActorBoundEventNode->EventOwner)
				{
					TargetName = ActorBoundEventNode->EventOwner->GetActorLabel();
				}

				FunctionName = FString::Printf(TEXT("%s_%s"), *ActorBoundEventNode->DelegatePropertyName.ToString(), *TargetName);
			}
			else if(CompBoundEventNode)
			{
				FunctionName = FString::Printf(TEXT("%s_%s"), *CompBoundEventNode->DelegatePropertyName.ToString(), *CompBoundEventNode->ComponentPropertyName.ToString());
			}
			else if(EventNode->CustomFunctionName != NAME_None)
			{
				FunctionName = EventNode->CustomFunctionName.ToString();
			}
			else if(EventNode->bOverrideFunction)
			{
				FunctionName = EventNode->EventSignatureName.ToString();
			}
			else
			{
				FunctionName = CustomEventNode->GetName().Replace(TEXT("K2Node_"), TEXT(""), ESearchCase::CaseSensitive);
			}

			// Ensure that the new event name doesn't already exist as a variable or function name
			if(InstanceGraph)
			{
				FunctionName += TEXT("_Copy");
				CustomEventNode->CustomFunctionName = FName(*FunctionName, FNAME_Find);
				if(CustomEventNode->CustomFunctionName != NAME_None
					&& ExistingNamesInUse.Contains(CustomEventNode->CustomFunctionName))
				{
					int32 i = 0;
					FString TempFuncName;

					do 
					{
						TempFuncName = FString::Printf(TEXT("%s_%d"), *FunctionName, ++i);
						CustomEventNode->CustomFunctionName = FName(*TempFuncName, FNAME_Find);
					}
					while(CustomEventNode->CustomFunctionName != NAME_None
						&& ExistingNamesInUse.Contains(CustomEventNode->CustomFunctionName));

					FunctionName = TempFuncName;
				}
			}

			// Should be a unique name now, go ahead and assign it
			CustomEventNode->CustomFunctionName = FName(*FunctionName);

			// Copy the pins from the old node to the new one that's replacing it
			CustomEventNode->Pins = EventNode->Pins;
			CustomEventNode->UserDefinedPins = EventNode->UserDefinedPins;

			// Clear out the pins from the old node so that links aren't broken later when it's destroyed
			EventNode->Pins.Empty();
			EventNode->UserDefinedPins.Empty();

			// Fixup pins
			for(int32 PinIndex = 0; PinIndex < CustomEventNode->Pins.Num(); ++PinIndex)
			{
				UEdGraphPin* Pin = CustomEventNode->Pins[PinIndex];
				check(Pin);

				// Reparent the pin to the new custom event node
				Pin->Rename(*Pin->GetName(), CustomEventNode);

				// Don't include execution or delegate output pins as user-defined pins
				if(!IsExecPin(*Pin) && !IsDelegateCategory(Pin->PinType.PinCategory))
				{
					// Check to see if this pin already exists as a user-defined pin on the custom event node
					bool bFoundUserDefinedPin = false;
					for(int32 UserDefinedPinIndex = 0; UserDefinedPinIndex < CustomEventNode->UserDefinedPins.Num() && !bFoundUserDefinedPin; ++UserDefinedPinIndex)
					{
						const FUserPinInfo& UserDefinedPinInfo = *CustomEventNode->UserDefinedPins[UserDefinedPinIndex].Get();
						bFoundUserDefinedPin = Pin->PinName == UserDefinedPinInfo.PinName && Pin->PinType == UserDefinedPinInfo.PinType;
					}

					if(!bFoundUserDefinedPin)
					{
						// Add a new entry into the user-defined pin array for the custom event node
						TSharedPtr<FUserPinInfo> UserPinInfo = MakeShareable(new FUserPinInfo());
						UserPinInfo->PinName = Pin->PinName;
						UserPinInfo->PinType = Pin->PinType;
						CustomEventNode->UserDefinedPins.Add(UserPinInfo);
					}
				}
			}

			// Return the new custom event node that we just created as a substitute for the original event node
			return CustomEventNode;
		}
	}

	// Use the default logic in all other cases
	return UEdGraphSchema::CreateSubstituteNode(Node, Graph, InstanceGraph);
}

int32 UEdGraphSchema_K2::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	int32 SelectionCount = 0;
	
	if( Blueprint )
	{
		SelectionCount = FKismetEditorUtilities::GetNumberOfSelectedNodes(Blueprint);
	}
	return SelectionCount;
}

TSharedPtr<FEdGraphSchemaAction> UEdGraphSchema_K2::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FEdGraphSchemaAction_K2AddComment));
}

UEdGraph* UEdGraphSchema_K2::DuplicateGraph(UEdGraph* GraphToDuplicate) const
{
	UEdGraph* NewGraph = NULL;

	if (CanDuplicateGraph())
	{
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(GraphToDuplicate);

		NewGraph = FEdGraphUtilities::CloneGraph(GraphToDuplicate, Blueprint);

		if (NewGraph)
		{
			FEdGraphUtilities::RenameGraphCloseToName(NewGraph,GraphToDuplicate->GetName());

			//Rename the entry node or any further renames will not update the entry node, also fixes a duplicate node issue on compile
			for (int32 NodeIndex = 0; NodeIndex < NewGraph->Nodes.Num(); ++NodeIndex)
			{
				UEdGraphNode* Node = NewGraph->Nodes[NodeIndex];
				if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
				{
					if (EntryNode->SignatureName == GraphToDuplicate->GetFName())
					{
						EntryNode->Modify();
						EntryNode->SignatureName = NewGraph->GetFName();
						break;
					}
				}
				// Rename any custom events to be unique
				else if (Node->GetClass()->GetFName() ==  TEXT("K2Node_CustomEvent"))
				{
					UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node);
					CustomEvent->RenameCustomEventCloseToName();
				}
			}
		}
	}
	return NewGraph;
}

/**
 * Attempts to best-guess the height of the node. This is necessary because we don't know the actual
 * size of the node until the next Slate tick
 *
 * @param Node The node to guess the height of
 * @return The estimated height of the specified node
 */
float UEdGraphSchema_K2::EstimateNodeHeight( UEdGraphNode* Node )
{
	float HeightEstimate = 0.0f;

	if ( Node != NULL )
	{
		float BaseNodeHeight = 48.0f;
		bool bConsiderNodePins = false;
		float HeightPerPin = 18.0f;

		if ( Node->IsA( UK2Node_CallFunction::StaticClass() ) )
		{
			BaseNodeHeight = 80.0f;
			bConsiderNodePins = true;
			HeightPerPin = 18.0f;
		}
		else if ( Node->IsA( UK2Node_Event::StaticClass() ) )
		{
			BaseNodeHeight = 48.0f;
			bConsiderNodePins = true;
			HeightPerPin = 16.0f;
		}

		HeightEstimate = BaseNodeHeight;

		if ( bConsiderNodePins )
		{
			int32 NumInputPins = 0;
			int32 NumOutputPins = 0;

			for ( int32 PinIndex = 0; PinIndex < Node->Pins.Num(); PinIndex++ )
			{
				UEdGraphPin* CurrentPin = Node->Pins[PinIndex];
				if ( CurrentPin != NULL && !CurrentPin->bHidden )
				{
					switch ( CurrentPin->Direction )
					{
						case EGPD_Input:
							{
								NumInputPins++;
							}
							break;
						case EGPD_Output:
							{
								NumOutputPins++;
							}
							break;
					}
				}
			}

			float MaxNumPins = float(FMath::Max<int32>( NumInputPins, NumOutputPins ));

			HeightEstimate += MaxNumPins * HeightPerPin;
		}
	}

	return HeightEstimate;
}


bool UEdGraphSchema_K2::CollapseGatewayNode(UK2Node* InNode, UEdGraphNode* InEntryNode, UEdGraphNode* InResultNode) const
{
	bool bSuccessful = true;

	for (int32 BoundaryPinIndex = 0; BoundaryPinIndex < InNode->Pins.Num(); ++BoundaryPinIndex)
	{
		UEdGraphPin* const BoundaryPin = InNode->Pins[BoundaryPinIndex];

		bool bFunctionNode = InNode->IsA(UK2Node_CallFunction::StaticClass());

		// For each pin in the gateway node, find the associated pin in the entry or result node.
		UEdGraphNode* const GatewayNode = (BoundaryPin->Direction == EGPD_Input) ? InEntryNode : InResultNode;
		UEdGraphPin* GatewayPin = NULL;
		if (GatewayNode)
		{
			for (int32 PinIdx=0; PinIdx < GatewayNode->Pins.Num(); PinIdx++)
			{
				UEdGraphPin* const Pin = GatewayNode->Pins[PinIdx];

				// Function graphs have a single exec path through them, so only one exec pin for input and another for output. In this fashion, they must not be handled by name.
				if(InNode->GetClass() == UK2Node_CallFunction::StaticClass() && Pin->PinType.PinCategory == PC_Exec && BoundaryPin->PinType.PinCategory == PC_Exec && (Pin->Direction != BoundaryPin->Direction))
				{
					GatewayPin = Pin;
					break;
				}
				else if ((Pin->PinName == BoundaryPin->PinName) && (Pin->Direction != BoundaryPin->Direction))
				{
					GatewayPin = Pin;
					break;
				}
			}
		}

		if (GatewayPin)
		{
			CombineTwoPinNetsAndRemoveOldPins(BoundaryPin, GatewayPin);
		}
		else
		{
			if (BoundaryPin->LinkedTo.Num() > 0)
			{
				UBlueprint* OwningBP = InNode->GetBlueprint();
				if( OwningBP )
				{
					// We had an input/output with a connection that wasn't twinned
					bSuccessful = false;
					OwningBP->Message_Warn( FString::Printf(*NSLOCTEXT("K2Node", "PinOnBoundryNode_Warning", "Warning: Pin '%s' on boundary node '%s' could not be found in the composite node '%s'").ToString(),
						*(BoundaryPin->PinName),
						(GatewayNode != NULL) ? *(GatewayNode->GetName()) : TEXT("(null)"),
						*(GetName()))					
						);
				}
				else
				{
					UE_LOG(LogBlueprint, Warning, TEXT("%s"), *FString::Printf(*NSLOCTEXT("K2Node", "PinOnBoundryNode_Warning", "Warning: Pin '%s' on boundary node '%s' could not be found in the composite node '%s'").ToString(),
						*(BoundaryPin->PinName),
						(GatewayNode != NULL) ? *(GatewayNode->GetName()) : TEXT("(null)"),
						*(GetName()))					
						);
				}
			}
			else
			{
				// Associated pin was not found but there were no links on this side either, so no harm no foul
			}
		}
	}

	return bSuccessful;
}

void UEdGraphSchema_K2::CombineTwoPinNetsAndRemoveOldPins(UEdGraphPin* InPinA, UEdGraphPin* InPinB) const
{
	check(InPinA != NULL);
	check(InPinB != NULL);

	ensure(InPinA->Direction != InPinB->Direction);
	ensure(InPinA->GetOwningNode() != InPinB->GetOwningNode());

	if ((InPinA->LinkedTo.Num() == 0) && (InPinA->Direction == EGPD_Input))
	{
		// Push the literal value of A to InPinB's connections
		for (int32 IndexB = 0; IndexB < InPinB->LinkedTo.Num(); ++IndexB)
		{
			UEdGraphPin* FarB = InPinB->LinkedTo[IndexB];
			// TODO: Michael N. says this if check should be unnecessary once the underlying issue is fixed.
			// (Probably should use a check() instead once it's removed though.  See additional cases below.
			if (FarB != NULL)
			{
				FarB->DefaultValue = InPinA->DefaultValue;
				FarB->DefaultObject = InPinA->DefaultObject;
				FarB->DefaultTextValue = InPinA->DefaultTextValue;
			}
		}
	}
	else if ((InPinB->LinkedTo.Num() == 0) && (InPinB->Direction == EGPD_Input))
	{
		// Push the literal value of B to InPinA's connections
		for (int32 IndexA = 0; IndexA < InPinA->LinkedTo.Num(); ++IndexA)
		{
			UEdGraphPin* FarA = InPinA->LinkedTo[IndexA];
			// TODO: Michael N. says this if check should be unnecessary once the underlying issue is fixed.
			// (Probably should use a check() instead once it's removed though.  See additional cases above and below.
			if (FarA != NULL)
			{
				FarA->DefaultValue = InPinB->DefaultValue;
				FarA->DefaultObject = InPinB->DefaultObject;
				FarA->DefaultTextValue = InPinB->DefaultTextValue;
			}
		}
	}
	else
	{
		// Make direct connections between the things that connect to A or B, removing A and B from the picture
		for (int32 IndexA = 0; IndexA < InPinA->LinkedTo.Num(); ++IndexA)
		{
			UEdGraphPin* FarA = InPinA->LinkedTo[IndexA];
			// TODO: Michael N. says this if check should be unnecessary once the underlying issue is fixed.
			// (Probably should use a check() instead once it's removed though.  See additional cases above.
			if (FarA != NULL)
			{
				for (int32 IndexB = 0; IndexB < InPinB->LinkedTo.Num(); ++IndexB)
				{
					UEdGraphPin* FarB = InPinB->LinkedTo[IndexB];
					FarA->Modify();
					FarB->Modify();
					FarA->MakeLinkTo(FarB);
				}
			}
		}
	}

	InPinA->BreakAllPinLinks();
	InPinB->BreakAllPinLinks();
}


/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
