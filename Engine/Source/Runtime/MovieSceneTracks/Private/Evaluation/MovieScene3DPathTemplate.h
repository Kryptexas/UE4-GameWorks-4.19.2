// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Curves/RichCurve.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Sections/MovieScene3DPathSection.h"
#include "MovieSceneObjectBindingID.h"
#include "MovieScene3DPathTemplate.generated.h"

USTRUCT()
struct FMovieScene3DPathSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieScene3DPathSectionTemplate() {}
	FMovieScene3DPathSectionTemplate(const UMovieScene3DPathSection& Section);

	/** The object binding ID of the path we should attach to */
	UPROPERTY()
	FMovieSceneObjectBindingID PathBindingID;

	/** The timing curve */
	UPROPERTY()
	FRichCurve TimingCurve;

	/** Front Axis */
	UPROPERTY()
	MovieScene3DPathSection_Axis FrontAxisEnum;

	/** Up Axis */
	UPROPERTY()
	MovieScene3DPathSection_Axis UpAxisEnum;

	/** Follow Curve */
	UPROPERTY()
	uint32 bFollow:1;

	/** Reverse Timing */
	UPROPERTY()
	uint32 bReverse:1;

	/** Force Upright */
	UPROPERTY()
	uint32 bForceUpright:1;

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
};
