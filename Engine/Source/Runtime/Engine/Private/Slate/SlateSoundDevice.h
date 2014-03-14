// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "EnginePrivate.h"
#include "Slate.h"

class FSlateSoundDevice : public ISlateSoundDevice
{
public:
	virtual ~FSlateSoundDevice(){}

private:
	virtual void PlaySound(const FSlateSound& Sound) const OVERRIDE;

	virtual float GetSoundDuration(const FSlateSound& Sound) const OVERRIDE;
};
