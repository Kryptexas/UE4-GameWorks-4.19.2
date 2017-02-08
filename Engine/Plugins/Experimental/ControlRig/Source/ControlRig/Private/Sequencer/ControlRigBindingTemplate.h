// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Evaluation/MovieSceneSpawnTemplate.h"
#include "ControlRigBindingTemplate.generated.h"

class UMovieSceneSpawnSection;

/** Binding track eval template based off spawn template */
USTRUCT()
struct CONTROLRIG_API FControlRigBindingTemplate : public FMovieSceneSpawnSectionTemplate
{
	GENERATED_BODY()
	
	FControlRigBindingTemplate()
		: ObjectBindingSequenceID(MovieSceneSequenceID::Root)
	{}

#if WITH_EDITORONLY_DATA
	FControlRigBindingTemplate(const UMovieSceneSpawnSection& SpawnSection, TWeakObjectPtr<> InObjectBinding);
#endif
	FControlRigBindingTemplate(const UMovieSceneSpawnSection& SpawnSection, FGuid InObjectBindingId, FMovieSceneSequenceIDRef InObjectBindingSequenceID);

	static FMovieSceneAnimTypeID GetAnimTypeID();

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

private:
#if WITH_EDITORONLY_DATA
	/** The current external object binding we are using */
	TWeakObjectPtr<UObject> ObjectBinding;
#endif

	/** The current internal object binding we are using */
	UPROPERTY()
	FGuid ObjectBindingId;

	/** The current sequence of the internal object binding we are using */
	UPROPERTY()
	FMovieSceneSequenceID ObjectBindingSequenceID;
};
