// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"

#include "SAnimationOutlinerView.h"
#include "ScopedTransaction.h"
#include "Sequencer.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "Engine/Selection.h"
#include "ISequencerObjectBindingManager.h"
#include "IKeyArea.h"

#define LOCTEXT_NAMESPACE "AnimationOutliner"


void SAnimationOutlinerTreeNode::Construct( const FArguments& InArgs, TSharedRef<FSequencerDisplayNode> Node )
{
	DisplayNode = Node;
	OnSelectionChanged = InArgs._OnSelectionChanged;

	SelectedBrush = FEditorStyle::GetBrush( "Sequencer.AnimationOutliner.SelectionBorder" );
	SelectedBrushInactive = FEditorStyle::GetBrush("Sequencer.AnimationOutliner.SelectionBorderInactive");
	NotSelectedBrush = FEditorStyle::GetBrush( "NoBorder" );
	ExpandedBrush = FEditorStyle::GetBrush( "TreeArrow_Expanded" );
	CollapsedBrush = FEditorStyle::GetBrush( "TreeArrow_Collapsed" );

	// Choose the font.  If the node is a root node, we show a larger font for it.
	FSlateFontInfo NodeFont = Node->GetParent().IsValid() ? 
		FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.RegularFont") :
		FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.BoldFont");

	TSharedRef<SWidget> TextWidget = 
		SNew( STextBlock )
		.Text(this, &SAnimationOutlinerTreeNode::GetDisplayName )
		.Font( NodeFont );

	TSharedRef<SWidget>	FinalWidget = 
		SNew( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign( VAlign_Center )
		[
			SNew(SButton)
			.ContentPadding(0)
			.Visibility( this, &SAnimationOutlinerTreeNode::GetExpanderVisibility )
			.OnClicked( this, &SAnimationOutlinerTreeNode::OnExpanderClicked )
			.ClickMethod( EButtonClickMethod::MouseDown )
			.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			[
				SNew( SImage )
				.Image( this, &SAnimationOutlinerTreeNode::OnGetExpanderImage)
			]
		]
		// Label Slot
		+ SHorizontalBox::Slot()
		.VAlign( VAlign_Center )
		.FillWidth(3)
		[
			TextWidget
		]
		// Editor slot
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			// @todo Sequencer - Remove this box and width override.
			SNew(SBox)
			.WidthOverride(100)
			[
				DisplayNode->GenerateEditWidgetForOutliner()
			]
		]
		// Previous key slot
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(3, 0, 0, 0)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "FlatButton")
			.Visibility(this, &SAnimationOutlinerTreeNode::GetKeyButtonVisibility)
			.ToolTipText(LOCTEXT("PreviousKeyButton", "Set the time to the previous key"))
			.OnClicked(this, &SAnimationOutlinerTreeNode::OnPreviousKeyClicked)
			.ContentPadding(0)
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "NormalText.Important")
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.7"))
				.Text(FText::FromString(FString(TEXT("\xf060"))) /*fa-arrow-left*/)
			]
		]
		// Add key slot
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "FlatButton")
			.Visibility(this, &SAnimationOutlinerTreeNode::GetKeyButtonVisibility)
			.ToolTipText(LOCTEXT("AddKeyButton", "Add a new key at the current time"))
			.OnClicked(this, &SAnimationOutlinerTreeNode::OnAddKeyClicked)
			.ContentPadding(0)
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "NormalText.Important")
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.7"))
				.Text(FText::FromString(FString(TEXT("\xf055"))) /*fa-plus-circle*/)
			]
		]
		// Next key slot
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "FlatButton")
			.Visibility(this, &SAnimationOutlinerTreeNode::GetKeyButtonVisibility)
			.ToolTipText(LOCTEXT("NextKeyButton", "Set the time to the next key"))
			.OnClicked(this, &SAnimationOutlinerTreeNode::OnNextKeyClicked)
			.ContentPadding(0)
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "NormalText.Important")
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.7"))
				.Text(FText::FromString(FString(TEXT("\xf061"))) /*fa-arrow-right*/)
			]
		];

	ChildSlot
	[
		SNew( SBorder )
		.VAlign( VAlign_Center )
		.Padding(0)
		.BorderImage( this, &SAnimationOutlinerTreeNode::GetNodeBorderImage )
		[
			FinalWidget
		]
	];

	SetVisibility( TAttribute<EVisibility>( this, &SAnimationOutlinerTreeNode::GetNodeVisibility ) );
}

