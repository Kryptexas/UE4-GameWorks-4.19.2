// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SAnimMontagePanel.h"
#include "ScopedTransaction.h"
#include "SCurveEditor.h"
#include "SAnimationSequenceBrowser.h"
#include "SAnimSegmentsPanel.h"
#include "SMontageEditor.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"

#define LOCTEXT_NAMESPACE "AnimMontagePanel"

//////////////////////////////////////////////////////////////////////////
// SMontageBranchingPointNode

const float MontageBranchingPointHandleWidth = 12.f;
const float BranchingPointAlignmentMarkerWidth = 8.f;

class FBranchingPointDragDropOp : public FTrackNodeDragDropOp
{
public:

	static TSharedRef<FTrackNodeDragDropOp> New(TSharedRef<STrackNode> TrackNode, const FVector2D &CursorPosition, const FVector2D &ScreenPositionOfNode)
	{
		TSharedRef<FBranchingPointDragDropOp> Operation = MakeShareable(new FBranchingPointDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FBranchingPointDragDropOp>(Operation);
		Operation->OriginalTrackNode = TrackNode;

		Operation->Offset = ScreenPositionOfNode - CursorPosition;
		Operation->StartingScreenPos = ScreenPositionOfNode;

		Operation->Construct();

		return Operation;
	}

	virtual void OnDragged( const class FDragDropEvent& DragDropEvent ) OVERRIDE;
};

class SMontageBranchingPointNode : public STrackNode
{
public:
	SLATE_BEGIN_ARGS( SMontageBranchingPointNode )
		: _Montage()
		, _BranchingPoint()
		{}
		SLATE_ARGUMENT( class UAnimMontage*, Montage )
		SLATE_ARGUMENT( FBranchingPoint*, BranchingPoint )
		SLATE_ATTRIBUTE( float, ViewInputMin )
		SLATE_ATTRIBUTE( float, ViewInputMax )
		SLATE_ATTRIBUTE( float, DataLength )
		SLATE_ATTRIBUTE( float, DataStartPos )
		SLATE_ATTRIBUTE( FString, NodeName )
		SLATE_ATTRIBUTE( FLinearColor, NodeColor )
		SLATE_ATTRIBUTE( FLinearColor, SelectedNodeColor )
		SLATE_ARGUMENT( STrackNodeSelectionSet *, NodeSelectionSet )
		SLATE_ARGUMENT( bool, AllowDrag )
		SLATE_EVENT( FOnTrackNodeDragged, OnTrackNodeDragged )
		SLATE_EVENT( FOnTrackNodeDropped, OnTrackNodeDropped )
		SLATE_EVENT( FOnNodeSelectionChanged, OnSelectionChanged )
		SLATE_EVENT( FOnNodeRightClickContextMenu, OnNodeRightClickContextMenu )
		SLATE_EVENT( FOnTrackNodeClicked, OnTrackNodeClicked )
		SLATE_ARGUMENT( bool, CenterOnPosition )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;

	virtual FVector2D GetDragDropScreenSpacePosition(const FGeometry& ParentAllottedGeometry, const FDragDropEvent& DragDropEvent) const OVERRIDE;

