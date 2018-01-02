// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Sections/MovieScene3DConstraintSection.h"
#include "MovieSceneObjectBindingID.h"


UMovieScene3DConstraintSection::UMovieScene3DConstraintSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


void UMovieScene3DConstraintSection::SetConstraintId(const FGuid& InConstraintId)
{
	if (TryModify())
	{
		SetConstraintBindingID(FMovieSceneObjectBindingID(InConstraintId, MovieSceneSequenceID::Root, EMovieSceneObjectBindingSpace::Local));
	}
}


FGuid UMovieScene3DConstraintSection::GetConstraintId() const
{
	return FGuid();
}


TOptional<float> UMovieScene3DConstraintSection::GetKeyTime(FKeyHandle KeyHandle) const
{
	return TOptional<float>();
}


void UMovieScene3DConstraintSection::SetKeyTime(FKeyHandle KeyHandle, float Time)
{
	// do nothing
}


void UMovieScene3DConstraintSection::OnBindingsUpdated(const TMap<FGuid, FGuid>& OldGuidToNewGuidMap)
{
	if (OldGuidToNewGuidMap.Contains(ConstraintBindingID.GetGuid()))
	{
		ConstraintBindingID.SetGuid(OldGuidToNewGuidMap[ConstraintBindingID.GetGuid()]);
	}
}

void UMovieScene3DConstraintSection::PostLoad()
{
	Super::PostLoad();

	if (ConstraintId_DEPRECATED.IsValid())
	{
		if (!ConstraintBindingID.IsValid())
		{
			ConstraintBindingID = FMovieSceneObjectBindingID(ConstraintId_DEPRECATED, MovieSceneSequenceID::Root, EMovieSceneObjectBindingSpace::Local);
		}
		ConstraintId_DEPRECATED.Invalidate();
	}
}