void GetAllKeyAreas(TSharedPtr<FSequencerDisplayNode> DisplayNode, TSet<TSharedPtr<IKeyArea>>& KeyAreas)
{
	TArray<TSharedPtr<FSequencerDisplayNode>> NodesToCheck;
	NodesToCheck.Add(DisplayNode);
	while (NodesToCheck.Num() > 0)
	{
		TSharedPtr<FSequencerDisplayNode> NodeToCheck = NodesToCheck[0];
		NodesToCheck.RemoveAt(0);

		if (NodeToCheck->GetType() == ESequencerNode::Track)
		{
			TSharedPtr<FTrackNode> TrackNode = StaticCastSharedPtr<FTrackNode>(NodeToCheck);
			TArray<TSharedRef<FSectionKeyAreaNode>> KeyAreaNodes;
			TrackNode->GetChildKeyAreaNodesRecursively(KeyAreaNodes);
			for (TSharedRef<FSectionKeyAreaNode> KeyAreaNode : KeyAreaNodes)
			{
				for (TSharedPtr<IKeyArea> KeyArea : KeyAreaNode->GetAllKeyAreas())
				{
					KeyAreas.Add(KeyArea);
				}
			}
		}
		else
		{
			if (NodeToCheck->GetType() == ESequencerNode::KeyArea)
			{
				TSharedPtr<FSectionKeyAreaNode> KeyAreaNode = StaticCastSharedPtr<FSectionKeyAreaNode>(NodeToCheck);
				for (TSharedPtr<IKeyArea> KeyArea : KeyAreaNode->GetAllKeyAreas())
				{
					KeyAreas.Add(KeyArea);
				}
			}
			for (TSharedRef<FSequencerDisplayNode> ChildNode : NodeToCheck->GetChildNodes())
			{
				NodesToCheck.Add(ChildNode);
			}
		}
	}
}

FReply SAnimationOutlinerTreeNode::OnPreviousKeyClicked()
{
	FSequencer& Sequencer = DisplayNode->GetSequencer();
	float ClosestPreviousKeyDistance = MAX_FLT;
	float CurrentTime = Sequencer.GetCurrentLocalTime(*Sequencer.GetFocusedMovieScene());
	float PreviousTime = 0;
	bool PreviousKeyFound = false;

	TSet<TSharedPtr<IKeyArea>> KeyAreas;
	GetAllKeyAreas(DisplayNode, KeyAreas);
	for (TSharedPtr<IKeyArea> KeyArea : KeyAreas)
	{
		for (FKeyHandle& KeyHandle : KeyArea->GetUnsortedKeyHandles())
		{
			float KeyTime = KeyArea->GetKeyTime(KeyHandle);
			if (KeyTime < CurrentTime && CurrentTime - KeyTime < ClosestPreviousKeyDistance)
			{
				PreviousTime = KeyTime;
				ClosestPreviousKeyDistance = CurrentTime - KeyTime;
				PreviousKeyFound = true;
			}
		}
	}

	if (PreviousKeyFound)
	{
		Sequencer.SetGlobalTime(PreviousTime);
	}
	return FReply::Handled();
}

FReply SAnimationOutlinerTreeNode::OnNextKeyClicked()
{
	FSequencer& Sequencer = DisplayNode->GetSequencer();
	float ClosestNextKeyDistance = MAX_FLT;
	float CurrentTime = Sequencer.GetCurrentLocalTime(*Sequencer.GetFocusedMovieScene());
	float NextTime = 0;
	bool NextKeyFound = false;

	TSet<TSharedPtr<IKeyArea>> KeyAreas;
	GetAllKeyAreas(DisplayNode, KeyAreas);
	for (TSharedPtr<IKeyArea> KeyArea : KeyAreas)
	{
		for (FKeyHandle& KeyHandle : KeyArea->GetUnsortedKeyHandles())
		{
			float KeyTime = KeyArea->GetKeyTime(KeyHandle);
			if (KeyTime > CurrentTime && KeyTime - CurrentTime < ClosestNextKeyDistance)
			{
				NextTime = KeyTime;
				ClosestNextKeyDistance = KeyTime - CurrentTime;
				NextKeyFound = true;
			}
		}
	}

	if (NextKeyFound)
	{
		Sequencer.SetGlobalTime(NextTime);
	}
	return FReply::Handled();
}