	virtual FReply BeginDrag( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;

	bool SnapToDragBars() const {return true;}

	virtual void OnSnapNodeDataPosition(float OriginalX, float SnappedX);

	FBranchingPoint* GetBranchingPoint() const { return BranchingPoint; }

private:
	UAnimMontage* Montage;

	FBranchingPoint* BranchingPoint;

	FString GetBranchingPointText() const;
	virtual FVector2D GetSize() const OVERRIDE;

	// virtual draw related functions STrackNode
	virtual FVector2D GetOffsetRelativeToParent(const FGeometry& ParentAllottedGeometry) const OVERRIDE;
	virtual FVector2D GetSizeRelativeToParent(const FGeometry& ParentAllottedGeometry) const OVERRIDE;
};

void FBranchingPointDragDropOp::OnDragged( const class FDragDropEvent& DragDropEvent )
{
	TSharedPtr<STrackNode> Node = OriginalTrackNode.Pin();

	FVector2D NodePos = (DragDropEvent.GetScreenSpacePosition() + Offset);

	if (Node.IsValid())
	{
		Node->OnDragged(DragDropEvent);
		const FGeometry& TrackGeometry = Node->GetTrackGeometry();
		NodePos = Node->GetOffsetRelativeToParent(TrackGeometry) + TrackGeometry.AbsolutePosition;
	}

	NodePos.Y = StartingScreenPos.Y;

	CursorDecoratorWindow->MoveWindowTo(NodePos);
}

void SMontageBranchingPointNode::Construct(const FArguments& InArgs)
{
	STrackNode::Construct( STrackNode::FArguments()
		.ViewInputMin(InArgs._ViewInputMin)
		.ViewInputMax(InArgs._ViewInputMax)
		.DataLength(InArgs._DataLength)
		.DataStartPos(InArgs._DataStartPos)
		.NodeName(InArgs._NodeName)
		.NodeColor(InArgs._NodeColor)
		.SelectedNodeColor(InArgs._SelectedNodeColor)
		.NodeSelectionSet(InArgs._NodeSelectionSet)
		.AllowDrag(InArgs._AllowDrag)
		.OnTrackNodeDragged(InArgs._OnTrackNodeDragged)
		.OnTrackNodeDropped(InArgs._OnTrackNodeDropped)
		.OnSelectionChanged(InArgs._OnSelectionChanged)
		.OnNodeRightClickContextMenu(InArgs._OnNodeRightClickContextMenu)
		.OnTrackNodeClicked(InArgs._OnTrackNodeClicked)
		.CenterOnPosition(false) ); // ignored anyway

	Montage = InArgs._Montage;
	BranchingPoint = InArgs._BranchingPoint;
}

FString SMontageBranchingPointNode::GetBranchingPointText() const
{
	return BranchingPoint->EventName.ToString();
}

FVector2D SMontageBranchingPointNode::GetDragDropScreenSpacePosition(const FGeometry& ParentAllottedGeometry, const FDragDropEvent& DragDropEvent) const
{
	return STrackNode::GetDragDropScreenSpacePosition(ParentAllottedGeometry, DragDropEvent) + FVector2D((MontageBranchingPointHandleWidth * 0.5f + 2), 0.0f);
}

FVector2D SMontageBranchingPointNode::GetOffsetRelativeToParent(const FGeometry& AllottedGeometry) const
{
	FTrackScaleInfo ScaleInfo(ViewInputMin.Get(), ViewInputMax.Get(), 0, 0, AllottedGeometry.Size);

	return FVector2D( ScaleInfo.InputToLocalX(DataStartPos.Get()) - (MontageBranchingPointHandleWidth * 0.5f + 2), 0);
}

FVector2D SMontageBranchingPointNode::GetSizeRelativeToParent(const FGeometry& AllottedGeometry) const
{
	return GetSize();
}

FVector2D SMontageBranchingPointNode::GetSize() const
{
	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector2D DrawSize = FontMeasureService->Measure(GetBranchingPointText(), Font)+FVector2D(5, 5)+FVector2D(MontageBranchingPointHandleWidth, 0);
	return DrawSize;
}

FReply SMontageBranchingPointNode::BeginDrag( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FVector2D ScreenCursorPos = MouseEvent.GetScreenSpacePosition();
	FVector2D CursorPos = MyGeometry.AbsoluteToLocal(ScreenCursorPos);
	FVector2D ScreenNodePosition = MyGeometry.AbsolutePosition;

	bBeingDragged = true;

	return FReply::Handled().BeginDragDrop(FBranchingPointDragDropOp::New(SharedThis(this), ScreenCursorPos, ScreenNodePosition));
}

void SMontageBranchingPointNode::OnSnapNodeDataPosition(float OriginalX, float SnappedX)
{
	EAnimEventTriggerOffsets::Type Offset = EAnimEventTriggerOffsets::NoOffset;
	if(OriginalX != SnappedX)
	{
		Offset = OriginalX < SnappedX ? EAnimEventTriggerOffsets::OffsetBefore : EAnimEventTriggerOffsets::OffsetAfter;
	}
	BranchingPoint->TriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);
}

int32 SMontageBranchingPointNode::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 TextLayerID = LayerId+1;

	FString Text = GetBranchingPointText();

	FVector2D DrawSize = AllottedGeometry.Size;//GetSize();
	FVector2D TextPos(0, 0); //GetPosition(AllottedGeometry);

	FPaintGeometry MyGeometry =	AllottedGeometry.ToPaintGeometry();
	// draw border first
	TArray<FVector2D> LinePoints;


