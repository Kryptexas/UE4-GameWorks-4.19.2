// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "IKeyArea.h"
#include "ISequencerSection.h"
#include "MovieSceneSection.h"
#include "MovieSceneTrack.h"
#include "Sequencer.h"
#include "ISequencerTrackEditor.h"

namespace SequencerNodeConstants
{
	extern const float CommonPadding;
}

/* FTrackNode structors
 *****************************************************************************/

FTrackNode::FTrackNode(FName NodeName, UMovieSceneTrack& InAssociatedType, ISequencerTrackEditor& InAssociatedEditor, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree)
	: FSequencerDisplayNode(NodeName, InParentNode, InParentTree)
	, AssociatedEditor(InAssociatedEditor)
	, AssociatedType(&InAssociatedType)
{ }


/* FTrackNode interface
 *****************************************************************************/

void FTrackNode::SetSectionAsKeyArea(TSharedRef<IKeyArea>& KeyArea)
{
	if( !TopLevelKeyNode.IsValid() )
	{
		bool bTopLevel = true;
		TopLevelKeyNode = MakeShareable( new FSequencerSectionKeyAreaNode( GetNodeName(), FText::GetEmpty(), nullptr, ParentTree, bTopLevel ) );
	}

	TopLevelKeyNode->AddKeyArea( KeyArea );
}


int32 FTrackNode::GetMaxRowIndex() const
{
	int32 MaxRowIndex = 0;

	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		MaxRowIndex = FMath::Max(MaxRowIndex, Sections[i]->GetSectionObject()->GetRowIndex());
	}

	return MaxRowIndex;
}


void FTrackNode::FixRowIndices()
{
	if (AssociatedType->SupportsMultipleRows())
	{
		// remove any empty track rows by waterfalling down sections to be as compact as possible
		TArray< TArray< TSharedRef<ISequencerSection> > > TrackIndices;
		TrackIndices.AddZeroed(GetMaxRowIndex() + 1);
		for (int32 i = 0; i < Sections.Num(); ++i)
		{
			TrackIndices[Sections[i]->GetSectionObject()->GetRowIndex()].Add(Sections[i]);
		}

		int32 NewIndex = 0;

		for (int32 i = 0; i < TrackIndices.Num(); ++i)
		{
			const TArray< TSharedRef<ISequencerSection> >& SectionsForThisIndex = TrackIndices[i];
			if (SectionsForThisIndex.Num() > 0)
			{
				for (int32 j = 0; j < SectionsForThisIndex.Num(); ++j)
				{
					SectionsForThisIndex[j]->GetSectionObject()->SetRowIndex(NewIndex);
				}

				++NewIndex;
			}
		}
	}
	else
	{
		// non master tracks can only have a single row
		for (int32 i = 0; i < Sections.Num(); ++i)
		{
			Sections[i]->GetSectionObject()->SetRowIndex(0);
		}
	}
}


/* FSequencerDisplayNode interface
 *****************************************************************************/

TSharedRef<SWidget> FTrackNode::GenerateEditWidgetForOutliner()
{
	TSharedPtr<FSequencerSectionKeyAreaNode> KeyAreaNode = GetTopLevelKeyNode();

	if (KeyAreaNode.IsValid())
	{
		// @todo - Sequencer - Support multiple sections/key areas?
		TArray<TSharedRef<IKeyArea>> KeyAreas = KeyAreaNode->GetAllKeyAreas();

		if (KeyAreas.Num() > 0)
		{
			if (KeyAreas[0]->CanCreateKeyEditor())
			{
				return KeyAreas[0]->CreateKeyEditor(&GetSequencer());
			}
		}
	}
	else
	{
		FGuid ObjectBinding;
		TSharedPtr<FSequencerDisplayNode> ParentNode = GetParent();

		if (ParentNode.IsValid() && (ParentNode->GetType() == ESequencerNode::Object))
		{
			ObjectBinding = StaticCastSharedPtr<FSequencerObjectBindingNode>(ParentNode)->GetObjectBinding();
		}

		TSharedPtr<SWidget> EditWidget = AssociatedEditor.BuildOutlinerEditWidget(ObjectBinding, AssociatedType.Get());
		if (EditWidget.IsValid())
		{
			return EditWidget.ToSharedRef();
		}
	}

	return FSequencerDisplayNode::GenerateEditWidgetForOutliner();
}


void FTrackNode::GetChildKeyAreaNodesRecursively(TArray<TSharedRef<FSequencerSectionKeyAreaNode>>& OutNodes) const
{
	FSequencerDisplayNode::GetChildKeyAreaNodesRecursively(OutNodes);

	if (TopLevelKeyNode.IsValid())
	{
		OutNodes.Add(TopLevelKeyNode.ToSharedRef());
	}
}


FText FTrackNode::GetDisplayName() const
{
	// @todo sequencer: is there a better way to get the section interface name for the animation outliner?
	return (TopLevelKeyNode.IsValid() && Sections.Num() > 0)
		? Sections[0]->GetDisplayName()
		: FText::FromName(NodeName);
}


float FTrackNode::GetNodeHeight() const
{
	return (Sections.Num() > 0)
		? Sections[0]->GetSectionHeight() * (GetMaxRowIndex() + 1)
		: SequencerLayoutConstants::SectionAreaDefaultHeight;
}


FNodePadding FTrackNode::GetNodePadding() const
{
	return FNodePadding(SequencerNodeConstants::CommonPadding);
}


ESequencerNode::Type FTrackNode::GetType() const
{
	return ESequencerNode::Track;
}

void FTrackNode::BuildContextMenu( FMenuBuilder& MenuBuilder )
{
	AssociatedEditor.BuildTrackContextMenu( MenuBuilder, AssociatedType.Get() );
	FSequencerDisplayNode::BuildContextMenu( MenuBuilder );
}
