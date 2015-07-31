// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerEditTool.h"

enum class ESequencerHotspot
{
	Key,
	Section,
	SectionResize_L,
	SectionResize_R,
};

/** A sequencer hotspot is used to identify specific areas on the sequencer track area */ 
struct ISequencerHotspot
{
	virtual ~ISequencerHotspot() { }
	virtual ESequencerHotspot GetType() const = 0;
	virtual TSharedPtr<IEditToolDragOperation> InitiateDrag(ISequencer&) { return nullptr; }
};