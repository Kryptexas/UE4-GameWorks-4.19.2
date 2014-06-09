// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SAnimNotifyPanel.h"
#include "ScopedTransaction.h"
#include "SCurveEditor.h"
#include "Editor/KismetWidgets/Public/SScrubWidget.h"
#include "AssetRegistryModule.h"
#include "Editor/UnrealEd/Public/AssetNotifications.h"
#include "AssetSelection.h"

// Track Panel drawing
const float NotificationTrackHeight = 20.0f;

// AnimNotify Drawing
const float NotifyHeightOffset = 0.f;
const float NotifyHeight = NotificationTrackHeight + 3.f;
const FVector2D ScrubHandleSize(8.f, NotifyHeight);
const FVector2D AlignmentMarkerSize(8.f, NotifyHeight);
const FVector2D TextBorderSize(1.f, 1.f);

#define LOCTEXT_NAMESPACE "AnimNotifyPanel"

DECLARE_DELEGATE_OneParam( FOnDeleteNotify, struct FAnimNotifyEvent*)
DECLARE_DELEGATE_RetVal_FourParams( FReply, FOnNotifyNodeDragStarted, TSharedRef<SAnimNotifyNode>, const FVector2D&, const FVector2D&, const bool)
DECLARE_DELEGATE_RetVal_FiveParams(FReply, FOnNotifyNodesDragStarted, TArray<TSharedPtr<SAnimNotifyNode>>, TSharedRef<SWidget>, const FVector2D&, const FVector2D&, const bool)
DECLARE_DELEGATE_RetVal( float, FOnGetDraggedNodePos )
DECLARE_DELEGATE_TwoParams( FPanTrackRequest, int32, FVector2D)
DECLARE_DELEGATE_FourParams(FPasteNotifies, SAnimNotifyTrack*, float, ENotifyPasteMode::Type, ENotifyPasteMultipleMode::Type)

class FNotifyDragDropOp;


//////////////////////////////////////////////////////////////////////////
// SAnimNotifyNode

class SAnimNotifyNode : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS( SAnimNotifyNode )
		: _Sequence()
		, _AnimNotify()
		, _OnNodeDragStarted()
		, _OnUpdatePanel()
		, _PanTrackRequest()
		, _ViewInputMin()
		, _ViewInputMax()
		, _MarkerBars()
	{
	}
	SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence )
	SLATE_ARGUMENT( FAnimNotifyEvent *, AnimNotify )
	SLATE_EVENT( FOnNotifyNodeDragStarted, OnNodeDragStarted )
	SLATE_EVENT( FOnUpdatePanel, OnUpdatePanel )
	SLATE_EVENT( FPanTrackRequest, PanTrackRequest )
	SLATE_ATTRIBUTE( float, ViewInputMin )
	SLATE_ATTRIBUTE( float, ViewInputMax )
	SLATE_ATTRIBUTE( TArray<FTrackMarkerBar>, MarkerBars)
	SLATE_END_ARGS()

	void Construct(const FArguments& Declaration);

	// SWidget interface
	virtual FReply OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;
	
	// End of SWidget interface

	// SNodePanel::SNode interface
	void UpdateSizeAndPosition(const FGeometry& AllottedGeometry);
	FVector2D GetWidgetPosition() const;
	FVector2D GetNotifyPosition() const;
	FVector2D GetNotifyPositionOffset() const;
	FVector2D GetSize() const;
	bool HitTest(const FGeometry& AllottedGeometry, FVector2D MouseLocalPose) const;

	// Extra hit testing to decide whether or not the duration handles were hit on a state node
	ENotifyStateHandleHit::Type DurationHandleHitTest(const FVector2D& CursorScreenPosition) const;

	UObject* GetObjectBeingDisplayed() const;
	// End of SNodePanel::SNode

	virtual FVector2D ComputeDesiredSize() const  OVERRIDE;
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;

	/** Helpers to draw scrub handles and snap offsets */
	void DrawHandleOffset( const float& Offset, const float& HandleCentre, FSlateWindowElementList& OutDrawElements, int32 MarkerLayer, const FGeometry &AllottedGeometry, const FSlateRect& MyClippingRect ) const;
	void DrawScrubHandle( float ScrubHandleCentre, FSlateWindowElementList& OutDrawElements, int32 ScrubHandleID, const FGeometry &AllottedGeometry, const FSlateRect& MyClippingRect, FLinearColor NodeColour ) const;

	FLinearColor GetNotifyColor() const;
	FText GetNotifyText() const;
	
	/** The notify that is being visualized by this node */
	FAnimNotifyEvent* NotifyEvent;

	void DropCancelled();

	/** Returns the size of this notifies duration in screen space */
	float GetDurationSize() { return NotifyDurationSizeX;}

	/** Sets the position the mouse was at when this node was last hit */
	void SetLastMouseDownPosition(const FVector2D& CursorPosition) {LastMouseDownPosition = CursorPosition;}

	/** The minimum possible duration that a notify state can have */
	static const float MinimumStateDuration;

	const FVector2D& GetScreenPosition() const
	{
		return ScreenPosition;
	}

	const float GetLastSnappedTime() const
	{
		return LastSnappedTime;
	}

	void ClearLastSnappedTime()
	{
		LastSnappedTime = -1.0f;
	}

	void SetLastSnappedTime(float NewSnapTime)
	{
		LastSnappedTime = NewSnapTime;
	}

private:
	FText GetNodeTooltip() const;

	/** Detects any overflow on the anim notify track and requests a track pan */
	float HandleOverflowPan( const FVector2D& ScreenCursorPos, float TrackScreenSpaceXPosition );

	/** Finds a snap position if possible for the provided scrub handle, if it is not possible, returns -1.0f */
	float GetScrubHandleSnapPosition(float NotifyLocalX, ENotifyStateHandleHit::Type HandleToCheck);

	/** The sequence that the AnimNotifyEvent for Notify lives in */
	UAnimSequenceBase* Sequence;
	FSlateFontInfo Font;

	TAttribute<float>			ViewInputMin;
	TAttribute<float>			ViewInputMax;
	FVector2D					CachedAllotedGeometrySize;
	FVector2D					ScreenPosition;
	float						LastSnappedTime;

	bool						bDrawTooltipToRight;
	bool						bBeingDragged;
	bool						bSelected;

	/** The scrub handle currently being dragged, if any */
	ENotifyStateHandleHit::Type CurrentDragHandle;
	
	float						NotifyTimePositionX;
	float						NotifyDurationSizeX;
	float						NotifyScrubHandleCentre;
	
	float						WidgetX;
	FVector2D					WidgetSize;
	
	FVector2D					TextSize;
	float						LabelWidth;

	/** Last position the user clicked in the widget */
	FVector2D					LastMouseDownPosition;

	/** Delegate that is called when the user initiates dragging */
	FOnNotifyNodeDragStarted	OnNodeDragStarted;

	/** Delegate to pan the track, needed if the markers are dragged out of the track */
	FPanTrackRequest			PanTrackRequest;

	/** Marker bars for snapping to when dragging the markers in a state notify node */
	TAttribute<TArray<FTrackMarkerBar>> MarkerBars;

	friend class SAnimNotifyTrack;
};

//////////////////////////////////////////////////////////////////////////
// SAnimNotifyTrack

class SAnimNotifyTrack : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SAnimNotifyTrack )
		: _Persona()
		, _Sequence(NULL)
		, _ViewInputMin()
		, _ViewInputMax()
		, _OnSelectionChanged()
		, _OnUpdatePanel()
		, _TrackColor(FLinearColor::White)
		, _TrackIndex()
		, _OnRequestTrackPan()
		, _OnRequestOffsetRefresh()
		, _OnDeleteNotify()
		, _OnDeselectAllNotifies()
		, _OnCopyNotifies()
		, _OnPasteNotifies()
		{}

		SLATE_ARGUMENT( TSharedPtr<FPersona>,	Persona )
		SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence )
		SLATE_ARGUMENT( TArray<FAnimNotifyEvent *>, AnimNotifies )
		SLATE_ATTRIBUTE( float, ViewInputMin )
		SLATE_ATTRIBUTE( float, ViewInputMax )
		SLATE_ATTRIBUTE( TArray<FTrackMarkerBar>, MarkerBars)
		SLATE_ARGUMENT( int32, TrackIndex )
		SLATE_ARGUMENT( FLinearColor, TrackColor )
		SLATE_EVENT(FOnTrackSelectionChanged, OnSelectionChanged)
		SLATE_EVENT( FOnUpdatePanel, OnUpdatePanel )
		SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
		SLATE_EVENT( FOnGetDraggedNodePos, OnGetDraggedNodePos )
		SLATE_EVENT( FOnNotifyNodesDragStarted, OnNodeDragStarted )
		SLATE_EVENT( FPanTrackRequest, OnRequestTrackPan )
		SLATE_EVENT( FRefreshOffsetsRequest, OnRequestOffsetRefresh )
		SLATE_EVENT( FDeleteNotify, OnDeleteNotify )
		SLATE_EVENT( FDeselectAllNotifies, OnDeselectAllNotifies)
		SLATE_EVENT( FCopyNotifies, OnCopyNotifies )
		SLATE_EVENT( FPasteNotifies, OnPasteNotifies )
		SLATE_END_ARGS()
public:

	/** Type used for list widget of tracks */
	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) {UpdateCachedGeometry(AllottedGeometry);}
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;
	// End of SWidget interface

	/** Returns the cached rendering geometry of this track */
	const FGeometry& GetCachedGeometry() const { return CachedGeometry; }

	FTrackScaleInfo GetCachedScaleInfo() const { return FTrackScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0.f, 0.f, CachedGeometry.Size); }

	/** Updates sequences when a notify node has been successfully dragged to a new position */
	void HandleNodeDrop(TSharedPtr<SAnimNotifyNode> Node);

	// Number of nodes in the track currently selected
	int32 GetNumSelectedNodes() const { return SelectedNotifyIndices.Num(); }

	// Index of the track in the notify panel
	int32 GetTrackIndex() const { return TrackIndex; }

	// Time at the position of the last mouseclick
	float GetLastClickedTime() const { return LastClickedTime; }

	// Removes the node widgets from the track and adds them to the provided Array
	void DisconnectSelectedNodesForDrag(TArray<TSharedPtr<SAnimNotifyNode>>& DragNodes);

	// Adds our current selection to the provided set
	void AppendSelectionToSet(FGraphPanelSelectionSet& SelectionSet);
	// Adds our current selection to the provided array
	void AppendSelectionToArray(TArray<const FAnimNotifyEvent*>& Selection) const;
	// Gets the currently selected SAnimNotifyNode instances
	void AppendSelectedNodeWidgetsToArray(TArray<TSharedPtr<SAnimNotifyNode>>& NodeArray) const;

	/**
	* Deselects all currently selected notify nodes
	* @param bUpdateSelectionSet - Whether we should report a selection change to the panel
	*/
	void DeselectAllNotifyNodes(bool bUpdateSelectionSet = true);

	// get Property Data of one element (NotifyIndex) from Notifies property of Sequence
	static uint8* FindNotifyPropertyData(UAnimSequenceBase* Sequence, int32 NotifyIndex, UArrayProperty*& ArrayProperty);

	// Paste a single Notify into this track from an exported string
	void PasteSingleNotify(FString& NotifyString, float PasteTime);

protected:
	void CreateCommands();

	// Build up a "New Notify..." menu
	void FillNewNotifyMenu(FMenuBuilder& MenuBuilder);
	void FillNewNotifyStateMenu(FMenuBuilder& MenuBuilder);
	void CreateNewBlueprintNotifyAtCursor(FString NewNotifyName, FString BlueprintPath);
	void CreateNewNotifyAtCursor(FString NewNotifyName, UClass* NotifyClass);
	void OnNewNotifyClicked();
	void AddNewNotify(const FText& NewNotifyName, ETextCommit::Type CommitInfo);
	void OnSetTriggerWeightNotifyClicked(int32 NotifyIndex);
	void SetTriggerWeight(const FText& TriggerWeight, ETextCommit::Type CommitInfo, int32 NotifyIndex);
	
	// Whether we have one node selected
	bool IsSingleNodeSelected();
	// Checks the clipboard for an anim notify buffer, and returns whether there's only one notify
	bool IsSingleNodeInClipboard();

	void OnSetDurationNotifyClicked(int32 NotifyIndex);
	void SetDuration(const FText& DurationText, ETextCommit::Type CommitInfo, int32 NotifyIndex);

	/** Function to copy anim notify event */
	void OnCopyNotifyClicked(int32 NotifyIndex);

	/** Function to check whether it is possible to paste anim notify event */
	bool CanPasteAnimNotify() const;

	/** Reads the contents of the clipboard for a notify to paste.
	 *  If successful OutBuffer points to the start of the notify object's data */
	bool ReadNotifyPasteHeader(FString& OutPropertyString, const TCHAR*& OutBuffer, float& OutOriginalTime, float& OutOriginalLength) const;

	/** Handler for context menu paste command */
	void OnPasteNotifyClicked(ENotifyPasteMode::Type PasteMode, ENotifyPasteMultipleMode::Type MultiplePasteType = ENotifyPasteMultipleMode::Absolute);

	/** Handler for popup window asking the user for a paste time */
	void OnPasteNotifyTimeSet(const FText& TimeText, ETextCommit::Type CommitInfo);

	/** Function to paste a previously copied notify */
	void OnPasteNotify(float TimeToPasteAt, ENotifyPasteMultipleMode::Type MultiplePasteType = ENotifyPasteMultipleMode::Absolute);

	/** Provides direct access to the notify menu from the context menu */
	void OnManageNotifies();

	/**
	 * Update the nodes to match the data that the panel is observing
	 */
	void Update();

	/**
	 * Selects a notify node in the graph. Supports multi selection
	 * @param NotifyIndex - Index of the notify node to select.
	 * @param Append - Whether to append to to current selection or start a new one.
	 */
	void SelectNotifyNode(int32 NotifyIndex, bool Append);
	
	/**
	 * Toggles the selection status of a notify node, for example when
	 * Control is held when clicking.
	 * @param NotifyIndex - Index of the notify to toggle the selection status of
	 */
	void ToggleNotifyNodeSelectionStatus(int32 NotifyIndex);

	/**
	 * Deselects requested notify node
	 * @param NotifyIndex - Index of the notify node to deselect
	 */
	void DeselectNotifyNode(int32 NotifyIndex);

	int32 GetHitNotifyNode(const FGeometry& MyGeometry, const FVector2D& Position);

	TSharedPtr<SWidget> SummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;

	float CalculateTime( const FGeometry& MyGeometry, FVector2D NodePos, bool bInputIsAbsolute = true );

	// Handler that is called when the user starts dragging a node
	FReply OnNotifyNodeDragStarted( TSharedRef<SAnimNotifyNode> NotifyNode, const FVector2D& ScreenCursorPos, const FVector2D& ScreenNodePosition, const bool bDragOnMarker, int32 NotifyIndex );

private:

	// Store the tracks geometry for later use
	void UpdateCachedGeometry(const FGeometry& InGeometry) {CachedGeometry = InGeometry;}

	// Returns the padding needed to render the notify in the correct track position
	FMargin GetNotifyTrackPadding(int32 NotifyIndex) const
	{
		float LeftMargin = NotifyNodes[NotifyIndex]->GetWidgetPosition().X;
		float RightMargin = CachedGeometry.Size.X - LeftMargin - NotifyNodes[NotifyIndex]->GetSize().X;
		return FMargin(LeftMargin, 0, RightMargin, 0);
	}

	// Builds a UObject selection set and calls the OnSelectionChanged delegate
	void SendSelectionChanged()
	{
		OnSelectionChanged.ExecuteIfBound();
	}

