// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "SAnimMontagePanel.h"
#include "ScopedTransaction.h"
#include "SCurveEditor.h"
#include "SAnimationSequenceBrowser.h"
#include "SAnimSegmentsPanel.h"
#include "SMontageEditor.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "SExpandableArea.h"
#include "STextEntryPopup.h"
#include "STextComboBox.h"


#define LOCTEXT_NAMESPACE "AnimMontagePanel"


//////////////////////////////////////////////////////////////////////////
// SMontageBranchingPointNode

const float MontageBranchingPointHandleWidth = 12.f;
const float BranchingPointAlignmentMarkerWidth = 8.f;

class FBranchingPointDragDropOp : public FTrackNodeDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FBranchingPointDragDropOp, FTrackNodeDragDropOp)

	static TSharedRef<FTrackNodeDragDropOp> New(TSharedRef<STrackNode> TrackNode, const FVector2D &CursorPosition, const FVector2D &ScreenPositionOfNode)
	{
		TSharedRef<FBranchingPointDragDropOp> Operation = MakeShareable(new FBranchingPointDragDropOp);
		Operation->OriginalTrackNode = TrackNode;

		Operation->Offset = ScreenPositionOfNode - CursorPosition;
		Operation->StartingScreenPos = ScreenPositionOfNode;

		Operation->Construct();

		return Operation;
	}

	virtual void OnDragged( const class FDragDropEvent& DragDropEvent ) override;
};

class SMontageBranchingPointNode : public STrackNode
{
public:
	SLATE_BEGIN_ARGS( SMontageBranchingPointNode )
		: _Montage()
		, _BranchingPoint()
		, _AllowDrag(true)
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
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D GetDragDropScreenSpacePosition(const FGeometry& ParentAllottedGeometry, const FDragDropEvent& DragDropEvent) const override;

