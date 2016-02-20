// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SAnimationOutlinerTreeNode.h"
#include "ScopedTransaction.h"
#include "Sequencer.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "MovieSceneCommonHelpers.h"
#include "Engine/Selection.h"
#include "IKeyArea.h"
#include "SEditableLabel.h"
#include "SSequencerTreeView.h"
#include "SColorPicker.h"


#define LOCTEXT_NAMESPACE "AnimationOutliner"

class STrackColorPicker : public SCompoundWidget
{
public:
	static void OnOpen()
	{
		if (!Transaction.IsValid())
		{
			Transaction.Reset(new FScopedTransaction(LOCTEXT("ChangeTrackColor", "Change Track Color")));
		}
	}

	static void OnClose()
	{
		if (!Transaction.IsValid())
		{
			return;
		}

		if (!bMadeChanges)
		{
			Transaction->Cancel();
		}
		Transaction.Reset();
	}

	SLATE_BEGIN_ARGS(STrackColorPicker){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UMovieSceneTrack* InTrack)
	{
		bMadeChanges = false;
		Track = InTrack;
		ChildSlot
		[
			SNew(SColorPicker)
			.DisplayInlineVersion(true)
			.TargetColorAttribute(this, &STrackColorPicker::GetTrackColor)
			.OnColorCommitted(this, &STrackColorPicker::SetTrackColor)
		];
	}

	FLinearColor GetTrackColor() const
	{
		return Track->GetColorTint().ReinterpretAsLinear();
	}

	void SetTrackColor(FLinearColor NewColor)
	{
		bMadeChanges = true;
		Track->Modify();
		Track->SetColorTint(NewColor.ToFColor(false));
	}

private:
	UMovieSceneTrack* Track;
	static TUniquePtr<FScopedTransaction> Transaction;
	static bool bMadeChanges;
};

TUniquePtr<FScopedTransaction> STrackColorPicker::Transaction;
bool STrackColorPicker::bMadeChanges = false;

SAnimationOutlinerTreeNode::~SAnimationOutlinerTreeNode()
{
	DisplayNode->OnRenameRequested().RemoveAll(this);
}

