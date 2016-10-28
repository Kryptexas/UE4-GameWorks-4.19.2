// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneFwd.h"
#include "MovieSceneTrackImplementation.h"
#include "MovieSceneEvalTemplate.h"
#include "MovieSceneFadeSection.h"
#include "Curves/RichCurve.h"

#include "MovieSceneFadeTemplate.generated.h"

class UMovieSceneFadeTrack;

USTRUCT()
struct FMovieSceneFadeSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneFadeSectionTemplate() {}
	FMovieSceneFadeSectionTemplate(const UMovieSceneFadeSection& Section);

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FRichCurve FadeCurve;
	
	UPROPERTY()
	FLinearColor FadeColor;

	UPROPERTY()
	uint32 bFadeAudio:1;
};