	virtual FReply BeginDrag( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;

	bool SnapToDragBars() const {return true;}

	virtual void OnSnapNodeDataPosition(float OriginalX, float SnappedX);

	FBranchingPoint* GetBranchingPoint() const { return BranchingPoint; }

private:
	UAnimMontage* Montage;

	FBranchingPoint* BranchingPoint;

	FString GetBranchingPointText() const;
	virtual FVector2D GetSize() const override;

	// virtual draw related functions STrackNode
	virtual FVector2D GetOffsetRelativeToParent(const FGeometry& ParentAllottedGeometry) const override;
	virtual FVector2D GetSizeRelativeToParent(const FGeometry& ParentAllottedGeometry) const override;
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

int32 SMontageBranchingPointNode::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
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
		float BranchingPointTime = BranchingPoint->GetTime();
		if(BranchingPointTime != 0.f && BranchingPointTime != Montage->SequenceLength) //Don't render offset when we are at the start/end of the sequence, doesn't help the user
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

		/************************************************************************/
		/* Status Bar                                                                     */
		/************************************************************************/
		{
			MontageSlots->AddSlot()
				.AutoHeight()
				[
					// Header, shows name of timeline we are editing
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
					.HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.FillWidth(3.f)
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						[
							SAssignNew(StatusBarWarningImage, SImage)
							.Image(FEditorStyle::GetBrush("AnimSlotManager.Warning"))
							.Visibility(EVisibility::Hidden)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(StatusBarTextBlock, STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12))
							.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
						]
					]
				];
		}

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
			int32 NumAnimTracks = Montage->SlotAnimTracks.Num();

			SlotNameComboBoxes.Empty(NumAnimTracks);
			SlotNameComboSelectedNames.Empty(NumAnimTracks);
			SlotWarningImages.Empty(NumAnimTracks);
			SlotNameComboBoxes.AddZeroed(NumAnimTracks);
			SlotNameComboSelectedNames.AddZeroed(NumAnimTracks);
			SlotWarningImages.AddZeroed(NumAnimTracks);

			RefreshComboLists();
			check(SlotNameComboBoxes.Num() == NumAnimTracks);
			check(SlotNameComboSelectedNames.Num() == NumAnimTracks);

			for (int32 SlotAnimIdx = 0; SlotAnimIdx < NumAnimTracks; SlotAnimIdx++)
			{
				TSharedRef<S2ColumnWidget> SectionTrack = Create2ColumnWidget(MontageSlots.ToSharedRef());
				
				int32 FoundIndex = SlotNameList.Find(SlotNameComboSelectedNames[SlotAnimIdx]);
				TSharedPtr<FString> ComboItem = SlotNameComboListItems[FoundIndex];

				// Right column
				SectionTrack->RightColumn->AddSlot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.FillHeight(1)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						SAssignNew(SlotNameComboBoxes[SlotAnimIdx], STextComboBox)
						.OptionsSource(&SlotNameComboListItems)
						.OnSelectionChanged(this, &SAnimMontagePanel::OnSlotNameChanged, SlotAnimIdx)
						.OnComboBoxOpening(this, &SAnimMontagePanel::OnSlotListOpening, SlotAnimIdx)
						.InitiallySelectedItem(ComboItem)
						.ContentPadding(2)
						.ToolTipText(*ComboItem)
					]

					+ SVerticalBox::Slot()
						.HAlign(HAlign_Left)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Left)
							[
								SNew(SButton)
								.Text(LOCTEXT("AnimSlotNode_DetailPanelManageButtonLabel", "Anim Slot Manager"))
								.ToolTipText(LOCTEXT("AnimSlotNode_DetailPanelManageButtonToolTipText", "Open Anim Slot Manager to edit Slots and Groups."))
								.OnClicked(this, &SAnimMontagePanel::OnOpenAnimSlotManager)
								.Content()
								[
									SNew(SImage)
									.Image(FEditorStyle::GetBrush("MeshPaint.FindInCB"))
								]
							]

							+ SHorizontalBox::Slot()
								.AutoWidth()
								.FillWidth(2.f)
								.HAlign(HAlign_Left)
								.VAlign(VAlign_Center)
								[
									SAssignNew(SlotWarningImages[SlotAnimIdx], SImage)
									.Image(FEditorStyle::GetBrush("AnimSlotManager.Warning"))
									.Visibility(EVisibility::Hidden)
								]
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

	UpdateSlotGroupWarningVisibility();
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
	MenuBuilder.BeginSection("AnimMontageSections", LOCTEXT("Sections", "Sections"));
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

	MenuBuilder.BeginSection("AnimMontageElementBulkActions", LOCTEXT("BulkLinkActions", "Bulk Link Actions"));
	{
		MenuBuilder.AddSubMenu(LOCTEXT("SetElementLink_SubMenu", "Set Elements to..."), LOCTEXT("SetElementLink_TooTip", "Sets all montage elements (Branching Points, Sections, Notifies) to a chosen link type."), FNewMenuDelegate::CreateSP(this, &SAnimMontagePanel::FillElementSubMenuForTimes));

		if(Montage->SlotAnimTracks.Num() > 1)
		{
			MenuBuilder.AddSubMenu(LOCTEXT("SetToSlotMenu", "Link all Elements to Slot..."), LOCTEXT("SetToSlotMenuToolTip", "Link all elements to a selected slot"), FNewMenuDelegate::CreateSP(this, &SAnimMontagePanel::FillSlotSubMenu));
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

void SAnimMontagePanel::FillElementSubMenuForTimes(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(LOCTEXT("SubLinkAbs", "Absolute"), LOCTEXT("SubLinkAbs", "Set all elements to absolute link"), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimMontagePanel::OnSetElementsToLinkMode, EAnimLinkMethod::Absolute)));
	MenuBuilder.AddMenuEntry(LOCTEXT("SubLinkRel", "Relative"), LOCTEXT("SubLinkRel", "Set all elements to relative link"), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimMontagePanel::OnSetElementsToLinkMode, EAnimLinkMethod::Relative)));
	MenuBuilder.AddMenuEntry(LOCTEXT("SubLinkPro", "Proportional"), LOCTEXT("SubLinkPro", "Set all elements to proportional link"), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimMontagePanel::OnSetElementsToLinkMode, EAnimLinkMethod::Proportional)));
}

void SAnimMontagePanel::FillSlotSubMenu(FMenuBuilder& Menubuilder)
{
	for(int32 SlotIdx = 0 ; SlotIdx < Montage->SlotAnimTracks.Num() ; ++SlotIdx)
	{
		FSlotAnimationTrack& Slot = Montage->SlotAnimTracks[SlotIdx];
		Menubuilder.AddMenuEntry(FText::Format(LOCTEXT("SubSlotMenuNameEntry", "{SlotName}"), FText::FromString(Slot.SlotName.ToString())), LOCTEXT("SubSlotEntry", "Set to link to this slot"), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimMontagePanel::OnSetElementsToSlot, SlotIdx)));
	}
}

