// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ReferenceViewerSchema.generated.h"

UCLASS()
class UReferenceViewerSchema : public UEdGraphSchema
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphSchema interface
	virtual void GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const OVERRIDE;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const OVERRIDE;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const OVERRIDE;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) OVERRIDE;
	virtual FPinConnectionResponse MovePinLinks(UEdGraphPin& MoveFromPin, UEdGraphPin& MoveToPin) const OVERRIDE;
	virtual FPinConnectionResponse CopyPinLinks(UEdGraphPin& CopyFromPin, UEdGraphPin& CopyToPin) const OVERRIDE;
	// End of UEdGraphSchema interface

private:
	/** Constructs the sub-menu for Make Collection With Referenced Asset */
	void GetMakeCollectionWithReferencedAssetsSubMenu(class FMenuBuilder& MenuBuilder);
};

