// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CustomEvent : public UK2Node_Event
{
	GENERATED_UCLASS_BODY()

	virtual bool IsEditable() const OVERRIDE;

	// Begin UEdGraphNode interface
	virtual void ReconstructNode() OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.CustomEvent_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	BLUEPRINTGRAPH_API virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	// End UK2Node interface

	// Begin UK2Node_EditablePinBase interface
	virtual UEdGraphPin* CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo) OVERRIDE;
	// Begin UK2Node_EditablePinBase interface

	virtual bool IsUsedByAuthorityOnlyDelegate() const OVERRIDE;

	// Rename this custom event to have unique name
	BLUEPRINTGRAPH_API void RenameCustomEventCloseToName(int32 StartIndex = 1);

	BLUEPRINTGRAPH_API static UK2Node_CustomEvent* CreateFromFunction(FVector2D GraphPosition, UEdGraph* ParentGraph, const FString& Name, const UFunction* Function, bool bSelectNewNode = true);

	/**
	 * Discernible from the base UK2Node_Event's bOverrideFunction field. This 
	 * checks to see if this UK2Node_CustomEvent overrides another CustomEvent
	 * declared in a parent blueprint.
	 * 
	 * @return True if this CustomEvent's name matches another CustomEvent's name, declared in a parent (false if not).
	 */
	BLUEPRINTGRAPH_API bool IsOverride() const;

	/**
	 * If a CustomEvent overrides another CustomEvent, then it inherits the 
	 * super's net flags. This method does that work for you, either returning
	 * the super function's flags, or this node's flags (if it's not an override).
	 * 
	 * @return If this CustomEvent is an override, then this is the super's net flags, otherwise it's from the FunctionFlags set on this node.
	 */
	BLUEPRINTGRAPH_API uint32 GetNetFlags() const;
};



