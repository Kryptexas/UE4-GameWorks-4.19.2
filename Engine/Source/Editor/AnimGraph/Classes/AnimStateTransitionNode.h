// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimStateTransitionNode.generated.h"


UCLASS(MinimalAPI, DependsOn=UAnimInstance, config=Editor)
class UAnimStateTransitionNode : public UAnimStateNodeBase
{
	GENERATED_UCLASS_BODY()

	// The transition logic graph for this transition (returning a boolean)
	UPROPERTY()
	class UEdGraph* BoundGraph;

	// The animation graph for this transition if it uses custom blending (reutrning a pose)
	UPROPERTY()
	class UEdGraph* CustomTransitionGraph;

	// The priority order of this transition. If multiple transitions out of a state go
	// true at the same time, the one with the smallest priority order will take precedent
	UPROPERTY(EditAnywhere, Category=Transition)
	int32 PriorityOrder;

	// The duration to cross-fade for
	UPROPERTY(EditAnywhere, Config, Category=Transition)
	float CrossfadeDuration;

	// The type of blending to use in the crossfade
	UPROPERTY(EditAnywhere, Config, Category=Transition)
	TEnumAsByte<ETransitionBlendMode::Type> CrossfadeMode;

	// What transition logic to use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Transition)
	TEnumAsByte<ETransitionLogicType::Type> LogicType;

	UPROPERTY(EditAnywhere, Category=Events)
	FAnimNotifyEvent TransitionStart;

	UPROPERTY(EditAnywhere, Category=Events)
	FAnimNotifyEvent TransitionEnd;

	UPROPERTY(EditAnywhere, Category=Events)
	FAnimNotifyEvent TransitionInterrupt;

	/** This transition can go both directions */
	UPROPERTY(EditAnywhere, Category=Transition)
	bool Bidirectional;

#if WITH_EDITORONLY_DATA
	/** The rules for this transition may be shared with other transition nodes */
	UPROPERTY()
	bool bSharedRules;

	/** The cross-fade settings of this node may be shared */
	UPROPERTY()
	bool bSharedCrossfade;

	/** What we call this transition if we are shared ('Transition X to Y' can't be used since its used in multiple places) */
	UPROPERTY()
	FString	SharedRulesName;

	/** Shared rules guid useful when copying between different state machines */
	UPROPERTY()
	FGuid SharedRulesGuid;

	/** Color we draw in the editor as if we are shared */
	UPROPERTY()
	FLinearColor SharedColor;

	UPROPERTY()
	FString SharedCrossfadeName;

	UPROPERTY()
	FGuid SharedCrossfadeGuid;

	UPROPERTY()
	int32 SharedCrossfadeIdx;

#endif

	// Begin UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	// End UObject interface

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual bool CanDuplicateNode() const { return true; }
	virtual void PrepareForCopying() OVERRIDE;
	virtual void PostPasteNode() OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UAnimStateNodeBase interface
	virtual UEdGraph* GetBoundGraph() const OVERRIDE { return BoundGraph; }
	virtual UEdGraphPin* GetInputPin() const OVERRIDE { return Pins[0]; }
	virtual UEdGraphPin* GetOutputPin() const OVERRIDE { return Pins[1]; }
	// End UAnimStateNodeBase interface

	// @return the name of this state
	ANIMGRAPH_API FString GetStateName() const;

	ANIMGRAPH_API UAnimStateNodeBase* GetPreviousState() const;
	ANIMGRAPH_API UAnimStateNodeBase* GetNextState() const;
	ANIMGRAPH_API void CreateConnections(UAnimStateNodeBase* PreviousState, UAnimStateNodeBase* NextState);
	
	ANIMGRAPH_API bool IsBoundGraphShared();

	ANIMGRAPH_API void MakeRulesShareable(FString ShareName);
	ANIMGRAPH_API void MakeCrossfadeShareable(FString ShareName);

	ANIMGRAPH_API void UnshareRules();
	ANIMGRAPH_API void UnshareCrossade();

	ANIMGRAPH_API void UseSharedRules(const UAnimStateTransitionNode* Node);
	ANIMGRAPH_API void UseSharedCrossfade(const UAnimStateTransitionNode* Node);
	
	void CopyCrossfadeSettings(const UAnimStateTransitionNode* SrcNode);
	void PropagateCrossfadeSettings();

	ANIMGRAPH_API bool IsReverseTrans(const UAnimStateNodeBase* Node);
protected:
	void CreateBoundGraph();
	void CreateCustomTransitionGraph();

public:
	UEdGraph* GetCustomTransitionGraph() const { return CustomTransitionGraph; }
};
