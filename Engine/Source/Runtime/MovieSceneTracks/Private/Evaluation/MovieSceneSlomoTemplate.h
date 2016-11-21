// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneFwd.h"
#include "MovieSceneTrackImplementation.h"
#include "MovieSceneEvalTemplate.h"
#include "MovieSceneSlomoSection.h"
#include "Curves/RichCurve.h"

#include "MovieSceneSlomoTemplate.generated.h"

class UMovieSceneSlomoTrack;

USTRUCT()
struct FMovieSceneSlomoSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneSlomoSectionTemplate() {}
	FMovieSceneSlomoSectionTemplate(const UMovieSceneSlomoSection& Section);

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FRichCurve SlomoCurve;
};