protected:
	TSharedPtr<FUICommandList> AnimSequenceEditorActions;

	float LastClickedTime;

	class UAnimSequenceBase*				Sequence; // need for menu generation of anim notifies - 
	TArray<TSharedPtr<SAnimNotifyNode>>		NotifyNodes;
	TArray<FAnimNotifyEvent*>				AnimNotifies;
	TAttribute<float>						ViewInputMin;
	TAttribute<float>						ViewInputMax;
	TAttribute<FLinearColor>				TrackColor;
	int32									TrackIndex;
	FOnTrackSelectionChanged				OnSelectionChanged;
	FOnUpdatePanel							OnUpdatePanel;
	FOnGetScrubValue						OnGetScrubValue;
	FOnGetDraggedNodePos					OnGetDraggedNodePos;
	FOnNotifyNodesDragStarted				OnNodeDragStarted;
	FPanTrackRequest						OnRequestTrackPan;
	FDeselectAllNotifies					OnDeselectAllNotifies;
	FCopyNotifies							OnCopyNotifies;
	FPasteNotifies							OnPasteNotifies;

	/** Delegate to call when offsets should be refreshed in a montage */
	FRefreshOffsetsRequest					OnRequestRefreshOffsets;

	/** Delegate to call when deleting notifies */
	FDeleteNotify							OnDeleteNotify;

	TSharedPtr<SBorder>						TrackArea;
	TWeakPtr<FPersona>						PersonaPtr;

	/** Cache the SOverlay used to store all this tracks nodes */
	TSharedPtr<SOverlay> NodeSlots;

	/** Cached for drag drop handling code */
	FGeometry CachedGeometry;

	/** Attribute for accessing any marker positions we have to draw */
	TAttribute<TArray<FTrackMarkerBar>>		MarkerBars;

	/** Nodes that are currently selected */
	TArray<int32> SelectedNotifyIndices;
};

//////////////////////////////////////////////////////////////////////////
// 

/** Widget for drawing a single track */
class SNotifyEdTrack : public SCompoundWidget
{
private:
	/** Index of Track in Sequence **/
	int32									TrackIndex;

	/** Anim Sequence **/
	class UAnimSequenceBase*				Sequence;

	/** Pointer to notify panel for drawing*/
	TWeakPtr<SAnimNotifyPanel>			AnimPanelPtr;
public:
	SLATE_BEGIN_ARGS( SNotifyEdTrack )
		: _TrackIndex(INDEX_NONE)
		, _Sequence()
		, _ViewInputMin()
		, _ViewInputMax()
		, _OnSelectionChanged()
		, _AnimNotifyPanel()
		, _WidgetWidth()
		, _OnUpdatePanel()
		, _OnDeleteNotify()
		, _OnDeselectAllNotifies()
		, _OnCopyNotifies()
	{}
	SLATE_ARGUMENT( int32, TrackIndex )
	SLATE_ARGUMENT( TSharedPtr<SAnimNotifyPanel>, AnimNotifyPanel)
	SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence )
	SLATE_ARGUMENT( float, WidgetWidth )
	SLATE_ATTRIBUTE( float, ViewInputMin )
	SLATE_ATTRIBUTE( float, ViewInputMax )
	SLATE_ATTRIBUTE( TArray<FTrackMarkerBar>, MarkerBars)
	SLATE_EVENT( FOnTrackSelectionChanged, OnSelectionChanged)
	SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
	SLATE_EVENT( FOnGetDraggedNodePos, OnGetDraggedNodePos )
	SLATE_EVENT( FOnUpdatePanel, OnUpdatePanel )
	SLATE_EVENT( FOnNotifyNodesDragStarted, OnNodeDragStarted )
	SLATE_EVENT( FRefreshOffsetsRequest, OnRequestRefreshOffsets )
	SLATE_EVENT( FDeleteNotify, OnDeleteNotify )
	SLATE_EVENT( FDeselectAllNotifies, OnDeselectAllNotifies)
	SLATE_EVENT( FCopyNotifies, OnCopyNotifies )
	SLATE_EVENT( FPasteNotifies, OnPasteNotifies )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	bool CanDeleteTrack();

	/** Pointer to actual anim notify track */
	TSharedPtr<class SAnimNotifyTrack>	NotifyTrack;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FNotifyDragDropOp : public FDragDropOperation
{
public:
	FNotifyDragDropOp(float& InCurrentDragXPosition) : 
		CurrentDragXPosition(InCurrentDragXPosition), 
		SnapTime(-1.f),
		SelectionTimeLength(0.0f)
	{
	}

	struct FTrackClampInfo
	{
		int32 TrackPos;
		int32 TrackSnapTestPos;
		float TrackMax;
		float TrackMin;
		float TrackWidth;
		TSharedPtr<SAnimNotifyTrack> NotifyTrack;
	};

	DRAG_DROP_OPERATOR_TYPE(FNotifyDragDropOp, FDragDropOperation)

	virtual void OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		if ( bDropWasHandled == false )
		{
			for(TSharedPtr<SAnimNotifyNode> Node : SelectedNodes)
			{
				const FTrackClampInfo& ClampInfo = GetTrackClampInfo(Node->GetScreenPosition());
				ClampInfo.NotifyTrack->HandleNodeDrop(Node);
				Node->DropCancelled();
			}
		}
		
		FDragDropOperation::OnDrop(bDropWasHandled, MouseEvent);
	}

	virtual void OnDragged( const class FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		// Reset snapped node pointer
		SnappedNode = NULL;

		NodeGroupPosition = DragDropEvent.GetScreenSpacePosition() + DragOffset;

		FVector2D SelectionBeginPosition = NodeGroupPosition + SelectedNodes[0]->GetNotifyPositionOffset();
		
		FTrackClampInfo* SelectionPositionClampInfo = &GetTrackClampInfo(DragDropEvent.GetScreenSpacePosition());
		if(SelectionPositionClampInfo->NotifyTrack->GetTrackIndex() < TrackSpan)
		{
			// Our selection has moved off the bottom of the notify panel, adjust the clamping information to keep it on the panel
			SelectionPositionClampInfo = &ClampInfos[ClampInfos.Num() - TrackSpan - 1];
		}

		const FGeometry& TrackGeom = SelectionPositionClampInfo->NotifyTrack->GetCachedGeometry();
		const FTrackScaleInfo& TrackScaleInfo = SelectionPositionClampInfo->NotifyTrack->GetCachedScaleInfo();
		
		// Tracks the movement amount to apply to the selection due to a snap.
		float SnapMovement= 0.0f;
		// Clamp the selection into the track
		const float SelectionLocalLength = TrackScaleInfo.PixelsPerInput * SelectionTimeLength;
		const float ClampedEnd = FMath::Clamp(SelectionBeginPosition.X + NodeGroupSize.X, SelectionPositionClampInfo->TrackMin, SelectionPositionClampInfo->TrackMax);
		const float ClampedBegin = FMath::Clamp(SelectionBeginPosition.X, SelectionPositionClampInfo->TrackMin, SelectionPositionClampInfo->TrackMax);
		if(ClampedBegin > SelectionBeginPosition.X)
		{
			SelectionBeginPosition.X = ClampedBegin;
		}
		else if(ClampedEnd < SelectionBeginPosition.X + SelectionLocalLength)
		{
			SelectionBeginPosition.X = ClampedEnd - SelectionLocalLength;
		}

		// Handle node snaps
		for(int32 NodeIdx = 0 ; NodeIdx < SelectedNodes.Num() ; ++NodeIdx)
		{
			TSharedPtr<SAnimNotifyNode> CurrentNode = SelectedNodes[NodeIdx];
			FAnimNotifyEvent* CurrentEvent = CurrentNode->NotifyEvent;
			
			// Clear off any snap time currently stored
			CurrentNode->ClearLastSnappedTime();

			const FTrackClampInfo& NodeClamp = GetTrackClampInfo(CurrentNode->GetScreenPosition());

			FVector2D EventPosition = SelectionBeginPosition + FVector2D(TrackScaleInfo.PixelsPerInput * NodeTimeOffsets[NodeIdx], 0.0f);

			// Look for a snap on the first scrub handle
			FVector2D TrackNodePos = TrackGeom.AbsoluteToLocal(EventPosition);
			const FVector2D OriginalNodePosition = TrackNodePos;

			float SnapX = GetSnapPosition(NodeClamp, TrackNodePos.X);
			if(SnapX >= 0.f)
			{
				EAnimEventTriggerOffsets::Type Offset = (SnapX < TrackNodePos.X) ? EAnimEventTriggerOffsets::OffsetAfter : EAnimEventTriggerOffsets::OffsetBefore;
				CurrentEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);
				CurrentNode->SetLastSnappedTime(TrackScaleInfo.LocalXToInput(SnapX));

				if(SnapMovement == 0.0f)
				{
					SnapMovement = SnapX - TrackNodePos.X;
					TrackNodePos.X = SnapX;
					SnapTime = TrackScaleInfo.LocalXToInput(SnapX);
					SnappedNode = CurrentNode;
				}
				EventPosition = NodeClamp.NotifyTrack->GetCachedGeometry().LocalToAbsolute(TrackNodePos);
			}
			else
			{
				CurrentEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
			}

			// Always clamp the Y to the current track
			SelectionBeginPosition.Y = SelectionPositionClampInfo->TrackPos;

			if(CurrentNode.IsValid() && CurrentEvent->Duration > 0)
			{
				// If we didn't snap the beginning of the node, attempt to snap the end
				if(SnapX == -1.0f)
				{
					FVector2D TrackNodeEndPos = TrackNodePos + CurrentNode->GetDurationSize();
					SnapX = GetSnapPosition(*SelectionPositionClampInfo, TrackNodeEndPos.X);

					// Only attempt to snap if the node will fit on the track
					if(SnapX >= CurrentNode->GetDurationSize())
					{
						EAnimEventTriggerOffsets::Type Offset = (SnapX < TrackNodeEndPos.X) ? EAnimEventTriggerOffsets::OffsetAfter : EAnimEventTriggerOffsets::OffsetBefore;
						CurrentEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);
						
						if(SnapMovement == 0.0f)
						{
							SnapMovement = SnapX - TrackNodeEndPos.X;
							SnapTime = TrackScaleInfo.LocalXToInput(SnapX) - CurrentEvent->Duration;
							SnappedNode = CurrentNode;
						}
					}
					else
					{
						// Remove any trigger time if we can't fit the node in.
						CurrentEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
					}
				}
			}
		}

		SelectionBeginPosition.X += SnapMovement;

		CurrentDragXPosition = SelectionPositionClampInfo->NotifyTrack->GetCachedGeometry().AbsoluteToLocal(FVector2D(SelectionBeginPosition.X,0.0f)).X;

		CursorDecoratorWindow->MoveWindowTo(SelectionBeginPosition - SelectedNodes[0]->GetNotifyPositionOffset());
		NodeGroupPosition = SelectionBeginPosition;

		//scroll view
		float MouseXPos = DragDropEvent.GetScreenSpacePosition().X;
		if(MouseXPos < SelectionPositionClampInfo->TrackMin)
		{
			float ScreenDelta = MouseXPos - SelectionPositionClampInfo->TrackMin;
			RequestTrackPan.Execute(ScreenDelta, FVector2D(SelectionPositionClampInfo->TrackWidth, 1.f));
		}
		else if(MouseXPos > SelectionPositionClampInfo->TrackMax)
		{
			float ScreenDelta = MouseXPos - SelectionPositionClampInfo->TrackMax;
			RequestTrackPan.Execute(ScreenDelta, FVector2D(SelectionPositionClampInfo->TrackWidth, 1.f));
		}
	}

	float GetSnapPosition(const FTrackClampInfo& ClampInfo, float WidgetSpaceNotifyPosition)
	{
		const FTrackScaleInfo& ScaleInfo = ClampInfo.NotifyTrack->GetCachedScaleInfo();

		const float MaxSnapDist = 5.f;

		float CurrentMinSnapDest = MaxSnapDist;
		float SnapPosition = -1.f;

		if(MarkerBars.IsBound())
		{
			const TArray<FTrackMarkerBar>& Bars = MarkerBars.Get();
			if(Bars.Num() > 0)
			{
				for(int32 i = 0; i < Bars.Num(); ++i)
				{
					float WidgetSpaceSnapPosition = ScaleInfo.InputToLocalX(Bars[i].Time); // Do comparison in screen space so that zoom does not cause issues
					float ThisMinSnapDist = FMath::Abs(WidgetSpaceSnapPosition - WidgetSpaceNotifyPosition);
					if(ThisMinSnapDist < CurrentMinSnapDest)
					{
						CurrentMinSnapDest = ThisMinSnapDist;
						SnapPosition = WidgetSpaceSnapPosition;
					}
				}
			}
		}
		return SnapPosition;
	}

	FTrackClampInfo& GetTrackClampInfo(const FVector2D NodePos)
	{
		int32 ClampInfoIndex = 0;
		int32 SmallestNodeTrackDist = FMath::Abs(ClampInfos[0].TrackSnapTestPos - NodePos.Y);
		for(int32 i = 0; i < ClampInfos.Num(); ++i)
		{
			int32 Dist = FMath::Abs(ClampInfos[i].TrackSnapTestPos - NodePos.Y);
			if(Dist < SmallestNodeTrackDist)
			{
				SmallestNodeTrackDist = Dist;
				ClampInfoIndex = i;
			}
		}
		return ClampInfos[ClampInfoIndex];
	}

	class UAnimSequenceBase*			Sequence;				// The owning anim sequence
	FVector2D							DragOffset;				// Offset from the mouse to place the decorator
	TArray<FTrackClampInfo>				ClampInfos;				// Clamping information for all of the available tracks
	float&								CurrentDragXPosition;	// Current X position of the drag operation
	FPanTrackRequest					RequestTrackPan;		// Delegate to request a pan along the edges of a zoomed track
	TArray<float>						NodeTimes;				// Times to drop each selected node at
	float								SnapTime;				// The time that the snapped node was snapped to
	TWeakPtr<SAnimNotifyNode>			SnappedNode;			// The node chosen for the snap
	TAttribute<TArray<FTrackMarkerBar>>	MarkerBars;				// Branching point markers
	TArray<TSharedPtr<SAnimNotifyNode>> SelectedNodes;			// The nodes that are in the current selection
	TArray<float>						NodeTimeOffsets;		// Time offsets from the beginning of the selection to the nodes.
	FVector2D							NodeGroupPosition;		// Position of the beginning of the selection
	FVector2D							NodeGroupSize;			// Size of the entire selection
	TSharedPtr<SWidget>					Decorator;				// The widget to display when dragging
	float								SelectionTimeLength;	// Length of time that the selection covers
	int32								TrackSpan;				// Number of tracks that the selection spans

	static TSharedRef<FNotifyDragDropOp> New(TArray<TSharedPtr<SAnimNotifyNode>> NotifyNodes, TSharedPtr<SWidget> Decorator, const TArray<TSharedPtr<SAnimNotifyTrack>>& NotifyTracks, class UAnimSequenceBase* InSequence, const FVector2D &CursorPosition, const FVector2D &SelectionScreenPosition, const FVector2D &SelectionSize, float& CurrentDragXPosition, FPanTrackRequest& RequestTrackPanDelegate, TAttribute<TArray<FTrackMarkerBar>>	MarkerBars)
	{
		TSharedRef<FNotifyDragDropOp> Operation = MakeShareable(new FNotifyDragDropOp(CurrentDragXPosition));
		Operation->Sequence = InSequence;
		Operation->RequestTrackPan = RequestTrackPanDelegate;

		Operation->NodeGroupPosition = SelectionScreenPosition;
		Operation->NodeGroupSize = SelectionSize;
		Operation->DragOffset = SelectionScreenPosition - CursorPosition;
		Operation->MarkerBars = MarkerBars;
		Operation->Decorator = Decorator;
		Operation->SelectedNodes = NotifyNodes;
		Operation->TrackSpan = NotifyNodes[0]->NotifyEvent->TrackIndex - NotifyNodes.Last()->NotifyEvent->TrackIndex;

		// Caclulate offsets for the selected nodes
		float BeginTime = MAX_flt;
		for(TSharedPtr<SAnimNotifyNode> Node : NotifyNodes)
		{
			if(Node->NotifyEvent->DisplayTime < BeginTime)
			{
				BeginTime = Node->NotifyEvent->DisplayTime;
			}
		}

		// Initialise node data
		for(TSharedPtr<SAnimNotifyNode> Node : NotifyNodes)
		{
			Node->ClearLastSnappedTime();
			Operation->NodeTimeOffsets.Add(Node->NotifyEvent->DisplayTime - BeginTime);
			Operation->NodeTimes.Add(Node->NotifyEvent->DisplayTime);

			// Calculate the time length of the selection. Because it is possible to have states
			// with arbitrary durations we need to search all of the nodes and find the furthest
			// possible point
			Operation->SelectionTimeLength = FMath::Max(Operation->SelectionTimeLength, Node->NotifyEvent->DisplayTime + Node->NotifyEvent->Duration - BeginTime);
		}

		Operation->Construct();

		for(int32 i = 0; i < NotifyTracks.Num(); ++i)
		{
			FTrackClampInfo Info;
			Info.NotifyTrack = NotifyTracks[i];
			const FGeometry& CachedGeometry = Info.NotifyTrack->GetCachedGeometry();
			Info.TrackPos = CachedGeometry.AbsolutePosition.Y;
			Info.TrackWidth = CachedGeometry.Size.X;
			Info.TrackMin = CachedGeometry.AbsolutePosition.X;
			Info.TrackMax = Info.TrackMin + Info.TrackWidth;
			Info.TrackSnapTestPos = Info.TrackPos + (CachedGeometry.Size.Y / 2);
			Operation->ClampInfos.Add(Info);
		}

		Operation->CursorDecoratorWindow->SetOpacity(0.5f);
		return Operation;
	}
	
	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		return Decorator;
	}

	FText GetHoverText() const
	{
		FText HoverText = LOCTEXT("Invalid", "Invalid");

		if(SelectedNodes[0].IsValid())
		{
			HoverText = FText::FromName( SelectedNodes[0]->NotifyEvent->NotifyName );
		}

		return HoverText;
	}
};

