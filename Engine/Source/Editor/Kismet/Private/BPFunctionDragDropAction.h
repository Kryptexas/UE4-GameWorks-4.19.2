// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditor.h"
#include "Editor/GraphEditor/Public/GraphEditorDragDropAction.h"

/*******************************************************************************
* FKismetDragDropAction
*******************************************************************************/

class FKismetDragDropAction : public FGraphSchemaActionDragDropAction
{
public:
	// GetTypeId is the parent: FGraphEditorDragDropAction

	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged() OVERRIDE;
	virtual FReply DroppedOnPanel( const TSharedRef< class SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	// End of FGraphSchemaActionDragDropAction

	DECLARE_DELEGATE_RetVal_ThreeParams(bool, FCanBeDroppedDelegate, TSharedPtr<FEdGraphSchemaAction> /*DropAction*/, UEdGraph* /*HoveredGraphIn*/, FText& /*ImpededReasonOut*/);

	static TSharedRef<FKismetDragDropAction> New(TSharedPtr<FEdGraphSchemaAction> InActionNode, FNodeCreationAnalytic AnalyticCallback, FCanBeDroppedDelegate CanBeDroppedDelegate)
	{
		TSharedRef<FKismetDragDropAction> Operation = MakeShareable(new FKismetDragDropAction);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FKismetDragDropAction>(Operation);
		Operation->ActionNode = InActionNode;
		Operation->AnalyticCallback = AnalyticCallback;
		Operation->CanBeDroppedDelegate = CanBeDroppedDelegate;
		Operation->Construct();
		return Operation;
	}

protected:
	bool ActionWillShowExistingNode() const;

	/** Analytic delegate to track node creation */
	FNodeCreationAnalytic AnalyticCallback;

	/** */
	FCanBeDroppedDelegate CanBeDroppedDelegate;
};

/*******************************************************************************
* FKismetFunctionDragDropAction
*******************************************************************************/

class FKismetFunctionDragDropAction : public FKismetDragDropAction
{
public:
	// GetTypeId is the parent: FGraphEditorDragDropAction

	FKismetFunctionDragDropAction();

	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged() OVERRIDE;
	virtual FReply DroppedOnPanel( const TSharedRef< class SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	// End of FGraphEditorDragDropAction

	static TSharedRef<FKismetFunctionDragDropAction> New(FName InFunctionName, UClass* InOwningClass, const FMemberReference& InCallOnMember, FNodeCreationAnalytic AnalyticCallback, FCanBeDroppedDelegate CanBeDroppedDelegate = FCanBeDroppedDelegate());

protected:
	/** Name of function being dragged */
	FName FunctionName;
	/** Class that function belongs to */
	UClass* OwningClass;
	/** Call on member reference */
	FMemberReference CallOnMember;

	/** Looks up the functions field on OwningClass using FunctionName */
	UFunction const* GetFunctionProperty() const;

	/** Constructs an action to execute, placing a function call node for the associated function */
	void GetDropAction(FGraphActionListBuilderBase::ActionGroup& DropActionOut) const;
};

/*******************************************************************************
* FKismetMacroDragDropAction
*******************************************************************************/

class FKismetMacroDragDropAction : public FKismetDragDropAction
{
public:
	// GetTypeId is the parent: FGraphEditorDragDropAction

	FKismetMacroDragDropAction();

	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged() OVERRIDE;
	virtual FReply DroppedOnPanel( const TSharedRef< class SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	// End of FGraphEditorDragDropAction

	static TSharedRef<FKismetMacroDragDropAction> New(FName InMacroName, UBlueprint* InBlueprint, UEdGraph* InMacro, FNodeCreationAnalytic AnalyticCallback);

protected:
	/** Name of macro being dragged */
	FName MacroName;
	/** Graph for the macro being dragged */
	UEdGraph* Macro;
	/** Blueprint we are operating on */
	UBlueprint* Blueprint;
};

