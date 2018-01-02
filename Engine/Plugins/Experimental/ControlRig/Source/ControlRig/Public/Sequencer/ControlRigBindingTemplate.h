// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Evaluation/MovieSceneSpawnTemplate.h"
#include "Animation/AnimData/BoneMaskFilter.h"
#include "ControlRigBindingTemplate.generated.h"

class UMovieSceneSpawnSection;

/** Binding track eval template based off spawn template */
USTRUCT()
struct CONTROLRIG_API FControlRigBindingTemplate : public FMovieSceneSpawnSectionTemplate
{
	GENERATED_BODY()
	
	FControlRigBindingTemplate(){}

	FControlRigBindingTemplate(const UMovieSceneSpawnSection& SpawnSection);

	static FMovieSceneAnimTypeID GetAnimTypeID();

	// Bind to a runtime (non-sequencer-controlled) object
#if WITH_EDITORONLY_DATA
	static void SetObjectBinding(TWeakObjectPtr<> InObjectBinding);
	static UObject* GetObjectBinding();
	static void ClearObjectBinding();
#endif

private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

private:
#if WITH_EDITORONLY_DATA
	/** The current external object binding we are using */
	static TWeakObjectPtr<UObject> ObjectBinding;
#endif
};
