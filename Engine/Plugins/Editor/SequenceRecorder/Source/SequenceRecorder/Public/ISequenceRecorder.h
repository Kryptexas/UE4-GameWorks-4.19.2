// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

class ISequenceRecorder : public IModuleInterface
{
public:
	/** 
	 * Start recording the passed-in actors.
	 * @param	World			The world we use to record actors
	 * @param	ActorFilter		Actor filter to gather actors spawned in this world for recording
	 * @return true if recording was successfully started
	 */
	virtual bool StartRecording(UWorld* World, const struct FSequenceRecorderActorFilter& ActorFilter) = 0;

	/** Stop recording current sequence, if any */
	virtual void StopRecording() = 0;

	/** Are we currently recording a sequence */
	virtual bool IsRecording() = 0;
};