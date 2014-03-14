// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GraphEditorCommon.h"

#include "GraphEditor.h"
#include "GraphEditorModule.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "SGraphEditorActionMenu.h"
#include "EdGraphUtilities.h"

/////////////////////////////////////////////////////
// SGraphEditorImpl

class SGraphEditorImpl : public SGraphEditor
{
public:
	SLATE_BEGIN_ARGS( SGraphEditorImpl )
		: _AdditionalCommands( TSharedPtr<FUICommandList>() )
		, _IsEditable(true)
		{}


		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, AdditionalCommands)
		SLATE_ATTRIBUTE( bool, IsEditable )
		SLATE_ARGUMENT( TSharedPtr<SWidget>, TitleBar )
		SLATE_ATTRIBUTE( bool, TitleBarEnabledOnly )
		SLATE_ATTRIBUTE( FGraphAppearanceInfo, Appearance )
		SLATE_ARGUMENT( UEdGraph*, GraphToEdit )
		SLATE_ARGUMENT( UEdGraph*, GraphToDiff )
		SLATE_ARGUMENT(SGraphEditor::FGraphEditorEvents, GraphEvents)
		SLATE_ARGUMENT(bool, AutoExpandActionMenu)
		SLATE_EVENT(FSimpleDelegate, OnNavigateHistoryBack)
		SLATE_EVENT(FSimpleDelegate, OnNavigateHistoryForward)
		SLATE_ARGUMENT(bool, ShowPIENotification)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );
private:
	TSharedPtr< FUICommandList > Commands;
	mutable TSet<UEdGraphNode*> SelectedNodeCache;

	/** The panel contains the GraphNode widgets, draws the connections, etc */
	SOverlay::FOverlaySlot* GraphPanelSlot;
	TSharedPtr<SGraphPanel> GraphPanel;
	TSharedPtr<SWidget>	TitleBar;

	UEdGraphPin* GraphPinForMenu;

	/** Do we need to refresh next tick? */
	bool bNeedsRefresh;

	/** Info on the appearance */
	TAttribute<FGraphAppearanceInfo> Appearance;

	SGraphEditor::FOnFocused OnFocused;
	SGraphEditor::FOnCreateActionMenu OnCreateActionMenu;

	TAttribute<bool> IsEditable;

	TAttribute<bool> TitleBarEnabledOnly;

	bool bAutoExpandActionMenu;

	bool bShowPIENotification;

	//FOnViewChanged	OnViewChanged;
	TWeakPtr<SGraphEditor> LockedGraph;

	/** Function to check whether PIE is active to display "Simulating" text in graph panel*/
	EVisibility PIENotification( ) const;

	/** Notification list to pass messages to editor users  */
	TSharedPtr<SNotificationList> NotificationListPtr;

	/** Callback to navigate backward in the history */
	FSimpleDelegate OnNavigateHistoryBack;
	/** Callback to navigate forward in the history */
	FSimpleDelegate OnNavigateHistoryForward;

	/** Invoked when a node is created by a keymap */
	FOnNodeSpawnedByKeymap OnNodeSpawnedByKeymap;

public:
	virtual ~SGraphEditorImpl();

	void OnClosedActionMenu();

	bool GraphEd_OnGetGraphEnabled() const;

	FActionMenuContent GraphEd_OnGetContextMenuFor( const FVector2D& NodeAddPosition, UEdGraphNode* InGraphNode, UEdGraphPin* InGraphPin, const TArray<UEdGraphPin*>& InDragFromPins );

	void GraphEd_OnPanelUpdated();

	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	void FocusLockedEditorHere();

	virtual FReply OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	// End of SWidget interface

	// SGraphEditor interface
	virtual const TSet<class UObject*>& GetSelectedNodes() const OVERRIDE;
	virtual void ClearSelectionSet() OVERRIDE;
	virtual void SetNodeSelection(UEdGraphNode* Node, bool bSelect) OVERRIDE;
	virtual void SelectAllNodes() OVERRIDE;
	virtual FVector2D GetPasteLocation() const OVERRIDE;
	virtual bool IsNodeTitleVisible( const UEdGraphNode* Node, bool bRequestRename ) OVERRIDE;
	virtual void JumpToNode( const UEdGraphNode* JumpToMe, bool bRequestRename = false ) OVERRIDE;
	virtual void JumpToPin( const UEdGraphPin* JumpToMe ) OVERRIDE;
	virtual UEdGraphPin* GetGraphPinForMenu() OVERRIDE;
	virtual void ZoomToFit(bool bOnlySelection) OVERRIDE;
	virtual bool GetBoundsForSelectedNodes( class FSlateRect& Rect, float Padding) OVERRIDE;
	virtual void NotifyGraphChanged();
	virtual TSharedPtr<SWidget> GetTitleBar() const OVERRIDE;
	virtual void SetViewLocation(const FVector2D& Location, float ZoomAmount) OVERRIDE;
	virtual void GetViewLocation(FVector2D& Location, float& ZoomAmount) OVERRIDE;
	virtual void LockToGraphEditor(TWeakPtr<SGraphEditor> Other) OVERRIDE;
	virtual void AddNotification ( FNotificationInfo& Info, bool bSuccess ) OVERRIDE;
	virtual void SetPinVisibility(SGraphEditor::EPinVisibility Visibility) OVERRIDE;
	// End of SGraphEditor interface
protected:
	//
	// COMMAND HANDLING
	// 
	bool CanReconstructNodes() const;
	bool CanBreakNodeLinks() const;
	bool CanBreakPinLinks() const;

	void ReconstructNodes();
	void BreakNodeLinks();
	void BreakPinLinks(bool bSendNodeNotification);

	// SGraphEditor interface
	virtual void OnGraphChanged( const FEdGraphEditAction& InAction ) OVERRIDE;
	// End of SGraphEditorInterface
private:
	FString GetZoomString() const;

	FSlateColor GetZoomTextColorAndOpacity() const;

	bool IsGraphEditable() const;
};

