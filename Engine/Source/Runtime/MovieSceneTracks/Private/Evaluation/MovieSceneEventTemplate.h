// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneFwd.h"
#include "MovieSceneEventSection.h"
#include "MovieSceneEvalTemplate.h"

#include "MovieSceneEventTemplate.generated.h"

class UMovieSceneEventTrack;

USTRUCT()
struct FMovieSceneEventSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneEventSectionTemplate() {}
	FMovieSceneEventSectionTemplate(const UMovieSceneEventSection& Section, const UMovieSceneEventTrack& Track);

	UPROPERTY()
	FMovieSceneEventSectionData EventData;

	UPROPERTY()
	uint32 bFireEventsWhenForwards : 1;

	UPROPERTY()
	uint32 bFireEventsWhenBackwards : 1;

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
};