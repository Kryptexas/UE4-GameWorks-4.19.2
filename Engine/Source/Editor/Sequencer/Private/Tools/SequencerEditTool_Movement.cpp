// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerEditTool_Movement.h"
#include "Sequencer.h"
#include "VirtualTrackArea.h"
#include "SequencerHotspots.h"
#include "EditToolDragOperations.h"

const FName FSequencerEditTool_Movement::Identifier = "Movement";

FSequencerEditTool_Movement::FSequencerEditTool_Movement(FSequencer& InSequencer)
	: FSequencerEditTool(InSequencer)
	, SequencerWidget(StaticCastSharedRef<SSequencer>(InSequencer.GetSequencerWidget()))
{ }


FReply FSequencerEditTool_Movement::OnMouseButtonDown(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<ISequencerHotspot> Hotspot = Sequencer.GetHotspot();

	DelayedDrag.Reset();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea();

		DelayedDrag = FDelayedDrag_Hotspot(VirtualTrackArea.CachedTrackAreaGeometry().AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), MouseEvent.GetEffectingButton(), Hotspot);

 		if (Sequencer.GetSettings()->GetSnapPlayTimeToDraggedKey() || (MouseEvent.IsShiftDown() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) )
		{
			if (DelayedDrag->Hotspot.IsValid())
			{
				if (DelayedDrag->Hotspot->GetType() == ESequencerHotspot::Key)
				{
					FSequencerSelectedKey& ThisKey = StaticCastSharedPtr<FKeyHotspot>(DelayedDrag->Hotspot)->Key;
					Sequencer.SetGlobalTime(ThisKey.KeyArea->GetKeyTime(ThisKey.KeyHandle.GetValue()));
				}
			}
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}


FReply FSequencerEditTool_Movement::OnMouseMove(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DelayedDrag.IsSet())
	{
		const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea();

		FReply Reply = FReply::Handled();

		if (DelayedDrag->IsDragging())
		{
			// If we're already dragging, just update the drag op if it exists
			if (DragOperation.IsValid())
			{
				DragOperation->OnDrag(MouseEvent, MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), VirtualTrackArea);
			}
		}
		// Otherwise we can attempt a new drag
		else if (DelayedDrag->AttemptDragStart(MouseEvent))
		{
			DragOperation = CreateDrag(MouseEvent);

			if (DragOperation.IsValid())
			{
				DragOperation->OnBeginDrag(MouseEvent, DelayedDrag->GetInitialPosition(), VirtualTrackArea);

				// Steal the capture, as we're now the authoritative widget in charge of a mouse-drag operation
				Reply.CaptureMouse(OwnerWidget.AsShared());
			}
		}

		return Reply;
	}
	return FReply::Unhandled();
}


TSharedPtr<ISequencerEditToolDragOperation> FSequencerEditTool_Movement::CreateDrag(const FPointerEvent& MouseEvent)
{
	FSequencerSelection& Selection = Sequencer.GetSelection();

	if (DelayedDrag->Hotspot.IsValid())
	{
		// Let the hotspot start a drag first, if it wants to
		auto HotspotDrag = DelayedDrag->Hotspot->InitiateDrag(Sequencer);
		if (HotspotDrag.IsValid())
		{
			return HotspotDrag;
		}

		// Ok, the hotspot doesn't know how to drag - let's decide for ourselves
		auto HotspotType = DelayedDrag->Hotspot->GetType();

		// Moving section(s)?
		if (HotspotType == ESequencerHotspot::Section)
		{
			TArray<FSectionHandle> SectionHandles;

			FSectionHandle& ThisHandle = StaticCastSharedPtr<FSectionHotspot>(DelayedDrag->Hotspot)->Section;
			UMovieSceneSection* ThisSection = ThisHandle.GetSectionObject();
			if (Selection.IsSelected(ThisSection))
			{
				SectionHandles = SequencerWidget.Pin()->GetSectionHandles(Selection.GetSelectedSections());
			}
			else
			{
				Selection.EmptySelectedKeys();
				Selection.EmptySelectedSections();
				Selection.AddToSelection(ThisSection);
				SectionHandles.Add(ThisHandle);
			}
			return MakeShareable( new FMoveSection( Sequencer, SectionHandles ) );
		}
		// Moving key(s)?
		else if (HotspotType == ESequencerHotspot::Key)
		{
			FSequencerSelectedKey& ThisKey = StaticCastSharedPtr<FKeyHotspot>(DelayedDrag->Hotspot)->Key;

			// If it's not selected, we'll treat this as a unique drag
			if (!Selection.IsSelected(ThisKey))
			{
				Selection.EmptySelectedKeys();
				Selection.EmptySelectedSections();
				Selection.AddToSelection(ThisKey);
			}

			// @todo sequencer: Make this a customizable UI command modifier?
			if (MouseEvent.IsAltDown())
			{
				return MakeShareable( new FDuplicateKeys( Sequencer, Selection.GetSelectedKeys() ) );
			}
			else
			{
				return MakeShareable( new FMoveKeys( Sequencer, Selection.GetSelectedKeys() ) );
			}
		}
	}
	// If we're not dragging a hotspot, sections take precedence over keys
	else if (Selection.GetSelectedSections().Num())
	{
		return MakeShareable( new FMoveSection( Sequencer, SequencerWidget.Pin()->GetSectionHandles(Selection.GetSelectedSections()) ) );
	}
	else if (Selection.GetSelectedKeys().Num())
	{
		return MakeShareable( new FMoveKeys( Sequencer, Selection.GetSelectedKeys() ) );
	}

	return nullptr;
}


FReply FSequencerEditTool_Movement::OnMouseButtonUp(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DelayedDrag.Reset();

	if (DragOperation.IsValid())
	{
		DragOperation->OnEndDrag(MouseEvent, MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), SequencerWidget.Pin()->GetVirtualTrackArea());
		DragOperation = nullptr;

		// Only return handled if we actually started a drag
		return FReply::Handled().ReleaseMouseCapture();
	}

	SequencerHelpers::PerformDefaultSelection(Sequencer, MouseEvent);

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		TSharedPtr<SWidget> MenuContent = SequencerHelpers::SummonContextMenu( Sequencer, MyGeometry, MouseEvent );
		if (MenuContent.IsValid())
		{
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();

			FSlateApplication::Get().PushMenu(
				OwnerWidget.AsShared(),
				WidgetPath,
				MenuContent.ToSharedRef(),
				MouseEvent.GetScreenSpacePosition(),
				FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
				);

			return FReply::Handled().SetUserFocus(MenuContent.ToSharedRef(), EFocusCause::SetDirectly).ReleaseMouseCapture();
		}
	}

	return FReply::Handled();
}


void FSequencerEditTool_Movement::OnMouseCaptureLost()
{
	DelayedDrag.Reset();
	DragOperation = nullptr;
}


FCursorReply FSequencerEditTool_Movement::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	TSharedPtr<ISequencerHotspot> Hotspot = Sequencer.GetHotspot();
	if (DelayedDrag.IsSet())
	{
		Hotspot = DelayedDrag->Hotspot;
	}

	if (Hotspot.IsValid())
	{
		FCursorReply Reply = Hotspot->GetCursor();
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}

	return FCursorReply::Cursor(EMouseCursor::CardinalCross);
}

FName FSequencerEditTool_Movement::GetIdentifier() const
{
	return Identifier;
}

bool FSequencerEditTool_Movement::CanDeactivate() const
{
	return !DelayedDrag.IsSet();
}