void SAnimationOutlinerTreeNode::Construct( const FArguments& InArgs, TSharedRef<FSequencerDisplayNode> Node, const TSharedRef<SSequencerTreeViewRow>& InTableRow )
{
	DisplayNode = Node;
	bIsTopLevelNode = !Node->GetParent().IsValid();

	if (bIsTopLevelNode)
	{
		ExpandedBackgroundBrush = FEditorStyle::GetBrush( "Sequencer.AnimationOutliner.TopLevelBorder_Expanded" );
		CollapsedBackgroundBrush = FEditorStyle::GetBrush( "Sequencer.AnimationOutliner.TopLevelBorder_Collapsed" );
	}
	else
	{
		ExpandedBackgroundBrush = FEditorStyle::GetBrush( "Sequencer.AnimationOutliner.DefaultBorder" );
		CollapsedBackgroundBrush = FEditorStyle::GetBrush( "Sequencer.AnimationOutliner.DefaultBorder" );
	}

	TableRowStyle = &FEditorStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");

	FSlateFontInfo NodeFont = FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.RegularFont");

	EditableLabel = SNew(SEditableLabel)
		.CanEdit(this, &SAnimationOutlinerTreeNode::HandleNodeLabelCanEdit)
		.Font(NodeFont)
		.OnTextChanged(this, &SAnimationOutlinerTreeNode::HandleNodeLabelTextChanged)
		.Text(this, &SAnimationOutlinerTreeNode::GetDisplayName);


	Node->OnRenameRequested().AddRaw(this, &SAnimationOutlinerTreeNode::EnterRenameMode);

	auto NodeHeight = [=]() -> FOptionalSize { return DisplayNode->GetNodeHeight(); };

	ForegroundColor.Bind(this, &SAnimationOutlinerTreeNode::GetForegroundBasedOnSelection);

	TSharedRef<SWidget>	FinalWidget = 
		SNew( SBorder )
		.VAlign( VAlign_Center )
		.BorderImage( this, &SAnimationOutlinerTreeNode::GetNodeBorderImage )
		.BorderBackgroundColor( this, &SAnimationOutlinerTreeNode::GetNodeBackgroundTint )
		.Padding(FMargin(0, Node->GetNodePadding().Combined() / 2))
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
			[
				SNew(SBox)
				.HeightOverride_Lambda(NodeHeight)
				.Padding(FMargin(5.0f, 0.0f))
				[
					SNew( SHorizontalBox )

					// Expand track lanes button
					+ SHorizontalBox::Slot()
					.Padding(FMargin(2.f, 0.f, 4.f, 0.f))
					.VAlign( VAlign_Center )
					.AutoWidth()
					[
						SNew(SExpanderArrow, InTableRow).IndentAmount(SequencerLayoutConstants::IndentAmount)
					]

					// Icon
					+ SHorizontalBox::Slot()
					.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SOverlay)

						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(InArgs._IconBrush)
						]

						+ SOverlay::Slot()
						.VAlign(VAlign_Top)
						.HAlign(HAlign_Right)
						[
							SNew(SImage)
							.Image(InArgs._IconOverlayBrush)
						]

						+ SOverlay::Slot()
						[
							SNew(SSpacer)
							.Visibility(EVisibility::Visible)
							.ToolTipText(InArgs._IconToolTipText)
						]
					]

					// Label Slot
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
					.AutoWidth()
					[
						EditableLabel.ToSharedRef()
					]

					// Arbitrary customization slot
					+ SHorizontalBox::Slot()
					[
						InArgs._CustomContent.Widget
					]
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.VAlign(VAlign_Fill)
				.HasDownArrow(false)
				.ButtonStyle(FEditorStyle::Get(), "Sequencer.AnimationOutliner.ColorStrip")
				.OnGetMenuContent(this, &SAnimationOutlinerTreeNode::OnGetColorPicker)
				.OnMenuOpenChanged_Lambda([](bool bIsOpen){
					if (bIsOpen)
					{
						STrackColorPicker::OnOpen();
					}
					else if (!bIsOpen)
					{
						STrackColorPicker::OnClose();
					}
				})
				.ButtonContent()
				[
					SNew(SBox)
					.WidthOverride(6.f)
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("WhiteBrush"))
						.ColorAndOpacity(this, &SAnimationOutlinerTreeNode::GetTrackColorTint)
					]
				]
			]
		];

	ChildSlot
	[
		FinalWidget
	];
}


void SAnimationOutlinerTreeNode::EnterRenameMode()
{
	EditableLabel->EnterTextMode();
}

TSharedPtr<FSequencerDisplayNode> SAnimationOutlinerTreeNode::GetRootNode(TSharedPtr<FSequencerDisplayNode> ObjectNode)
{
	if (!ObjectNode->GetParent().IsValid())
	{
		return ObjectNode;
	}

	return GetRootNode(ObjectNode->GetParent());
}


void SAnimationOutlinerTreeNode::GetAllDescendantNodes(TSharedPtr<FSequencerDisplayNode> RootNode, TArray<TSharedRef<FSequencerDisplayNode> >& AllNodes)
{
	if (!RootNode.IsValid())
	{
		return;
	}

	AllNodes.Add(RootNode.ToSharedRef());

	const FSequencerDisplayNode* RootNodeC = RootNode.Get();

	for (TSharedRef<FSequencerDisplayNode> ChildNode : RootNodeC->GetChildNodes())
	{
		AllNodes.Add(ChildNode);
		GetAllDescendantNodes(ChildNode, AllNodes);
	}
}