//////////////////////////////////////////////////////////////////////////
// FAnimSequenceEditorCommands

class FAnimSequenceEditorCommands : public TCommands<FAnimSequenceEditorCommands>
{
public:
	FAnimSequenceEditorCommands()
		: TCommands<FAnimSequenceEditorCommands>(
		TEXT("AnimSequenceEditor"),
		NSLOCTEXT("Contexts", "AnimSequenceEditor", "Sequence Editor"),
		NAME_None, FEditorStyle::GetStyleSetName()
		)
	{
	}

	virtual void RegisterCommands() OVERRIDE
	{
		UI_COMMAND( SomeSequenceAction, "Some Sequence Action", "Does some sequence action", EUserInterfaceActionType::Button, FInputGesture() );
	}
public:
	TSharedPtr<FUICommandInfo> SomeSequenceAction;
};

//////////////////////////////////////////////////////////////////////////
// SAnimNotifyNode

const float SAnimNotifyNode::MinimumStateDuration = (1.0f / 30.0f);

void SAnimNotifyNode::Construct(const FArguments& InArgs)
{
	Sequence = InArgs._Sequence;
	NotifyEvent = InArgs._AnimNotify;
	Font = FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 );
	bBeingDragged = false;
	CurrentDragHandle = ENotifyStateHandleHit::None;
	bDrawTooltipToRight = true;
	bSelected = false;

	OnNodeDragStarted = InArgs._OnNodeDragStarted;
	PanTrackRequest = InArgs._PanTrackRequest;

	ViewInputMin = InArgs._ViewInputMin;
	ViewInputMax = InArgs._ViewInputMax;

	MarkerBars = InArgs._MarkerBars;
}

FReply SAnimNotifyNode::OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FVector2D ScreenNodePosition = MyGeometry.AbsolutePosition;
	
	// Whether the drag has hit a duration marker
	bool bDragOnMarker = false;
	bBeingDragged = true;

	if(GetDurationSize() > 0.0f)
	{
		// This is a state node, check for a drag on the markers before movement. Use last screen space position before the drag started
		// as using the last position in the mouse event gives us a mouse position after the drag was started.
		ENotifyStateHandleHit::Type MarkerHit = DurationHandleHitTest(LastMouseDownPosition);
		if(MarkerHit == ENotifyStateHandleHit::Start || MarkerHit == ENotifyStateHandleHit::End)
		{
			bDragOnMarker = true;
			bBeingDragged = false;
			CurrentDragHandle = MarkerHit;
		}
	}

	return OnNodeDragStarted.Execute(SharedThis(this), MouseEvent.GetScreenSpacePosition(), ScreenNodePosition, bDragOnMarker);

}

FLinearColor SAnimNotifyNode::GetNotifyColor() const
{
	FLinearColor BaseColor;

	if (NotifyEvent->Notify)
	{
		BaseColor = NotifyEvent->Notify->GetEditorColor();
	}
	else if (NotifyEvent->NotifyStateClass)
	{
		BaseColor = NotifyEvent->NotifyStateClass->GetEditorColor();
	}
	else
	{
		// Color for Custom notifies
		BaseColor = FLinearColor(1, 1, 0.5f);
	}

	BaseColor.A = 0.67f;

	return BaseColor;
}

FText SAnimNotifyNode::GetNotifyText() const
{
	// Combine comment from notify struct and from function on object
	return FText::FromName( NotifyEvent->NotifyName );
}

FText SAnimNotifyNode::GetNodeTooltip() const
{
	const float Time = NotifyEvent->DisplayTime;
	const float Percentage = Time / Sequence->SequenceLength;
	const FText Frame = FText::AsNumber( (int32)(Percentage * Sequence->GetNumberOfFrames()) );
	const FText Seconds = FText::AsNumber( Time );

	if (NotifyEvent->Duration > 0.0f)
	{
		const FText Duration = FText::AsNumber( NotifyEvent->Duration );
		return FText::Format( LOCTEXT("NodeToolTipLong", "@ {0} sec (frame {1}) for {2} sec"), Seconds, Frame, Duration );
	}

	return FText::Format( LOCTEXT("NodeToolTipShort", "@ {0} sec (frame {1})"), Seconds, Frame );
}

/** @return the Node's position within the graph */
UObject* SAnimNotifyNode::GetObjectBeingDisplayed() const
{
	if (NotifyEvent->Notify)
	{
		return NotifyEvent->Notify;
	}

	if (NotifyEvent->NotifyStateClass)
	{
		return NotifyEvent->NotifyStateClass;
	}

	return Sequence;
}

void SAnimNotifyNode::DropCancelled()
{
	bBeingDragged = false;
}

FVector2D SAnimNotifyNode::ComputeDesiredSize() const 
{
	return GetSize();
}

bool SAnimNotifyNode::HitTest(const FGeometry& AllottedGeometry, FVector2D MouseLocalPose) const
{
	FVector2D Position = GetWidgetPosition();
	FVector2D Size = GetSize();

	return MouseLocalPose >= Position && MouseLocalPose <= (Position + Size);
}

ENotifyStateHandleHit::Type SAnimNotifyNode::DurationHandleHitTest(const FVector2D& CursorTrackPosition) const
{
	ENotifyStateHandleHit::Type MarkerHit = ENotifyStateHandleHit::None;

	// Make sure this node has a duration box (meaning it is a state node)
	if(NotifyDurationSizeX > 0.0f)
	{
		// Test for mouse inside duration box with handles included
		float ScrubHandleHalfWidth = ScrubHandleSize.X / 2.0f;

		// Position and size of the notify node including the scrub handles
		FVector2D NotifyNodePosition(NotifyScrubHandleCentre - ScrubHandleHalfWidth, 0.0f);
		FVector2D NotifyNodeSize(NotifyDurationSizeX + ScrubHandleHalfWidth * 2.0f, NotifyHeight);

		FVector2D MouseRelativePosition(CursorTrackPosition - GetWidgetPosition());

		if(MouseRelativePosition > NotifyNodePosition && MouseRelativePosition < (NotifyNodePosition + NotifyNodeSize))
		{
			// Definitely inside the duration box, need to see which handle we hit if any
			if(MouseRelativePosition.X <= (NotifyNodePosition.X + ScrubHandleSize.X))
			{
				// Left Handle
				MarkerHit = ENotifyStateHandleHit::Start;
			}
			else if(MouseRelativePosition.X >= (NotifyNodePosition.X + NotifyNodeSize.X - ScrubHandleSize.X))
			{
				// Right Handle
				MarkerHit = ENotifyStateHandleHit::End;
			}
		}
	}

	return MarkerHit;
}

void SAnimNotifyNode::UpdateSizeAndPosition(const FGeometry& AllottedGeometry)
{
	FTrackScaleInfo ScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0, 0, AllottedGeometry.Size);

	// Cache the geometry information, the alloted geometry is the same size as the track.
	CachedAllotedGeometrySize = AllottedGeometry.Size;

	NotifyTimePositionX = ScaleInfo.InputToLocalX(NotifyEvent->DisplayTime);
	NotifyDurationSizeX = ScaleInfo.PixelsPerInput * NotifyEvent->Duration;

	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	TextSize = FontMeasureService->Measure( GetNotifyText(), Font );

	LabelWidth = TextSize.X + (TextBorderSize.X * 2.f) + (ScrubHandleSize.X/2.f);


	// Work out where we will have to draw the tool tip
	FVector2D Size = GetSize();
	float LeftEdgeToNotify = NotifyTimePositionX;
	float RightEdgeToNotify = AllottedGeometry.Size.X - NotifyTimePositionX;
	bDrawTooltipToRight = (RightEdgeToNotify > LabelWidth) || (RightEdgeToNotify > LeftEdgeToNotify);

	//Calculate scrub handle box size (the notional box around the scrub handle and the alignment marker)
	float NotifyHandleBoxWidth = FMath::Max(ScrubHandleSize.X, AlignmentMarkerSize.X*2);

	// Calculate widget width/position based on where we are drawing the tool tip
	WidgetX = bDrawTooltipToRight ? (NotifyTimePositionX - (NotifyHandleBoxWidth / 2.f)) : (NotifyTimePositionX - LabelWidth);
	WidgetSize = bDrawTooltipToRight ? FVector2D(FMath::Max(LabelWidth, NotifyDurationSizeX), NotifyHeight) : FVector2D((LabelWidth + NotifyDurationSizeX), NotifyHeight);
	WidgetSize.X += NotifyHandleBoxWidth;

	// Widget position of the notify marker
	NotifyScrubHandleCentre = bDrawTooltipToRight ? NotifyHandleBoxWidth / 2.f : LabelWidth;
}

/** @return the Node's position within the track */
FVector2D SAnimNotifyNode::GetWidgetPosition() const
{
	return FVector2D(WidgetX, NotifyHeightOffset);
}

FVector2D SAnimNotifyNode::GetNotifyPosition() const
{
	return FVector2D(NotifyTimePositionX, NotifyHeightOffset);
}

FVector2D SAnimNotifyNode::GetNotifyPositionOffset() const
{
	return GetNotifyPosition() - GetWidgetPosition();
}

FVector2D SAnimNotifyNode::GetSize() const
{
	return WidgetSize;
}

int32 SAnimNotifyNode::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MarkerLayer = LayerId + 1;
	int32 ScrubHandleID = MarkerLayer + 1;
	int32 TextLayerID = ScrubHandleID + 1;

	const FSlateBrush* StyleInfo = FEditorStyle::GetBrush( TEXT("SpecialEditableTextImageNormal") );
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId, 
		AllottedGeometry.ToPaintGeometry(FVector2D(0,0), AllottedGeometry.Size), 
		StyleInfo, 
		MyClippingRect, 
		ESlateDrawEffect::None,
		FLinearColor(1.0f, 1.0f, 1.0f,0.1f));

	FText Text = GetNotifyText();
	FLinearColor NodeColour = bSelected ? FLinearColor(1.0f, 0.5f, 0.0f) : FLinearColor::Red;
	
	float HalfScrubHandleWidth = ScrubHandleSize.X / 2.0f;

	// Show duration of AnimNotifyState
	if( NotifyDurationSizeX > 0.f )
	{
		FLinearColor BoxColor = (NotifyEvent->TrackIndex % 2) == 0 ? FLinearColor(0.0f,1.0f,0.5f,0.5f) : FLinearColor(0.0f,0.5f,1.0f,0.5f);
		FVector2D DurationBoxSize = FVector2D(NotifyDurationSizeX, NotifyHeight);
		FVector2D DurationBoxPosition = FVector2D(NotifyScrubHandleCentre, 0.f);
		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			LayerId, 
			AllottedGeometry.ToPaintGeometry(DurationBoxPosition, DurationBoxSize), 
			StyleInfo, 
			MyClippingRect, 
			ESlateDrawEffect::None,
			BoxColor);

		DrawScrubHandle(DurationBoxPosition.X + DurationBoxSize.X, OutDrawElements, ScrubHandleID, AllottedGeometry, MyClippingRect, NodeColour);
		
		// Render offsets if necessary
		if(NotifyEvent->EndTriggerTimeOffset != 0.f) //Do we have an offset to render?
		{
			float EndTime = NotifyEvent->DisplayTime + NotifyEvent->Duration;
			if(EndTime != Sequence->SequenceLength) //Don't render offset when we are at the end of the sequence, doesnt help the user
			{
				// ScrubHandle
				float HandleCentre = NotifyDurationSizeX + ScrubHandleSize.X;
				DrawHandleOffset(NotifyEvent->EndTriggerTimeOffset, HandleCentre, OutDrawElements, MarkerLayer, AllottedGeometry, MyClippingRect);
			}
		}
	}

	// Background
	FVector2D LabelSize = TextSize + TextBorderSize * 2.f;
	LabelSize.X += HalfScrubHandleWidth;

	float LabelX = bDrawTooltipToRight ? NotifyScrubHandleCentre : NotifyScrubHandleCentre - LabelSize.X;
	float BoxHeight = (NotifyDurationSizeX > 0.f) ? (NotifyHeight - LabelSize.Y) : ((NotifyHeight - LabelSize.Y) / 2.f);

	FVector2D LabelPosition(LabelX, BoxHeight); 
	
	FLinearColor NodeColor = SAnimNotifyNode::GetNotifyColor();

	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId, 
		AllottedGeometry.ToPaintGeometry(LabelPosition, LabelSize), 
		StyleInfo, 
		MyClippingRect, 
		ESlateDrawEffect::None,
		NodeColor);

	// Frame
	// Drawing lines is slow, reserved for single selected node
	if( bSelected )
	{
		TArray<FVector2D> LinePoints;

		LinePoints.Empty();
		LinePoints.Add(LabelPosition);
		LinePoints.Add(LabelPosition + FVector2D(LabelSize.X, 0.f));
		LinePoints.Add(LabelPosition + FVector2D(LabelSize.X, LabelSize.Y));
		LinePoints.Add(LabelPosition + FVector2D(0.f, LabelSize.Y));
		LinePoints.Add(LabelPosition);

		FSlateDrawElement::MakeLines( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			FLinearColor::Black, 
			false
			);
	}

	// Text
	FVector2D TextPosition = LabelPosition + TextBorderSize;
	if(bDrawTooltipToRight)
	{
		TextPosition.X += HalfScrubHandleWidth;
	}
	TextPosition -= FVector2D(1.f, 1.f);

	FSlateDrawElement::MakeText( 
		OutDrawElements,
		TextLayerID,
		AllottedGeometry.ToPaintGeometry(TextPosition, TextSize),
		Text,
		Font,
		MyClippingRect,
		ESlateDrawEffect::None,
		FLinearColor::Black
		);

	DrawScrubHandle(NotifyScrubHandleCentre , OutDrawElements, ScrubHandleID, AllottedGeometry, MyClippingRect, NodeColour);

	if(NotifyEvent->TriggerTimeOffset != 0.f) //Do we have an offset to render?
	{
		if(NotifyEvent->DisplayTime != 0.f && NotifyEvent->DisplayTime != Sequence->SequenceLength) //Don't render offset when we are at the start/end of the sequence, doesnt help the user
		{
			float HandleCentre = NotifyScrubHandleCentre;
			float &Offset = NotifyEvent->TriggerTimeOffset;
			
			DrawHandleOffset(NotifyEvent->TriggerTimeOffset, NotifyScrubHandleCentre, OutDrawElements, MarkerLayer, AllottedGeometry, MyClippingRect);
		}
	}
	return TextLayerID;
}

