// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Timeline.generated.h"

UCLASS(MinimalAPI)
class UK2Node_Timeline : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** The name of the timeline. Used to name ONLY the member variable (Component). To obtain the name of timeline template use UTimelineTemplate::TimelineVariableNameToTemplateName */
	UPROPERTY()
	FName TimelineName;

	/** If the timeline is set to autoplay */
	UPROPERTY(Transient)
	uint32 bAutoPlay:1;

	/** Unique ID for the template we use, required to indentify the timeline after a paste */
	UPROPERTY()
	FGuid TimelineGuid;

	/** If the timeline is set to loop */
	UPROPERTY(Transient)
	uint32 bLoop:1;

	/** If the timeline is set to loop */
	UPROPERTY(Transient)
	uint32 bReplicated:1;

#if WITH_EDITOR
	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual void PostPasteNode() OVERRIDE;
	virtual void PrepareForCopying() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const OVERRIDE;
	virtual void FindDiffs(class UEdGraphNode* OtherNode, struct FDiffResults& Results )  OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Timeline_16x"); }
	// End UEdGraphNode interface.

	// Begin UK2Node interface.
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface.

	/** Get the 'play' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetPlayPin() const;

	/** Get the 'play from start' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetPlayFromStartPin() const;

	/** Get the 'stop' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetStopPin() const;
	
	/** Get the 'update' output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetUpdatePin() const;

	/** Get the 'reverse' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetReversePin() const;

	/** Get the 'reverse from end' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetReverseFromEndPin() const;

	/** Get the 'finished' output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetFinishedPin() const;

	/** Get the 'newtime' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetNewTimePin() const;

	/** Get the 'setnewtime' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetSetNewTimePin() const;

	/** Get the 'Direction' output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetDirectionPin() const;

	/** Get the 'Direction' output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetTrackPin(const FName TrackName) const;

	/** Try to rename the timeline
	 * @param NewName	The newname for the timeline
	 * @return bool	true if node was successfully renamed, false otherwise
	 */
	BLUEPRINTGRAPH_API bool RenameTimeline(const FString& NewName);

private:
	void ExpandForPin(UEdGraphPin* TimelinePin, const FName PropertyName, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

#endif
};