	LinePoints.Empty();
	LinePoints.Add(TextPos);
	LinePoints.Add(TextPos+FVector2D(DrawSize.X, 0.f));
	LinePoints.Add(TextPos+FVector2D(DrawSize.X, DrawSize.Y));
	LinePoints.Add(TextPos+FVector2D(0.f, DrawSize.Y));
	LinePoints.Add(TextPos);

	FSlateDrawElement::MakeLines( 
		OutDrawElements,
		TextLayerID,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		MyClippingRect,
		ESlateDrawEffect::None,
		FLinearColor::Black, 
		false
		);

	FLinearColor UseNodeColor = NodeColor.Get();
	FLinearColor HandleColor = NodeColor.Get();
	if (IsSelected())
	{
		UseNodeColor = SelectedNodeColor.Get();
		HandleColor = FLinearColor::Red;
	}

	const FSlateBrush* StyleInfo = FEditorStyle::GetBrush( TEXT( "SpecialEditableTextImageNormal" ) );
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId, 
		MyGeometry, 
		StyleInfo, 
		MyClippingRect, 
		ESlateDrawEffect::None,
		UseNodeColor);

	MyGeometry = AllottedGeometry.ToPaintGeometry( TextPos + FVector2D(2+MontageBranchingPointHandleWidth, 2), DrawSize );
	FSlateDrawElement::MakeText( 
		OutDrawElements,
		TextLayerID,
		MyGeometry,
		Text,
		Font,
		MyClippingRect,
		ESlateDrawEffect::None,
		FLinearColor::Black
		);

	MyGeometry = AllottedGeometry.ToPaintGeometry( FVector2D(2+TextPos.X, TextPos.Y), FVector2D(MontageBranchingPointHandleWidth, 16) );
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId, 
		MyGeometry, 
		FEditorStyle::GetBrush( TEXT( "Sequencer.Timeline.ScrubHandleUp" ) ), 
		MyClippingRect, 
		ESlateDrawEffect::None,
		HandleColor
		);

	if(BranchingPoint->TriggerTimeOffset != 0.f) //Do we have an offset to render?
	{
		if(BranchingPoint->DisplayTime != 0.f && BranchingPoint->DisplayTime != Montage->SequenceLength) //Don't render offset when we are at the start/end of the sequence, doesn't help the user
		{
			// ScrubHandle
			FVector2D MarkerPosition = TextPos;
			FVector2D MarkerSize = FVector2D(BranchingPointAlignmentMarkerWidth,16.f);

			if(BranchingPoint->TriggerTimeOffset > 0.f)
			{
				MarkerPosition.X += 2.f*BranchingPointAlignmentMarkerWidth;
				MarkerSize.X = -MarkerSize.X;
			}

			FSlateDrawElement::MakeBox( 
				OutDrawElements,
				LayerId + 1, 
				AllottedGeometry.ToPaintGeometry(MarkerPosition, MarkerSize), 
				FEditorStyle::GetBrush( TEXT( "Sequencer.Timeline.NotifyAlignmentMarker" ) ), 
				MyClippingRect, 
				ESlateDrawEffect::None,
				FLinearColor(0.f, 0.f, 1.f)
				);
		}
	}

	return TextLayerID;
}

//////////////////////////////////////////////////////////////////////////
// SAnimMontagePanel