FReply SAnimationOutlinerTreeNode::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (!DisplayNode->IsSelectable())
	{
		return FReply::Unhandled();
	}

	if ((MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) &&
		(MouseEvent.GetEffectingButton() != EKeys::RightMouseButton))
	{
		return FReply::Unhandled();
	}

	FSequencer& Sequencer = DisplayNode->GetSequencer();
	bool bSelected = Sequencer.GetSelection().IsSelected(DisplayNode.ToSharedRef());

	TArray<TSharedPtr<FSequencerDisplayNode> > AffectedNodes;
	AffectedNodes.Add(DisplayNode.ToSharedRef());

	Sequencer.GetSelection().SuspendBroadcast();

	if (MouseEvent.IsShiftDown())
	{
		FSequencerNodeTree& ParentTree = DisplayNode->GetParentTree();

		const TArray< TSharedRef<FSequencerDisplayNode> >RootNodes = ParentTree.GetRootNodes();

		// Get all nodes in order
		TArray<TSharedRef<FSequencerDisplayNode> > AllNodes;
		for (int32 i = 0; i < RootNodes.Num(); ++i)
		{
			GetAllDescendantNodes(RootNodes[i], AllNodes);
		}

		int32 FirstIndexToSelect = INT32_MAX;
		int32 LastIndexToSelect = INT32_MIN;

		for (int32 ChildIndex = 0; ChildIndex < AllNodes.Num(); ++ChildIndex)
		{
			TSharedRef<FSequencerDisplayNode> ChildNode = AllNodes[ChildIndex];

			if (ChildNode == DisplayNode.ToSharedRef() || Sequencer.GetSelection().IsSelected(ChildNode))
			{
				if (ChildIndex < FirstIndexToSelect)
				{
					FirstIndexToSelect = ChildIndex;
				}

				if (ChildIndex > LastIndexToSelect)
				{
					LastIndexToSelect = ChildIndex;
				}
			}
		}

		if (FirstIndexToSelect != INT32_MAX && LastIndexToSelect != INT32_MIN)
		{
			for (int32 ChildIndex = FirstIndexToSelect; ChildIndex <= LastIndexToSelect; ++ChildIndex)
			{
				TSharedRef<FSequencerDisplayNode> ChildNode = AllNodes[ChildIndex];

				if (!Sequencer.GetSelection().IsSelected(ChildNode))
				{
					Sequencer.GetSelection().AddToSelection(ChildNode);
					AffectedNodes.Add(ChildNode);
				}
			}
		}
	}
	else if( MouseEvent.IsControlDown() )
	{
		// Toggle selection when control is down
		if (bSelected)
		{
			Sequencer.GetSelection().RemoveFromSelection(DisplayNode.ToSharedRef());
		}
		else
		{
			Sequencer.GetSelection().AddToSelection(DisplayNode.ToSharedRef());
		}
	}
	else if (MouseEvent.GetEffectingButton() != EKeys::RightMouseButton || !bSelected)
	{
		// Deselect the other nodes and select this node.
		Sequencer.GetSelection().EmptySelectedOutlinerNodes();
		Sequencer.GetSelection().AddToSelection(DisplayNode.ToSharedRef());
	}

	OnSelectionChanged( AffectedNodes );

	Sequencer.GetSelection().ResumeBroadcast();
	Sequencer.GetSelection().GetOnOutlinerNodeSelectionChanged().Broadcast();

	return FReply::Handled();
}


FReply SAnimationOutlinerTreeNode::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if( MouseEvent.GetEffectingButton() != EKeys::RightMouseButton )
	{
		return FReply::Unhandled();
	}

	TSharedPtr<SWidget> MenuContent = DisplayNode->OnSummonContextMenu(MyGeometry, MouseEvent);

	if (!MenuContent.IsValid())
	{
		return FReply::Handled();
	}

	FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();

	FSlateApplication::Get().PushMenu(
		AsShared(),
		WidgetPath,
		MenuContent.ToSharedRef(),
		MouseEvent.GetScreenSpacePosition(),
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
	);
			
	return FReply::Handled().SetUserFocus(MenuContent.ToSharedRef(), EFocusCause::SetDirectly);
}

void SAnimationOutlinerTreeNode::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DisplayNode->GetParentTree().SetHoveredNode(DisplayNode);
	SWidget::OnMouseEnter(MyGeometry, MouseEvent);
}

void SAnimationOutlinerTreeNode::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	DisplayNode->GetParentTree().SetHoveredNode(nullptr);
	SWidget::OnMouseLeave(MouseEvent);
}

const FSlateBrush* SAnimationOutlinerTreeNode::GetNodeBorderImage() const
{
	return DisplayNode->IsExpanded() ? ExpandedBackgroundBrush : CollapsedBackgroundBrush;
}

