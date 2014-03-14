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
DECLARE_DELEGATE_RetVal( float, FOnGetDraggedNodePos )
DECLARE_DELEGATE_TwoParams( FPanTrackRequest, int32, FVector2D)

class FNotifyDragDropOp;

namespace ENotifyPasteMode
{
	enum Type
	{
		MousePosition,
		SpecifiedTime,
		OriginalTime
	};
}

namespace ENotifyStateHandleHit
{
	enum Type
	{
		Start,
		End,
		None
	};
}
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

	bool						bDrawTooltipToRight;
	bool						bBeingDragged;	

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
		{}

		SLATE_ARGUMENT( TSharedPtr<FPersona>,	Persona )
		SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence )
		SLATE_ARGUMENT( TArray<FAnimNotifyEvent *>, AnimNotifies )
		SLATE_ATTRIBUTE( float, ViewInputMin )
		SLATE_ATTRIBUTE( float, ViewInputMax )
		SLATE_ATTRIBUTE( TArray<FTrackMarkerBar>, MarkerBars)
		SLATE_ARGUMENT( int32, TrackIndex )
		SLATE_ARGUMENT( FLinearColor, TrackColor )
		SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )
		SLATE_EVENT( FOnUpdatePanel, OnUpdatePanel )
		SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
		SLATE_EVENT( FOnGetDraggedNodePos, OnGetDraggedNodePos )
		SLATE_EVENT( FOnNotifyNodeDragStarted, OnNodeDragStarted )
		SLATE_EVENT( FPanTrackRequest, OnRequestTrackPan )
		SLATE_EVENT( FRefreshOffsetsRequest, OnRequestOffsetRefresh )
		SLATE_EVENT( FDeleteNotify, OnDeleteNotify )
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
	void HandleNodeDrop(const FNotifyDragDropOp& DragDropOp);

protected:
	void CreateCommands();

	// Build up a "New Notify..." menu
	void FillNewNotifyMenu(FMenuBuilder& MenuBuilder);
	void FillNewNotifyStateMenu(FMenuBuilder& MenuBuilder);
	void CreateNewNotifyAtCursor(FString NewNotifyName, FString NotifyClassPath);
	void OnNewNotifyClicked();
	void AddNewNotify(const FText& NewNotifyName, ETextCommit::Type CommitInfo);
	void OnSetTriggerWeightNotifyClicked(int32 NotifyIndex);
	void SetTriggerWeight(const FText& TriggerWeight, ETextCommit::Type CommitInfo, int32 NotifyIndex);

	void OnSetDurationNotifyClicked(int32 NotifyIndex);
	void SetDuration(const FText& DurationText, ETextCommit::Type CommitInfo, int32 NotifyIndex);

	/** Function to copy anim notify event */
	void OnCopyNotifyClicked(int32 NotifyIndex);

	/** Function to check whether it is possible to paste anim notify event */
	bool CanPasteAnimNotify() const;

	/** Reads the contents of the clipboard for a notify to paste.
	 *  If successful OutBuffer points to the start of the notify object's data */
	bool ReadNotifyPasteHeader(FString& OutPropertyString, const TCHAR*& OutBuffer, float& OutOriginalTime) const;

	/** Handler for context menu paste command */
	void OnPasteNotifyClicked(ENotifyPasteMode::Type PasteMode);

	/** Handler for popup window asking the user for a paste time */
	void OnPasteNotifyTimeSet(const FText& TimeText, ETextCommit::Type CommitInfo);

	/** Function to paste a previously copied notify */
	void OnPasteNotify(float TimeToPasteAt);

	/** Provides direct access to the notify menu from the context menu */
	void OnManageNotifies();

	/**
	 * Update the nodes to match the data that the panel is observing
	 */
	void Update();

	void SelectNotifyNode(int32 NotifyIndex);

	int32 GetHitNotifyNode(const FGeometry& MyGeometry, const FVector2D& Position);

	TSharedPtr<SWidget> SummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;

	float CalculateTime( const FGeometry& MyGeometry, FVector2D NodePos );

	// Handler that is called when the user starts dragging a node
	FReply OnNotifyNodeDragStarted( TSharedRef<SAnimNotifyNode> NotifyNode, const FVector2D& ScreenCursorPos, const FVector2D& ScreenNodePosition, const bool bDragOnMarker, int32 NotifyIndex );