void SAnimMontagePanel::Construct(const FArguments& InArgs)
{
	SAnimTrackPanel::Construct( SAnimTrackPanel::FArguments()
		.WidgetWidth(InArgs._WidgetWidth)
		.ViewInputMin(InArgs._ViewInputMin)
		.ViewInputMax(InArgs._ViewInputMax)
		.InputMin(InArgs._InputMin)
		.InputMax(InArgs._InputMax)
		.OnSetInputViewRange(InArgs._OnSetInputViewRange));

	Persona = InArgs._Persona;
	Montage = InArgs._Montage;
	MontageEditor = InArgs._MontageEditor;

	//TrackStyle = TRACK_Double;

	CurrentPosition = InArgs._CurrentPosition;

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew( SExpandableArea )
			.AreaTitle( LOCTEXT("Montage", "Montage") )
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

/** This is the main function that creates the UI widgets for the montage tool.*/
void SAnimMontagePanel::Update()
{
	ClearSelected();
	if ( Montage != NULL )
	{
		SMontageEditor * Editor = MontageEditor.Pin().Get();
		int32 ColorIdx=0;
		//FLinearColor Colors[] = { FLinearColor(0.9f, 0.9f, 0.9f, 0.9f), FLinearColor(0.5f, 0.5f, 0.5f) };
		
		TSharedPtr<FTrackColorTracker> ColourTracker = MakeShareable(new FTrackColorTracker);
		ColourTracker->AddColor(FLinearColor(0.9f, 0.9f, 0.9f, 0.9f));
		ColourTracker->AddColor(FLinearColor(0.5f, 0.5f, 0.5f));

		FLinearColor NodeColor = FLinearColor(0.f, 0.5f, 0.0f, 0.5f);
		//FLinearColor SelectedColor = FLinearColor(1.0f,0.65,0.0f);

		FLinearColor BranchNodeColor = FLinearColor(0.f, 0.5f, 0.0f, 0.5f);
		FLinearColor BranchSelectedColor = FLinearColor(1.0f,0.65,0.0f);
		

		TSharedPtr<SVerticalBox> MontageSlots;
		PanelArea->SetContent(
			SAssignNew( MontageSlots, SVerticalBox )
			);

		// ===================================
		// Section Name Track
		// ===================================
		{
			TSharedRef<S2ColumnWidget> SectionTrack = Create2ColumnWidget(MontageSlots.ToSharedRef());

			SectionTrack->LeftColumn->ClearChildren();
			SectionTrack->LeftColumn->AddSlot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				.Padding( FMargin(0.5f, 0.5f) )
				[
					SNew(STrack)
					.ViewInputMin(ViewInputMin)
					.ViewInputMax(ViewInputMax)
					.TrackColor( ColourTracker->GetNextColor() )
					.OnBarDrag(Editor, &SMontageEditor::OnEditSectionTime)
					.OnBarDrop(Editor, &SMontageEditor::OnEditSectionTimeFinish)
					.OnBarClicked(SharedThis(this), &SAnimMontagePanel::ShowSectionInDetailsView)
					.DraggableBars(Editor, &SMontageEditor::GetSectionStartTimes)
					.DraggableBarSnapPositions(Editor, &SMontageEditor::GetAnimSegmentStartTimes)
					.DraggableBarLabels(Editor, &SMontageEditor::GetSectionNames)
					.TrackMaxValue(Montage->SequenceLength) // FIXME -> Make actual attribute!
					.TrackNumDiscreteValues(Montage->GetNumberOfFrames())
					.OnTrackRightClickContextMenu( this, &SAnimMontagePanel::SummonTrackContextMenu, static_cast<int>(INDEX_NONE))
					.ScrubPosition( Editor, &SMontageEditor::GetScrubValue )
				];
		}

		// ===================================
		// Anim Segment Tracks
		// ===================================
		{
			for (int32 SlotAnimIdx=0; SlotAnimIdx < Montage->SlotAnimTracks.Num(); SlotAnimIdx++)
			{
				TSharedRef<S2ColumnWidget> SectionTrack = Create2ColumnWidget(MontageSlots.ToSharedRef());

				// Right column
				SectionTrack->RightColumn->AddSlot()
					.VAlign(VAlign_Center)
				.FillHeight(1)
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( FMargin(5.0f, 0.5f) )
					[
						SNew(SEditableTextBox)
						.Style( FEditorStyle::Get(), "SpecialEditableTextBox" )
						.HintText( LOCTEXT("SlotName", "Slot Name") )
						.Text( Editor, &SMontageEditor::GetMontageSlotName, SlotAnimIdx )
						.Font( FEditorStyle::GetFontStyle("Editor.SearchBoxFont") )
						.SelectAllTextWhenFocused( true )
						.RevertTextOnEscape( true )
						.ClearKeyboardFocusOnCommit( false )
						.OnTextCommitted( this, &SAnimMontagePanel::OnSlotNodeNameChangeCommit, SlotAnimIdx )
					]
				];

				SectionTrack->LeftColumn->AddSlot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				[
					SNew(SAnimSegmentsPanel)
					.AnimTrack(&Montage->SlotAnimTracks[SlotAnimIdx].AnimTrack)
					.NodeSelectionSet(&SelectionSet)
					.ViewInputMin(ViewInputMin)
					.ViewInputMax(ViewInputMax)
					.ColorTracker(ColourTracker)
					.NodeColor(NodeColor)
					.DraggableBars(Editor, &SMontageEditor::GetSectionStartTimes)
					.DraggableBarSnapPositions(Editor, &SMontageEditor::GetAnimSegmentStartTimes)
					.ScrubPosition( Editor, &SMontageEditor::GetScrubValue )
					.TrackMaxValue(Montage->SequenceLength)
					.TrackNumDiscreteValues(Montage->GetNumberOfFrames())

					.OnAnimSegmentNodeClicked( this, &SAnimMontagePanel::ShowSegmentInDetailsView, SlotAnimIdx )
					.OnPreAnimUpdate( Editor, &SMontageEditor::PreAnimUpdate )
					.OnPostAnimUpdate( Editor, &SMontageEditor::PostAnimUpdate )
					.OnBarDrag(Editor, &SMontageEditor::OnEditSectionTime)
					.OnBarDrop(Editor, &SMontageEditor::OnEditSectionTimeFinish)
					.OnBarClicked(SharedThis(this), &SAnimMontagePanel::ShowSectionInDetailsView)
					.OnTrackRightClickContextMenu( this, &SAnimMontagePanel::SummonTrackContextMenu, static_cast<int>(SlotAnimIdx))
				];
			}
		}
		// ===================================
		// Branch PointTrack
		// ===================================
		{
			TSharedPtr<STrack> BranchTrack;
			TSharedRef<S2ColumnWidget> BranchEdTrack = Create2ColumnWidget(MontageSlots.ToSharedRef());
			BranchEdTrack->LeftColumn->ClearChildren();
			BranchEdTrack->LeftColumn->AddSlot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				.Padding( FMargin(0.5f, 0.5f) )
				[
					SAssignNew(BranchTrack, STrack)
					.ViewInputMin(ViewInputMin)
					.ViewInputMax(ViewInputMax)
					.TrackColor( ColourTracker->GetNextColor() )
					.OnBarDrag(Editor, &SMontageEditor::OnEditSectionTime)
					.OnBarDrop(Editor, &SMontageEditor::OnEditSectionTimeFinish)
					.OnBarClicked(SharedThis(this), &SAnimMontagePanel::ShowSectionInDetailsView)
					.DraggableBars(Editor, &SMontageEditor::GetSectionStartTimes)
					.DraggableBarSnapPositions(Editor, &SMontageEditor::GetAnimSegmentStartTimes)
					.TrackMaxValue(Montage->SequenceLength) // FIXME -> Make actual attribute!
					.TrackNumDiscreteValues(Montage->GetNumberOfFrames())
					.OnTrackRightClickContextMenu( this, &SAnimMontagePanel::SummonTrackContextMenu, static_cast<int>(INDEX_NONE))
					.ScrubPosition( Editor, &SMontageEditor::GetScrubValue )
				];

			// Generate Branch Point Nodes and add them to the track
			for (int32 BranchIdx=0; BranchIdx < Montage->BranchingPoints.Num(); BranchIdx++)
			{
				BranchTrack->AddTrackNode(
					SNew(SMontageBranchingPointNode)
					.Montage(Montage)
					.BranchingPoint(&Montage->BranchingPoints[BranchIdx])
					.ViewInputMax(this->ViewInputMax)
					.ViewInputMin(this->ViewInputMin)
					.NodeColor(BranchNodeColor)
					.SelectedNodeColor(BranchSelectedColor)
					.DataLength( 0.f )
					.CenterOnPosition(true)
					.DataStartPos(Editor, &SMontageEditor::GetBranchPointStartPos, BranchIdx)
					.ToolTipText(Editor, &SMontageEditor::GetBranchPointName, BranchIdx)
					.OnTrackNodeDragged( Editor, &SMontageEditor::SetBranchPointStartPos, BranchIdx)
					.OnTrackNodeDropped( Editor, &SMontageEditor::OnMontageChange, static_cast<UObject*>(NULL), true)
					.OnNodeRightClickContextMenu( this, &SAnimMontagePanel::SummonBranchNodeContextMenu, BranchIdx)
					.OnTrackNodeClicked( this, &SAnimMontagePanel::ShowBranchPointInDetailsView, BranchIdx )
					.NodeSelectionSet(&SelectionSet)
					);
			}
		}
	}
}

