// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DisplayNodes/SequencerTrackNode.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SBox.h"
#include "DisplayNodes/SequencerObjectBindingNode.h"
#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "UObject/UnrealType.h"
#include "MovieSceneTrack.h"
#include "SSequencer.h"
#include "MovieSceneNameableTrack.h"
#include "ISequencerTrackEditor.h"
#include "ScopedTransaction.h"
#include "SKeyNavigationButtons.h"

namespace SequencerNodeConstants
{
	extern const float CommonPadding;
}


/* FTrackNode structors
 *****************************************************************************/

FSequencerTrackNode::FSequencerTrackNode(UMovieSceneTrack& InAssociatedTrack, ISequencerTrackEditor& InAssociatedEditor, bool bInCanBeDragged, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree)
	: FSequencerDisplayNode(InAssociatedTrack.GetTrackName(), InParentNode, InParentTree)
	, AssociatedEditor(InAssociatedEditor)
	, AssociatedTrack(&InAssociatedTrack)
	, bCanBeDragged(bInCanBeDragged)
	, SubTrackMode(ESubTrackMode::None)
{ }


/* FTrackNode interface
 *****************************************************************************/

void FSequencerTrackNode::SetSectionAsKeyArea(TSharedRef<IKeyArea>& KeyArea)
{
	if( !TopLevelKeyNode.IsValid() )
	{
		bool bTopLevel = true;
		TopLevelKeyNode = MakeShareable( new FSequencerSectionKeyAreaNode( GetNodeName(), FText::GetEmpty(), nullptr, ParentTree, bTopLevel ) );
	}

	TopLevelKeyNode->AddKeyArea( KeyArea );
}

void FSequencerTrackNode::AddKey(const FGuid& ObjectGuid)
{
	AssociatedEditor.AddKey(ObjectGuid);
}

FSequencerTrackNode::ESubTrackMode FSequencerTrackNode::GetSubTrackMode() const
{
	return SubTrackMode;
}

void FSequencerTrackNode::SetSubTrackMode(FSequencerTrackNode::ESubTrackMode InSubTrackMode)
{
	SubTrackMode = InSubTrackMode;
}

int32 FSequencerTrackNode::GetRowIndex() const
{
	return RowIndex;
}

void FSequencerTrackNode::SetRowIndex(int32 InRowIndex)
{
	RowIndex = InRowIndex;
}


/* FSequencerDisplayNode interface
 *****************************************************************************/

void FSequencerTrackNode::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	AssociatedEditor.BuildTrackContextMenu(MenuBuilder, AssociatedTrack.Get());
	FSequencerDisplayNode::BuildContextMenu(MenuBuilder );
}


bool FSequencerTrackNode::CanRenameNode() const
{
	auto NameableTrack = Cast<UMovieSceneNameableTrack>(AssociatedTrack.Get());

	if (NameableTrack != nullptr)
	{
		return NameableTrack->CanRename();
	}
	return false;
}

