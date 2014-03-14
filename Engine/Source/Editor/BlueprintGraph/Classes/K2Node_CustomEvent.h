// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_CustomEvent.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CustomEvent : public UK2Node_Event
{
	GENERATED_UCLASS_BODY()

	virtual bool IsEditable() const OVERRIDE;

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void ReconstructNode() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.CustomEvent_16x"); }
	// End UEdGraphNode interface

	// Begin K2Node_EditablePinBase interface
	virtual UEdGraphPin* CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo) OVERRIDE;
	// Begin K2Node_EditablePinBase interface

	virtual bool IsUsedByAuthorityOnlyDelegate() const OVERRIDE;

	// Rename this custom event to have unique name
	BLUEPRINTGRAPH_API void RenameCustomEventCloseToName(int32 StartIndex = 1);

	BLUEPRINTGRAPH_API static UK2Node_CustomEvent* CreateFromFunction(FVector2D GraphPosition, UEdGraph* ParentGraph, const FString& Name, const UFunction* Function, bool bSelectNewNode = true);
#endif
};