void SAnimMontagePanel::SetMontage(class UAnimMontage * InMontage)
{
	if (InMontage != Montage)
	{
		Montage = InMontage;
		Update();
	}
}

FReply SAnimMontagePanel::OnMouseButtonDown( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	FReply Reply = SAnimTrackPanel::OnMouseButtonDown( InMyGeometry, InMouseEvent );

	ClearSelected();

	return Reply;
}

void SAnimMontagePanel::SummonTrackContextMenu( FMenuBuilder& MenuBuilder, float DataPosX, int32 SectionIndex, int32 AnimSlotIndex )
{
	FUIAction UIAction;

	MenuBuilder.BeginSection("AnimMontageBranch", LOCTEXT("Branch Point", "Branch Point"));
	{
		UIAction.ExecuteAction.BindRaw(this, &SAnimMontagePanel::OnNewBranchClicked, static_cast<float>(DataPosX));
		MenuBuilder.AddMenuEntry(LOCTEXT("NewBranchPoint", "New Branch Point"), LOCTEXT("NewBranchPointToolTip", "Adds a new Branch Point"), FSlateIcon(), UIAction);
	}
	MenuBuilder.EndSection();

	// Sections
	MenuBuilder.BeginSection("AnimMontageSections", LOCTEXT("Sections", "Sections") );
	{
		UIAction.ExecuteAction.BindRaw(this, &SAnimMontagePanel::OnNewSectionClicked, static_cast<float>(DataPosX));
		MenuBuilder.AddMenuEntry(LOCTEXT("NewMontageSection", "New Montage Section"), LOCTEXT("NewMontageSectionToolTip", "Adds a new Montage Section"), FSlateIcon(), UIAction);
	
		if (SectionIndex != INDEX_NONE && Montage->CompositeSections.Num() > 1) // Can't delete the last section!
		{
			UIAction.ExecuteAction.BindRaw(MontageEditor.Pin().Get(), &SMontageEditor::RemoveSection, SectionIndex);
			MenuBuilder.AddMenuEntry(LOCTEXT("DeleteMontageSection", "Delete Montage Section"), LOCTEXT("DeleteMontageSectionToolTip", "Deletes Montage Section"), FSlateIcon(), UIAction);
		}
	}
	MenuBuilder.EndSection();

	// Slots
	MenuBuilder.BeginSection("AnimMontageSlots", LOCTEXT("Slots", "Slots") );
	{
		UIAction.ExecuteAction.BindRaw(this, &SAnimMontagePanel::OnNewSlotClicked);
		MenuBuilder.AddMenuEntry(LOCTEXT("NewSlot", "New Slot"), LOCTEXT("NewSlotToolTip", "Adds a new Slot"), FSlateIcon(), UIAction);

		if(AnimSlotIndex != INDEX_NONE)
		{
			UIAction.ExecuteAction.BindRaw(MontageEditor.Pin().Get(), &SMontageEditor::RemoveMontageSlot, AnimSlotIndex);
			MenuBuilder.AddMenuEntry(LOCTEXT("DeleteSlot", "Delete Slot"), LOCTEXT("DeleteSlotToolTip", "Deletes Slot"), FSlateIcon(), UIAction);
		}
	}
	MenuBuilder.EndSection();

	LastContextHeading.Empty();
}

