// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneFwd.h"
#include "IMovieSceneTrackInstance.h"

#include "MovieSceneLegacyTrackInstanceTemplate.generated.h"

USTRUCT()
struct MOVIESCENE_API FMovieSceneLegacyTrackInstanceTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FMovieSceneLegacyTrackInstanceTemplate() {}
	FMovieSceneLegacyTrackInstanceTemplate(const UMovieSceneTrack* InTrack);

	
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const;
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag);
	}

	UPROPERTY()
	const UMovieSceneTrack* Track;
};