TSharedRef<SWidget> FSequencerTrackNode::GetCustomOutlinerContent()
{
	TSharedPtr<FSequencerSectionKeyAreaNode> KeyAreaNode = GetTopLevelKeyNode();
	if (KeyAreaNode.IsValid())
	{
		// @todo - Sequencer - Support multiple sections/key areas?
		TArray<TSharedRef<IKeyArea>> KeyAreas = KeyAreaNode->GetAllKeyAreas();

		if (KeyAreas.Num() > 0)
		{
			// Create the widgets for the key editor and key navigation buttons
			TSharedRef<SHorizontalBox> BoxPanel = SNew(SHorizontalBox);

			if (KeyAreas[0]->CanCreateKeyEditor())
			{
				BoxPanel->AddSlot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(100)
					.HAlign(HAlign_Left)
					.IsEnabled(!GetSequencer().IsReadOnly())
					[
						KeyAreas[0]->CreateKeyEditor(&GetSequencer())
					]
				];
			}
			else
			{
				BoxPanel->AddSlot()
				.AutoWidth()
				[
					SNew(SSpacer)
				];
			}

			BoxPanel->AddSlot()
			.VAlign(VAlign_Center)
			[
				SNew(SKeyNavigationButtons, AsShared())
			];

			return SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				[
					BoxPanel
				];
		}
	}
	else
	{
		FGuid ObjectBinding;
		TSharedPtr<FSequencerDisplayNode> ParentSeqNode = GetParent();

		if (ParentSeqNode.IsValid() && (ParentSeqNode->GetType() == ESequencerNode::Object))
		{
			ObjectBinding = StaticCastSharedPtr<FSequencerObjectBindingNode>(ParentSeqNode)->GetObjectBinding();
		}
		FBuildEditWidgetParams Params;
		Params.NodeIsHovered = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FSequencerDisplayNode::IsHovered));

		TSharedPtr<SWidget> Widget = GetSequencer().IsReadOnly() ? SNullWidget::NullWidget : AssociatedEditor.BuildOutlinerEditWidget(ObjectBinding, AssociatedTrack.Get(), Params);

		TSharedRef<SHorizontalBox> BoxPanel = SNew(SHorizontalBox);

		bool bHasKeyableAreas = false;

		TArray<TSharedRef<FSequencerSectionKeyAreaNode>> ChildKeyAreaNodes;
		FSequencerDisplayNode::GetChildKeyAreaNodesRecursively(ChildKeyAreaNodes);
		for (int32 ChildIndex = 0; ChildIndex < ChildKeyAreaNodes.Num() && !bHasKeyableAreas; ++ChildIndex)
		{
			TArray< TSharedRef<IKeyArea> > ChildKeyAreas = ChildKeyAreaNodes[ChildIndex]->GetAllKeyAreas();

			for (int32 ChildKeyAreaIndex = 0; ChildKeyAreaIndex < ChildKeyAreas.Num() && !bHasKeyableAreas; ++ChildKeyAreaIndex)
			{
				if (ChildKeyAreas[ChildKeyAreaIndex]->CanCreateKeyEditor())
				{
					bHasKeyableAreas = true;
				}
			}
		}

		if (Widget.IsValid())
		{
			BoxPanel->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				Widget.ToSharedRef()
			];
		}

		if (bHasKeyableAreas)
		{
			BoxPanel->AddSlot()
			.VAlign(VAlign_Center)
			[
				SNew(SKeyNavigationButtons, AsShared())
			];
		}

		return SNew(SBox)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				BoxPanel
			];
	}


	return SNew(SSpacer);
}


const FSlateBrush* FSequencerTrackNode::GetIconBrush() const
{
	return AssociatedEditor.GetIconBrush();
}


bool FSequencerTrackNode::CanDrag() const
{
	return bCanBeDragged;
}

bool FSequencerTrackNode::IsResizable() const
{
	UMovieSceneTrack* Track = GetTrack();
	return Track && AssociatedEditor.IsResizable(Track);
}

void FSequencerTrackNode::Resize(float NewSize)
{
	UMovieSceneTrack* Track = GetTrack();

	float PaddingAmount = 2 * SequencerNodeConstants::CommonPadding;
	if (Track && Sections.Num())
	{
		PaddingAmount *= (Track->GetMaxRowIndex() + 1);
	}
	
	NewSize -= PaddingAmount;

	if (Track && AssociatedEditor.IsResizable(Track))
	{
		AssociatedEditor.Resize(NewSize, Track);
	}
}

void FSequencerTrackNode::GetChildKeyAreaNodesRecursively(TArray<TSharedRef<FSequencerSectionKeyAreaNode>>& OutNodes) const
{
	FSequencerDisplayNode::GetChildKeyAreaNodesRecursively(OutNodes);

	if (TopLevelKeyNode.IsValid())
	{
		OutNodes.Add(TopLevelKeyNode.ToSharedRef());
	}
}


FText FSequencerTrackNode::GetDisplayName() const
{
	return AssociatedTrack.IsValid() ? AssociatedTrack->GetDisplayName() : FText::GetEmpty();
}


float FSequencerTrackNode::GetNodeHeight() const
{
	float SectionHeight = Sections.Num() > 0
		? Sections[0]->GetSectionHeight()
		: SequencerLayoutConstants::SectionAreaDefaultHeight;
	float PaddedSectionHeight = SectionHeight + (2 * SequencerNodeConstants::CommonPadding);

	if (SubTrackMode == ESubTrackMode::None && AssociatedTrack.IsValid())
	{
		return PaddedSectionHeight * (AssociatedTrack->GetMaxRowIndex() + 1);
	}
	else
	{
		return PaddedSectionHeight;
	}
}


FNodePadding FSequencerTrackNode::GetNodePadding() const
{
	return FNodePadding(0.f);
}


ESequencerNode::Type FSequencerTrackNode::GetType() const
{
	return ESequencerNode::Track;
}


void FSequencerTrackNode::SetDisplayName(const FText& NewDisplayName)
{
	auto NameableTrack = Cast<UMovieSceneNameableTrack>(AssociatedTrack.Get());

	if (NameableTrack != nullptr)
	{
		NameableTrack->SetDisplayName(NewDisplayName);
		GetSequencer().NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
	}
}
