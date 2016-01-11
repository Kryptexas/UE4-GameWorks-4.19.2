// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerHotspot.h"
#include "SequencerDisplayNode.h"
#include "ISequencerSection.h"

/** A hotspot representing a key */
struct FKeyHotspot : ISequencerHotspot
{
	FKeyHotspot(FSelectedKey InKey, TSharedPtr<FTrackNode> InTrackNode)
		: Key(InKey)
		, TrackNode(MoveTemp(InTrackNode))
	{}

	virtual ESequencerHotspot GetType() const override { return ESequencerHotspot::Key; }

	/** The key itself */
	FSelectedKey Key;
	/** The track node that holds the section the key is contained in */
	TSharedPtr<FTrackNode> TrackNode;
};

/** Structure used to encapsulate a section and its track node */
struct FSectionHandle
{
	FSectionHandle(TSharedPtr<FTrackNode> InTrackNode, int32 InSectionIndex)
		: SectionIndex(InSectionIndex), TrackNode(MoveTemp(InTrackNode))
	{}

	UMovieSceneSection* GetSectionObject() const
	{
		return TrackNode->GetSections()[SectionIndex]->GetSectionObject();
	}

	int32 SectionIndex;
	TSharedPtr<FTrackNode> TrackNode;
};

/** A hotspot representing a section */
struct FSectionHotspot : ISequencerHotspot
{
	FSectionHotspot(FSectionHandle InSection)
		: Section(InSection)
	{}

	virtual ESequencerHotspot GetType() const override { return ESequencerHotspot::Section; }

	/** Handle to the section */
	FSectionHandle Section;
};

/** A hotspot representing a resize handle on a section */
struct FSectionResizeHotspot : ISequencerHotspot
{
	enum EHandle { Left, Right };

	FSectionResizeHotspot(EHandle InHandleType, FSectionHandle InSection) : Section(InSection), HandleType(InHandleType) {}

	virtual ESequencerHotspot GetType() const override { return HandleType == Left ? ESequencerHotspot::SectionResize_L : ESequencerHotspot::SectionResize_R; }
	
	virtual TSharedPtr<IEditToolDragOperation> InitiateDrag(ISequencer& Sequencer) override;

	/** Handle to the section */
	FSectionHandle Section;

private:
	EHandle HandleType;
};