FReply SAnimNotifyNode::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Don't do scrub handle dragging if we haven't captured the mouse.
	if(!this->HasMouseCapture()) return FReply::Unhandled();

	// IF we get this far we should definitely have a handle to move.
	check(CurrentDragHandle != ENotifyStateHandleHit::None);
	
	FTrackScaleInfo ScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0, 0, CachedAllotedGeometrySize);
	
	float TrackScreenSpaceXPosition = MyGeometry.AbsolutePosition.X - MyGeometry.Position.X;

	if(CurrentDragHandle == ENotifyStateHandleHit::Start)
	{
		// Check track bounds
		float OldDisplayTime = NotifyEvent->DisplayTime;

		if(MouseEvent.GetScreenSpacePosition().X >= TrackScreenSpaceXPosition && MouseEvent.GetScreenSpacePosition().X <= TrackScreenSpaceXPosition + CachedAllotedGeometrySize.X)
		{
			float NewDisplayTime = ScaleInfo.LocalXToInput((MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition + MyGeometry.Position).X);
			float NewDuration = NotifyEvent->Duration + OldDisplayTime - NewDisplayTime;

			// Check to make sure the duration is not less than the minimum allowed
			if(NewDuration < MinimumStateDuration)
			{
				NewDisplayTime -= MinimumStateDuration - NewDuration;
			}

			NotifyEvent->DisplayTime = NewDisplayTime;
			NotifyEvent->Duration += OldDisplayTime - NotifyEvent->DisplayTime;
		}
		else if(NotifyEvent->Duration > MinimumStateDuration)
		{
			float Overflow = HandleOverflowPan(MouseEvent.GetScreenSpacePosition(), TrackScreenSpaceXPosition);

			// Update scale info to the new view inputs after panning
			ScaleInfo.ViewMinInput = ViewInputMin.Get();
			ScaleInfo.ViewMaxInput = ViewInputMax.Get();

			NotifyEvent->DisplayTime = ScaleInfo.LocalXToInput(Overflow < 0.0f ? 0.0f : CachedAllotedGeometrySize.X);
			NotifyEvent->Duration += OldDisplayTime - NotifyEvent->DisplayTime;

			// Adjust incase we went under the minimum
			if(NotifyEvent->Duration < MinimumStateDuration)
			{
				float EndTimeBefore = NotifyEvent->DisplayTime + NotifyEvent->Duration;
				NotifyEvent->DisplayTime += NotifyEvent->Duration - MinimumStateDuration;
				NotifyEvent->Duration = MinimumStateDuration;
				float EndTimeAfter = NotifyEvent->DisplayTime + NotifyEvent->Duration;
			}
		}

		// Now we know where the marker should be, look for possible snaps on montage marker bars
		float NodePositionX = ScaleInfo.InputToLocalX(NotifyEvent->DisplayTime);
		float MarkerSnap = GetScrubHandleSnapPosition(NodePositionX, ENotifyStateHandleHit::Start);
		if(MarkerSnap != -1.0f)
		{
			// We're near to a snap bar
			EAnimEventTriggerOffsets::Type Offset = (MarkerSnap < NodePositionX) ? EAnimEventTriggerOffsets::OffsetAfter : EAnimEventTriggerOffsets::OffsetBefore;
			NotifyEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);
			NodePositionX = MarkerSnap;

			// Adjust our start marker
			OldDisplayTime = NotifyEvent->DisplayTime;
			NotifyEvent->DisplayTime = ScaleInfo.LocalXToInput(NodePositionX);
			NotifyEvent->Duration += OldDisplayTime - NotifyEvent->DisplayTime;
		}
		else
		{
			NotifyEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
		}
	}
	else
	{
		if(MouseEvent.GetScreenSpacePosition().X >= TrackScreenSpaceXPosition && MouseEvent.GetScreenSpacePosition().X <= TrackScreenSpaceXPosition + CachedAllotedGeometrySize.X)
		{
			float NewDuration = ScaleInfo.LocalXToInput((MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition + MyGeometry.Position).X) - NotifyEvent->DisplayTime;

			NotifyEvent->Duration = FMath::Max(NewDuration, MinimumStateDuration);
		}
		else if(NotifyEvent->Duration > MinimumStateDuration)
		{
			float Overflow = HandleOverflowPan(MouseEvent.GetScreenSpacePosition(), TrackScreenSpaceXPosition);

			// Update scale info to the new view inputs after panning
			ScaleInfo.ViewMinInput = ViewInputMin.Get();
			ScaleInfo.ViewMaxInput = ViewInputMax.Get();

			NotifyEvent->Duration = FMath::Max(ScaleInfo.LocalXToInput(Overflow > 0.0f ? CachedAllotedGeometrySize.X : 0.0f) - NotifyEvent->DisplayTime, MinimumStateDuration);
		}

		// Now we know where the scrub handle should be, look for possible snaps on montage marker bars
		float NodePositionX = ScaleInfo.InputToLocalX(NotifyEvent->DisplayTime + NotifyEvent->Duration);
		float MarkerSnap = GetScrubHandleSnapPosition(NodePositionX, ENotifyStateHandleHit::End);
		if(MarkerSnap != -1.0f)
		{
			// We're near to a snap bar
			EAnimEventTriggerOffsets::Type Offset = (MarkerSnap < NodePositionX) ? EAnimEventTriggerOffsets::OffsetAfter : EAnimEventTriggerOffsets::OffsetBefore;
			NotifyEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);
			NodePositionX = MarkerSnap;

			// Adjust our end marker
			NotifyEvent->Duration = ScaleInfo.LocalXToInput(NodePositionX) - NotifyEvent->DisplayTime;
		}
		else
		{
			NotifyEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
		}
	}

	return FReply::Handled();
}

FReply SAnimNotifyNode::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	bool bLeftButton = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;

	if(bLeftButton && CurrentDragHandle != ENotifyStateHandleHit::None)
	{
		// Clear the drag marker and give the mouse back
		CurrentDragHandle = ENotifyStateHandleHit::None;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

float SAnimNotifyNode::GetScrubHandleSnapPosition( float NotifyLocalX, ENotifyStateHandleHit::Type HandleToCheck )
{
	FTrackScaleInfo ScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0, 0, CachedAllotedGeometrySize);

	const float MaxSnapDist = 5.0f;

	float CurrentMinSnapDistance = MaxSnapDist;
	float SnapPosition = -1.0f;
	float SnapTime = -1.0f;

	if(MarkerBars.IsBound())
	{
		const TArray<FTrackMarkerBar>& Bars = MarkerBars.Get();

		if(Bars.Num() > 0)
		{
			for(auto Iter(Bars.CreateConstIterator()) ; Iter ; ++Iter)
			{
				const FTrackMarkerBar& Bar = *Iter;
				float LocalSnapPosition = ScaleInfo.InputToLocalX(Bar.Time);
				float ThisNodeMinSnapDistance = FMath::Abs(LocalSnapPosition - NotifyLocalX);
				if(ThisNodeMinSnapDistance < CurrentMinSnapDistance)
				{
					CurrentMinSnapDistance = ThisNodeMinSnapDistance;
					SnapPosition = LocalSnapPosition;
				}
			}
		}
	}

	return SnapPosition;
}

float SAnimNotifyNode::HandleOverflowPan( const FVector2D &ScreenCursorPos, float TrackScreenSpaceXPosition )
{
	float Overflow = 0.0f;
	if(ScreenCursorPos.X < TrackScreenSpaceXPosition)
	{
		// Overflow left edge
		Overflow = ScreenCursorPos.X - TrackScreenSpaceXPosition;
	}
	else
	{
		// Overflow right edge
		Overflow = ScreenCursorPos.X - (TrackScreenSpaceXPosition + CachedAllotedGeometrySize.X);
	}
	PanTrackRequest.ExecuteIfBound(Overflow, CachedAllotedGeometrySize);

	return Overflow;
}

void SAnimNotifyNode::DrawScrubHandle( float ScrubHandleCentre, FSlateWindowElementList& OutDrawElements, int32 ScrubHandleID, const FGeometry &AllottedGeometry, const FSlateRect& MyClippingRect, FLinearColor NodeColour ) const
{
	FVector2D ScrubHandlePosition(ScrubHandleCentre - ScrubHandleSize.X / 2.0f, (NotifyHeight - ScrubHandleSize.Y) / 2.f);
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		ScrubHandleID, 
		AllottedGeometry.ToPaintGeometry(ScrubHandlePosition, ScrubHandleSize), 
		FEditorStyle::GetBrush( TEXT( "Sequencer.Timeline.ScrubHandleWhole" ) ), 
		MyClippingRect, 
		ESlateDrawEffect::None,
		NodeColour
		);
}

void SAnimNotifyNode::DrawHandleOffset( const float& Offset, const float& HandleCentre, FSlateWindowElementList& OutDrawElements, int32 MarkerLayer, const FGeometry &AllottedGeometry, const FSlateRect& MyClippingRect ) const
{
	FVector2D MarkerPosition;
	FVector2D MarkerSize = AlignmentMarkerSize;

	if(Offset < 0.f)
	{
		MarkerPosition.Set( (HandleCentre) - AlignmentMarkerSize.X, (NotifyHeight - AlignmentMarkerSize.Y) / 2.f);
	}
	else
	{
		MarkerPosition.Set( HandleCentre + AlignmentMarkerSize.X, (NotifyHeight - AlignmentMarkerSize.Y) / 2.f);
		MarkerSize.X = -AlignmentMarkerSize.X;
	}

	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		MarkerLayer, 
		AllottedGeometry.ToPaintGeometry(MarkerPosition, MarkerSize), 
		FEditorStyle::GetBrush( TEXT( "Sequencer.Timeline.NotifyAlignmentMarker" ) ), 
		MyClippingRect, 
		ESlateDrawEffect::None,
		FLinearColor(0.f, 0.f, 1.f)
		);
}

void SAnimNotifyNode::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	ScreenPosition = AllottedGeometry.AbsolutePosition;
}

//////////////////////////////////////////////////////////////////////////
// SAnimNotifyTrack

void SAnimNotifyTrack::Construct(const FArguments& InArgs)
{
	FAnimSequenceEditorCommands::Register();
	CreateCommands();

	PersonaPtr = InArgs._Persona;
	Sequence = InArgs._Sequence;
	ViewInputMin = InArgs._ViewInputMin;
	ViewInputMax = InArgs._ViewInputMax;
	OnSelectionChanged = InArgs._OnSelectionChanged;
	AnimNotifies = InArgs._AnimNotifies;
	OnUpdatePanel = InArgs._OnUpdatePanel;
	TrackIndex = InArgs._TrackIndex;
	OnGetScrubValue = InArgs._OnGetScrubValue;
	OnGetDraggedNodePos = InArgs._OnGetDraggedNodePos;
	OnNodeDragStarted = InArgs._OnNodeDragStarted;
	TrackColor = InArgs._TrackColor;
	MarkerBars = InArgs._MarkerBars;
	OnRequestTrackPan = InArgs._OnRequestTrackPan;
	OnRequestRefreshOffsets = InArgs._OnRequestOffsetRefresh;
	OnDeleteNotify = InArgs._OnDeleteNotify;
	OnDeselectAllNotifies = InArgs._OnDeselectAllNotifies;
	OnCopyNotifies = InArgs._OnCopyNotifies;
	OnPasteNotifies = InArgs._OnPasteNotifies;

	this->ChildSlot
	[
			SAssignNew( TrackArea, SBorder )
			.BorderImage( FEditorStyle::GetBrush("NoBorder") )
			.Padding( FMargin(0.f, 0.f) )
	];

	Update();
}

FVector2D SAnimNotifyTrack::ComputeDesiredSize() const 
{
	FVector2D Size;
	Size.X = 200;
	Size.Y = NotificationTrackHeight;
	return Size;
}

int32 SAnimNotifyTrack::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FSlateBrush* StyleInfo = FEditorStyle::GetBrush( TEXT( "Persona.NotifyEditor.NotifyTrackBackground" ) );
	FLinearColor Color = TrackColor.Get();

	FPaintGeometry MyGeometry = AllottedGeometry.ToPaintGeometry();
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId, 
		MyGeometry, 
		StyleInfo, 
		MyClippingRect, 
		ESlateDrawEffect::None, 
		Color
		);

	int32 CustomLayerId = LayerId + 1; 

	// draw line for every 1/4 length
	FTrackScaleInfo ScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0.f, 0.f, AllottedGeometry.Size);
	if (Sequence->GetNumberOfFrames() > 0 )
	{
		int32 Divider = SScrubWidget::GetDivider( ViewInputMin.Get(), ViewInputMax.Get(), AllottedGeometry.Size, Sequence->SequenceLength, Sequence->GetNumberOfFrames());

		float TimePerKey = Sequence->SequenceLength/(float)Sequence->GetNumberOfFrames();
		for (int32 I=1; I<Sequence->GetNumberOfFrames(); ++I)
		{
			if ( I % Divider == 0 )
			{
				float XPos = ScaleInfo.InputToLocalX(TimePerKey*I);

				TArray<FVector2D> LinePoints;
				LinePoints.Add(FVector2D(XPos, 0.f));
				LinePoints.Add(FVector2D(XPos, AllottedGeometry.Size.Y));

				FSlateDrawElement::MakeLines( 
					OutDrawElements,
					CustomLayerId,
					MyGeometry,
					LinePoints,
					MyClippingRect,
					ESlateDrawEffect::None,
					FLinearColor::Black
					);
			}
		}
	}

	++CustomLayerId;
	for ( int32 I=0; I<NotifyNodes.Num(); ++I )
	{
		if ( NotifyNodes[I].Get()->bBeingDragged == false )
		{
			NotifyNodes[I].Get()->UpdateSizeAndPosition(AllottedGeometry);
		}
	}

	++CustomLayerId;

	float Value = 0.f;

	if ( OnGetScrubValue.IsBound() )
	{
		Value = OnGetScrubValue.Execute();
	}

	{
		float XPos = ScaleInfo.InputToLocalX(Value);

		TArray<FVector2D> LinePoints;
		LinePoints.Add(FVector2D(XPos, 0.f));
		LinePoints.Add(FVector2D(XPos, AllottedGeometry.Size.Y));


		FSlateDrawElement::MakeLines( 
			OutDrawElements,
			CustomLayerId,
			MyGeometry,
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			FLinearColor(1, 0, 0)
			);
	}

	++CustomLayerId;

	if ( OnGetDraggedNodePos.IsBound() )
	{
		Value = OnGetDraggedNodePos.Execute();

		if(Value >= 0.0f)
		{
			float XPos = Value;
			TArray<FVector2D> LinePoints;
			LinePoints.Add(FVector2D(XPos, 0.f));
			LinePoints.Add(FVector2D(XPos, AllottedGeometry.Size.Y));


			FSlateDrawElement::MakeLines( 
				OutDrawElements,
				CustomLayerId,
				MyGeometry,
				LinePoints,
				MyClippingRect,
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 0.5f, 0.0f)
				);
		}
	}

	++CustomLayerId;

	// Draggable Bars
	for ( int32 I=0; MarkerBars.IsBound() && I < MarkerBars.Get().Num(); I++ )
	{
		// Draw lines
		FTrackMarkerBar Bar = MarkerBars.Get()[I];

		float XPos = ScaleInfo.InputToLocalX(Bar.Time);

		TArray<FVector2D> LinePoints;
		LinePoints.Add(FVector2D(XPos, 0.f));
		LinePoints.Add(FVector2D(XPos, AllottedGeometry.Size.Y));

		FSlateDrawElement::MakeLines( 
			OutDrawElements,
			CustomLayerId,
			MyGeometry,
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Bar.DrawColour
			);
	}

	return SCompoundWidget::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, CustomLayerId, InWidgetStyle, bParentEnabled);
}

