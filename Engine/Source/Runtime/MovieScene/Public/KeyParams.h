// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

UENUM()
enum EMovieSceneKeyInterpolation
{
	/** Auto. */
	MSKI_Auto UMETA(DisplayName="Auto"),

	/** User. */
	MSKI_User UMETA(DisplayName="User"),

	/** Break. */
	MSKI_Break UMETA(DisplayName="Break"),

	/** Linear. */
	MSKI_Linear UMETA(DisplayName="Linear"),

	/** Constant. */
	MSKI_Constant UMETA(DisplayName="Constant"),
};

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
		KeyInterpolation = MSKI_Auto;
	}

	FKeyParams(const FKeyParams& InKeyParams)
	{
		bCreateHandleIfMissing = InKeyParams.bCreateHandleIfMissing;
		bCreateTrackIfMissing = InKeyParams.bCreateTrackIfMissing;
		bAddKeyEvenIfUnchanged = InKeyParams.bAddKeyEvenIfUnchanged;
		bAutoKeying = InKeyParams.bAutoKeying;
		KeyInterpolation = InKeyParams.KeyInterpolation;
	}

	/** Create handle if it doesn't exist. */
	bool bCreateHandleIfMissing;
	/** Create track if it doesn't exist. */
	bool bCreateTrackIfMissing;
	/** Create a key even if it's unchanged. */
	bool bAddKeyEvenIfUnchanged;
	/** Auto keying on.*/
	bool bAutoKeying;
	/** Key interpolation. */
	EMovieSceneKeyInterpolation KeyInterpolation;
};
