// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

class SGraphActionMenu;

/////////////////////////////////////////////////////////////////////////////////////////////////

class SBlueprintActionMenu : public SBorder
{
public:
	/** Delegate for the OnCloseReason event which is always raised when the SBlueprintActionMenu closes */
	DECLARE_DELEGATE_ThreeParams( FClosedReason, bool /*bActionExecuted*/, bool /*bContextSensitiveChecked*/, bool /*bGraphPinContext*/);

	SLATE_BEGIN_ARGS( SBlueprintActionMenu )
		: _GraphObj( static_cast<UEdGraph*>(NULL) )
		, _NewNodePosition( FVector2D::ZeroVector )
		, _AutoExpandActionMenu( false )
	{}

		SLATE_ARGUMENT( UEdGraph*, GraphObj )
		SLATE_ARGUMENT( FVector2D, NewNodePosition )
		SLATE_ARGUMENT( TArray<UEdGraphPin*>, DraggedFromPins )
		SLATE_ARGUMENT( SGraphEditor::FActionMenuClosed, OnClosedCallback )
		SLATE_ARGUMENT( bool, AutoExpandActionMenu )
		SLATE_EVENT( FClosedReason, OnCloseReason )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InEditor );

	~SBlueprintActionMenu();

	TSharedRef<SEditableTextBox> GetFilterTextBox();

protected:
	/** UI Callback functions */
	EVisibility GetTypeImageVisibility() const;
	FText GetSearchContextDesc() const;
	void OnContextToggleChanged(ESlateCheckBoxState::Type CheckState);
	ESlateCheckBoxState::Type ContextToggleIsChecked() const;

	void OnActionSelected( const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedAction );

	TSharedRef<SWidget> OnCreateWidgetForAction(struct FCreateWidgetForActionData* const InCreateData);

	/** Callback used to populate all actions list in SGraphActionMenu */
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);

	/** Functioin to try to insert a promote to variable entry if it is possible to do so. */
	void TryInsertPromoteToVariable( FGraphContextMenuBuilder &ContextMenuBuilder, FGraphActionListBuilderBase &OutAllActions );

private:
	UEdGraph* GraphObj;
	TArray<UEdGraphPin*> DraggedFromPins;
	FVector2D NewNodePosition;
	bool bAutoExpandActionMenu;

	SGraphEditor::FActionMenuClosed OnClosedCallback;
	FClosedReason OnCloseReasonCallback;

	TSharedPtr<SGraphActionMenu> GraphActionMenu;
	TWeakPtr<FBlueprintEditor> EditorPtr;

	bool bActionExecuted;
};

