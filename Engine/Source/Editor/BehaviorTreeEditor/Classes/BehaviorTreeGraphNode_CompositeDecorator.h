// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_CompositeDecorator.generated.h"

UCLASS()
class UBehaviorTreeGraphNode_CompositeDecorator : public UBehaviorTreeGraphNode
{
	GENERATED_UCLASS_BODY()

	// The logic graph for this decorator (returning a boolean)
	UPROPERTY()
	class UEdGraph* BoundGraph;

	UPROPERTY(EditAnywhere, Category=Description)
	FString CompositeName;

	/** if set, all logic operations will be shown in description */
	UPROPERTY(EditAnywhere, Category=Description)
	uint32 bShowOperations : 1;

	/** updated with internal graph changes, set when decorators inside can abort flow */
	UPROPERTY()
	uint32 bCanAbortFlow : 1;

	FString GetNodeTypeDescription() const;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetDescription() const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual UEdGraph* GetBoundGraph() const { return BoundGraph; }
	virtual bool IsSubNode() const OVERRIDE;

	virtual void PrepareForCopying() OVERRIDE;
	virtual void PostCopyNode() OVERRIDE;

	void CollectDecoratorData(TArray<class UBTDecorator*>& NodeInstances, TArray<struct FBTDecoratorLogic>& Operations) const;
	void SetDecoratorData(class UBTCompositeNode* InParentNode, uint8 InChildIndex);
	void InitializeDecorator(class UBTDecorator* InnerDecorator);
	void OnBlackboardUpdate();
	void OnInnerGraphChanged();
	void BuildDescription();

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;

	void ResetExecutionRange();

	/** Execution index range of internal nodes, used by debugger */
	uint16 FirstExecutionIndex;
	uint16 LastExecutionIndex;

protected:
	void CreateBoundGraph();

	UPROPERTY()
	class UBTCompositeNode* ParentNodeInstance;

	uint8 ChildIndex;

	UPROPERTY()
	FString CachedDescription;
};
