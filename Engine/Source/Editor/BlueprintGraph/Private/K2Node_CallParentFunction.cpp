// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_CallParentFunction::UK2Node_CallParentFunction(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsFinalFunction = true;
}

FText UK2Node_CallParentFunction::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UFunction* Function = FunctionReference.ResolveMember<UFunction>(this);
	FString FunctionName = Function ? GetUserFacingFunctionName( Function ) : FunctionReference.GetMemberName().ToString();

	if( GEditor && GetDefault<UEditorStyleSettings>()->bShowFriendlyNames )
	{
		FunctionName = FName::NameToDisplayString(FunctionName, false);
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("FunctionName"),  FText::FromString( FunctionName ));
	return FText::Format( LOCTEXT( "CallSuperFunction", "Parent: {FunctionName}" ), Args);
}

FString UK2Node_CallParentFunction::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	UFunction* Function = FunctionReference.ResolveMember<UFunction>(this);
	FString FunctionName = Function ? GetUserFacingFunctionName( Function ) : FunctionReference.GetMemberName().ToString();
	FString Result = FString::Printf( TEXT("Parent: %s" ), *FunctionName);

	if( GEditor && GetDefault<UEditorStyleSettings>()->bShowFriendlyNames )
	{
		Result = FName::NameToDisplayString(Result, false);
	}
	return Result;
}

FLinearColor UK2Node_CallParentFunction::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->ParentFunctionCallNodeTitleColor;
}

void UK2Node_CallParentFunction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	UEdGraphPin* SelfPin = Schema->FindSelfPin(*this, EGPD_Input);
	if( SelfPin )
	{
		SelfPin->bHidden = true;
	}
}

void UK2Node_CallParentFunction::SetFromFunction(const UFunction* Function)
{
	if (Function != NULL)
	{
		bIsPureFunc = Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
		bIsConstFunc = Function->HasAnyFunctionFlags(FUNC_Const);

		FGuid FunctionGuid;
		if (Function->GetOwnerClass())
		{
			UBlueprint::GetGuidFromClassByFieldName<UFunction>(Function->GetOwnerClass(), Function->GetFName(), FunctionGuid);
		}

		FunctionReference.SetDirect(Function->GetFName(), FunctionGuid, Function->GetOwnerClass(), false);
	}
}

void UK2Node_CallParentFunction::PostPlacedNewNode()
{
	// We don't want to check if our function exists in the current scope

	UK2Node::PostPlacedNewNode();
}

#undef LOCTEXT_NAMESPACE
