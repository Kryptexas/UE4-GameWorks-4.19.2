// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Event.generated.h"

UCLASS(MinimalAPI)
class UK2Node_Event : public UK2Node_EditablePinBase
{
	GENERATED_UCLASS_BODY()
	BLUEPRINTGRAPH_API static const FString DelegateOutputName;

	/** Name of function signature that this event implements */
	UPROPERTY()
	FName EventSignatureName;

	/** Class that the function signature is from. */
	UPROPERTY()
	TSubclassOf<class UObject> EventSignatureClass;

	/** If true, we are actually overriding this function, not making a new event with a signature that matches */
	UPROPERTY()
	uint32 bOverrideFunction:1;

	/** If true, this event is internal machinery, and should not be marked BlueprintCallable */
	UPROPERTY()
	uint32 bInternalEvent:1;

	/** If this is not an override, allow user to specify a name for the function created by this entry point */
	UPROPERTY()
	FName CustomFunctionName;

	/** Additional function flags to apply to this function */
	UPROPERTY()
	uint32 FunctionFlags;

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	BLUEPRINTGRAPH_API virtual void AllocateDefaultPins() OVERRIDE;
	BLUEPRINTGRAPH_API virtual FString GetTooltip() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FString GetKeywords() const OVERRIDE;	
	BLUEPRINTGRAPH_API virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual bool CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FName GetCornerIcon() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual bool IsDeprecated() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FString GetDeprecationMessage() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual UObject* GetJumpTargetForDoubleClick() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Event_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool DrawNodeAsEntry() const OVERRIDE { return true; }
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	BLUEPRINTGRAPH_API virtual void GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const OVERRIDE;
	BLUEPRINTGRAPH_API virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	BLUEPRINTGRAPH_API virtual void PinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	BLUEPRINTGRAPH_API virtual void PostReconstructNode() OVERRIDE;
	BLUEPRINTGRAPH_API virtual FString GetDocumentationLink() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FString GetDocumentationExcerptName() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const OVERRIDE;
	BLUEPRINTGRAPH_API virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	BLUEPRINTGRAPH_API virtual FText GetToolTipHeading() const;
	// End UK2Node interface

	/** Checks whether the parameters for this event node are compatible with the specified function entry node */
	BLUEPRINTGRAPH_API virtual bool IsFunctionEntryCompatible(const class UK2Node_FunctionEntry* EntryNode) const;
	
	UFunction* FindEventSignatureFunction();
	BLUEPRINTGRAPH_API void UpdateDelegatePin();
	BLUEPRINTGRAPH_API FName GetFunctionName() const;
	BLUEPRINTGRAPH_API virtual bool IsUsedByAuthorityOnlyDelegate() const { return false; }
	BLUEPRINTGRAPH_API virtual bool IsCosmeticTickEvent() const;

	/** Returns localized string describing replication settings. 
	 *		Calling - whether this function is being called ("sending") or showing implementation ("receiving"). Determined whether we output "Replicated To Server" or "Replicated From Client".
	 */
	static FString GetLocalizedNetString(uint32 NetFlags, bool Calling);
#endif
};