void SAnimMontagePanel::SummonBranchNodeContextMenu( FMenuBuilder& MenuBuilder, int32 BranchIndex )
{
	FUIAction UIAction;

	MenuBuilder.BeginSection("AnimMontagePanelBranchPoint", LOCTEXT("Branch Point", "Branch Point") );
	{
		UIAction.ExecuteAction.BindRaw(MontageEditor.Pin().Get(), &SMontageEditor::RemoveBranchPoint, BranchIndex);
		MenuBuilder.AddMenuEntry(LOCTEXT("DeleteBranch", "Delete Branch"), LOCTEXT("DeleteBranchToolTip", "Deletes Branch"), FSlateIcon(), UIAction);
	}
	MenuBuilder.EndSection();

	LastContextHeading = "Branch Point";
}

/** Slots */
void SAnimMontagePanel::OnNewSlotClicked()
{
	// Show dialog to enter new track name
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label( LOCTEXT("NewSlotNameLabel", "Slot Name") )
		.OnTextCommitted( this, &SAnimMontagePanel::CreateNewSlot );


	// Show dialog to enter new event name
	FSlateApplication::Get().PushMenu(
		AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	TextEntry->FocusDefaultWidget();
}

void SAnimMontagePanel::CreateNewSlot(const FText& NewSlotName, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		MontageEditor.Pin()->AddNewMontageSlot(NewSlotName.ToString());
	}

	FSlateApplication::Get().DismissAllMenus();
}