/** Slots */
void SAnimMontagePanel::OnNewSlotClicked()
{
	MontageEditor.Pin()->AddNewMontageSlot(FAnimSlotGroup::DefaultSlotName.ToString());
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

void SAnimMontagePanel::OnSlotNameChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo, int32 AnimSlotIndex)
{
	// if it's set from code, we did that on purpose
	if (SelectInfo != ESelectInfo::Direct)
	{
		int32 ItemIndex = SlotNameComboListItems.Find(NewSelection);
		if (ItemIndex != INDEX_NONE)
		{
			FName NewSlotName = SlotNameList[ItemIndex];

			SlotNameComboSelectedNames[AnimSlotIndex] = NewSlotName;
			if (SlotNameComboBoxes[AnimSlotIndex].IsValid())
			{
				SlotNameComboBoxes[AnimSlotIndex]->SetToolTipText(FText::FromString(*NewSelection));
			}

			if (Montage->GetSkeleton()->ContainsSlotName(NewSlotName))
			{
				MontageEditor.Pin()->RenameSlotNode(AnimSlotIndex, NewSlotName.ToString());
			}
			
			UpdateSlotGroupWarningVisibility();

			// Clear selection, so Details panel for AnimNotifies doesn't show outdated information.
			ClearSelected();
		}
	}
}

void SAnimMontagePanel::OnSlotListOpening(int32 AnimSlotIndex)
{
	// Refresh Slot Names, in case we used the Anim Slot Manager to make changes.
	RefreshComboLists(true);
}

FReply SAnimMontagePanel::OnOpenAnimSlotManager()
{
	if (Persona.IsValid())
	{
		Persona.Pin()->GetTabManager()->InvokeTab(FPersonaTabs::SkeletonSlotNamesID);
	}
	return FReply::Handled();
}

void SAnimMontagePanel::RefreshComboLists(bool bOnlyRefreshIfDifferent /*= false*/)
{
	// Make sure all slots defined in the montage are registered in our skeleton.
	int32 NumAnimTracks = Montage->SlotAnimTracks.Num();
	for (int32 TrackIndex = 0; TrackIndex < NumAnimTracks; TrackIndex++)
	{
		FName TrackSlotName = Montage->SlotAnimTracks[TrackIndex].SlotName;
		Montage->GetSkeleton()->RegisterSlotNode(TrackSlotName);
		SlotNameComboSelectedNames[TrackIndex] = TrackSlotName;
	}

	// Refresh Slot Names
	{
		TArray<TSharedPtr<FString>>	NewSlotNameComboListItems;
		TArray<FName> NewSlotNameList;

		bool bIsSlotNameListDifferent = false;

		const TArray<FAnimSlotGroup>& SlotGroups = Montage->GetSkeleton()->GetSlotGroups();
		for (auto SlotGroup : SlotGroups)
		{
			int32 Index = 0;
			for (auto SlotName : SlotGroup.SlotNames)
			{
				NewSlotNameList.Add(SlotName);

				FString ComboItemString = FString::Printf(TEXT("%s.%s"), *SlotGroup.GroupName.ToString(), *SlotName.ToString());
				NewSlotNameComboListItems.Add(MakeShareable(new FString(ComboItemString)));

				bIsSlotNameListDifferent = bIsSlotNameListDifferent || (!SlotNameComboListItems.IsValidIndex(Index) || (SlotNameComboListItems[Index] != NewSlotNameComboListItems[Index]));
				Index++;
			}
		}

		// Refresh if needed
		if (bIsSlotNameListDifferent || !bOnlyRefreshIfDifferent || (NewSlotNameComboListItems.Num() == 0))
		{
			SlotNameComboListItems = NewSlotNameComboListItems;
			SlotNameList = NewSlotNameList;

			// Update Combo Boxes
			for (int32 TrackIndex = 0; TrackIndex < NumAnimTracks; TrackIndex++)
			{
				if (SlotNameComboBoxes[TrackIndex].IsValid())
				{
					FName SelectedSlotName = SlotNameComboSelectedNames[TrackIndex];
					if (Montage->GetSkeleton()->ContainsSlotName(SelectedSlotName))
					{
						int32 FoundIndex = SlotNameList.Find(SelectedSlotName);
						TSharedPtr<FString> ComboItem = SlotNameComboListItems[FoundIndex];

						SlotNameComboBoxes[TrackIndex]->SetSelectedItem(ComboItem);
						SlotNameComboBoxes[TrackIndex]->SetToolTipText(FText::FromString(*ComboItem));
					}
					SlotNameComboBoxes[TrackIndex]->RefreshOptions();
				}
			}
		}
	}
}

