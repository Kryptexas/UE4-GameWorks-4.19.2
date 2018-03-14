// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Evaluation/MovieScenePropertyTemplate.h"

#include "MovieSceneMediaTemplate.generated.h"

class UMovieSceneMediaSection;
class UMovieSceneMediaTrack;
class UMediaSoundComponent;
class UMediaSource;
class UMediaTexture;


USTRUCT()
struct FMovieSceneMediaSectionParams
{
	GENERATED_BODY()

	UPROPERTY()
	UMediaSoundComponent* MediaSoundComponent;

	UPROPERTY()
	UMediaSource* MediaSource;

	UPROPERTY()
	UMediaTexture* MediaTexture;

	UPROPERTY()
	FString Proxy;

	UPROPERTY()
	float SectionEndTime;

	UPROPERTY()
	float SectionStartTime;
};


USTRUCT()
struct FMovieSceneMediaSectionTemplate
	: public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	/** Default constructor. */
	FMovieSceneMediaSectionTemplate() { }

	/**
	 * Create and initialize a new instance.
	 *
	 * @param InSection
	 * @param InTrack
	 */
	FMovieSceneMediaSectionTemplate(const UMovieSceneMediaSection& InSection, const UMovieSceneMediaTrack& InTrack);

public:

	//~ FMovieSceneEvalTemplate interface

	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
	virtual UScriptStruct& GetScriptStructImpl() const override;
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const;
	virtual void SetupOverrides() override;
	virtual void TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;

private:

	UPROPERTY()
	FMovieSceneMediaSectionParams Params;
};