FSlateColor SAnimationOutlinerTreeNode::GetNodeBackgroundTint() const
{
	FSequencer& Sequencer = DisplayNode->GetSequencer();
	const bool bIsSelected = Sequencer.GetSelection().IsSelected(DisplayNode.ToSharedRef());
	const bool bIsSelectionActive = Sequencer.GetSelection().GetActiveSelection() == FSequencerSelection::EActiveSelection::OutlinerNode;

	if (bIsSelected)
	{
		if (bIsSelectionActive)
		{
			return FEditorStyle::GetSlateColor("SelectionColor_Pressed");
		}
		else
		{
			return FEditorStyle::GetSlateColor("SelectionColor_Inactive");
		}
	}
	else if (DisplayNode->IsHovered())
	{
		return bIsTopLevelNode ? FLinearColor(FColor(52, 52, 52, 255)) : FLinearColor(FColor(72, 72, 72, 255));
	}
	else
	{
		return bIsTopLevelNode ? FLinearColor(FColor(48, 48, 48, 255)) : FLinearColor(FColor(62, 62, 62, 255));
	}
}

TSharedRef<SWidget> SAnimationOutlinerTreeNode::OnGetColorPicker() const
{
	UMovieSceneTrack* Track = nullptr;

	FSequencerDisplayNode* Current = DisplayNode.Get();
	while (Current && Current->GetType() != ESequencerNode::Object)
	{
		if (Current->GetType() == ESequencerNode::Track)
		{
			Track = static_cast<FSequencerTrackNode*>(Current)->GetTrack();
			if (Track)
			{
				break;
			}
		}
		Current = Current->GetParent().Get();
	}

	if (!Track)
	{
		return SNullWidget::NullWidget;
	}

	return SNew(STrackColorPicker, Track);
}

FSlateColor SAnimationOutlinerTreeNode::GetTrackColorTint() const
{
	FSequencerDisplayNode* Current = DisplayNode.Get();
	while (Current && Current->GetType() != ESequencerNode::Object)
	{
		if (Current->GetType() == ESequencerNode::Track)
		{
			UMovieSceneTrack* Track = static_cast<FSequencerTrackNode*>(Current)->GetTrack();
			if (Track)
			{
				return FLinearColor(Track->GetColorTint()).CopyWithNewOpacity(1.f);
			}
		}
		Current = Current->GetParent().Get();
	}

	return FLinearColor::Transparent;
}

FSlateColor SAnimationOutlinerTreeNode::GetForegroundBasedOnSelection() const
{
	FSequencer& Sequencer = DisplayNode->GetSequencer();
	const bool bIsSelected = Sequencer.GetSelection().IsSelected(DisplayNode.ToSharedRef());

	return bIsSelected ? TableRowStyle->SelectedTextColor : TableRowStyle->TextColor;
}


EVisibility SAnimationOutlinerTreeNode::GetExpanderVisibility() const
{
	return DisplayNode->GetNumChildren() > 0 ? EVisibility::Visible : EVisibility::Hidden;
}


FText SAnimationOutlinerTreeNode::GetDisplayName() const
{
	return DisplayNode->GetDisplayName();
}


bool SAnimationOutlinerTreeNode::HandleNodeLabelCanEdit() const
{
	return DisplayNode->CanRenameNode();
}


void SAnimationOutlinerTreeNode::HandleNodeLabelTextChanged(const FText& NewLabel)
{
	DisplayNode->SetDisplayName(NewLabel);
}


