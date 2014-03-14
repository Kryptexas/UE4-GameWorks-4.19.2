// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once


/** Interface that Slate uses to play sounds. */
class SLATE_API ISlateSoundDevice
{
public:
	/** Play the sound resource within the given Slate sound object */
	virtual void PlaySound(const FSlateSound& Sound) const = 0;

	/** @return The duration of the given sound resource */
	virtual float GetSoundDuration(const FSlateSound& Sound) const = 0;

	/** Virtual destructor: because it is an interface / pure virtual base class. */
	virtual ~ISlateSoundDevice(){}
};

/** Silent implementation of ISlateSoundDevice; it plays nothing. */
class SLATE_API FNullSlateSoundDevice : public ISlateSoundDevice
{
public:
	virtual void PlaySound(const FSlateSound&) const OVERRIDE
	{
	}

	virtual float GetSoundDuration(const FSlateSound& Sound) const OVERRIDE
	{
		return 0.0f;
	}

	virtual ~FNullSlateSoundDevice(){}
};
