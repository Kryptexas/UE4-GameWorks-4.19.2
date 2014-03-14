// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "K2Node_Composite.generated.h"

UCLASS(MinimalAPI)
class UK2Node_Composite : public UK2Node_Tunnel
{
	GENERATED_UCLASS_BODY()

	// The graph that this composite node is representing
	UPROPERTY()
	class UEdGraph* BoundGraph;


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	BLUEPRINTGRAPH_API virtual void AllocateDefaultPins() OVERRIDE;
	BLUEPRINTGRAPH_API virtual void DestroyNode() OVERRIDE;
	BLUEPRINTGRAPH_API virtual void PostPasteNode() OVERRIDE;
	BLUEPRINTGRAPH_API virtual FString GetTooltip() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	BLUEPRINTGRAPH_API virtual bool CanUserDeleteNode() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual UObject* GetJumpTargetForDoubleClick() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual void PostPlacedNewNode() OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool DrawNodeAsExit() const OVERRIDE { return false; }
	virtual bool DrawNodeAsEntry() const OVERRIDE { return false; }
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	// End UK2Node interface

	// Get the entry/exit nodes inside this collapsed graph
	BLUEPRINTGRAPH_API UK2Node_Tunnel* GetEntryNode() const;
	BLUEPRINTGRAPH_API UK2Node_Tunnel* GetExitNode() const;

private:
	/** Rename the BoundGraph to a unique name
		- this is a special case rename: RenameGraphCloseToName() assumes that we are only concurrned with the immediate Outer() scope,
		  but in the case of the BoundGraph we must also look into the Outer of the composite node(a Graph), and make sure no graphs in its SubGraph
		  array are already using the name.  */
	void RenameBoundGraphCloseToName(const FString& Name);

	/** Determine if the name already used by another graph in composite nodes chain */
	bool IsCompositeNameAvailable( const FString& NewName );

#endif
};



