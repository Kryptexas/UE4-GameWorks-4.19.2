// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundClassGraphSchema.generated.h"

/** Action to add a node to the graph */
USTRUCT()
struct UNREALED_API FSoundClassGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	// Simple type info
	static FString StaticGetTypeId() {static FString Type = TEXT("FSoundClassGraphSchemaAction_NewNode"); return Type;}

	FSoundClassGraphSchemaAction_NewNode() 
		: FEdGraphSchemaAction()
		, NewSoundClassName(TEXT("ClassName"))
	{}

	FSoundClassGraphSchemaAction_NewNode(const FString& InNodeCategory, const FString& InMenuDesc, const FString& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
		, NewSoundClassName(TEXT("ClassName"))
	{}

	// FEdGraphSchemaAction interface
	virtual FString GetTypeId() const OVERRIDE { return StaticGetTypeId(); } 
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE;
	// End of FEdGraphSchemaAction interface

	/** Name for the new SoundClass */
	FString NewSoundClassName;
};

UCLASS(MinimalAPI)
class USoundClassGraphSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

	/** Check whether connecting these pins would cause a loop */
	bool ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const;
	/** Get menu for breaking links to specific nodes*/
	void GetBreakLinkToSubMenuActions(class FMenuBuilder& MenuBuilder, class UEdGraphPin* InGraphPin);

	// Begin EdGraphSchema interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const OVERRIDE;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const OVERRIDE;
	virtual bool TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const OVERRIDE;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const OVERRIDE;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const OVERRIDE;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const OVERRIDE;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const OVERRIDE;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) OVERRIDE;
	virtual void DroppedAssetsOnGraph(const TArray<class FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const OVERRIDE;
	// End EdGraphSchema interface
};

