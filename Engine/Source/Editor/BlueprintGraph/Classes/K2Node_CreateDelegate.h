// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "K2Node_CreateDelegate.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CreateDelegate : public UK2Node
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FName SelectedFunctionName;

public:
	// UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual void PinTypeChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual void NodeConnectionListChanged() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual UObject* GetJumpTargetForDoubleClick() const OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual void PostReconstructNode() OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End of UK2Node interface

	bool IsValid(FString* OutMsg = NULL, bool bDontUseSkeletalClassForSelf = false) const;

	BLUEPRINTGRAPH_API UFunction* GetDelegateSignature() const;
	BLUEPRINTGRAPH_API UClass* GetScopeClass(bool bDontUseSkeletalClassForSelf = false) const;

	BLUEPRINTGRAPH_API FName GetFunctionName() const;
	BLUEPRINTGRAPH_API UEdGraphPin* GetDelegateOutPin() const;
	BLUEPRINTGRAPH_API UEdGraphPin* GetObjectInPin() const;
 
	BLUEPRINTGRAPH_API void HandleAnyChange(bool bForceModify = false);
	BLUEPRINTGRAPH_API void HandleAnyChangeInner();

	BLUEPRINTGRAPH_API void ValidationAfterFunctionsAreCreated(class FCompilerResultsLog& MessageLog, bool bFullCompile) const;

	// return Graph and Blueprint, when thay should be notified about change. It allown to call BroadcastChanged only once per blueprint.
	BLUEPRINTGRAPH_API void HandleAnyChange(UEdGraph* & OutGraph, UBlueprint* & OutBlueprint);
};
