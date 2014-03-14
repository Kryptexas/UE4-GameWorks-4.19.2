// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_StateMachineBase.generated.h"

UCLASS(Abstract)
class ANIMGRAPH_API UAnimGraphNode_StateMachineBase : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	// Editor state machine representation
	UPROPERTY()
	class UAnimationStateMachineGraph* EditorStateMachineGraph;

	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual UObject* GetJumpTargetForDoubleClick() const OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual void PostPasteNode() OVERRIDE;
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const OVERRIDE;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End of UAnimGraphNode_Base interface

	//  @return the name of this state machine
	FString GetStateMachineName();

	// Interface for derived classes to implement
	virtual FAnimNode_StateMachine& GetNode() PURE_VIRTUAL(UAnimGraphNode_StateMachineBase::GetNode, static FAnimNode_StateMachine Dummy; return Dummy;);
	// End of my interface
};
