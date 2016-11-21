// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSequenceID.h"

enum class ESpawnOwnership : uint8;

class IMovieScenePlayer;
class UMovieScene;
class UMovieSceneTrack;
class UMovieSceneSection;
class UMovieSceneSequence;

struct FMovieSceneContext;
struct FMovieSceneSpawnable;
struct FMovieSceneEvaluationState;
struct FMovieSceneEvaluationFieldTrackPtr;
struct FMovieSceneEvaluationFieldSegmentPtr;
struct FMovieSceneEvalTemplate;
struct FMovieSceneEvaluationTrack;
struct FMovieSceneSharedDataId;
struct IMovieSceneTemplateGenerator;
struct FMovieSceneSubSequenceData;
struct FMovieSceneSequenceTransform;
struct IMovieSceneTemplateGenerator;
struct FPersistentEvaluationData;
struct FMovieSceneExecutionTokens;
struct FMovieSceneEvaluationOperand;
struct IMovieScenePreAnimatedTokenProducer;
struct IMovieScenePreAnimatedGlobalTokenProducer;

DECLARE_STATS_GROUP(TEXT("Movie Scene Evaluation"), STATGROUP_MovieSceneEval, STATCAT_Advanced);

#ifndef MOVIESCENE_DETAILED_STATS
	#define MOVIESCENE_DETAILED_STATS 0
#endif

#if MOVIESCENE_DETAILED_STATS
	#define MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER SCOPE_CYCLE_COUNTER
#else
	#define MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(...)
#endif