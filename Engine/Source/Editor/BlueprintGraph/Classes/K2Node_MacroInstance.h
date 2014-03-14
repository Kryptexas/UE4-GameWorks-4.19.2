// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_MacroInstance.generated.h"

UCLASS(MinimalAPI)
class UK2Node_MacroInstance : public UK2Node_Tunnel
{
	GENERATED_UCLASS_BODY()

private:
	/** A macro is like a composite node, except that the associated graph lives
	  * in another blueprint, and can be instanced multiple times. */
	UPROPERTY()
	class UEdGraph* MacroGraph_DEPRECATED;

	UPROPERTY()
	FGraphReference MacroGraphReference;

public:
	/** Stored type info for what type the wildcard pins in this macro should become. */
	UPROPERTY()
	struct FEdGraphPinType ResolvedWildcardType;

	/** Whether we need to reconstruct the node after the pins have changed */
	bool bReconstructNode;

#if WITH_EDITOR

	// Begin UObject interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End UObject interface

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual void PostPasteNode() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return true; }
	void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual void NodeConnectionListChanged() OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool DrawNodeAsExit() const OVERRIDE { return false; }
	virtual bool DrawNodeAsEntry() const OVERRIDE { return false; }
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual void PostReconstructNode() OVERRIDE;
	virtual FText GetActiveBreakpointToolTipText() const OVERRIDE;
	// End UK2Node interface

	void SetMacroGraph(UEdGraph* Graph) { MacroGraphReference.SetGraph(Graph); }
	UEdGraph* GetMacroGraph() const { return MacroGraphReference.GetGraph(); }

	// Finds the associated metadata for the macro instance if there is any; this function is not particularly fast.
	BLUEPRINTGRAPH_API static FKismetUserDeclaredFunctionMetadata* GetAssociatedGraphMetadata(const UEdGraph* AssociatedMacroGraph);
	static void FindInContentBrowser(TWeakObjectPtr<UK2Node_MacroInstance> MacroInstance);
#endif
};