void SAnimNotifyTrack::FillNewNotifyStateMenu(FMenuBuilder& MenuBuilder)
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Collect a full list of assets with the specified class
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetData);

	const FName BPParentClassName( TEXT( "ParentClass" ) );
	const FString BPAnimNotify( TEXT("Class'/Script/Engine.AnimNotifyState'" ));

	for (int32 AssetIndex = 0; AssetIndex < AssetData.Num(); ++AssetIndex)
	{
		FString TagValue = AssetData[ AssetIndex ].TagsAndValues.FindRef(BPParentClassName);
		if(TagValue == BPAnimNotify)
		{
			FString BlueprintPath = AssetData[AssetIndex].ObjectPath.ToString();
			FString Label = AssetData[AssetIndex].AssetName.ToString();
			Label = Label.Replace(TEXT("AnimNotifyState_"), TEXT(""), ESearchCase::CaseSensitive);

			const FText Description = LOCTEXT("AddsAnExistingAnimNotify", "Add an existing notify");
			const FText LabelText = FText::FromString( Label );

			FUIAction UIAction;
			UIAction.ExecuteAction.BindRaw(
				this, &SAnimNotifyTrack::CreateNewBlueprintNotifyAtCursor,
				Label,
				BlueprintPath);

			MenuBuilder.AddMenuEntry(LabelText, Description, FSlateIcon(), UIAction);
		}
	}

	MenuBuilder.BeginSection("NativeNotifyStates", LOCTEXT("NewStateNotifyMenu_Native", "Native Notify States"));
	{
		for(TObjectIterator<UClass> It ; It ; ++It)
		{
			UClass* Class = *It;

			if(Class->IsChildOf(UAnimNotifyState::StaticClass()) && !Class->HasAllClassFlags(CLASS_Abstract) && !Class->IsInBlueprint())
			{
				// Found a native anim notify class (that isn't UAnimNotify)
				const FText Description = LOCTEXT("NewNotifyStateSubMenu_NativeToolTip", "Add an existing native notify state");
				FString Label = Class->GetDisplayNameText().ToString();
				const FText LabelText = FText::FromString(Label);

				FUIAction UIAction;
				UIAction.ExecuteAction.BindRaw(
					this, &SAnimNotifyTrack::CreateNewNotifyAtCursor,
					Label,
					Class);

				MenuBuilder.AddMenuEntry(LabelText, Description, FSlateIcon(), UIAction);
			}
		}
	}
	MenuBuilder.EndSection();
}

void SAnimNotifyTrack::FillNewNotifyMenu(FMenuBuilder& MenuBuilder)
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Collect a full list of assets with the specified class
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetData);

	const FName BPParentClassName( TEXT( "ParentClass" ) );
	const FString BPAnimNotify( TEXT("Class'/Script/Engine.AnimNotify'" ));
		
	for (int32 AssetIndex = 0; AssetIndex < AssetData.Num(); ++AssetIndex)
	{
		FString TagValue = AssetData[ AssetIndex ].TagsAndValues.FindRef(BPParentClassName);
		if(TagValue == BPAnimNotify)
		{
			FString BlueprintPath = AssetData[AssetIndex].ObjectPath.ToString();
			FString Label = AssetData[AssetIndex].AssetName.ToString();
			Label = Label.Replace(TEXT("AnimNotify_"), TEXT(""), ESearchCase::CaseSensitive);

			const FText Description = LOCTEXT("NewNotifySubMenu_ToolTip", "Add an existing notify");
			const FText LabelText = FText::FromString( Label );

			FUIAction UIAction;
			UIAction.ExecuteAction.BindRaw(
				this, &SAnimNotifyTrack::CreateNewBlueprintNotifyAtCursor,
				Label,
				BlueprintPath);

			MenuBuilder.AddMenuEntry(LabelText, Description, FSlateIcon(), UIAction);
		}
	}

	MenuBuilder.BeginSection("NativeNotifies", LOCTEXT("NewNotifyMenu_Native", "Native Notifies"));
	{
		for(TObjectIterator<UClass> It ; It ; ++It)
		{
			UClass* Class = *It;

			if(Class->IsChildOf(UAnimNotify::StaticClass()) && !Class->HasAllClassFlags(CLASS_Abstract) && !Class->IsInBlueprint())
			{
				// Found a native anim notify class (that isn't UAnimNotify)
				const FText Description = LOCTEXT("NewNotifySubMenu_NativeToolTip", "Add an existing native notify");
				FString Label = Class->GetName();
				const FText LabelText = FText::FromString(Label);

				FUIAction UIAction;
				UIAction.ExecuteAction.BindRaw(
					this, &SAnimNotifyTrack::CreateNewNotifyAtCursor,
					Label,
					Class);

				MenuBuilder.AddMenuEntry(LabelText, Description, FSlateIcon(), UIAction);
			}
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AnimNotifyCustom", LOCTEXT("NewNotifySubMenu_Custom", "Custom"));
	{
		// now add custom anim notifiers
		USkeleton * SeqSkeleton = Sequence->GetSkeleton();
		if (SeqSkeleton)
		{
			for (int32 I = 0; I<SeqSkeleton->AnimationNotifies.Num(); ++I)
			{
				FName NotifyName = SeqSkeleton->AnimationNotifies[I];
				FString Label = NotifyName.ToString();

				const FText Description = LOCTEXT("NewNotifySubMenu_ToolTip", "Add an existing notify");

				FUIAction UIAction;
				UIAction.ExecuteAction.BindRaw(
					this, &SAnimNotifyTrack::CreateNewNotifyAtCursor,
					Label, 
					(UClass*)nullptr);

				MenuBuilder.AddMenuEntry( FText::FromString( Label ), Description, FSlateIcon(), UIAction);
			}
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AnimNotifyCreateNew");
	{
		FUIAction UIAction;
		UIAction.ExecuteAction.BindRaw(
			this, &SAnimNotifyTrack::OnNewNotifyClicked);
		MenuBuilder.AddMenuEntry(LOCTEXT("NewNotify", "New Notify"), LOCTEXT("NewNotifyToolTip", "Create a new animation notify"), FSlateIcon(), UIAction);
	}
	MenuBuilder.EndSection();
}

void SAnimNotifyTrack::CreateNewBlueprintNotifyAtCursor(FString NewNotifyName, FString BlueprintPath)
{
	TSubclassOf<UObject> BlueprintClass = NULL;
	if( !BlueprintPath.IsEmpty() )
	{
		UBlueprint* BlueprintLibPtr = LoadObject<UBlueprint>(NULL, *BlueprintPath, NULL, 0, NULL);
		BlueprintClass = Cast<UClass>(BlueprintLibPtr->GeneratedClass);
	}
	check(BlueprintClass)
	CreateNewNotifyAtCursor(NewNotifyName, BlueprintClass);
}

void SAnimNotifyTrack::CreateNewNotifyAtCursor(FString NewNotifyName, UClass* NotifyClass)
{
	const float NewTime = LastClickedTime;
	const FScopedTransaction Transaction( LOCTEXT("AddNotifyEvent", "Add Anim Notify") );
	Sequence->Modify();

	// Insert a new notify record and spawn the new notify object
	int32 NewNotifyIndex = Sequence->Notifies.AddZeroed();
	FAnimNotifyEvent& NewEvent = Sequence->Notifies[NewNotifyIndex];
	NewEvent.NotifyName = FName(*NewNotifyName);
	NewEvent.DisplayTime = NewTime;
	NewEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(Sequence->CalculateOffsetForNotify(NewTime));
	NewEvent.TrackIndex = TrackIndex;

	if( NotifyClass )
	{
		class UObject * AnimNotifyClass = NewObject<UObject>(Sequence, NotifyClass);
		NewEvent.NotifyStateClass = Cast<UAnimNotifyState>(AnimNotifyClass);
		NewEvent.Notify = Cast<UAnimNotify>(AnimNotifyClass);

		// Set default duration to 1 frame for AnimNotifyState.
		if( NewEvent.NotifyStateClass )
		{
			NewEvent.Duration = 1 / 30.f;
		}
	}
	else
	{
		NewEvent.Notify = NULL;
		NewEvent.NotifyStateClass = NULL;
	}

	if(NewEvent.Notify)
	{
		TArray<FAssetData> SelectedAssets;
		AssetSelectionUtils::GetSelectedAssets(SelectedAssets);

		for( TFieldIterator<UObjectProperty> PropIt(NewEvent.Notify->GetClass()); PropIt; ++PropIt )
		{
			if(PropIt->GetBoolMetaData(TEXT("ExposeOnSpawn")))
			{
				UObjectProperty* Property = *PropIt;
				const FAssetData* Asset = SelectedAssets.FindByPredicate([Property](const FAssetData& Other)
				{
					return Other.GetAsset()->IsA(Property->PropertyClass);
				});

				if( Asset )
				{
					uint8* Offset = (*PropIt)->ContainerPtrToValuePtr<uint8>(NewEvent.Notify);
					(*PropIt)->ImportText( *Asset->GetAsset()->GetPathName(), Offset, 0, NewEvent.Notify );
					break;
				}
			}
		}
	}

	Sequence->MarkPackageDirty();
	OnUpdatePanel.ExecuteIfBound();
}

FReply SAnimNotifyTrack::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	bool bLeftMouseButton =  MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
	bool bRightMouseButton =  MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
	bool bShift = MouseEvent.IsShiftDown();
	bool bCtrl = MouseEvent.IsControlDown();

	if ( bRightMouseButton )
	{
		TSharedPtr<SWidget> WidgetToFocus;
		WidgetToFocus = SummonContextMenu(MyGeometry, MouseEvent);

		return (WidgetToFocus.IsValid())
			? FReply::Handled().ReleaseMouseCapture().SetKeyboardFocus( WidgetToFocus.ToSharedRef(), EKeyboardFocusCause::SetDirectly )
			: FReply::Handled().ReleaseMouseCapture();
	}
	else if ( bLeftMouseButton )
	{
		FVector2D CursorPos = MouseEvent.GetScreenSpacePosition();
		CursorPos = MyGeometry.AbsoluteToLocal(CursorPos);
		int32 NotifyIndex = GetHitNotifyNode(MyGeometry, CursorPos);

		if(NotifyIndex == INDEX_NONE)
		{
			// Clicked in empty space, clear selection
			OnDeselectAllNotifies.ExecuteIfBound();
		}
		else
		{
			if(bCtrl)
			{
				ToggleNotifyNodeSelectionStatus(NotifyIndex);
			}
			else
			{
				SelectNotifyNode(NotifyIndex, bShift);
			}
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SAnimNotifyTrack::SelectNotifyNode(int32 NotifyIndex, bool Append)
{
	if( NotifyIndex != INDEX_NONE )
	{
		// Deselect all other notifies if necessary.
		if (Sequence && !Append)
		{
			OnDeselectAllNotifies.ExecuteIfBound();
		}

		// Check to see if we've already selected this node
		if (!SelectedNotifyIndices.Contains(NotifyIndex))
		{
			// select new one
			if (NotifyNodes.IsValidIndex(NotifyIndex))
			{
				TSharedPtr<SAnimNotifyNode> Node = NotifyNodes[NotifyIndex];
				Node->bSelected = true;
				SelectedNotifyIndices.Add(NotifyIndex);
				SendSelectionChanged();
			}
		}
	}
}

void SAnimNotifyTrack::ToggleNotifyNodeSelectionStatus( int32 NotifyIndex )
{
	check(NotifyNodes.IsValidIndex(NotifyIndex));

	bool bSelected = SelectedNotifyIndices.Contains(NotifyIndex);
	if(bSelected)
	{
		SelectedNotifyIndices.Remove(NotifyIndex);
	}
	else
	{
		SelectedNotifyIndices.Add(NotifyIndex);
	}

	TSharedPtr<SAnimNotifyNode> Node = NotifyNodes[NotifyIndex];
	Node->bSelected = !Node->bSelected;
	SendSelectionChanged();
}

void SAnimNotifyTrack::DeselectNotifyNode( int32 NotifyIndex )
{
	check(NotifyNodes.IsValidIndex(NotifyIndex));
	TSharedPtr<SAnimNotifyNode> Node = NotifyNodes[NotifyIndex];
	Node->bSelected = false;

	int32 ItemsRemoved = SelectedNotifyIndices.Remove(NotifyIndex);
	check(ItemsRemoved > 0);

	SendSelectionChanged();
}

void SAnimNotifyTrack::DeselectAllNotifyNodes(bool bUpdateSelectionSet)
{
	for(TSharedPtr<SAnimNotifyNode> Node : NotifyNodes)
	{
		Node->bSelected = false;
	}
	SelectedNotifyIndices.Empty();

	if(bUpdateSelectionSet)
	{
		SendSelectionChanged();
	}
}

TSharedPtr<SWidget> SAnimNotifyTrack::SummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FVector2D CursorPos = MouseEvent.GetScreenSpacePosition();
	int32 NotifyIndex = GetHitNotifyNode(MyGeometry, MyGeometry.AbsoluteToLocal(CursorPos));
	LastClickedTime = CalculateTime(MyGeometry, MouseEvent.GetScreenSpacePosition());

	const bool bCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bCloseWindowAfterMenuSelection, AnimSequenceEditorActions );
	FUIAction NewAction;

	MenuBuilder.BeginSection("AnimNotify", LOCTEXT("NotifyHeading", "Notify") );
	{
		if ( NotifyIndex != INDEX_NONE )
		{
			if (!NotifyNodes[NotifyIndex]->bSelected)
			{
				SelectNotifyNode(NotifyIndex, MouseEvent.IsControlDown());
			}

			// add menu to get threshold weight for triggering this notify
			NewAction.ExecuteAction.BindRaw(
				this, &SAnimNotifyTrack::OnSetTriggerWeightNotifyClicked, NotifyIndex);
			NewAction.CanExecuteAction.BindRaw(
				this, &SAnimNotifyTrack::IsSingleNodeSelected);

			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = 5;

			const FText Threshold = FText::AsNumber( AnimNotifies[NotifyIndex]->TriggerWeightThreshold, &Options );
			const FText MinTriggerWeightText = FText::Format( LOCTEXT("MinTriggerWeight", "Min Trigger Weight: {0}..."), Threshold );
			MenuBuilder.AddMenuEntry(MinTriggerWeightText, LOCTEXT("MinTriggerWeightToolTip", "The minimum weight to trigger this notify"), FSlateIcon(), NewAction);

			// Add menu for changing duration if this is an AnimNotifyState
			if( AnimNotifies[NotifyIndex]->NotifyStateClass )
			{
				NewAction.ExecuteAction.BindRaw(
					this, &SAnimNotifyTrack::OnSetDurationNotifyClicked, NotifyIndex);
				NewAction.CanExecuteAction.BindRaw(
					this, &SAnimNotifyTrack::IsSingleNodeSelected);

				FText SetAnimStateDurationText = FText::Format( LOCTEXT("SetAnimStateDuration", "Set AnimNotifyState duration ({0})"), FText::AsNumber( AnimNotifies[NotifyIndex]->Duration ) );
				MenuBuilder.AddMenuEntry(SetAnimStateDurationText, LOCTEXT("SetAnimStateDuration_ToolTip", "The duration of this AnimNotifyState"), FSlateIcon(), NewAction);
			}

		}
		else
		{
			MenuBuilder.AddSubMenu(
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotify", "Add Notify..."),
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotifyToolTip", "Add AnimNotifyEvent"),
				FNewMenuDelegate::CreateRaw( this, &SAnimNotifyTrack::FillNewNotifyMenu ) );

			MenuBuilder.AddSubMenu(
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotifyState", "Add Notify State..."),
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotifyStateToolTip","Add AnimNotifyState"),
				FNewMenuDelegate::CreateRaw( this, &SAnimNotifyTrack::FillNewNotifyStateMenu ) );

			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("NewNotifySubMenu", "ManageNotifies", "Manage Notifies..."),
				NSLOCTEXT("NewNotifySubMenu", "ManageNotifiesToolTip", "Opens the Manage Notifies window"),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP( this, &SAnimNotifyTrack::OnManageNotifies ) ) );
		}
	}
	MenuBuilder.EndSection(); //AnimNotify

	NewAction.CanExecuteAction = 0;

	MenuBuilder.BeginSection("AnimEdit", LOCTEXT("NotifyEditHeading", "Edit") );
	{
		if ( NotifyIndex != INDEX_NONE )
		{
			// copy notify menu item
			NewAction.ExecuteAction.BindRaw(
				this, &SAnimNotifyTrack::OnCopyNotifyClicked, NotifyIndex);
			MenuBuilder.AddMenuEntry(LOCTEXT("Copy", "Copy"), LOCTEXT("CopyToolTip", "Copy animation notify event"), FSlateIcon(), NewAction);

			// allow it to delete
			NewAction.ExecuteAction = OnDeleteNotify;
			MenuBuilder.AddMenuEntry(LOCTEXT("Delete", "Delete"), LOCTEXT("DeleteToolTip", "Delete animation notify"), FSlateIcon(), NewAction);
		}
		else
		{
			FString PropertyString;
			const TCHAR* Buffer;
			float OriginalTime;
			float OriginalLength;

			//Check whether can we show menu item to paste anim notify event
			if( ReadNotifyPasteHeader(PropertyString, Buffer, OriginalTime, OriginalLength) )
			{
				// paste notify menu item
				if (IsSingleNodeInClipboard())
				{
					NewAction.ExecuteAction.BindRaw(
						this, &SAnimNotifyTrack::OnPasteNotifyClicked, ENotifyPasteMode::MousePosition, ENotifyPasteMultipleMode::Absolute);

					MenuBuilder.AddMenuEntry(LOCTEXT("Paste", "Paste"), LOCTEXT("PasteToolTip", "Paste animation notify event here"), FSlateIcon(), NewAction);
				}
				else
				{
					NewAction.ExecuteAction.BindRaw(
						this, &SAnimNotifyTrack::OnPasteNotifyClicked, ENotifyPasteMode::MousePosition, ENotifyPasteMultipleMode::Relative);

					MenuBuilder.AddMenuEntry(LOCTEXT("PasteMultRel", "Paste Multiple Relative"), LOCTEXT("PasteToolTip", "Paste multiple notifies beginning at the mouse cursor, maintaining the same relative spacing as the source."), FSlateIcon(), NewAction);

					NewAction.ExecuteAction.BindRaw(
						this, &SAnimNotifyTrack::OnPasteNotifyClicked, ENotifyPasteMode::MousePosition, ENotifyPasteMultipleMode::Absolute);

					MenuBuilder.AddMenuEntry(LOCTEXT("PasteMultAbs", "Paste Multiple Absolute"), LOCTEXT("PasteToolTip", "Paste multiple notifies beginning at the mouse cursor, maintaining absolute spacing."), FSlateIcon(), NewAction);
				}

				if(OriginalTime < Sequence->SequenceLength)
				{
					NewAction.ExecuteAction.BindRaw(
						this, &SAnimNotifyTrack::OnPasteNotifyClicked, ENotifyPasteMode::OriginalTime, ENotifyPasteMultipleMode::Absolute);

					FText DisplayText = FText::Format( LOCTEXT("PasteAtOriginalTime", "Paste at original time ({0})"), FText::AsNumber( OriginalTime) );
					MenuBuilder.AddMenuEntry(DisplayText, LOCTEXT("PasteAtOriginalTimeToolTip", "Paste animation notify event at the time it was set to when it was copied"), FSlateIcon(), NewAction);
				}
				
			}
		}
	}
	MenuBuilder.EndSection(); //AnimEdit

	// Display the newly built menu
	TWeakPtr<SWindow> ContextMenuWindow =
		FSlateApplication::Get().PushMenu( SharedThis( this ), MenuBuilder.MakeWidget(), CursorPos, FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu ));

	return TSharedPtr<SWidget>();
}

