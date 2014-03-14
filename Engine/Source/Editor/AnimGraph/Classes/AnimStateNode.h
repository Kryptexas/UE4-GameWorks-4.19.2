// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimStateNode.generated.h"

UENUM()
enum EAnimStateType
{
	AST_SingleAnimation UMETA(DisplayName="Single animation"),
	AST_BlendGraph UMETA(DisplayName="Blend graph"),
};


UCLASS(MinimalAPI)
class UAnimStateNode : public UAnimStateNodeBase
{
	GENERATED_UCLASS_BODY()
public:

	// The animation graph for this state
	UPROPERTY()
	class UEdGraph* BoundGraph;

	// The type of the contents of this state
	UPROPERTY()
	TEnumAsByte<EAnimStateType> StateType;

	UPROPERTY(EditAnywhere, Category=Events)
	FAnimNotifyEvent StateEntered;
	
	UPROPERTY(EditAnywhere, Category=Events)
	FAnimNotifyEvent StateLeft;

	UPROPERTY(EditAnywhere, Category=Events)
	FAnimNotifyEvent StateFullyBlended;

	// Begin UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End UObject interface

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual bool CanDuplicateNode() const { return true; }
	virtual void PostPasteNode() OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	// End UEdGraphNode interface
	
	// Begin UAnimStateNodeBase interface
	virtual UEdGraphPin* GetInputPin() const OVERRIDE;
	virtual UEdGraphPin* GetOutputPin() const OVERRIDE;
	virtual FString GetStateName() const OVERRIDE;
	virtual void GetTransitionList(TArray<class UAnimStateTransitionNode*>& OutTransitions, bool bWantSortedList = false) OVERRIDE;
	// End of UAnimStateNodeBase interface

	// @return the pose pin of the state sink node within the anim graph of this state
	ANIMGRAPH_API UEdGraphPin* GetPoseSinkPinInsideState() const;

public:
	virtual UEdGraph* GetBoundGraph() const OVERRIDE { return BoundGraph; }
};