FReply SAnimationOutlinerTreeNode::OnAddKeyClicked()
{
	FSequencer& Sequencer = DisplayNode->GetSequencer();
	float CurrentTime = Sequencer.GetCurrentLocalTime(*Sequencer.GetFocusedMovieScene());

	TSet<TSharedPtr<IKeyArea>> KeyAreas;
	GetAllKeyAreas(DisplayNode, KeyAreas);

	FScopedTransaction Transaction(LOCTEXT("AddKeys", "Add keys at current time"));
	for (TSharedPtr<IKeyArea> KeyArea : KeyAreas)
	{
		UMovieSceneSection* OwningSection = KeyArea->GetOwningSection();
		OwningSection->SetFlags(RF_Transactional);
		OwningSection->Modify();
		KeyArea->AddKeyUnique(CurrentTime);
	}
	return FReply::Handled();
}

FReply SAnimationOutlinerTreeNode::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && DisplayNode->IsSelectable() )
	{
		FSequencer& Sequencer = DisplayNode->GetSequencer();
		bool bSelected = Sequencer.GetSelection()->IsSelected(DisplayNode.ToSharedRef());

		if( MouseEvent.IsControlDown() )
		{
			// Select the node if we were clicked on
			Sequencer.GetSelection()->AddToSelection(DisplayNode.ToSharedRef());
		}
		else
		{
			// Deselect the other nodes and select this node.
			Sequencer.GetSelection()->EmptySelectedOutlinerNodes();
			Sequencer.GetSelection()->AddToSelection(DisplayNode.ToSharedRef());
		}

		OnSelectionChanged.ExecuteIfBound( DisplayNode );
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SAnimationOutlinerTreeNode::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		TSharedPtr<SWidget> MenuContent = DisplayNode->OnSummonContextMenu(MyGeometry, MouseEvent);
		if (MenuContent.IsValid())
		{
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

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SAnimationOutlinerTreeNode::OnExpanderClicked() 
{
	DisplayNode->ToggleExpansion();
	return FReply::Handled();
}

const FSlateBrush* SAnimationOutlinerTreeNode::GetNodeBorderImage() const
{
	// Display a highlight when the node is selected
	FSequencer& Sequencer = DisplayNode->GetSequencer();
	const bool bIsSelected = Sequencer.GetSelection()->IsSelected(DisplayNode.ToSharedRef());
	if (bIsSelected)
	{
		if (Sequencer.GetSelection()->GetActiveSelection() == FSequencerSelection::EActiveSelection::OutlinerNode)
		{
			return SelectedBrush;
		}
		else
		{
			return SelectedBrushInactive;
		}
	}
	else
	{
		return  NotSelectedBrush;
	}
}

const FSlateBrush* SAnimationOutlinerTreeNode::OnGetExpanderImage() const
{
	const bool bIsExpanded = DisplayNode->IsExpanded();
	return bIsExpanded ? ExpandedBrush : CollapsedBrush;
}

EVisibility SAnimationOutlinerTreeNode::GetNodeVisibility() const
{
	return DisplayNode->IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SAnimationOutlinerTreeNode::GetExpanderVisibility() const
{
	return DisplayNode->GetNumChildren() > 0 ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility SAnimationOutlinerTreeNode::GetKeyButtonVisibility() const
{
	return DisplayNode->GetType() == ESequencerNode::Object ? EVisibility::Hidden : EVisibility::Visible;
}

FText SAnimationOutlinerTreeNode::GetDisplayName() const
{
	return DisplayNode->GetDisplayName();
}

void SAnimationOutlinerView::Construct( const FArguments& InArgs, TSharedRef<FSequencerDisplayNode> InRootNode, FSequencer* InSequencer )
{
	SAnimationOutlinerViewBase::Construct( SAnimationOutlinerViewBase::FArguments(), InRootNode );

	Sequencer = InSequencer;

	GenerateWidgetForNode( InRootNode );

	SetVisibility( TAttribute<EVisibility>( this, &SAnimationOutlinerView::GetNodeVisibility ) );
}

SAnimationOutlinerView::~SAnimationOutlinerView()
{
}

EVisibility SAnimationOutlinerView::GetNodeVisibility() const
{
	return RootNode->IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}


void SAnimationOutlinerView::GenerateWidgetForNode( TSharedRef<FSequencerDisplayNode>& InLayoutNode )
{
	Children.Add( 
		SNew( SAnimationOutlinerTreeNode, InLayoutNode)
		.OnSelectionChanged( this, &SAnimationOutlinerView::OnSelectionChanged )
		);

	// Object nodes do not generate widgets for their children
	if( InLayoutNode->GetType() != ESequencerNode::Object )
	{
		const TArray< TSharedRef<FSequencerDisplayNode> >& ChildNodes = InLayoutNode->GetChildNodes();
		
		for( int32 ChildIndex = 0; ChildIndex < ChildNodes.Num(); ++ChildIndex )
		{
			TSharedRef<FSequencerDisplayNode> ChildNode = ChildNodes[ChildIndex];
			GenerateWidgetForNode( ChildNode );
		}
	}
}


void SAnimationOutlinerView::OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	const float Padding = SequencerLayoutConstants::NodePadding;
	const float IndentAmount = SequencerLayoutConstants::IndentAmount;

	float CurrentHeight = 0;
	for (int32 WidgetIndex = 0; WidgetIndex < Children.Num(); ++WidgetIndex)
	{
		const TSharedRef<SAnimationOutlinerTreeNode>& Widget = Children[WidgetIndex];

		EVisibility Visibility = Widget->GetVisibility();
		if( ArrangedChildren.Accepts( Visibility ) )
		{
			const TSharedPtr<const FSequencerDisplayNode>& DisplayNode = Widget->GetDisplayNode();
			// How large to make this node
			float HeightIncrement = DisplayNode->GetNodeHeight();
			// How far to indent the widget
			float WidgetIndentOffset = IndentAmount*DisplayNode->GetTreeLevel();
			// Place the widget at the current height, at the nodes desired size
			ArrangedChildren.AddWidget( 
				Visibility, 
				AllottedGeometry.MakeChild( Widget, FVector2D( WidgetIndentOffset, CurrentHeight ), FVector2D( AllottedGeometry.GetDrawSize().X-WidgetIndentOffset, HeightIncrement ) ) 
				);
		
			// Compute the start height for the next widget
			CurrentHeight += HeightIncrement+Padding;
		}
	}
}


void SAnimationOutlinerView::OnSelectionChanged( TSharedPtr<FSequencerDisplayNode> AffectedNode )
{
	// Select objects bound to the object node
	if( AffectedNode->GetType() == ESequencerNode::Object )
	{
		const TSharedPtr<const FObjectBindingNode> ObjectNode = StaticCastSharedPtr<const FObjectBindingNode>( AffectedNode );

		// Get the bound objects
		TArray<UObject*> RuntimeObjects;
		Sequencer->GetRuntimeObjects( Sequencer->GetFocusedMovieSceneInstance(), ObjectNode->GetObjectBinding(), RuntimeObjects );
		
		if( RuntimeObjects.Num() > 0 && Sequencer->IsLevelEditorSequencer() )
		{
			const bool bNotifySelectionChanged = false;
			const bool bDeselectBSP = true;
			const bool bWarnAboutTooManyActors = false;

			// Clear selection
			GEditor->SelectNone(bNotifySelectionChanged,bDeselectBSP,bWarnAboutTooManyActors);
			GEditor->GetSelectedActors()->BeginBatchSelectOperation();

			// Select each actor
			bool bActorSelected = false;
			for( int32 ObjectIndex = 0; ObjectIndex < RuntimeObjects.Num(); ++ObjectIndex )
			{
				AActor* Actor = Cast<AActor>( RuntimeObjects[ObjectIndex] );

				if( Actor )
				{
					const bool bSelectActor = true;

					GEditor->SelectActor(Actor, bSelectActor, bNotifySelectionChanged );
					bActorSelected = true;
				}
			}

			GEditor->GetSelectedActors()->EndBatchSelectOperation();

			if( bActorSelected )
			{
				GEditor->NoteSelectionChange();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE