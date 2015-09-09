// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Parameters for determining keying behavior
 */
struct MOVIESCENE_API FKeyParams
{
	FKeyParams()
	{
		bCreateHandleIfMissing = false;
		bCreateTrackIfMissing = false;
		bAddKeyEvenIfUnchanged = false;
		bAutoKeying = false;
	}

	FKeyParams(const FKeyParams& InKeyParams)
	{
		bCreateHandleIfMissing = InKeyParams.bCreateHandleIfMissing;
		bCreateTrackIfMissing = InKeyParams.bCreateTrackIfMissing;
		bAddKeyEvenIfUnchanged = InKeyParams.bAddKeyEvenIfUnchanged;
		bAutoKeying = InKeyParams.bAutoKeying;
	}

	/** Create handle if it doesn't exist. */
	bool bCreateHandleIfMissing;
	/** Create track if it doesn't exist. */
	bool bCreateTrackIfMissing;
	/** Create a key even if it's unchanged. */
	bool bAddKeyEvenIfUnchanged;
	/** Auto keying on.*/
	bool bAutoKeying;
};