void SAnimMontagePanel::UpdateSlotGroupWarningVisibility()
{
	bool bShowStatusBarWarning = false;
	FName MontageGroupName = Montage->GetGroupName();

	int32 NumAnimTracks = Montage->SlotAnimTracks.Num();
	if (NumAnimTracks > 0)
	{
		TArray<FName> UniqueSlotNameList;
		for (int32 TrackIndex = 0; TrackIndex < NumAnimTracks; TrackIndex++)
		{
			FName CurrentSlotName = SlotNameComboSelectedNames[TrackIndex];
			FName CurrentSlotGroupName = Montage->GetSkeleton()->GetSlotGroupName(CurrentSlotName);

			int32 SlotNameCount = 0;
			for (int32 Index = 0; Index < NumAnimTracks; Index++)
			{
				if (CurrentSlotName == SlotNameComboSelectedNames[Index])
				{
					SlotNameCount++;
				}
			}
			
			// Verify that slot names are unique.
			bool bSlotNameAlreadyInUse = UniqueSlotNameList.Contains(CurrentSlotName);
			if (!bSlotNameAlreadyInUse)
			{
				UniqueSlotNameList.Add(CurrentSlotName);
			}

			bool bDifferentGroupName = (CurrentSlotGroupName != MontageGroupName);
			bool bShowWarning = bDifferentGroupName || bSlotNameAlreadyInUse;
			bShowStatusBarWarning = bShowStatusBarWarning || bShowWarning;

			SlotWarningImages[TrackIndex]->SetVisibility(bShowWarning ? EVisibility::Visible : EVisibility::Hidden);
			if (bDifferentGroupName)
			{
				FText WarningText = FText::Format(LOCTEXT("AnimMontagePanel_SlotGroupMismatchToolTipText", "Slot's group '{0}' is different than the Montage's group '{1}'. All slots must belong to the same group."), FText::FromName(CurrentSlotGroupName), FText::FromName(MontageGroupName));
				SlotWarningImages[TrackIndex]->SetToolTipText(WarningText);
				StatusBarTextBlock->SetText(WarningText);
				StatusBarTextBlock->SetToolTipText(WarningText);
			}
			if (bSlotNameAlreadyInUse)
			{
				FText WarningText = FText::Format(LOCTEXT("AnimMontagePanel_SlotNameAlreadyInUseToolTipText", "Slot named '{0}' is already used in this Montage. All slots must be unique"), FText::FromName(CurrentSlotName));
				SlotWarningImages[TrackIndex]->SetToolTipText(WarningText);
				StatusBarTextBlock->SetText(WarningText);
				StatusBarTextBlock->SetToolTipText(WarningText);
			}
		}
	}

	// Update Status bar
	StatusBarWarningImage->SetVisibility(bShowStatusBarWarning ? EVisibility::Visible : EVisibility::Hidden);
	if (!bShowStatusBarWarning)
	{
		StatusBarTextBlock->SetText(FText::Format(LOCTEXT("AnimMontagePanel_StatusBarText", "Montage Group: '{0}'"), FText::FromName(MontageGroupName)));
		StatusBarTextBlock->SetToolTipText(LOCTEXT("AnimMontagePanel_StatusBarToolTipText", "The Montage Group is set by the first slot's group."));
	}
}

void SAnimMontagePanel::OnSetElementsToLinkMode(EAnimLinkMethod::Type NewLinkMethod)
{
	TArray<FAnimLinkableElement*> Elements;
	CollectLinkableElements(Elements);

	for(FAnimLinkableElement* Element : Elements)
	{
		Element->ChangeLinkMethod(NewLinkMethod);
	}

	// Handle notify state links
	for(FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		if(Notify.GetDuration() > 0.0f)
		{
			// Always keep link methods in sync between notifies and duration links
			if(Notify.GetLinkMethod() != Notify.EndLink.GetLinkMethod())
			{
				Notify.EndLink.ChangeLinkMethod(Notify.GetLinkMethod());
			}
		}
	}
}

void SAnimMontagePanel::OnSetElementsToSlot(int32 SlotIndex)
{
	TArray<FAnimLinkableElement*> Elements;
	CollectLinkableElements(Elements);

	for(FAnimLinkableElement* Element : Elements)
	{
		Element->ChangeSlotIndex(SlotIndex);
	}

	// Handle notify state links
	for(FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		if(Notify.GetDuration() > 0.0f)
		{
			// Always keep link methods in sync between notifies and duration links
			if(Notify.GetSlotIndex() != Notify.EndLink.GetSlotIndex())
			{
				Notify.EndLink.ChangeSlotIndex(Notify.GetSlotIndex());
			}
		}
	}
}

void SAnimMontagePanel::CollectLinkableElements(TArray<FAnimLinkableElement*> &Elements)
{
	for(auto& Composite : Montage->CompositeSections)
	{
		FAnimLinkableElement* Element = &Composite;
		Elements.Add(Element);
	}

	for(auto& BranchingPoint : Montage->BranchingPoints)
	{
		FAnimLinkableElement* Element = &BranchingPoint;
		Elements.Add(Element);
	}

	for(auto& Notify : Montage->Notifies)
	{
		FAnimLinkableElement* Element = &Notify;
		Elements.Add(Element);
	}
}

#undef LOCTEXT_NAMESPACE