// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_DelegateSet.generated.h"

UCLASS(MinimalAPI)
class UK2Node_DelegateSet : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** Delegate property name that this event is associated with on the target */
	UPROPERTY()
	FName DelegatePropertyName;

	/** Class that the delegate property is defined in */
	UPROPERTY()
	TSubclassOf<class UObject>  DelegatePropertyClass;


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const { return FColor(216,88,88); }
	// End UEdGraphNode interface


	// Begin UK2Node interface
	virtual bool DrawNodeAsEntry() const OVERRIDE { return true; }
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface

	// @todo document
	BLUEPRINTGRAPH_API UEdGraphPin* GetDelegateOwner() const;

	// @todo document
	BLUEPRINTGRAPH_API FName GetDelegateTargetEntryPointName() const
	{
		const FString TargetName = GetName() + TEXT("_") + DelegatePropertyName.ToString() + TEXT("_EP");
		return FName(*TargetName);
	}

	// @todo document
	BLUEPRINTGRAPH_API UFunction* GetDelegateSignature();
	BLUEPRINTGRAPH_API UFunction* GetDelegateSignature() const;

#endif
};

