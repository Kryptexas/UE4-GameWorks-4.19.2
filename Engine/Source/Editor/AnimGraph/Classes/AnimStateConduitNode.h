// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimStateConduitNode.generated.h"

UCLASS(MinimalAPI)
class UAnimStateConduitNode : public UAnimStateNodeBase
{
	GENERATED_UCLASS_BODY()
public:

	// The transition graph for this conduit; it's a logic graph, not an animation graph
	UPROPERTY()
	class UEdGraph* BoundGraph;

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual bool CanDuplicateNode() const OVERRIDE { return true; }
	virtual void PostPasteNode() OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UAnimStateNodeBase interface
	virtual UEdGraphPin* GetInputPin() const OVERRIDE;
	virtual UEdGraphPin* GetOutputPin() const OVERRIDE;
	virtual FString GetStateName() const OVERRIDE;
	virtual void GetTransitionList(TArray<class UAnimStateTransitionNode*>& OutTransitions, bool bWantSortedList = true) OVERRIDE;
	virtual FString GetDesiredNewNodeName() const OVERRIDE;
	virtual UEdGraph* GetBoundGraph() const OVERRIDE { return BoundGraph; }
	// End of UAnimStateNodeBase interface
};
