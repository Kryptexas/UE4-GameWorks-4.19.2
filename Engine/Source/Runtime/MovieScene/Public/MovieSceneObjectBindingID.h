// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "MovieSceneFwd.h"
#include "MovieSceneSequenceID.h"
#include "MovieSceneObjectBindingID.generated.h"

/** Persistent identifier to a specific object binding within a sequence hierarchy. */
USTRUCT(BlueprintType, meta=(HasNativeMake))
struct FMovieSceneObjectBindingID
{
	GENERATED_BODY()

	/** Default construction to an invalid object binding ID */
	FMovieSceneObjectBindingID()
		: SequenceID(int32(MovieSceneSequenceID::Root.GetInternalValue()))
	{
	}

	/** Construction from an object binding guid, and the specific sequence instance ID in which it resides */
	FMovieSceneObjectBindingID(const FGuid& InGuid, FMovieSceneSequenceID InSequenceID)
		: SequenceID(int32(InSequenceID.GetInternalValue())), Guid(InGuid)
	{
	}

	/**
	 * Check whether this object binding ID has been set to something valied
	 * @note: does not imply that the ID resolves to a valid object
	 */
	bool IsValid() const
	{
		return Guid.IsValid();
	}

	/**
	 * Access the identifier for the sequence in which the object binding resides
	 */
	FMovieSceneSequenceID GetSequenceID() const
	{
		return FMovieSceneSequenceID(uint32(SequenceID));
	}

	/**
	 * Access the guid that identifies the object binding within the sequence
	 */
	const FGuid& GetGuid() const
	{
		return Guid;
	}

public:

	friend uint32 GetTypeHash(const FMovieSceneObjectBindingID& A)
	{
		return GetTypeHash(A.Guid) ^ GetTypeHash(A.SequenceID);
	}

	friend bool operator==(const FMovieSceneObjectBindingID& A, const FMovieSceneObjectBindingID& B)
	{
		return A.Guid == B.Guid && A.SequenceID == B.SequenceID;
	}

	friend bool operator!=(const FMovieSceneObjectBindingID& A, const FMovieSceneObjectBindingID& B)
	{
		return A.Guid != B.Guid || A.SequenceID != B.SequenceID;
	}

private:

	/** Sequence ID stored as an int32 so that it can be used in the blueprint VM */
	UPROPERTY()
	int32 SequenceID;

	/** Identifier for the object binding within the sequence */
	UPROPERTY(EditAnywhere, Category="Binding")
	FGuid Guid;
};