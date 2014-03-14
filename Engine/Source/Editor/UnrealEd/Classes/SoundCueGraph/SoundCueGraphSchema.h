// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundCueGraphSchema.generated.h"

class USoundCue;
class USoundNodeWavePlayer;

/** Action to add a node to the graph */
USTRUCT()
struct UNREALED_API FSoundCueGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	/** Class of node we want to create */
	UPROPERTY()
	class UClass* SoundNodeClass;


	FSoundCueGraphSchemaAction_NewNode() 
		: FEdGraphSchemaAction()
		, SoundNodeClass(NULL)
	{}

	FSoundCueGraphSchemaAction_NewNode(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping) 
		, SoundNodeClass(NULL)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface

private:
	/** Connects new node to output of selected nodes */
	void ConnectToSelectedNodes(USoundNode* NewNodeclass, UEdGraph* ParentGraph) const;
};

/** Action to add nodes to the graph based on selected objects*/
USTRUCT()
struct UNREALED_API FSoundCueGraphSchemaAction_NewFromSelected : public FSoundCueGraphSchemaAction_NewNode
{
	GENERATED_USTRUCT_BODY();

	FSoundCueGraphSchemaAction_NewFromSelected() 
		: FSoundCueGraphSchemaAction_NewNode()
	{}

	FSoundCueGraphSchemaAction_NewFromSelected(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FSoundCueGraphSchemaAction_NewNode(InNodeCategory, InMenuDesc, InToolTip, InGrouping) 
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

/** Action to create new comment */
USTRUCT()
struct UNREALED_API FSoundCueGraphSchemaAction_NewComment : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	FSoundCueGraphSchemaAction_NewComment() 
		: FEdGraphSchemaAction()
	{}

	FSoundCueGraphSchemaAction_NewComment(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

/** Action to paste clipboard contents into the graph */
USTRUCT()
struct UNREALED_API FSoundCueGraphSchemaAction_Paste : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	FSoundCueGraphSchemaAction_Paste() 
		: FEdGraphSchemaAction()
	{}

	FSoundCueGraphSchemaAction_Paste(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface
};

UCLASS(MinimalAPI)
class USoundCueGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

	/** Check whether connecting these pins would cause a loop */
	bool ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const;

	/** Helper method to add items valid to the palette list */
	UNREALED_API void GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder) const;

	/** Attempts to connect the output of multiple nodes to the inputs of a single one */
	void TryConnectNodes(const TArray<USoundNode*>& OutputNodes, USoundNode* InputNode) const;

	// Begin EdGraphSchema interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const OVERRIDE;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const OVERRIDE;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const OVERRIDE;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const OVERRIDE;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const OVERRIDE;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const OVERRIDE;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const OVERRIDE;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const OVERRIDE;
	virtual void DroppedAssetsOnGraph(const TArray<class FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const OVERRIDE;
	virtual int32 GetNodeSelectionCount(const UEdGraph* Graph) const OVERRIDE;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const OVERRIDE;
	// End EdGraphSchema interface

private:
	/** Adds actions for creating every type of SoundNode */
	void GetAllSoundNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, bool bShowSelectedActions) const;
	/** Adds action for creating a comment */
	void GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph = NULL) const;

private:
	/** Generates a list of all available SoundNode classes */
	static void InitSoundNodeClasses();

	/** A list of all available SoundNode classes */
	static TArray<UClass*> SoundNodeClasses;
	/** Whether the list of SoundNode classes has been populated */
	static bool bSoundNodeClassesInitialized;
};

