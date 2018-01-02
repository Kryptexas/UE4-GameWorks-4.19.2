// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PersistentEvaluationData.h"
#include "IMovieScenePlayer.h"
#include "Evaluation/MovieSceneEvaluationTemplateInstance.h"

FPersistentEvaluationData::FPersistentEvaluationData(IMovieScenePlayer& InPlayer)
	: Player(InPlayer)
	, EntityData(Player.State.PersistentEntityData)
	, SharedData(Player.State.PersistentSharedData)
{}

const FMovieSceneSequenceInstanceData* FPersistentEvaluationData::GetInstanceData() const
{
	const FMovieSceneSubSequenceData* SubData = Player.GetEvaluationTemplate().GetHierarchy().FindSubData(TrackKey.SequenceID);
	return SubData && SubData->InstanceData.IsValid() ? &SubData->InstanceData.GetValue() : nullptr;
}