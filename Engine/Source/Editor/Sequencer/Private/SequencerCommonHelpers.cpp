// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerCommonHelpers.h"
#include "MovieSceneSection.h"
#include "SequencerHotspots.h"
#include "VirtualTrackArea.h"
#include "SequencerContextMenus.h"
#include "MovieSceneTrack.h"

void SequencerHelpers::GetAllKeyAreas(TSharedPtr<FSequencerDisplayNode> DisplayNode, TSet<TSharedPtr<IKeyArea>>& KeyAreas)
{
	TArray<TSharedPtr<FSequencerDisplayNode>> NodesToCheck;
	NodesToCheck.Add(DisplayNode);
	while (NodesToCheck.Num() > 0)
	{
		TSharedPtr<FSequencerDisplayNode> NodeToCheck = NodesToCheck[0];
		NodesToCheck.RemoveAt(0);

		if (NodeToCheck->GetType() == ESequencerNode::Track)
		{
			TSharedPtr<FSequencerTrackNode> TrackNode = StaticCastSharedPtr<FSequencerTrackNode>(NodeToCheck);
			TArray<TSharedRef<FSequencerSectionKeyAreaNode>> KeyAreaNodes;
			TrackNode->GetChildKeyAreaNodesRecursively(KeyAreaNodes);
			for (TSharedRef<FSequencerSectionKeyAreaNode> KeyAreaNode : KeyAreaNodes)
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
				TSharedPtr<FSequencerSectionKeyAreaNode> KeyAreaNode = StaticCastSharedPtr<FSequencerSectionKeyAreaNode>(NodeToCheck);
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

void SequencerHelpers::GetDescendantNodes(TSharedRef<FSequencerDisplayNode> DisplayNode, TSet<TSharedRef<FSequencerDisplayNode>>& Nodes)
{
	for (auto ChildNode : DisplayNode.Get().GetChildNodes())
	{
		Nodes.Add(ChildNode);

		GetDescendantNodes(ChildNode, Nodes);
	}
}

void SequencerHelpers::GetAllSections(TSharedRef<FSequencerDisplayNode> DisplayNode, TSet<TWeakObjectPtr<UMovieSceneSection>>& Sections)
{
	TSet<TSharedRef<FSequencerDisplayNode> > AllNodes;
	AllNodes.Add(DisplayNode);
	GetDescendantNodes(DisplayNode, AllNodes);

	for (auto NodeToCheck : AllNodes)
	{
		TSet<TSharedPtr<IKeyArea> > KeyAreas;
		GetAllKeyAreas(NodeToCheck, KeyAreas);
		
		for (auto KeyArea : KeyAreas)
		{
			UMovieSceneSection* OwningSection = KeyArea->GetOwningSection();
			if (OwningSection != nullptr)
			{
				Sections.Add(OwningSection);	
			}
		}

		if (NodeToCheck->GetType() == ESequencerNode::Track)
		{
			TSharedRef<const FSequencerTrackNode> TrackNode = StaticCastSharedRef<const FSequencerTrackNode>( NodeToCheck );
			UMovieSceneTrack* Track = TrackNode->GetTrack();
			if (Track != nullptr)
			{
				for (auto Section : Track->GetAllSections())
				{
					Sections.Add(Section);
				}
			}
		}
	}
}

bool SequencerHelpers::FindObjectBindingNode(TSharedRef<FSequencerDisplayNode> DisplayNode, TSharedRef<FSequencerDisplayNode>& ObjectBindingNode)
{
	if (DisplayNode->GetType() == ESequencerNode::Object)
	{
		ObjectBindingNode = DisplayNode;
		return true;
	}

	if (DisplayNode->GetParent().IsValid())
	{
		return FindObjectBindingNode(DisplayNode->GetParent().ToSharedRef(), ObjectBindingNode);
	}

	return false;
}

int32 SequencerHelpers::TimeToFrame(float Time, float FrameRate)
{
	float Frame = Time * FrameRate;
	return FMath::RoundToInt(Frame);
}

float SequencerHelpers::FrameToTime(int32 Frame, float FrameRate)
{
	return Frame / FrameRate;
}

void SequencerHelpers::PerformDefaultSelection(FSequencer& Sequencer, const FPointerEvent& MouseEvent)
{
	FSequencerSelection& Selection = Sequencer.GetSelection();

	// @todo: selection in transactions
	auto ConditionallyClearSelection = [&]{
		if (!MouseEvent.IsShiftDown() && !MouseEvent.IsControlDown())
		{
			Selection.EmptySelectedSections();
			Selection.EmptySelectedKeys();
		}
	};

	TSharedPtr<ISequencerHotspot> Hotspot = Sequencer.GetHotspot();
	if (!Hotspot.IsValid())
	{
		ConditionallyClearSelection();
		return;
	}

	// Handle right-click selection separately since we never deselect on right click (except for clearing on exclusive selection)
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (Hotspot->GetType() == ESequencerHotspot::Key)
		{
			FSequencerSelectedKey Key = static_cast<FKeyHotspot*>(Hotspot.Get())->Key;
			if (!Selection.IsSelected(Key))
			{
				ConditionallyClearSelection();
				Selection.AddToSelection(Key);
			}
		}
		else if (Hotspot->GetType() == ESequencerHotspot::Section)
		{
			UMovieSceneSection* Section = static_cast<FSectionHotspot*>(Hotspot.Get())->Section.GetSectionObject();
			if (!Selection.IsSelected(Section))
			{
				ConditionallyClearSelection();
				Selection.AddToSelection(Section);
			}
		}

		return;
	}

	// Normal selection
	ConditionallyClearSelection();

	bool bForceSelect = !MouseEvent.IsControlDown();

	if (Hotspot->GetType() == ESequencerHotspot::Key)
	{
		FSequencerSelectedKey Key = static_cast<FKeyHotspot*>(Hotspot.Get())->Key;
		if (bForceSelect || !Selection.IsSelected(Key))
		{
			Selection.AddToSelection(Key);
		}
		else
		{
			Selection.RemoveFromSelection(Key);
		}
	}
	else if (Hotspot->GetType() == ESequencerHotspot::Section)
	{
		UMovieSceneSection* Section = static_cast<FSectionHotspot*>(Hotspot.Get())->Section.GetSectionObject();
		if (bForceSelect || !Selection.IsSelected(Section))
		{
			Selection.AddToSelection(Section);
		}
		else
		{
			Selection.RemoveFromSelection(Section);
		}
	}
}

TSharedPtr<SWidget> SequencerHelpers::SummonContextMenu(FSequencer& Sequencer, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// @todo sequencer replace with UI Commands instead of faking it

	FVector2D MouseDownPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	// Attempt to paste into either the current node selection, or the clicked on track
	TSharedRef<SSequencer> SequencerWidget = StaticCastSharedRef<SSequencer>(Sequencer.GetSequencerWidget());
	const float PasteAtTime = SequencerWidget->GetVirtualTrackArea().PixelToTime(MouseDownPos.X);

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, Sequencer.GetCommandBindings());

	TSharedPtr<ISequencerHotspot> Hotspot = Sequencer.GetHotspot();

	if (Hotspot.IsValid())
	{
		if (Hotspot->GetType() == ESequencerHotspot::Section || Hotspot->GetType() == ESequencerHotspot::Key)
		{
			Hotspot->PopulateContextMenu(MenuBuilder, Sequencer, PasteAtTime);

			return MenuBuilder.MakeWidget();
		}
	}
	else if (Sequencer.GetClipboardStack().Num() != 0)
	{
		TSharedPtr<FPasteContextMenu> PasteMenu = FPasteContextMenu::CreateMenu(Sequencer, SequencerWidget->GeneratePasteArgs(PasteAtTime));
		if (PasteMenu.IsValid() && PasteMenu->IsValidPaste())
		{
			PasteMenu->PopulateMenu(MenuBuilder);

			return MenuBuilder.MakeWidget();
		}
	}

	return nullptr;
}