private:
	// get Property Data of one element (NotifyIndex) from Notifies property of Sequence
	uint8* FindNotifyPropertyData( int32 NotifyIndex, UArrayProperty*& ArrayProperty );

	// Store the tracks geometry for later use
	void UpdateCachedGeometry(const FGeometry& InGeometry) {CachedGeometry = InGeometry;}

	// Returns the padding needed to render the notify in the correct track position
	FMargin GetNotifyTrackPadding(int32 NotifyIndex) const
	{
		float LeftMargin = NotifyNodes[NotifyIndex]->GetWidgetPosition().X;
		float RightMargin = CachedGeometry.Size.X - LeftMargin - NotifyNodes[NotifyIndex]->GetSize().X;
		return FMargin(LeftMargin, 0, RightMargin, 0);
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
	FOnSelectionChanged						OnSelectionChanged;
	FOnUpdatePanel							OnUpdatePanel;
	FOnGetScrubValue						OnGetScrubValue;
	FOnGetDraggedNodePos					OnGetDraggedNodePos;
	FOnNotifyNodeDragStarted				OnNodeDragStarted;
	FPanTrackRequest						OnRequestTrackPan;

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
	{}
	SLATE_ARGUMENT( int32, TrackIndex )
	SLATE_ARGUMENT( TSharedPtr<SAnimNotifyPanel>, AnimNotifyPanel)
	SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence )
	SLATE_ARGUMENT( float, WidgetWidth )
	SLATE_ATTRIBUTE( float, ViewInputMin )
	SLATE_ATTRIBUTE( float, ViewInputMax )
	SLATE_ATTRIBUTE( TArray<FTrackMarkerBar>, MarkerBars)
	SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )
	SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
	SLATE_EVENT( FOnGetDraggedNodePos, OnGetDraggedNodePos )
	SLATE_EVENT( FOnUpdatePanel, OnUpdatePanel )
	SLATE_EVENT( FOnNotifyNodeDragStarted, OnNodeDragStarted )
	SLATE_EVENT( FRefreshOffsetsRequest, OnRequestRefreshOffsets )
	SLATE_EVENT( FDeleteNotify, OnDeleteNotify )
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
	FNotifyDragDropOp(float& InCurrentDragXPosition) : CurrentDragXPosition(InCurrentDragXPosition), SnapTime(-1.f) {}
	struct FTrackClampInfo
	{
		int32 TrackPos;
		int32 TrackSnapTestPos;
		float TrackMax;
		float TrackMin;
		float TrackWidth;
		TSharedPtr<SAnimNotifyTrack> NotifyTrack;
	};

	static FString GetTypeId() {static FString Type = TEXT("FNotifyDragDropOp"); return Type;}

	virtual void OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		if ( bDropWasHandled == false )
		{
			const FTrackClampInfo& ClampInfo = GetTrackClampInfo(LastNodePos+NotifyOffset);

			ClampInfo.NotifyTrack->HandleNodeDrop(*this);

			if (OriginalNotifyNode.IsValid())
			{
				OriginalNotifyNode.Pin()->DropCancelled();
			}
		}
		
		FDragDropOperation::OnDrop(bDropWasHandled, MouseEvent);
	}

	virtual void OnDragged( const class FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		FVector2D NodePos = DragDropEvent.GetScreenSpacePosition() + DragOffset + NotifyOffset;
		const FTrackClampInfo& ClampInfo = GetTrackClampInfo(DragDropEvent.GetScreenSpacePosition());
		
		// Look for a snap on the first scrub handle
		FVector2D TrackNodePos = ClampInfo.NotifyTrack->GetCachedGeometry().AbsoluteToLocal(NodePos);
		float SnapX = GetSnapPosition(ClampInfo, TrackNodePos.X);
		if(SnapX >= 0.f)
		{
			EAnimEventTriggerOffsets::Type Offset = (SnapX < TrackNodePos.X) ? EAnimEventTriggerOffsets::OffsetAfter : EAnimEventTriggerOffsets::OffsetBefore;
			NotifyEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);
			TrackNodePos.X = SnapX;
			NodePos = ClampInfo.NotifyTrack->GetCachedGeometry().LocalToAbsolute(TrackNodePos);
		}
		else
		{
			NotifyEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
		}

		NodePos.Y = ClampInfo.TrackPos;
		NodePos.X = FMath::Clamp(NodePos.X, ClampInfo.TrackMin, ClampInfo.TrackMax);

		if(OriginalNotifyNode.IsValid() && NotifyEvent->Duration > 0)
		{
			// If we didn't snap the beginning of the node, attempt to snap the end
			if(SnapX == -1.0f)
			{
				FVector2D TrackNodeEndPos = TrackNodePos + OriginalNotifyNode.Pin()->GetDurationSize();
				SnapX = GetSnapPosition(ClampInfo, TrackNodeEndPos.X);

				// Only attempt to snap if the node will fit on the track
				if(SnapX >= OriginalNotifyNode.Pin()->GetDurationSize())
				{
					EAnimEventTriggerOffsets::Type Offset = (SnapX < TrackNodeEndPos.X) ? EAnimEventTriggerOffsets::OffsetAfter : EAnimEventTriggerOffsets::OffsetBefore;
					NotifyEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);
					TrackNodeEndPos.X = SnapX;
					NodePos = ClampInfo.NotifyTrack->GetCachedGeometry().LocalToAbsolute(TrackNodeEndPos - OriginalNotifyNode.Pin()->GetDurationSize());
					
					// Fix up the snap time to correctly position the node.
					SnapTime -= NotifyEvent->Duration;

					NodePos.Y = ClampInfo.TrackPos;
					NodePos.X = FMath::Clamp(NodePos.X, ClampInfo.TrackMin, ClampInfo.TrackMax);
				}
				else
				{
					// Remove any trigger time if we can't fit the node in.
					NotifyEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
				}
			}

			//If we have a duration we need to handle clamping at the end of it
			float NodeWidth = OriginalNotifyNode.Pin()->GetDurationSize();
			float NotifyEnd = NodePos.X + NodeWidth;
			float ClampedEnd = FMath::Clamp(NotifyEnd, ClampInfo.TrackMin, ClampInfo.TrackMax);

			if(ClampedEnd != NotifyEnd)
			{
				NodePos.X = ClampedEnd - NodeWidth;
			}
		}

		CurrentDragXPosition = ClampInfo.NotifyTrack->GetCachedGeometry().AbsoluteToLocal(FVector2D(NodePos.X,0.0f)).X;

		NodePos.X -= NotifyOffset.X;
		LastNodePos = NodePos;

		CursorDecoratorWindow->MoveWindowTo(NodePos);

		//scroll view
		float MouseXPos = DragDropEvent.GetScreenSpacePosition().X;
		if(MouseXPos < ClampInfo.TrackMin)
		{
			float ScreenDelta = MouseXPos - ClampInfo.TrackMin;
			RequestTrackPan.Execute(ScreenDelta, FVector2D(ClampInfo.TrackWidth, 1.f));
		}
		else if(MouseXPos > ClampInfo.TrackMax)
		{
			float ScreenDelta = MouseXPos - ClampInfo.TrackMax;
			RequestTrackPan.Execute(ScreenDelta, FVector2D(ClampInfo.TrackWidth, 1.f));
		}
	}

	float GetSnapPosition(const FTrackClampInfo& ClampInfo, float WidgetSpaceNotifyPosition)
	{
		const FTrackScaleInfo& ScaleInfo = ClampInfo.NotifyTrack->GetCachedScaleInfo();

		const float MaxSnapDist = 5.f;

		float CurrentMinSnapDest = MaxSnapDist;
		float SnapPosition = -1.f;
		SnapTime = -1.f;

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
						SnapTime = Bars[i].Time;
						CurrentMinSnapDest = ThisMinSnapDist;
						SnapPosition = WidgetSpaceSnapPosition;
					}
				}
			}
		}
		return SnapPosition;
	}

	const FTrackClampInfo& GetTrackClampInfo(const FVector2D NodePos) const
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

	FAnimNotifyEvent*			NotifyEvent;
	class UAnimSequenceBase*	Sequence;
	FVector2D					DragOffset;
	FVector2D					NotifyOffset;
	FVector2D					LastNodePos;
	TWeakPtr<SAnimNotifyNode>	OriginalNotifyNode;
	TArray<FTrackClampInfo>		ClampInfos;
	float&						CurrentDragXPosition;
	FPanTrackRequest			RequestTrackPan;
	float						SnapTime;
	TAttribute<TArray<FTrackMarkerBar>>	MarkerBars;

	static TSharedRef<FNotifyDragDropOp> New(TSharedRef<SAnimNotifyNode> NotifyNode, const TArray<TSharedPtr<SAnimNotifyTrack>>& NotifyTracks, class UAnimSequenceBase* InSequence, const FVector2D &CursorPosition, const FVector2D &ScreenPositionOfNode, float& CurrentDragXPosition, FPanTrackRequest& RequestTrackPanDelegate, TAttribute<TArray<FTrackMarkerBar>>	MarkerBars)
	{
		TSharedRef<FNotifyDragDropOp> Operation = MakeShareable(new FNotifyDragDropOp(CurrentDragXPosition));
		FSlateApplication::GetDragDropReflector().RegisterOperation<FNotifyDragDropOp>(Operation);
		Operation->NotifyEvent = NotifyNode->NotifyEvent;
		Operation->Sequence = InSequence;
		Operation->OriginalNotifyNode = NotifyNode;
		Operation->RequestTrackPan = RequestTrackPanDelegate;

		Operation->DragOffset = ScreenPositionOfNode - CursorPosition;
		Operation->NotifyOffset = NotifyNode->GetNotifyPositionOffset();
		Operation->MarkerBars = MarkerBars;

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
		return OriginalNotifyNode.Pin();
	}

	FText GetHoverText() const
	{
		FText HoverText = LOCTEXT("Invalid", "Invalid");

		if (NotifyEvent)
		{
			HoverText = FText::FromName( NotifyEvent->NotifyName );
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
	FLinearColor NodeColour = NotifyEvent->bSelected ? FLinearColor(1.0f, 0.5f, 0.0f) : FLinearColor::Red;
	
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
	if( NotifyEvent->bSelected )
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
				this, &SAnimNotifyTrack::CreateNewNotifyAtCursor,
				Label,
				BlueprintPath);

			MenuBuilder.AddMenuEntry(LabelText, Description, FSlateIcon(), UIAction);
		}
	}
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
				this, &SAnimNotifyTrack::CreateNewNotifyAtCursor,
				Label,
				BlueprintPath);

			MenuBuilder.AddMenuEntry(LabelText, Description, FSlateIcon(), UIAction);
		}
	}

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
					FString());

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

