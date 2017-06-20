// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneInitialValueStore.h"
#include "MovieSceneBlendingActuator.h"

struct FMovieSceneRemoveInitialValueToken : IMovieScenePreAnimatedGlobalToken
{
	FMovieSceneRemoveInitialValueToken(FObjectKey InObjectKey, TWeakPtr<IMovieSceneBlendingActuator> InWeakActuator)
		: ObjectKey(InObjectKey), WeakActuator(InWeakActuator)
	{
	}

	virtual void RestoreState(IMovieScenePlayer& Player) override
	{
		TSharedPtr<IMovieSceneBlendingActuator> Store = WeakActuator.Pin();
		if (Store.IsValid())
		{
			Store->RemoveInitialValueForObject(ObjectKey);
		}
	}

private:

	/** The object to remove an initial value for */
	FObjectKey ObjectKey;
	/** The store to remove the initial value from */
	TWeakPtr<IMovieSceneBlendingActuator> WeakActuator;
};

FMovieSceneRemoveInitialValueTokenProducer::FMovieSceneRemoveInitialValueTokenProducer(FObjectKey InObjectKey, TWeakPtr<IMovieSceneBlendingActuator> InWeakActuator)
	: ObjectKey(InObjectKey), WeakActuator(InWeakActuator)
{
}

IMovieScenePreAnimatedGlobalTokenPtr FMovieSceneRemoveInitialValueTokenProducer::CacheExistingState() const
{
	return FMovieSceneRemoveInitialValueToken(ObjectKey, WeakActuator);
}