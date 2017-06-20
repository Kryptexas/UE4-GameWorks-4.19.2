// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTemplateInterrogation.h"

/** Definition needs to exist after the definition of FMovieSceneBlendingAccumulator */
template<typename DataType>
void TBlendableTokenStack<DataType>::ComputeAndActuate(UObject* InObject, FMovieSceneBlendingAccumulator& Accumulator, FMovieSceneBlendingActuatorID InActuatorType, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
{
	TMovieSceneBlendingActuator<DataType>* Actuator = Accumulator.FindActuator<DataType>(InActuatorType);
	if (!ensure(Actuator))
	{
		return;
	}

	TMovieSceneInitialValueStore<DataType> InitialValues(*Actuator, *this, InObject, &Player);

	typename TBlendableTokenTraits<DataType>::WorkingDataType WorkingTotal{};
	for (const TBlendableToken<DataType>* Token : Tokens)
	{
		Token->AddTo(WorkingTotal, InitialValues);
	}

	DataType FinalResult = WorkingTotal.Resolve(InitialValues);
	Actuator->Actuate(InObject, FinalResult, *this, Context, PersistentData, Player);
}


template<typename DataType>
void TBlendableTokenStack<DataType>::Interrogate(UObject* AnimatedObject, FMovieSceneInterrogationData& InterrogationData, FMovieSceneBlendingAccumulator& Accumulator, FMovieSceneBlendingActuatorID InActuatorType, const FMovieSceneContext& Context)
{
	TMovieSceneBlendingActuator<DataType>* Actuator = Accumulator.FindActuator<DataType>(InActuatorType);
	if (!ensure(Actuator))
	{
		return;
	}

	TMovieSceneInitialValueStore<DataType> InitialValues(*Actuator, *this, AnimatedObject, nullptr);

	typename TBlendableTokenTraits<DataType>::WorkingDataType WorkingTotal{};
	for (const TBlendableToken<DataType>* Token : Tokens)
	{
		Token->AddTo(WorkingTotal, InitialValues);
	}

	DataType FinalResult = WorkingTotal.Resolve(InitialValues);
	Actuator->Actuate(InterrogationData, FinalResult, *this, Context);
}