void SAnimNotifyTrack::CreateCommands()
{
	check(!AnimSequenceEditorActions.IsValid());
	AnimSequenceEditorActions = MakeShareable(new FUICommandList);

	const FAnimSequenceEditorCommands& Commands = FAnimSequenceEditorCommands::Get();

/*
	FUICommandList& ActionList = *AnimSequenceEditorActions;

	ActionList.MapAction( 
		Commands.DeleteNotify, 
		FExecuteAction::CreateRaw( this, &SAnimNotifyTrack::OnDeleteNotifyClicked )
		);*/

}

void SAnimNotifyTrack::OnCopyNotifyClicked(int32 NotifyIndex)
{
	OnCopyNotifies.ExecuteIfBound();
}

bool SAnimNotifyTrack::CanPasteAnimNotify() const
{
	FString PropertyString;
	const TCHAR* Buffer;
	float OriginalTime;
	float OriginalLength;
	return ReadNotifyPasteHeader(PropertyString, Buffer, OriginalTime, OriginalLength);
}

bool SAnimNotifyTrack::ReadNotifyPasteHeader(FString& OutPropertyString, const TCHAR*& OutBuffer, float& OutOriginalTime, float& OutOriginalLength) const
{
	OutBuffer = NULL;
	OutOriginalTime = -1.f;

	FPlatformMisc::ClipboardPaste(OutPropertyString);

	if( !OutPropertyString.IsEmpty() )
	{
		//Remove header text
		const FString HeaderString( TEXT("COPY_ANIMNOTIFYEVENT") );
		
		//Check for string identifier in order to determine whether the text represents an FAnimNotifyEvent.
		if(OutPropertyString.StartsWith( HeaderString ) && OutPropertyString.Len() > HeaderString.Len() )
		{
			int32 HeaderSize = HeaderString.Len();
			OutBuffer = *OutPropertyString;
			OutBuffer += HeaderSize;

			FString ReadLine;
			// Read the original time from the first notify
			FParse::Line(&OutBuffer, ReadLine);
			FParse::Value( *ReadLine, TEXT( "OriginalTime=" ), OutOriginalTime );
			FParse::Value( *ReadLine, TEXT( "OriginalLength=" ), OutOriginalLength );
			return true;
		}
	}

	return false;
}

void SAnimNotifyTrack::OnPasteNotifyClicked(ENotifyPasteMode::Type PasteMode, ENotifyPasteMultipleMode::Type MultiplePasteType)
{
	float ClickTime = PasteMode == ENotifyPasteMode::MousePosition ? LastClickedTime : -1.0f;
	OnPasteNotifies.ExecuteIfBound(this, ClickTime, PasteMode, MultiplePasteType);
}

void SAnimNotifyTrack::OnPasteNotifyTimeSet(const FText& TimeText, ETextCommit::Type CommitInfo)
{
	if ( CommitInfo == ETextCommit::OnEnter || CommitInfo == ETextCommit::OnUserMovedFocus )
	{
		float PasteTime = FCString::Atof( *TimeText.ToString() );
		PasteTime = FMath::Clamp<float>(PasteTime, 0.f, Sequence->SequenceLength-0.01f);
		OnPasteNotify(PasteTime);
	}

	FSlateApplication::Get().DismissAllMenus();
}

void SAnimNotifyTrack::OnPasteNotify(float TimeToPasteAt, ENotifyPasteMultipleMode::Type MultiplePasteType)
{
	FString PropertyString;
	const TCHAR* Buffer;
	float OriginalTime;				// Time in the original track that the node was placed
	float OriginalLength;			// Length of the original anim sequence
	float Shift = 0.0f;				// The amount to shift the saved display time of subsequent nodes
	float ScaleMultiplier = 1.0f;	// Scale value to place nodes relatively in different sequences

	if( ReadNotifyPasteHeader(PropertyString, Buffer, OriginalTime, OriginalLength) )
	{
		// If we're using relative mode, calculate the scale between the two sequences
		if (MultiplePasteType == ENotifyPasteMultipleMode::Relative)
		{
			ScaleMultiplier = Sequence->SequenceLength / OriginalLength;
		}

		//Store object to undo pasting anim notify
		const FScopedTransaction Transaction( LOCTEXT("PasteNotifyEvent", "Paste Anim Notify") );

		Sequence->Modify();

		FString NotifyLine;
		while (FParse::Line(&Buffer, NotifyLine))
		{
			// Add a new Anim Notify Event structure to the array
			int32 NewNotifyIndex = Sequence->Notifies.AddZeroed();

			//Get array property
			UArrayProperty* ArrayProperty = NULL;
			uint8* PropData = Sequence->FindNotifyPropertyData(NewNotifyIndex, ArrayProperty);

			if (PropData && ArrayProperty)
			{
				ArrayProperty->Inner->ImportText(*NotifyLine, PropData, PPF_Copy, NULL);

				FAnimNotifyEvent& NewNotify = Sequence->Notifies[NewNotifyIndex];

				if (TimeToPasteAt >= 0.f)
				{
					Shift = TimeToPasteAt - (NewNotify.DisplayTime * ScaleMultiplier);
					NewNotify.DisplayTime = TimeToPasteAt;

					// Don't want to paste subsequent nodes at this position
					TimeToPasteAt = -1.0f;
				}
				else
				{
					NewNotify.DisplayTime = (NewNotify.DisplayTime * ScaleMultiplier) + Shift;
					// Clamp into the bounds of the sequence
					NewNotify.DisplayTime = FMath::Clamp(NewNotify.DisplayTime, 0.0f, Sequence->SequenceLength);
				}
				NewNotify.TriggerTimeOffset = GetTriggerTimeOffsetForType(Sequence->CalculateOffsetForNotify(NewNotify.DisplayTime));
				NewNotify.TrackIndex = TrackIndex;

				// Get anim notify ptr of the new notify event array element (This ptr is the same as the source anim notify ptr)
				UAnimNotify* AnimNotifyPtr = NewNotify.Notify;
				if (AnimNotifyPtr)
				{
					//Create a new instance of anim notify subclass.
					UAnimNotify* NewAnimNotifyPtr = Cast<UAnimNotify>(StaticDuplicateObject(AnimNotifyPtr, Sequence, TEXT("None")));
					if (NewAnimNotifyPtr)
					{
						//Reassign new anim notify ptr to the pasted anim notify event.
						NewNotify.Notify = NewAnimNotifyPtr;
					}
				}

				UAnimNotifyState * AnimNotifyStatePtr = NewNotify.NotifyStateClass;
				if (AnimNotifyStatePtr)
				{
					//Create a new instance of anim notify subclass.
					UAnimNotifyState * NewAnimNotifyStatePtr = Cast<UAnimNotifyState>(StaticDuplicateObject(AnimNotifyStatePtr, Sequence, TEXT("None")));
					if (NewAnimNotifyStatePtr)
					{
						//Reassign new anim notify ptr to the pasted anim notify event.
						NewNotify.NotifyStateClass = NewAnimNotifyStatePtr;
					}

					if ((NewNotify.DisplayTime + NewNotify.Duration) > Sequence->SequenceLength)
					{
						NewNotify.Duration = Sequence->SequenceLength - NewNotify.DisplayTime;
					}
				}
			}
			else
			{
				//Remove newly created array element if pasting is not successful
				Sequence->Notifies.RemoveAt(NewNotifyIndex);
			}
		}

		OnDeselectAllNotifies.ExecuteIfBound();
		Sequence->MarkPackageDirty();
		OnUpdatePanel.ExecuteIfBound();
	}
}

void SAnimNotifyTrack::OnManageNotifies()
{
	TSharedPtr< FPersona > PersonalPin = PersonaPtr.Pin();
	if( PersonalPin.IsValid() )
	{
		PersonalPin->GetTabManager()->InvokeTab( FPersonaTabs::SkeletonAnimNotifiesID );
	}
}