void SAnimationOutlinerTreeNode::OnSelectionChanged( TArray<TSharedPtr<FSequencerDisplayNode> > AffectedNodes )
{
	FSequencer& Sequencer = DisplayNode->GetSequencer();
	if (!Sequencer.IsLevelEditorSequencer())
	{
		return;
	}

	TArray<TSharedPtr<FSequencerDisplayNode> > AffectedRootNodes;

	for (int32 NodeIdx = 0; NodeIdx < AffectedNodes.Num(); ++NodeIdx)
	{
		TSharedPtr<FSequencerDisplayNode> RootNode = GetRootNode(AffectedNodes[NodeIdx]);
		if (RootNode.IsValid() && RootNode->GetType() == ESequencerNode::Object)
		{
			AffectedRootNodes.Add(RootNode);
		}
	}

	if (!AffectedRootNodes.Num())
	{
		return;
	}

	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	bool IsControlDown = ModifierKeys.IsControlDown();
	bool IsShiftDown = ModifierKeys.IsShiftDown();

	TArray<AActor*> ActorsToSelect;
	TArray<AActor*> ActorsToRemainSelected;
	TArray<AActor*> ActorsToDeselect;

	// Find objects bound to the object node and determine what actors need to be selected in the editor
	for (int32 ObjectIdx = 0; ObjectIdx < AffectedRootNodes.Num(); ++ObjectIdx)
	{
		const TSharedPtr<const FSequencerObjectBindingNode> ObjectNode = StaticCastSharedPtr<const FSequencerObjectBindingNode>( AffectedRootNodes[ObjectIdx] );

		// Get the bound objects
		TArray<UObject*> RuntimeObjects;
		Sequencer.GetRuntimeObjects( Sequencer.GetFocusedMovieSceneSequenceInstance(), ObjectNode->GetObjectBinding(), RuntimeObjects );
		
		if( RuntimeObjects.Num() > 0 )
		{
			for( int32 ActorIdx = 0; ActorIdx < RuntimeObjects.Num(); ++ActorIdx )
			{
				AActor* Actor = Cast<AActor>( RuntimeObjects[ActorIdx] );

				if (Actor == nullptr)
				{
					continue;
				}

				bool bIsActorSelected = GEditor->GetSelectedActors()->IsSelected(Actor);
				if (IsControlDown)
				{
					if (!bIsActorSelected)
					{
						ActorsToSelect.Add(Actor);
					}
					else
					{
						ActorsToDeselect.Add(Actor);
					}
				}
				else
				{
					if (!bIsActorSelected)
					{
						ActorsToSelect.Add(Actor);
					}
					else
					{
						ActorsToRemainSelected.Add(Actor);
					}
				}
			}
		}
	}

	if (!IsControlDown && !IsShiftDown)
	{
		for (FSelectionIterator SelectionIt(*GEditor->GetSelectedActors()); SelectionIt; ++SelectionIt)
		{
			AActor* Actor = CastChecked< AActor >( *SelectionIt );
			if (Actor)
			{
				if (ActorsToSelect.Find(Actor) == INDEX_NONE && 
					ActorsToDeselect.Find(Actor) == INDEX_NONE &&
					ActorsToRemainSelected.Find(Actor) == INDEX_NONE)
				{
					ActorsToDeselect.Add(Actor);
				}
			}
		}
	}

	// If there's no selection to change, return early
	if (ActorsToSelect.Num() + ActorsToDeselect.Num() == 0)
	{
		return;
	}

	// Mark that the user is selecting so that the UI doesn't respond to the selection changes in the following block
	TSharedRef<SSequencer> SequencerWidget = StaticCastSharedRef<SSequencer>(Sequencer.GetSequencerWidget());
	SequencerWidget->SetUserIsSelecting(true);

	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "ClickingOnActors", "Clicking on Actors"));

	const bool bNotifySelectionChanged = false;
	const bool bDeselectBSP = true;
	const bool bWarnAboutTooManyActors = false;
	const bool bSelectEvenIfHidden = true;

	GEditor->GetSelectedActors()->Modify();

	if (!IsControlDown && !IsShiftDown)
	{
		GEditor->SelectNone(bNotifySelectionChanged, bDeselectBSP, bWarnAboutTooManyActors);
	}

	GEditor->GetSelectedActors()->BeginBatchSelectOperation();

	for (auto ActorToSelect : ActorsToSelect)
	{
		GEditor->SelectActor(ActorToSelect, true, bNotifySelectionChanged, bSelectEvenIfHidden);
	}

	for (auto ActorToDeselect : ActorsToDeselect)
	{
		GEditor->SelectActor(ActorToDeselect, false, bNotifySelectionChanged, bSelectEvenIfHidden);
	}

	GEditor->GetSelectedActors()->EndBatchSelectOperation();
	GEditor->NoteSelectionChange();

	// Unlock the selection so that the sequencer widget can now respond to selection changes in the level
	SequencerWidget->SetUserIsSelecting(false);
}


#undef LOCTEXT_NAMESPACE