/** Sections */
void SAnimMontagePanel::OnNewSectionClicked(float DataPosX)
{
	// Show dialog to enter new track name
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label( LOCTEXT("NewSectionNameLabel", "Section Name") )
		.OnTextCommitted( this, &SAnimMontagePanel::CreateNewSection, DataPosX );


	// Show dialog to enter new event name
	FSlateApplication::Get().PushMenu(
		AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	TextEntry->FocusDefaultWidget();
}

void SAnimMontagePanel::CreateNewSection(const FText& NewSectionName, ETextCommit::Type CommitInfo, float StartTime)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		MontageEditor.Pin()->AddNewSection(StartTime,NewSectionName.ToString());
	}
	FSlateApplication::Get().DismissAllMenus();
}

/** Branches */
void SAnimMontagePanel::OnNewBranchClicked(float DataPosX)
{
	// Show dialog to enter new track name
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label( LOCTEXT("NewBranchNameLabel", "Branch Name") )
		.OnTextCommitted( this, &SAnimMontagePanel::CreateNewBranch, DataPosX );


	// Show dialog to enter new event name
	FSlateApplication::Get().PushMenu(
		AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	TextEntry->FocusDefaultWidget();
}

void SAnimMontagePanel::CreateNewBranch(const FText& NewBranchName, ETextCommit::Type CommitInfo, float StartTime)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		MontageEditor.Pin()->AddBranchPoint(StartTime,NewBranchName.ToString());
	}
	FSlateApplication::Get().DismissAllMenus();
}

void SAnimMontagePanel::OnBranchPointClicked( int32 BranchPointIndex )
{
	// Show dialog to enter new track name
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label( LOCTEXT("BranchPointClickedLabel", "Branch Name") )
		.DefaultText( FText::FromName(Montage->BranchingPoints[BranchPointIndex].EventName) )
		.OnTextCommitted( this, &SAnimMontagePanel::RenameBranchPoint, BranchPointIndex );


	// Show dialog to enter new event name
	FSlateApplication::Get().PushMenu(
		AsShared(), // Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	TextEntry->FocusDefaultWidget();
}

void SAnimMontagePanel::RenameBranchPoint(const FText &NewName, ETextCommit::Type CommitInfo, int32 BranchPointIndex)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		MontageEditor.Pin()->RenameBranchPoint(BranchPointIndex, NewName.ToString() );
		FSlateApplication::Get().DismissAllMenus();
	}
}

void SAnimMontagePanel::ShowSegmentInDetailsView(int32 AnimSegmentIndex, int32 AnimSlotIndex)
{
	UEditorAnimSegment *Obj = Cast<UEditorAnimSegment>(MontageEditor.Pin()->ShowInDetailsView(UEditorAnimSegment::StaticClass()));
	if(Obj != NULL)
	{
		Obj->InitAnimSegment(AnimSlotIndex,AnimSegmentIndex);
	}
}

void SAnimMontagePanel::ShowBranchPointInDetailsView(int32 BranchPointIndex)
{
	UEditorBranchPoint *Obj = Cast<UEditorBranchPoint>(MontageEditor.Pin()->ShowInDetailsView(UEditorBranchPoint::StaticClass()));
	if(Obj != NULL)
	{
		Obj->InitBranchPoint(BranchPointIndex);
	}
}

void SAnimMontagePanel::ShowSectionInDetailsView(int32 SectionIndex)
{
	MontageEditor.Pin()->ShowSectionInDetailsView(SectionIndex);
}

void SAnimMontagePanel::ClearSelected()
{
	SelectionSet.Empty();
	MontageEditor.Pin()->ClearDetailsView();
}

void SAnimMontagePanel::OnSlotNodeNameChangeCommit( const FText& NewText, ETextCommit::Type CommitInfo, int32 SlodeNodeIndex )
{
	MontageEditor.Pin()->RenameSlotNode(SlodeNodeIndex, NewText.ToString());
}

#undef LOCTEXT_NAMESPACE