void SAnimNotifyTrack::SetTriggerWeight(const FText& TriggerWeight, ETextCommit::Type CommitInfo, int32 NotifyIndex)
{
	if ( CommitInfo == ETextCommit::OnEnter || CommitInfo == ETextCommit::OnUserMovedFocus )
	{
		if ( AnimNotifies.IsValidIndex(NotifyIndex) )
		{
			float NewWeight = FMath::Max(FCString::Atof( *TriggerWeight.ToString() ), ZERO_ANIMWEIGHT_THRESH);
			AnimNotifies[NotifyIndex]->TriggerWeightThreshold = NewWeight;
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

bool SAnimNotifyTrack::IsSingleNodeSelected()
{
	return SelectedNotifyIndices.Num() == 1;
}

bool SAnimNotifyTrack::IsSingleNodeInClipboard()
{
	FString PropString;
	const TCHAR* Buffer;
	float OriginalTime;
	float OriginalLength;
	uint32 Count = 0;
	if (ReadNotifyPasteHeader(PropString, Buffer, OriginalTime, OriginalLength))
	{
		// If reading a single line empties the buffer then we only have one notify in there.
		FString TempLine;
		FParse::Line(&Buffer, TempLine);
		return *Buffer == 0;
	}
	return false;
}

void SAnimNotifyTrack::OnSetTriggerWeightNotifyClicked(int32 NotifyIndex)
{
	if (AnimNotifies.IsValidIndex(NotifyIndex))
	{
		FString DefaultText = FString::Printf(TEXT("%0.6f"), AnimNotifies[NotifyIndex]->TriggerWeightThreshold);

		// Show dialog to enter weight
		TSharedRef<STextEntryPopup> TextEntry =
			SNew(STextEntryPopup)
			.Label( LOCTEXT("TriggerWeightNotifyClickedLabel", "Trigger Weight") )
			.DefaultText( FText::FromString(DefaultText) )
			.OnTextCommitted( this, &SAnimNotifyTrack::SetTriggerWeight, NotifyIndex );

		FSlateApplication::Get().PushMenu(
			AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
			TextEntry,
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
			);

		TextEntry->FocusDefaultWidget();		
	}
}

void SAnimNotifyTrack::OnSetDurationNotifyClicked(int32 NotifyIndex)
{
	if (AnimNotifies.IsValidIndex(NotifyIndex))
	{
		FString DefaultText = FString::Printf(TEXT("%f"), AnimNotifies[NotifyIndex]->Duration);

		// Show dialog to enter weight
		TSharedRef<STextEntryPopup> TextEntry =
			SNew(STextEntryPopup)
			.Label(LOCTEXT("DurationNotifyClickedLabel", "Duration"))
			.DefaultText( FText::FromString(DefaultText) )
			.OnTextCommitted( this, &SAnimNotifyTrack::SetDuration, NotifyIndex );

		FSlateApplication::Get().PushMenu(
			AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
			TextEntry,
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
			);

		TextEntry->FocusDefaultWidget();		
	}
}

void SAnimNotifyTrack::SetDuration(const FText& DurationText, ETextCommit::Type CommitInfo, int32 NotifyIndex)
{
	if ( CommitInfo == ETextCommit::OnEnter || CommitInfo == ETextCommit::OnUserMovedFocus )
	{
		if ( AnimNotifies.IsValidIndex(NotifyIndex) )
		{
			float NewDuration = FMath::Max(FCString::Atof( *DurationText.ToString() ), SAnimNotifyNode::MinimumStateDuration);
			float MaxDuration = Sequence->SequenceLength - AnimNotifies[NotifyIndex]->DisplayTime;
			NewDuration = FMath::Min(NewDuration, MaxDuration);
			AnimNotifies[NotifyIndex]->Duration = NewDuration;

			// If we have a delegate bound to refresh the offsets, call it.
			// This is used by the montage editor to keep the offsets up to date.
			OnRequestRefreshOffsets.ExecuteIfBound();
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

void SAnimNotifyTrack::OnNewNotifyClicked()
{
	// Show dialog to enter new track name
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label( LOCTEXT("NewNotifyLabel", "Notify Name") )
		.OnTextCommitted( this, &SAnimNotifyTrack::AddNewNotify );

	// Show dialog to enter new event name
	FSlateApplication::Get().PushMenu(
		AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	TextEntry->FocusDefaultWidget();
}

void SAnimNotifyTrack::AddNewNotify(const FText& NewNotifyName, ETextCommit::Type CommitInfo)
{
	USkeleton * SeqSkeleton = Sequence->GetSkeleton();
	if ((CommitInfo == ETextCommit::OnEnter) && SeqSkeleton)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddNewNotifyEvent", "Add New Anim Notify") );
		FName NewName = FName( *NewNotifyName.ToString() );
		SeqSkeleton->AddNewAnimationNotify(NewName);

		CreateNewNotifyAtCursor(NewNotifyName.ToString(), (UClass*)nullptr);
	}

	FSlateApplication::Get().DismissAllMenus();
}

void SAnimNotifyTrack::Update()
{
	NotifyNodes.Empty();

	TrackArea->SetContent(
		SAssignNew( NodeSlots, SOverlay )
		);

	if ( AnimNotifies.Num() > 0 )
	{
		for (int32 NotifyIndex = 0; NotifyIndex < AnimNotifies.Num(); ++NotifyIndex)
		{
			FAnimNotifyEvent* Event = AnimNotifies[NotifyIndex];

			TSharedPtr<SAnimNotifyNode> AnimNotifyNode;
			
			NodeSlots->AddSlot()
			.Padding(TAttribute<FMargin>::Create(TAttribute<FMargin>::FGetter::CreateSP(this, &SAnimNotifyTrack::GetNotifyTrackPadding, NotifyIndex)))
			.VAlign(VAlign_Center)
			[
				SAssignNew( AnimNotifyNode, SAnimNotifyNode )
				.Sequence(Sequence)
				.AnimNotify(Event)
				.OnNodeDragStarted(this, &SAnimNotifyTrack::OnNotifyNodeDragStarted, NotifyIndex)
				.OnUpdatePanel(OnUpdatePanel)
				.PanTrackRequest(OnRequestTrackPan)
				.ViewInputMin(ViewInputMin)
				.ViewInputMax(ViewInputMax)
				.MarkerBars(MarkerBars)
			];

			NotifyNodes.Add(AnimNotifyNode);
		}
	}
}

int32 SAnimNotifyTrack::GetHitNotifyNode(const FGeometry& MyGeometry, const FVector2D& CursorPosition)
{
	for ( int32 I=NotifyNodes.Num()-1; I>=0; --I ) //Run through from 'top most' Notify to bottom
	{
		if ( NotifyNodes[I].Get()->HitTest(MyGeometry, CursorPosition) )
		{
			return I;
		}
	}

	return INDEX_NONE;
}

FReply SAnimNotifyTrack::OnNotifyNodeDragStarted( TSharedRef<SAnimNotifyNode> NotifyNode, const FVector2D& ScreenCursorPos, const FVector2D& ScreenNodePosition, const bool bDragOnMarker, int32 NotifyIndex ) 
{
	// Check to see if we've already selected the triggering node
	if (!NotifyNode->bSelected)
	{
		SelectNotifyNode(NotifyIndex, false);
	}
	
	// Sort our nodes so we're acessing them in time order
	SelectedNotifyIndices.Sort([this](const int32& A, const int32& B)
	{
		FAnimNotifyEvent* First = AnimNotifies[A];
		FAnimNotifyEvent* Second = AnimNotifies[B];
		return First->DisplayTime < Second->DisplayTime;
	});

	// If we're dragging one of the direction markers we don't need to call any further as we don't want the drag drop op
	if(!bDragOnMarker)
	{
		TArray<TSharedPtr<SAnimNotifyNode>> NodesToDrag;
		const float FirstNodeX = NotifyNodes[SelectedNotifyIndices[0]]->GetWidgetPosition().X;
		
		TSharedRef<SOverlay> DragBox = SNew(SOverlay);
		for (auto Iter = SelectedNotifyIndices.CreateIterator(); Iter; ++Iter)
		{
			TSharedPtr<SAnimNotifyNode> Node = NotifyNodes[*Iter];
			NodesToDrag.Add(Node);
		}
		
		FVector2D DecoratorPosition = NodesToDrag[0]->GetWidgetPosition();
		DecoratorPosition = CachedGeometry.LocalToAbsolute(DecoratorPosition);
		return OnNodeDragStarted.Execute(NodesToDrag, DragBox, ScreenCursorPos, DecoratorPosition, bDragOnMarker);
	}
	else
	{
		// Capture the mouse in the node
		return FReply::Handled().CaptureMouse(NotifyNode).UseHighPrecisionMouseMovement(NotifyNode);
	}
}

FReply SAnimNotifyTrack::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		FVector2D CursorPos = MouseEvent.GetScreenSpacePosition();
		CursorPos = MyGeometry.AbsoluteToLocal(CursorPos);
		int32 HitIndex = GetHitNotifyNode(MyGeometry, CursorPos);

		if (HitIndex!=INDEX_NONE)
		{
			// Hit a node, record the mouse position for use later so we can know when / where a
			// drag happened on the node handles if necessary.
			NotifyNodes[HitIndex]->SetLastMouseDownPosition(CursorPos);
			
			return FReply::Handled().DetectDrag( NotifyNodes[HitIndex].ToSharedRef(), EKeys::LeftMouseButton );
		}
	}

	return FReply::Unhandled();
}

float SAnimNotifyTrack::CalculateTime( const FGeometry& MyGeometry, FVector2D NodePos, bool bInputIsAbsolute )
{
	if(bInputIsAbsolute)
	{
		NodePos = MyGeometry.AbsoluteToLocal(NodePos);
	}
	FTrackScaleInfo ScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0, 0, MyGeometry.Size);
	return FMath::Clamp<float>( ScaleInfo.LocalXToInput(NodePos.X), 0.f, Sequence->SequenceLength );
}

FReply SAnimNotifyTrack::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	return FReply::Unhandled();
}

void SAnimNotifyTrack::HandleNodeDrop(TSharedPtr<SAnimNotifyNode> Node)
{
	ensure(Node.IsValid());

	OnDeselectAllNotifies.ExecuteIfBound();
	const FScopedTransaction Transaction(LOCTEXT("MoveNotifyEvent", "Move Anim Notify"));
	Sequence->Modify();

	float LocalX = GetCachedGeometry().AbsoluteToLocal(Node->GetScreenPosition() + ScrubHandleSize).X;
	float Time = GetCachedScaleInfo().LocalXToInput(LocalX);
	FAnimNotifyEvent* DroppedEvent = Node->NotifyEvent;

	if(Node->GetLastSnappedTime() != -1.0f)
	{
		DroppedEvent->DisplayTime = Node->GetLastSnappedTime();
	}
	else
	{
		DroppedEvent->DisplayTime = Time;
	}
	DroppedEvent->RefreshTriggerOffset(Sequence->CalculateOffsetForNotify(DroppedEvent->DisplayTime));
	DroppedEvent->RefreshEndTriggerOffset(Sequence->CalculateOffsetForNotify(DroppedEvent->DisplayTime + DroppedEvent->Duration));
	DroppedEvent->TrackIndex = TrackIndex;

	Sequence->MarkPackageDirty();
	OnUpdatePanel.ExecuteIfBound();
}

void SAnimNotifyTrack::DisconnectSelectedNodesForDrag(TArray<TSharedPtr<SAnimNotifyNode>>& DragNodes)
{
	if(SelectedNotifyIndices.Num() == 0)
	{
		return;
	}

	const float FirstNodeX = NotifyNodes[SelectedNotifyIndices[0]]->GetWidgetPosition().X;

	for(auto Iter = SelectedNotifyIndices.CreateIterator(); Iter; ++Iter)
	{
		TSharedPtr<SAnimNotifyNode> Node = NotifyNodes[*Iter];
		NodeSlots->RemoveSlot(Node->AsShared());

		DragNodes.Add(Node);
	}
}

void SAnimNotifyTrack::AppendSelectionToSet(FGraphPanelSelectionSet& SelectionSet)
{
	// Add our selection to the provided set
	for(auto Iter = SelectedNotifyIndices.CreateConstIterator(); Iter; ++Iter)
	{
		int32 Index = *Iter;
		check(AnimNotifies.IsValidIndex(Index));
		FAnimNotifyEvent* Event = AnimNotifies[Index];

		if(Event->Notify)
		{
			SelectionSet.Add(Event->Notify);
		}
		else if(Event->NotifyStateClass)
		{
			SelectionSet.Add(Event->NotifyStateClass);
		}
	}
}

void SAnimNotifyTrack::AppendSelectionToArray(TArray<const FAnimNotifyEvent*>& Selection) const
{
	for(int32 Idx : SelectedNotifyIndices)
	{
		check(AnimNotifies.IsValidIndex(Idx));
		FAnimNotifyEvent* Event = AnimNotifies[Idx];

		if(Event)
		{
			Selection.Add(Event);
		}
	}
}

void SAnimNotifyTrack::PasteSingleNotify(FString& NotifyString, float PasteTime)
{
	int32 NewIdx = Sequence->Notifies.AddZeroed();
	UArrayProperty* ArrayProperty = NULL;
	uint8* PropertyData = Sequence->FindNotifyPropertyData(NewIdx, ArrayProperty);

	if(PropertyData && ArrayProperty)
	{
		ArrayProperty->Inner->ImportText(*NotifyString, PropertyData, PPF_Copy, NULL);

		FAnimNotifyEvent& NewNotify = Sequence->Notifies[NewIdx];

		if(PasteTime != -1.0f)
		{
			NewNotify.DisplayTime = PasteTime;
		}

		// Make sure the notify is within the track area
		NewNotify.DisplayTime = FMath::Clamp(NewNotify.DisplayTime, 0.0f, Sequence->SequenceLength);
		NewNotify.TriggerTimeOffset = GetTriggerTimeOffsetForType(Sequence->CalculateOffsetForNotify(NewNotify.DisplayTime));
		NewNotify.TrackIndex = TrackIndex;

		if(NewNotify.Notify)
		{
			UAnimNotify* NewNotifyObject = Cast<UAnimNotify>(StaticDuplicateObject(NewNotify.Notify, Sequence, TEXT("None")));
			check(NewNotifyObject);
			NewNotify.Notify = NewNotifyObject;
		}
		else if(NewNotify.NotifyStateClass)
		{
			UAnimNotifyState* NewNotifyStateObject = Cast<UAnimNotifyState>(StaticDuplicateObject(NewNotify.NotifyStateClass, Sequence, TEXT("None")));
			check(NewNotifyStateObject);
			NewNotify.NotifyStateClass = NewNotifyStateObject;

			// Clamp duration into the sequence
			NewNotify.Duration = FMath::Clamp(NewNotify.Duration, 1 / 30.0f, Sequence->SequenceLength - NewNotify.DisplayTime);
			NewNotify.EndTriggerTimeOffset = GetTriggerTimeOffsetForType(Sequence->CalculateOffsetForNotify(NewNotify.DisplayTime + NewNotify.Duration));
		}
	}
	else
	{
		// Paste failed, remove the notify
		Sequence->Notifies.RemoveAt(NewIdx);
	}

	OnDeselectAllNotifies.ExecuteIfBound();
	Sequence->MarkPackageDirty();
	OnUpdatePanel.ExecuteIfBound();
}

void SAnimNotifyTrack::AppendSelectedNodeWidgetsToArray(TArray<TSharedPtr<SAnimNotifyNode>>& NodeArray) const
{
	for(TSharedPtr<SAnimNotifyNode> Node : NotifyNodes)
	{
		if(Node->bSelected)
		{
			NodeArray.Add(Node);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// SSequenceEdTrack

void SNotifyEdTrack::Construct(const FArguments& InArgs)
{
	Sequence = InArgs._Sequence;
	TrackIndex = InArgs._TrackIndex;
	FAnimNotifyTrack & Track = Sequence->AnimNotifyTracks[InArgs._TrackIndex];
	// @Todo anim: we need to fix this to allow track color to be customizable. 
	// for now name, and track color are given
	Track.TrackColor = ((TrackIndex & 1) != 0) ? FLinearColor(0.9f, 0.9f, 0.9f, 0.9f) : FLinearColor(0.5f, 0.5f, 0.5f);

	TSharedRef<SAnimNotifyPanel> PanelRef = InArgs._AnimNotifyPanel.ToSharedRef();
	AnimPanelPtr = InArgs._AnimNotifyPanel;

	//////////////////////////////
	this->ChildSlot
	[
		SNew( SBorder )
		.Padding( FMargin(2.0f, 2.0f) )
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1)
			[
				// Notification editor panel
				SAssignNew(NotifyTrack, SAnimNotifyTrack)
				.Persona(PanelRef->GetPersona().Pin())
				.Sequence(Sequence)
				.TrackIndex(TrackIndex)
				.AnimNotifies(Track.Notifies)
				.ViewInputMin(InArgs._ViewInputMin)
				.ViewInputMax(InArgs._ViewInputMax)
				.OnSelectionChanged(InArgs._OnSelectionChanged)
				.OnUpdatePanel(InArgs._OnUpdatePanel)
				.OnGetScrubValue(InArgs._OnGetScrubValue)
				.OnGetDraggedNodePos(InArgs._OnGetDraggedNodePos)
				.OnNodeDragStarted(InArgs._OnNodeDragStarted)
				.MarkerBars(InArgs._MarkerBars)
				.TrackColor(Track.TrackColor)
				.OnRequestTrackPan(FPanTrackRequest::CreateSP(PanelRef, &SAnimNotifyPanel::PanInputViewRange))
				.OnRequestOffsetRefresh(InArgs._OnRequestRefreshOffsets)
				.OnDeleteNotify(InArgs._OnDeleteNotify)
				.OnDeselectAllNotifies(InArgs._OnDeselectAllNotifies)
				.OnCopyNotifies(InArgs._OnCopyNotifies)
				.OnPasteNotifies(InArgs._OnPasteNotifies)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride(InArgs._WidgetWidth)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.FillWidth(1)
					[
						// Name of track
						SNew(STextBlock)
						.Text( FText::FromName( Track.TrackName ) )
						.ColorAndOpacity(Track.TrackColor)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						// Name of track
						SNew(SButton)
						.Text(LOCTEXT("AddTrackButtonLabel", "+"))
						.ToolTipText( LOCTEXT("AddTrackTooltip", "Add track above here") )
						.OnClicked( PanelRef, &SAnimNotifyPanel::InsertTrack, TrackIndex + 1 )
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						// Name of track
						SNew(SButton)
						.Text(LOCTEXT("RemoveTrackButtonLabel", "-"))
						.ToolTipText( LOCTEXT("RemoveTrackTooltip", "Remove this track") )
						.OnClicked( PanelRef, &SAnimNotifyPanel::DeleteTrack, TrackIndex )
						.IsEnabled( SNotifyEdTrack::CanDeleteTrack() )
					]
				]
			]
		]
	];
}

bool SNotifyEdTrack::CanDeleteTrack()
{
	return AnimPanelPtr.Pin()->CanDeleteTrack(TrackIndex);
}

//////////////////////////////////////////////////////////////////////////
// SAnimNotifyPanel

void SAnimNotifyPanel::Construct(const FArguments& InArgs)
{
	SAnimTrackPanel::Construct( SAnimTrackPanel::FArguments()
		.WidgetWidth(InArgs._WidgetWidth)
		.ViewInputMin(InArgs._ViewInputMin)
		.ViewInputMax(InArgs._ViewInputMax)
		.InputMin(InArgs._InputMin)
		.InputMax(InArgs._InputMax)
		.OnSetInputViewRange(InArgs._OnSetInputViewRange));

	PersonaPtr = InArgs._Persona;
	Sequence = InArgs._Sequence;
	MarkerBars = InArgs._MarkerBars;

	// @todo anim : this is kinda hack to make sure it has 1 track is alive
	// we can do this whenever import or asset is created, but it's more places to handle than here
	// the function name in that case will need to change
	Sequence->InitializeNotifyTrack();
	Sequence->RegisterOnNotifyChanged(UAnimSequenceBase::FOnNotifyChanged::CreateSP( this, &SAnimNotifyPanel::RefreshNotifyTracks )  );
	PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP( this, &SAnimNotifyPanel::PostUndo ) );
	PersonaPtr.Pin()->RegisterOnGenericDelete(FPersona::FOnDeleteGeneric::CreateSP(this, &SAnimNotifyPanel::OnGenericDelete));

	CurrentPosition = InArgs._CurrentPosition;
	OnSelectionChanged = InArgs._OnSelectionChanged;
	WidgetWidth = InArgs._WidgetWidth;
	OnGetScrubValue = InArgs._OnGetScrubValue;
	OnRequestRefreshOffsets = InArgs._OnRequestRefreshOffsets;

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew( SExpandableArea )
			.AreaTitle( LOCTEXT("Notifies", "Notifies") )
			.BodyContent()
			[
				SAssignNew( PanelArea, SBorder )
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				.Padding( FMargin(2.0f, 2.0f) )
				.ColorAndOpacity( FLinearColor::White )
			]
		]
	];

	Update();
}

SAnimNotifyPanel::~SAnimNotifyPanel()
{
	Sequence->UnregisterOnNotifyChanged(this);
	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->UnregisterOnPostUndo(this);
		PersonaPtr.Pin()->UnregisterOnGenericDelete(this);
	}
}

