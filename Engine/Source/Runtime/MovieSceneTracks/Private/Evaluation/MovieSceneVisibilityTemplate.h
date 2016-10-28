// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneFwd.h"
#include "MovieScenePropertyTemplates.h"

#include "MovieSceneVisibilityTemplate.generated.h"

class UMovieSceneBoolSection;


USTRUCT()
struct FMovieSceneVisibilitySectionTemplate : public FMovieSceneBoolPropertySectionTemplate
{
	GENERATED_BODY()
	
	FMovieSceneVisibilitySectionTemplate(){}
	FMovieSceneVisibilitySectionTemplate(const UMovieSceneBoolSection& Section, const UMovieScenePropertyTrack& Track);

	/** Temporarily hidden in game */
	UPROPERTY()
	bool bTemporarilyHiddenInGame;

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
};