void SAnimNotifyTrack::CreateNewNotifyAtCursor(FString NewNotifyName, FString BlueprintPath)
{
	TSubclassOf<UObject> BlueprintClass = NULL;
	if( !BlueprintPath.IsEmpty() )
	{
		UBlueprint* BlueprintLibPtr = LoadObject<UBlueprint>(NULL, *BlueprintPath, NULL, 0, NULL);
		BlueprintClass = Cast<UClass>(BlueprintLibPtr->GeneratedClass);
	}

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

	if( BlueprintClass )
	{
		class UObject * AnimNotifyClass = NewObject<UObject>(Sequence, *BlueprintClass);
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
		struct FindClassPred
		{
			FindClassPred( const UClass* InClass )
				: Class(InClass)
			{
			}
			bool operator()( const FAssetData& AssetData ) const
			{
				if( AssetData.GetAsset()->IsA( Class ) )
				{
					return true;
				}
				return false;
			}
		private:
			const UClass* Class;
		};

		TArray<FAssetData> SelectedAssets;
		AssetSelectionUtils::GetSelectedAssets(SelectedAssets);

		for( TFieldIterator<UObjectProperty> PropIt(NewEvent.Notify->GetClass()); PropIt; ++PropIt )
		{
			if(PropIt->GetBoolMetaData(TEXT("ExposeOnSpawn")))
			{
				const FAssetData* Asset = SelectedAssets.FindByPredicate( FindClassPred(PropIt->PropertyClass) );
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
		SelectNotifyNode(NotifyIndex);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SAnimNotifyTrack::SelectNotifyNode(int32 NotifyIndex)
{
	if( NotifyIndex != INDEX_NONE )
	{
		// Deselect all other notifies first.
		if( Sequence )
		{
			for(int32 Index=0; Index<Sequence->Notifies.Num(); Index++)
			{
				FAnimNotifyEvent & NotifyEvent = Sequence->Notifies[Index];
				NotifyEvent.bSelected = false;
			}
		}

		// select new one
		FGraphPanelSelectionSet ObjectSet;
		if( NotifyIndex < NotifyNodes.Num() )
		{
			FAnimNotifyEvent * NotifyEvent = NotifyNodes[NotifyIndex].Get()->NotifyEvent;
			NotifyEvent->bSelected = true;
			if( NotifyEvent->Notify )
			{
				ObjectSet.Add(NotifyEvent->Notify);	
				OnSelectionChanged.ExecuteIfBound(ObjectSet);
			}
			else if( NotifyEvent->NotifyStateClass )
			{
				ObjectSet.Add(NotifyEvent->NotifyStateClass);	
				OnSelectionChanged.ExecuteIfBound(ObjectSet);
			}
		}
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
			SelectNotifyNode(NotifyIndex);

			// add menu to get threshold weight for triggering this notify
			NewAction.ExecuteAction.BindRaw(
				this, &SAnimNotifyTrack::OnSetTriggerWeightNotifyClicked, NotifyIndex);

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

			//Check whether can we show menu item to paste anim notify event
			if( ReadNotifyPasteHeader(PropertyString, Buffer, OriginalTime) )
			{
				// paste notify menu item
				NewAction.ExecuteAction.BindRaw(
					this, &SAnimNotifyTrack::OnPasteNotifyClicked, ENotifyPasteMode::MousePosition);

				MenuBuilder.AddMenuEntry(LOCTEXT("Paste", "Paste"), LOCTEXT("PasteToolTip", "Paste animation notify event here"), FSlateIcon(), NewAction);

				NewAction.ExecuteAction.BindRaw(
					this, &SAnimNotifyTrack::OnPasteNotifyClicked, ENotifyPasteMode::SpecifiedTime);

				MenuBuilder.AddMenuEntry(LOCTEXT("PasteAt", "Paste at..."), LOCTEXT("PasteAtToolTip", "Paste animation notify event at a specified time"), FSlateIcon(), NewAction);

				if(OriginalTime < Sequence->SequenceLength)
				{
					NewAction.ExecuteAction.BindRaw(
						this, &SAnimNotifyTrack::OnPasteNotifyClicked, ENotifyPasteMode::OriginalTime);

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

uint8* SAnimNotifyTrack::FindNotifyPropertyData( int32 NotifyIndex, UArrayProperty*& ArrayProperty )
{
	// initialize to NULL
	ArrayProperty = NULL;

	if ( Sequence && Sequence->Notifies.IsValidIndex(NotifyIndex) )
	{
		// find Notifies property start point
		UProperty* Property = FindField<UProperty>( Sequence->GetClass(), TEXT("Notifies"));

		// found it and if it is array
		if( Property && Property->IsA( UArrayProperty::StaticClass() ))
		{
			// find Property Value from UObject we got
			uint8* PropertyValue = Property->ContainerPtrToValuePtr<uint8>(Sequence);

			// it is array, so now get ArrayHelper and find the raw ptr of the data
			ArrayProperty = CastChecked<UArrayProperty>(Property);
			FScriptArrayHelper ArrayHelper(ArrayProperty, PropertyValue);

			if (ArrayProperty->Inner && NotifyIndex < ArrayHelper.Num())
			{
				//Get property data based on selected index
				return ArrayHelper.GetRawPtr(NotifyIndex);
			}
		}
	}

	return NULL;
}

void SAnimNotifyTrack::OnCopyNotifyClicked(int32 NotifyIndex)
{
	if (AnimNotifies.IsValidIndex(NotifyIndex))
	{
		int32 NotifyEventIndex = INDEX_NONE;
		for( int32 i=0; i<Sequence->Notifies.Num(); i++ )
		{
			// find which index is what I owned
			if( &(Sequence->Notifies[i]) == AnimNotifies[NotifyIndex] )
			{
				NotifyEventIndex = i;
				break;
			}
		}


		//Get array property
		UArrayProperty* ArrayProperty = NULL;
		uint8* PropData = FindNotifyPropertyData( NotifyEventIndex, ArrayProperty );

		if (PropData)
		{
			//Append a sting identifier at the start in order to check whether the text represents an FAnimNotifyEvent structure.
			const FString HeaderString( TEXT("COPY_ANIMNOTIFYEVENT") );
			FString StrValue(HeaderString);
			StrValue += FString::Printf( TEXT("OriginalTime=%f\n"), Sequence->Notifies[NotifyEventIndex].DisplayTime);

			//Export property to text
			ArrayProperty->Inner->ExportTextItem( StrValue, PropData, PropData, Sequence, PPF_Copy );
			FPlatformMisc::ClipboardCopy( *StrValue );
		}
	}
}

bool SAnimNotifyTrack::CanPasteAnimNotify() const
{
	FString PropertyString;
	const TCHAR* Buffer;
	float OriginalTime;
	return ReadNotifyPasteHeader(PropertyString, Buffer, OriginalTime);
}

bool SAnimNotifyTrack::ReadNotifyPasteHeader(FString& OutPropertyString, const TCHAR*& OutBuffer, float& OutOriginalTime) const
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
			FParse::Line( &OutBuffer, ReadLine );	// Need this to advance PastePtr, for multiple sockets
			FParse::Value( *ReadLine, TEXT( "OriginalTime=" ), OutOriginalTime );
			return true;
		}
	}

	return false;
}

void SAnimNotifyTrack::OnPasteNotifyClicked(ENotifyPasteMode::Type PasteMode)
{
	switch(PasteMode)
	{
		case ENotifyPasteMode::MousePosition:
		{
			OnPasteNotify(LastClickedTime);
			break;
		}
		case ENotifyPasteMode::SpecifiedTime:
		{
			FString DefaultText = FString::Printf(TEXT("%f"), LastClickedTime);

			TSharedRef<STextEntryPopup> TextEntry =
				SNew(STextEntryPopup)
				.Label(LOCTEXT("PasteAtTimeLabel", "Time"))
				.DefaultText( FText::FromString(DefaultText) )
				.OnTextCommitted( this, &SAnimNotifyTrack::OnPasteNotifyTimeSet );

			FSlateApplication::Get().PushMenu(
				AsShared(),
				TextEntry,
				FSlateApplication::Get().GetCursorPos(),
				FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
				);

			TextEntry->FocusDefaultWidget();
			break;
		}
		case ENotifyPasteMode::OriginalTime:
		{
			OnPasteNotify(-1.f);
			break;
		}
	}
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

void SAnimNotifyTrack::OnPasteNotify(float TimeToPasteAt)
{
	FString PropertyString;
	const TCHAR* Buffer;
	float OriginalTime;

	if( ReadNotifyPasteHeader(PropertyString, Buffer, OriginalTime) )
	{
		//Store object to undo pasting anim notify
		const FScopedTransaction Transaction( LOCTEXT("PasteNotifyEvent", "Paste Anim Notify") );
		Sequence->Modify();

		// Add a new Anim Notify Event structure to the array
		int32 NewNotifyIndex = Sequence->Notifies.AddZeroed();

		//Get array property
		UArrayProperty* ArrayProperty = NULL;
		uint8* PropData = FindNotifyPropertyData( NewNotifyIndex, ArrayProperty );

		if( PropData && ArrayProperty )
		{
			ArrayProperty->Inner->ImportText( Buffer, PropData, PPF_Copy, NULL );

			FAnimNotifyEvent& NewNotify = Sequence->Notifies[NewNotifyIndex];

			if(TimeToPasteAt >= 0.f)
			{
				NewNotify.DisplayTime = TimeToPasteAt;
				NewNotify.TriggerTimeOffset = GetTriggerTimeOffsetForType(Sequence->CalculateOffsetForNotify(TimeToPasteAt));
			}
			NewNotify.TrackIndex = TrackIndex;

			// Get anim notify ptr of the new notify event array element (This ptr is the same as the source anim notify ptr)
			UAnimNotify* AnimNotifyPtr = NewNotify.Notify;
			if(AnimNotifyPtr)
			{
				//Create a new instance of anim notify subclass.
				UAnimNotify* NewAnimNotifyPtr = Cast<UAnimNotify>(StaticDuplicateObject(AnimNotifyPtr, Sequence, TEXT("None")));
				if( NewAnimNotifyPtr )
				{
					//Reassign new anim notify ptr to the pasted anim notify event.
					NewNotify.Notify = NewAnimNotifyPtr;
				}
			}

			UAnimNotifyState * AnimNotifyStatePtr = NewNotify.NotifyStateClass;
			if( AnimNotifyStatePtr )
			{
				//Create a new instance of anim notify subclass.
				UAnimNotifyState * NewAnimNotifyStatePtr = Cast<UAnimNotifyState>(StaticDuplicateObject(AnimNotifyStatePtr, Sequence, TEXT("None")));
				if( NewAnimNotifyStatePtr )
				{
					//Reassign new anim notify ptr to the pasted anim notify event.
					NewNotify.NotifyStateClass = NewAnimNotifyStatePtr;
				}

				if( (NewNotify.DisplayTime + NewNotify.Duration) > Sequence->SequenceLength)
				{
					NewNotify.Duration = Sequence->SequenceLength - NewNotify.DisplayTime;
				}
			}

			Sequence->MarkPackageDirty();
			OnUpdatePanel.ExecuteIfBound();
		}	
		else
		{
			//Remove newly created array element if pasting is not successful
			Sequence->Notifies.RemoveAt(NewNotifyIndex);
		}
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

		CreateNewNotifyAtCursor(NewNotifyName.ToString(), "");
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
	SelectNotifyNode(NotifyIndex);

	// If we're dragging one of the direction markers we don't need to call any further as we don't want the drag drop op
	if(!bDragOnMarker)
	{
		NodeSlots->RemoveSlot(NotifyNode);
		return OnNodeDragStarted.Execute(NotifyNode, ScreenCursorPos, ScreenNodePosition, bDragOnMarker);
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

float SAnimNotifyTrack::CalculateTime( const FGeometry& MyGeometry, FVector2D NodePos )
{
	NodePos = MyGeometry.AbsoluteToLocal(NodePos);
	FTrackScaleInfo ScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0, 0, MyGeometry.Size);
	return FMath::Clamp<float>( ScaleInfo.LocalXToInput(NodePos.X), 0.f, Sequence->SequenceLength );
	}

FReply SAnimNotifyTrack::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FNotifyDragDropOp>( DragDropEvent.GetOperation() ) )
	{
		TSharedPtr<FNotifyDragDropOp> DragDropOp = StaticCastSharedPtr<FNotifyDragDropOp>(DragDropEvent.GetOperation());

		HandleNodeDrop(*DragDropOp);

		return FReply::Handled();
}

	return FReply::Unhandled();
}

void SAnimNotifyTrack::HandleNodeDrop(const FNotifyDragDropOp& DragDropOp)
{
	// if sample is highlighted, overwrite the animation for it
	// make sure if it's same sequence we're dealing with 
	UAnimSequenceBase* DroppedSequence = DragDropOp.Sequence;
	FAnimNotifyEvent* DroppedEvent = DragDropOp.NotifyEvent; 
	if (DroppedSequence == Sequence)
	{
		DragDropOp.CurrentDragXPosition = -1.0f;

		const FScopedTransaction Transaction( LOCTEXT("MoveNotifyEvent", "Move Anim Notify") );
		Sequence->Modify();

		if(DragDropOp.SnapTime != -1)
		{
			DroppedEvent->DisplayTime = DragDropOp.SnapTime;
		}
		else
		{
			DroppedEvent->DisplayTime = CalculateTime(CachedGeometry, DragDropOp.LastNodePos+DragDropOp.NotifyOffset);
		}
		DroppedEvent->RefreshTriggerOffset(Sequence->CalculateOffsetForNotify(DroppedEvent->DisplayTime));
		DroppedEvent->RefreshEndTriggerOffset(Sequence->CalculateOffsetForNotify(DroppedEvent->DisplayTime + DroppedEvent->Duration));

		DroppedEvent->TrackIndex = TrackIndex;

		Sequence->MarkPackageDirty();
		OnUpdatePanel.ExecuteIfBound();
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
		if (Notify == &Sequence->Notifies[I])
		{
			Sequence->Notifies.RemoveAt(I);
			Sequence->MarkPackageDirty();
			break;
		}
	}

	// clear selection before updating
	FGraphPanelSelectionSet ObjectSet;
	OnSelectionChanged.ExecuteIfBound(ObjectSet);

	Update();
}

void SAnimNotifyPanel::Update()
{
	PersonaPtr.Pin()->OnAnimNotifiesChanged.Broadcast();
	if(Sequence != NULL)
	{
		Sequence->UpdateAnimNotifyTrackCache();
	}
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
			.OnSelectionChanged(OnSelectionChanged)
			.OnNodeDragStarted(this, &SAnimNotifyPanel::OnNotifyNodeDragStarted)
			.MarkerBars(MarkerBars)
			.OnRequestRefreshOffsets(OnRequestRefreshOffsets)
			.OnDeleteNotify(this, &SAnimNotifyPanel::OnGenericDelete)
		];

		NotifyAnimTracks.Add(EdTrack->NotifyTrack);
	}
}

float SAnimNotifyPanel::CalculateDraggedNodePos() const
{
	return CurrentDragXPosition;
}

FReply SAnimNotifyPanel::OnNotifyNodeDragStarted(TSharedRef<SAnimNotifyNode> NotifyNode, const FVector2D& ScreenCursorPos, const FVector2D& ScreenNodePosition, const bool bDragOnMarker)
{
	FPanTrackRequest PanRequestDelegate = FPanTrackRequest::CreateSP(this, &SAnimNotifyPanel::PanInputViewRange);
	return FReply::Handled().BeginDragDrop(FNotifyDragDropOp::New(NotifyNode, NotifyAnimTracks, Sequence, ScreenCursorPos, ScreenNodePosition, CurrentDragXPosition, PanRequestDelegate, MarkerBars));
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
		// Find selected notifies and delete them
		for(int32 NotifyIndex = 0 ; NotifyIndex < Sequence->Notifies.Num() ;)
		{
			FAnimNotifyEvent& Event = Sequence->Notifies[NotifyIndex];
			if(Event.bSelected)
			{
				DeleteNotify(&Event);
			}
			else
			{
				// Only increment if we don't remove something
				NotifyIndex++;
			}
		}
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

#undef LOCTEXT_NAMESPACE