FReply SAnimNotifyPanel::InsertTrack(int32 TrackIndexToInsert)
{
	// before insert, make sure everything behind is fixed
	for (int32 I=TrackIndexToInsert; I<Sequence->AnimNotifyTracks.Num(); ++I)
	{
		FAnimNotifyTrack & Track = Sequence->AnimNotifyTracks[I];
		for (int32 J=0; J<Track.Notifies.Num(); ++J)
		{
			// fix notifies indices
			Track.Notifies[J]->TrackIndex = I+1;
		}

		// fix track names, for now it's only index
		Track.TrackName = *FString::FromInt(I+2);
	}

	FAnimNotifyTrack NewItem;
	NewItem.TrackName = *FString::FromInt(TrackIndexToInsert+1);
	NewItem.TrackColor = FLinearColor::White;

	Sequence->AnimNotifyTracks.Insert(NewItem, TrackIndexToInsert);
	Sequence->MarkPackageDirty();

	Update();
	return FReply::Handled();
}

FReply SAnimNotifyPanel::DeleteTrack(int32 TrackIndexToDelete)
{
	if (Sequence->AnimNotifyTracks.IsValidIndex(TrackIndexToDelete))
	{
		if (Sequence->AnimNotifyTracks[TrackIndexToDelete].Notifies.Num() == 0)
		{
			// before insert, make sure everything behind is fixed
			for (int32 I=TrackIndexToDelete+1; I<Sequence->AnimNotifyTracks.Num(); ++I)
			{
				FAnimNotifyTrack & Track = Sequence->AnimNotifyTracks[I];
				for (int32 J=0; J<Track.Notifies.Num(); ++J)
				{
					// fix notifies indices
					Track.Notifies[J]->TrackIndex = I-1;
				}

				// fix track names, for now it's only index
				Track.TrackName = *FString::FromInt(I);
			}

			Sequence->AnimNotifyTracks.RemoveAt(TrackIndexToDelete);
			Sequence->MarkPackageDirty();
			Update();
		}
	}
	return FReply::Handled();
}

bool SAnimNotifyPanel::CanDeleteTrack(int32 TrackIndexToDelete)
{
	if (Sequence->AnimNotifyTracks.Num() > 1 && Sequence->AnimNotifyTracks.IsValidIndex(TrackIndexToDelete))
	{
		return Sequence->AnimNotifyTracks[TrackIndexToDelete].Notifies.Num() == 0;
	}

	return false;
}

void SAnimNotifyPanel::DeleteNotify(FAnimNotifyEvent* Notify)
{
	for (int32 I=0; I<Sequence->Notifies.Num(); ++I)
	{
		if (*Notify == Sequence->Notifies[I])
		{
			Sequence->Notifies.RemoveAt(I);
			Sequence->MarkPackageDirty();
			break;
		}
	}
}

void SAnimNotifyPanel::Update()
{
	PersonaPtr.Pin()->OnAnimNotifiesChanged.Broadcast();
	if(Sequence != NULL)
	{
		Sequence->UpdateAnimNotifyTrackCache();
	}
}

bool SAnimNotifyPanel::ReadNotifyPasteHeader(FString& OutPropertyString, const TCHAR*& OutBuffer, float& OutOriginalTime, float& OutOriginalLength, int32& OutTrackSpan) const
{
	OutBuffer = NULL;
	OutOriginalTime = -1.f;

	FPlatformMisc::ClipboardPaste(OutPropertyString);

	if(!OutPropertyString.IsEmpty())
	{
		//Remove header text
		const FString HeaderString(TEXT("COPY_ANIMNOTIFYEVENT"));

		//Check for string identifier in order to determine whether the text represents an FAnimNotifyEvent.
		if(OutPropertyString.StartsWith(HeaderString) && OutPropertyString.Len() > HeaderString.Len())
		{
			int32 HeaderSize = HeaderString.Len();
			OutBuffer = *OutPropertyString;
			OutBuffer += HeaderSize;

			FString ReadLine;
			// Read the original time from the first notify
			FParse::Line(&OutBuffer, ReadLine);
			FParse::Value(*ReadLine, TEXT("OriginalTime="), OutOriginalTime);
			FParse::Value(*ReadLine, TEXT("OriginalLength="), OutOriginalLength);
			FParse::Value(*ReadLine, TEXT("TrackSpan="), OutTrackSpan);
			return true;
		}
	}

	return false;
}

void SAnimNotifyPanel::RefreshNotifyTracks()
{
	check (Sequence);

	TSharedPtr<SVerticalBox> NotifySlots;
	PanelArea->SetContent(
		SAssignNew( NotifySlots, SVerticalBox )
		);

	NotifyAnimTracks.Empty();

	for(int32 I=Sequence->AnimNotifyTracks.Num()-1; I>=0; --I)
	{
		FAnimNotifyTrack & Track = Sequence->AnimNotifyTracks[I];
		TSharedPtr<SNotifyEdTrack> EdTrack;

		NotifySlots->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		[
			SAssignNew(EdTrack, SNotifyEdTrack)
			.TrackIndex(I)
			.Sequence(Sequence)
			.AnimNotifyPanel(SharedThis(this))
			.WidgetWidth(WidgetWidth)
			.ViewInputMin(ViewInputMin)
			.ViewInputMax(ViewInputMax)
			.OnGetScrubValue(OnGetScrubValue)
			.OnGetDraggedNodePos(this, &SAnimNotifyPanel::CalculateDraggedNodePos)
			.OnUpdatePanel(this, &SAnimNotifyPanel::Update)
			.OnSelectionChanged(this, &SAnimNotifyPanel::OnTrackSelectionChanged)
			.OnNodeDragStarted(this, &SAnimNotifyPanel::OnNotifyNodeDragStarted)
			.MarkerBars(MarkerBars)
			.OnRequestRefreshOffsets(OnRequestRefreshOffsets)
			.OnDeleteNotify(this, &SAnimNotifyPanel::OnGenericDelete)
			.OnDeselectAllNotifies(this, &SAnimNotifyPanel::DeselectAllNotifies)
			.OnCopyNotifies(this, &SAnimNotifyPanel::CopySelectedNotifiesToClipboard)
			.OnPasteNotifies(this, &SAnimNotifyPanel::OnPasteNotifies)
		];

		NotifyAnimTracks.Add(EdTrack->NotifyTrack);
	}
}

float SAnimNotifyPanel::CalculateDraggedNodePos() const
{
	return CurrentDragXPosition;
}

FReply SAnimNotifyPanel::OnNotifyNodeDragStarted(TArray<TSharedPtr<SAnimNotifyNode>> NotifyNodes, TSharedRef<SWidget> Decorator, const FVector2D& ScreenCursorPos, const FVector2D& ScreenNodePosition, const bool bDragOnMarker)
{
	TSharedRef<SOverlay> NodeDragDecorator = SNew(SOverlay);
	TArray<TSharedPtr<SAnimNotifyNode>> Nodes;

	for(TSharedPtr<SAnimNotifyTrack> Track : NotifyAnimTracks)
	{
		Track->DisconnectSelectedNodesForDrag(Nodes);
	}

	FVector2D OverlayOrigin = Nodes[0]->GetScreenPosition();
	FVector2D OverlayExtents = Nodes[0]->GetScreenPosition();
	for(int32 Idx = 1 ; Idx < Nodes.Num() ; ++Idx)
	{
		TSharedPtr<SAnimNotifyNode> Node = Nodes[Idx];
		FVector2D NodePosition = Node->GetScreenPosition();
		float NodeDuration = Node->GetDurationSize();

		if(NodePosition.X < OverlayOrigin.X)
		{
			OverlayOrigin.X = NodePosition.X;
		}
		else if(NodePosition.X + NodeDuration > OverlayExtents.X)
		{
			OverlayExtents.X = NodePosition.X + NodeDuration;
		}

		if(NodePosition.Y < OverlayOrigin.Y)
		{
			OverlayOrigin.Y = NodePosition.Y;
		}
		else if(NodePosition.Y + NotifyHeight > OverlayExtents.Y)
		{
			OverlayExtents.Y = NodePosition.Y + NotifyHeight;
		}
	}
	OverlayExtents -= OverlayOrigin;

	for(TSharedPtr<SAnimNotifyNode> Node : Nodes)
	{
		FVector2D OffsetFromFirst(Node->GetScreenPosition() - OverlayOrigin);

		NodeDragDecorator->AddSlot()
			.Padding(FMargin(OffsetFromFirst.X, OffsetFromFirst.Y, 0.0f, 0.0f))
			[
				Node->AsShared()
			];
	}

	FPanTrackRequest PanRequestDelegate = FPanTrackRequest::CreateSP(this, &SAnimNotifyPanel::PanInputViewRange);
	return FReply::Handled().BeginDragDrop(FNotifyDragDropOp::New(Nodes, NodeDragDecorator, NotifyAnimTracks, Sequence, ScreenCursorPos, OverlayOrigin, OverlayExtents, CurrentDragXPosition, PanRequestDelegate, MarkerBars));
}

void SAnimNotifyPanel::PostUndo()
{
	if(Sequence != NULL)
	{
		Sequence->UpdateAnimNotifyTrackCache();
	}
}

void SAnimNotifyPanel::OnGenericDelete()
{
	// If there's no focus on the panel it's likely the user is not editing notifies
	// so don't delete anything when the key is pressed.
	if(HasKeyboardFocus() || HasFocusedDescendants()) 
	{
		TArray<TSharedPtr<SAnimNotifyNode>> SelectedNodes;
		for(TSharedPtr<SAnimNotifyTrack> Track : NotifyAnimTracks)
		{
			Track->AppendSelectedNodeWidgetsToArray(SelectedNodes);
		}

		if(SelectedNodes.Num() > 0)
		{
			FScopedTransaction Transaction(LOCTEXT("DeleteNotifies", "Delete Animation Notifies"));
			Sequence->Modify(true);
			for(TSharedPtr<SAnimNotifyNode> Node : SelectedNodes)
			{
				DeleteNotify(Node->NotifyEvent);
			}
		}

		// clear selection and update the panel
		FGraphPanelSelectionSet ObjectSet;
		OnSelectionChanged.ExecuteIfBound(ObjectSet);

		Update();
	}
}

void SAnimNotifyPanel::SetSequence(class UAnimSequenceBase *	InSequence)
{
	if (InSequence != Sequence)
	{
		Sequence = InSequence;
		// @todo anim : this is kinda hack to make sure it has 1 track is alive
		// we can do this whenever import or asset is created, but it's more places to handle than here
		// the function name in that case will need to change
		Sequence->InitializeNotifyTrack();
		Update();
	}
}

void SAnimNotifyPanel::OnTrackSelectionChanged()
{
	// Need to collect selection info from all tracks
	FGraphPanelSelectionSet SelectionSet;
	for(TSharedPtr<SAnimNotifyTrack> Track : NotifyAnimTracks)
	{
		Track->AppendSelectionToSet(SelectionSet);
	}

	OnSelectionChanged.ExecuteIfBound(SelectionSet);
}

void SAnimNotifyPanel::DeselectAllNotifies()
{
	for(TSharedPtr<SAnimNotifyTrack> Track : NotifyAnimTracks)
	{
		Track->DeselectAllNotifyNodes(false);
	}

	OnTrackSelectionChanged();
}

void SAnimNotifyPanel::CopySelectedNotifiesToClipboard() const
{
	// Grab the selected events
	TArray<const FAnimNotifyEvent*> SelectedNotifies;
	for(TSharedPtr<SAnimNotifyTrack> Track : NotifyAnimTracks)
	{
		Track->AppendSelectionToArray(SelectedNotifies);
	}

	const FString HeaderString(TEXT("COPY_ANIMNOTIFYEVENT"));
	
	if(SelectedNotifies.Num() > 0)
	{
		FString StrValue(HeaderString);

		// Sort by track
		SelectedNotifies.Sort([](const FAnimNotifyEvent& A, const FAnimNotifyEvent& B)
		{
			return (A.TrackIndex > B.TrackIndex) || (A.TrackIndex == B.TrackIndex && A.DisplayTime < B.DisplayTime);
		});

		// Need to find how many tracks this selection spans and the minimum time to use as the beginning of the selection
		int32 MinTrack = MAX_int32;
		int32 MaxTrack = MIN_int32;
		float MinTime = MAX_flt;
		for(const FAnimNotifyEvent* Event : SelectedNotifies)
		{
			MinTrack = FMath::Min(MinTrack, Event->TrackIndex);
			MaxTrack = FMath::Max(MaxTrack, Event->TrackIndex);
			MinTime = FMath::Min(MinTime, Event->DisplayTime);
		}

		int32 TrackSpan = MaxTrack - MinTrack + 1;

		StrValue += FString::Printf(TEXT("OriginalTime=%f,"), MinTime);
		StrValue += FString::Printf(TEXT("OriginalLength=%f,"), Sequence->SequenceLength);
		StrValue += FString::Printf(TEXT("TrackSpan=%d"), TrackSpan);

		for(const FAnimNotifyEvent* Event : SelectedNotifies)
		{
			// Locate the notify in the sequence, we need the sequence index; but also need to
			// keep the order we're currently in.
			int32 Index = INDEX_NONE;
			for(int32 NotifyIdx = 0 ; NotifyIdx < Sequence->Notifies.Num() ; ++NotifyIdx)
			{
				if(Event == &Sequence->Notifies[NotifyIdx])
				{
					Index = NotifyIdx;
					break;
				}
			}

			check(Index != INDEX_NONE);
			
			StrValue += "\n";
			UArrayProperty* ArrayProperty = NULL;
			uint8* PropertyData = Sequence->FindNotifyPropertyData(Index, ArrayProperty);
			if(PropertyData && ArrayProperty)
			{
				ArrayProperty->Inner->ExportTextItem(StrValue, PropertyData, PropertyData, Sequence, PPF_Copy);
			}

		}
		FPlatformMisc::ClipboardCopy(*StrValue);
	}
}

void SAnimNotifyPanel::OnPasteNotifies(SAnimNotifyTrack* RequestTrack, float ClickTime, ENotifyPasteMode::Type PasteMode, ENotifyPasteMultipleMode::Type MultiplePasteType)
{
	int32 PasteIdx = RequestTrack->GetTrackIndex();
	int32 NumTracks = NotifyAnimTracks.Num();
	FString PropString;
	const TCHAR* Buffer;
	float OrigBeginTime;
	float OrigLength;
	int32 TrackSpan;
	int32 FirstTrack = -1;
	float ScaleMultiplier = 1.0f;

	if(ReadNotifyPasteHeader(PropString, Buffer, OrigBeginTime, OrigLength, TrackSpan))
	{
		DeselectAllNotifies();

		FScopedTransaction Transaction(LOCTEXT("PasteNotifyEvent", "Paste Anim Notifies"));
		Sequence->Modify();

		if(ClickTime == -1.0f)
		{
			// We want to place the notifies exactly where they were
			ClickTime = OrigBeginTime;
		}

		// Expand the number of tracks if we don't have enough.
		check(TrackSpan > 0);
		if(PasteIdx - (TrackSpan - 1) < 0)
		{
			int32 TracksToAdd = PasteIdx + TrackSpan - 1;
			while(TracksToAdd)
			{
				InsertTrack(PasteIdx++);
				--TracksToAdd;
			}
			NumTracks = NotifyAnimTracks.Num();
			RequestTrack = NotifyAnimTracks[NumTracks - 1 - PasteIdx].Get();
		}

		// Scaling for relative paste
		if(MultiplePasteType == ENotifyPasteMultipleMode::Relative)
		{
			ScaleMultiplier = Sequence->SequenceLength / OrigLength;
		}

		// Process each line of the paste buffer and spawn notifies
		FString CurrentLine;
		while(FParse::Line(&Buffer, CurrentLine))
		{
			int32 OriginalTrack;
			float OrigTime;
			float PasteTime = -1.0f;
			if(FParse::Value(*CurrentLine, TEXT("TrackIndex="), OriginalTrack) && FParse::Value(*CurrentLine, TEXT("DisplayTime="), OrigTime))
			{
				// Store the first track so we know where to place notifies
				if(FirstTrack < 0)
				{
					FirstTrack = OriginalTrack;
				}
				int32 TrackOffset = OriginalTrack - FirstTrack;

				float TimeOffset = OrigTime - OrigBeginTime;
				float TimeToPaste = ClickTime + TimeOffset * ScaleMultiplier;

				// Have to invert the index here as tracks are stored in reverse
				TSharedPtr<SAnimNotifyTrack> TrackToUse = NotifyAnimTracks[NotifyAnimTracks.Num() - 1 - (PasteIdx + TrackOffset)];

				TrackToUse->PasteSingleNotify(CurrentLine, TimeToPaste);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
