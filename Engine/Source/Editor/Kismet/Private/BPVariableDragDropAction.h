// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditor.h"
#include "Editor/GraphEditor/Public/GraphEditorDragDropAction.h"

/** DragDropAction class for dropping a Variable onto a graph */
class FKismetVariableDragDropAction : public FGraphEditorDragDropAction
{
public:
	// GetTypeId is the parent: FGraphEditorDragDropAction

	// FGraphEditorDragDropAction interface
	virtual void HoverTargetChanged();
	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) OVERRIDE;
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) OVERRIDE;
	virtual FReply DroppedOnPanel( const TSharedRef< class SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) OVERRIDE;
	virtual FReply DroppedOnAction(TSharedRef<struct FEdGraphSchemaAction> Action) OVERRIDE;
	virtual FReply DroppedOnCategory(FString Category) OVERRIDE;
	// End of FGraphEditorDragDropAction

	static TSharedRef<FKismetVariableDragDropAction> New(FName InVariableName, UClass* InClass, FNodeCreationAnalytic AnalyticCallback)
	{
		TSharedRef<FKismetVariableDragDropAction> Operation = MakeShareable(new FKismetVariableDragDropAction);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FKismetVariableDragDropAction>(Operation);
		Operation->VariableName = InVariableName;
		Operation->VariableSourceClass = InClass;
		Operation->AnalyticCallback = AnalyticCallback;
		Operation->Construct();
		return Operation;
	}

	/** Helper method to see if we're dragging in the same blueprint */
	bool IsFromBlueprint(class UBlueprint* InBlueprint) const
	{ 
		check(VariableSourceClass.IsValid());
		UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(VariableSourceClass.Get());
		return (InBlueprint == Blueprint);
	}

	UProperty* GetVariableProperty()
	{
		check(VariableSourceClass.IsValid());
		check(VariableName != NAME_None);
		UProperty* VariableProperty = FindField<UProperty>(VariableSourceClass.Get(), VariableName);
		check(VariableProperty != NULL);
		return VariableProperty;
	}

	/** Configure the supplied variable node based on this action's info */
	static void ConfigureVarNode(UK2Node_Variable* VarNode, FName VariableName, UClass* VariableSourceClass, UBlueprint* TargetBlueprint);

	/** Set if operation is modified by alt */
	void SetAltDrag(bool InIsAltDrag) {	bAltDrag = InIsAltDrag;}

	/** Set if operation is modified by the ctrl key */
	void SetCtrlDrag(bool InIsCtrlDrag) {bControlDrag = InIsCtrlDrag;}

protected:
	 /** Construct a FKismetVariableDragDropAction */
	FKismetVariableDragDropAction();

	/** Called when user selects to create a Getter for the variable */
	static void MakeGetter(FVector2D GraphPosition, UEdGraph* Graph, FName VariableName, UClass* VariableSourceClass);
	/** Called when user selects to create a Setter for the variable */
	static void MakeSetter(FVector2D GraphPosition, UEdGraph* Graph, FName VariableName, UClass* VariableSourceClass);
	/** Called too check if we can execute a setter on a given property */
	static bool CanExecuteMakeSetter(UEdGraph* Graph, UProperty* VariableProperty, UClass* VariableSourceClass);

	/**
	 * Test new variable type against existing links for node and get any links that will break
	 *
	 * @param	Node						The node with existing links
	 * @param	NewVariableProperty			the property for the new variable type 
	 * @param	OutBroken						All of the links which are NOT compatible with the new type
	 */
	void GetLinksThatWillBreak(UEdGraphNode* Node, UProperty* NewVariableProperty, TArray<class UEdGraphPin*>& OutBroken);

	/** Indicates if replacing the variable node, with the new property will require any links to be broken*/
	bool WillBreakLinks( UEdGraphNode* Node, UProperty* NewVariableProperty ) 
	{
		TArray<class UEdGraphPin*> BadLinks;
		GetLinksThatWillBreak(Node,NewVariableProperty,BadLinks);
		return BadLinks.Num() > 0;
	}

protected:
	/** Name of variable being dragged */
	FName VariableName;
	/** Class this variable belongs to */
	TWeakObjectPtr<UClass> VariableSourceClass;
	/** Was ctrl held down at start of drag */
	bool bControlDrag;
	/** Was alt held down at the start of drag */
	bool bAltDrag;
	/** Analytic delegate to track node creation */
	FNodeCreationAnalytic AnalyticCallback;
};