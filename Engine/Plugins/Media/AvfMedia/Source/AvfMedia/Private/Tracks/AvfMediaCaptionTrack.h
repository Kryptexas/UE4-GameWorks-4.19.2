// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMediaCaptionTrack.h"


/**
 * Caption Track implementation using the AV Foundation Framework.
 */
class FAvfMediaCaptionTrack
	: public FAvfMediaTrack
	, public IMediaCaptionTrack
{
public:

	/** Default constructor. */
	FAvfMediaCaptionTrack()
		: FAvfMediaTrack()
	{ }

public:

	// IMediaCaptionTrack interface

	virtual IMediaStream& GetStream() override
	{
		return *this;
	}
};
